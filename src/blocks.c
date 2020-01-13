/**
 * rofi-top
 *
 * MIT/X11 License
 * Copyright (c) 2019 Omar Castro <omar.castro.360@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#define G_LOG_DOMAIN    "BlocksMode"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <gmodule.h>
#include <time.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>

#include <rofi/mode.h>
#include <rofi/helper.h>
#include <rofi/mode-private.h>

#include <glib-object.h>
#include <json-glib/json-glib.h>

#include <stdint.h>

typedef struct RofiViewState RofiViewState;
void rofi_view_switch_mode ( RofiViewState *state, Mode *mode );
RofiViewState * rofi_view_get_active ( void );
extern void rofi_view_set_overlay(RofiViewState * state, const char *text);
extern void rofi_view_reload ( void );
const char * rofi_view_get_user_input ( const RofiViewState *state );
void rofi_view_handle_text ( RofiViewState *state, char *text );
void rofi_view_clear_input ( RofiViewState *state );
G_MODULE_EXPORT Mode mode;


const gchar* CmdArg__BLOCKS_WRAP = "-blocks-wrap";
const gchar* CmdArg__MARKUP_ROWS = "-markup-rows";

const gchar* EMPTY_STRING = "";

#define NELEMS(x)  (sizeof(x) / sizeof((x)[0]))

typedef enum {
    InputAction__SEND_ACTION,
    InputAction__FILTER_USING_ROFI
} InputAction;

static const char *input_action_names[] = {
    "send",
    "filter"
};

size_t NUM_OF_INPUT_ACTIONS = NELEMS(input_action_names);

typedef enum {
    Event__INPUT_CHANGE,
    Event__CUSTOM_KEY,
    Event__SELECT_ENTRY, 
    Event__DELETE_ENTRY, 
    Event__EXEC_CUSTOM_INPUT
} Event;

static const char *event_enum_labels[] = {
    "INPUT_CHANGE",
    "CUSTOM_KEY",
    "SELECT_ENTRY", 
    "DELETE_ENTRY", 
    "EXEC_CUSTOM_INPUT"
};

static const char *event_labels[] = {
    "input change",
    "custom key",
    "select entry",
    "delete entry",
    "execute custom input"
};

typedef struct
{
    gchar *text;
    gboolean urgent;
    gboolean highlight;
    gboolean markup;
} LineData;

typedef struct
{
    gboolean markup_default;
    GString *message;
    GString *overlay;
    GString *prompt;
    GString *input;
    GArray *lines;
} PageData;

typedef struct
{
    PageData * currentPageData;
    GString * input_format;
    InputAction input_action;

    JsonParser *parser;
    JsonObject *root;
    GError *error;
    GString * active_line;
    GString * buffer;
    
    GPid cmd_pid;
    gboolean close_on_child_exit;
    GIOChannel * write_channel;
    GIOChannel * read_channel;
    int write_channel_fd;
    int read_channel_fd;
    guint read_channel_watcher;


} BlocksModePrivateData;


/**************
  utils
***************/

// Result is an allocated a new string
char *str_replace(const char *orig, const char *rep, const char *with) {
    char *result; // the return string
    char *ins;    // the next insert point
    char *remainder; // remainder point
    char *tmp;    // varies
    int len_rep;  // length of rep (the string to remove)
    int len_with; // length of with (the string to replace rep with)
    int len_front; // distance between rep and end of last rep
    int count;    // number of replacements

    // sanity checks and initialization
    if (!orig || !rep)
        return NULL;
    len_rep = strlen(rep);
    if (len_rep == 0)
        return NULL; // empty rep causes infinite loop during count
    if (!with)
        with = "";
    len_with = strlen(with);

    // count the number of replacements needed
    ins = remainder = (char *) orig;
    for (count = 0; tmp = strstr(ins, rep); ++count) {
        ins = tmp + len_rep;
    }

    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    while (count--) {
        ins = strstr(remainder, rep);
        len_front = ins - remainder;
        tmp = strncpy(tmp, remainder, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        remainder += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, remainder);
    return result;
}

char *str_replace_in(char **orig, const char *rep, const char *with) {
    char * result = str_replace(*orig, rep, with);
    if( result != NULL ){
        free(*orig);
        *orig = result;
    }
    return *orig;
}

char *str_replace_in_escaped(char **orig, const char *rep, const char *with) {
    const gchar * escaped_with = g_strescape(with, NULL);
    char * result = str_replace_in(orig, rep, escaped_with);
    g_free((char *) escaped_with);
    return result;
}

pid_t popen2(const char *command, int *infp, int *outfp){

    const short READ = 0;
    const short WRITE = 1;
    int p_stdin[2], p_stdout[2];
    pid_t pid;
    if (pipe(p_stdin) != 0 || pipe(p_stdout) != 0)
        return -1;

    pid = fork();

    if (pid < 0)
        return pid;

    else if (pid == 0)
    {
        close(p_stdin[WRITE]);
        dup2(p_stdin[READ], READ);
        close(p_stdout[READ]);
        dup2(p_stdout[WRITE], WRITE);
        execl(command, NULL);
        printf("{\"close on exit\": false, \"message\":\"Error loading %s:%s\"}\n", command, strerror(errno));
        perror("execl");
        exit(1);
    }

    if (infp == NULL)
        close(p_stdin[WRITE]);
    else
        *infp = p_stdin[WRITE];
    if (outfp == NULL)
        close(p_stdout[READ]);
    else
        *outfp = p_stdout[READ];
    return pid;

}


/*************************
  json glib extensions
**************************/

gboolean json_node_get_boolean_or_else(JsonNode * node, gboolean else_value){
    return node != NULL &&
           json_node_get_value_type(node) == G_TYPE_BOOLEAN ?
           json_node_get_boolean(node) : else_value;
}

const gchar * json_node_get_string_or_else(JsonNode * node, const gchar * else_value){
    return node != NULL &&
           json_node_get_value_type(node) == G_TYPE_STRING ?
           json_node_get_string(node) : else_value;
}


gboolean json_object_get_boolean_member_or_else(JsonObject * node, const gchar * member, gboolean else_value){
    return json_node_get_boolean_or_else(json_object_get_member(node, member), else_value);
}

const gchar * json_object_get_string_member_or_else(JsonObject * node, const gchar * member, const gchar * else_value){
    return json_node_get_string_or_else(json_object_get_member(node, member), else_value);
}


/***********************
    Page data methods
************************/

PageData * page_data_new(){
    PageData * pageData = g_malloc0( sizeof ( *pageData ) );
    pageData->markup_default = find_arg(CmdArg__MARKUP_ROWS) >= 0 ? TRUE : FALSE;
    pageData->message = g_string_sized_new(256);
    pageData->overlay = g_string_sized_new(64);
    pageData->prompt = g_string_sized_new(64);
    pageData->input = g_string_sized_new(64);
    pageData->lines = g_array_new (FALSE, TRUE, sizeof (LineData));
    return pageData;
}

void page_data_free(PageData * pageData){
    g_string_free(pageData->message, TRUE);
    g_string_free(pageData->overlay, TRUE);
    g_string_free(pageData->prompt, TRUE);
    g_string_free(pageData->input, TRUE);
    g_array_free (pageData->lines, TRUE);
    g_free(pageData);
}

void page_data_add_line(PageData * pageData, const gchar * label, gboolean urgent, gboolean highlight, gboolean markup){
    LineData line = { .text = g_strdup(label), .urgent = urgent, .highlight = highlight, .markup = markup };
    g_array_append_val(pageData->lines, line);
}

void page_data_add_line_json_node(PageData * pageData, JsonNode * element){
    if(JSON_NODE_HOLDS_VALUE(element) && json_node_get_value_type(element) == G_TYPE_STRING){
        page_data_add_line(pageData, json_node_get_string(element), FALSE, FALSE, pageData->markup_default);
    } else if(JSON_NODE_HOLDS_OBJECT(element)){
        JsonObject * line_obj = json_node_get_object(element);
        JsonNode * text_node = json_object_get_member(line_obj, "text");
        JsonNode * urgent_node = json_object_get_member(line_obj, "urgent");
        JsonNode * highlight_node = json_object_get_member(line_obj, "highlight");
        JsonNode * markup_node = json_object_get_member(line_obj, "markup");
        const gchar * text = json_node_get_string_or_else(text_node, EMPTY_STRING);
        gboolean urgent = json_node_get_boolean_or_else(urgent_node, FALSE);
        gboolean highlight = json_node_get_boolean_or_else(highlight_node, FALSE);
        gboolean markup = json_node_get_boolean_or_else(markup_node, pageData->markup_default);
        page_data_add_line(pageData, text, urgent, highlight, markup);
    }
}

void page_data_clear_lines(PageData * pageData){
    GArray * lines = pageData->lines;
    int lines_length = lines->len;
    for (int i = 0; i < lines_length; i++){
        LineData line = g_array_index (lines, LineData, i);
        g_free(line.text);
    }
    g_array_set_size(pageData->lines, 0);
}

/**************************************
  extended mode pirvate data methods
**************************************/

static void blocks_mode_private_data_update_string(BlocksModePrivateData * data, GString * str, const char * json_root_member){
    const gchar* memberVal = json_object_get_string_member_or_else(data->root, json_root_member, NULL);
    if(memberVal != NULL){
        g_string_assign(str,memberVal);
    }
}

static void blocks_mode_private_data_update_input_action(BlocksModePrivateData * data){
    const gchar* input_action = json_object_get_string_member_or_else(data->root, "input action", NULL);
    if(input_action != NULL){
        for (int i = 0; i < NUM_OF_INPUT_ACTIONS; ++i)
        {
            if(g_strcmp0(input_action, input_action_names[i]) == 0){
                data->input_action = (InputAction) i;
            }
        }
    }
}

static void blocks_mode_private_data_update_message(BlocksModePrivateData * data){
    blocks_mode_private_data_update_string(data, data->currentPageData->message, "message");
}

static void blocks_mode_private_data_update_overlay(BlocksModePrivateData * data){
    blocks_mode_private_data_update_string(data, data->currentPageData->overlay, "overlay");
}

static void blocks_mode_private_data_update_prompt(BlocksModePrivateData * data){
    blocks_mode_private_data_update_string(data, data->currentPageData->prompt, "prompt");
}

static void blocks_mode_private_data_update_input(BlocksModePrivateData * data){
    blocks_mode_private_data_update_string(data, data->currentPageData->input, "input");
}

static void blocks_mode_private_data_update_input_format(BlocksModePrivateData * data){
    blocks_mode_private_data_update_string(data, data->input_format, "event format");
}

static void blocks_mode_private_data_update_close_on_child_exit(BlocksModePrivateData * data){
    gboolean orig = data->close_on_child_exit;
    gboolean now = json_object_get_boolean_member_or_else(data->root, "close on exit" , orig);
    data->close_on_child_exit = now;
}

static void blocks_mode_private_data_update_lines(BlocksModePrivateData * data){
    JsonObject *root = data->root;
    PageData * pageData = data->currentPageData;
    const char * LINES_PROP = "lines";
    if(json_object_has_member(root, LINES_PROP)){
        JsonArray* lines = json_object_get_array_member(data->root, LINES_PROP);
        page_data_clear_lines( pageData );
        size_t len = json_array_get_length(lines);
        for(int index = 0; index < len; ++index){
            page_data_add_line_json_node(pageData, json_array_get_element(lines, index));
        }
    }
}

static void blocks_mode_private_data_update_page(BlocksModePrivateData * data){
    GError * error = NULL;
    json_parser_load_from_data(data->parser,data->active_line->str,data->active_line->len,&error);
    data->root = json_node_get_object(json_parser_get_root(data->parser));

    blocks_mode_private_data_update_input_action(data);
    blocks_mode_private_data_update_message(data);
    blocks_mode_private_data_update_overlay(data);
    blocks_mode_private_data_update_input(data);
    blocks_mode_private_data_update_prompt(data);
    blocks_mode_private_data_update_close_on_child_exit(data);
    blocks_mode_private_data_update_input_format(data);
    blocks_mode_private_data_update_lines(data);
    
}

void blocks_mode_private_data_write_to_channel ( BlocksModePrivateData * data, Event event, const char * action_value){
        GIOChannel * write_channel = data->write_channel;
        const gchar * format = data->input_format->str;
        gchar * format_result = str_replace(format, "{{name}}", event_labels[event]);
        format_result = str_replace_in(&format_result, "{{name_enum}}", event_enum_labels[event]);
        format_result = str_replace_in(&format_result, "{{value}}", action_value);
        format_result = str_replace_in_escaped(&format_result, "{{name_escaped}}", event_labels[event]);
        format_result = str_replace_in_escaped(&format_result, "{{value_escaped}}", action_value);
        g_debug("sending event: %s", format_result);
        gsize bytes_witten;
        g_io_channel_write_chars(write_channel, format_result, -1, &bytes_witten, &data->error);
        g_io_channel_write_unichar(write_channel, '\n', &data->error);
        g_io_channel_flush(write_channel, &data->error);
        g_free(format_result);
}

void blocks_mode_verify_input_change ( BlocksModePrivateData * data, const char * new_input_value){
    PageData * pageData = data->currentPageData;
    GString * inputStr = pageData->input;
    if(data->input_action == InputAction__SEND_ACTION && g_strcmp0(inputStr->str, new_input_value) != 0){
        g_string_assign(inputStr, new_input_value);
        blocks_mode_private_data_write_to_channel(data, Event__INPUT_CHANGE, new_input_value);
    }
}

/**************************
  mode extension methods
**************************/

static BlocksModePrivateData * mode_get_private_data_extended_mode(const Mode *sw){
    return (BlocksModePrivateData *) mode_get_private_data( sw );
}

static PageData * mode_get_private_data_current_page(const Mode *sw){
    return mode_get_private_data_extended_mode( sw )->currentPageData;
}

/*******************
 main loop sources
********************/

// GIOChannel watch, called when there is output to read from child proccess 
static gboolean on_new_input ( GIOChannel *source, GIOCondition condition, gpointer context )
{
    Mode *sw = (Mode *) context;
    RofiViewState * state = rofi_view_get_active();

    BlocksModePrivateData *data = mode_get_private_data_extended_mode( sw );

    GString * buffer = data->buffer;
    GString * active_line = data->active_line;

    gboolean newline = FALSE;

    GError * error = NULL;
    gchar unichar;
    gsize bytes_read;

    g_io_channel_read_chars(source, &unichar, 1, &bytes_read, &error);
    while(bytes_read > 0) {
        g_string_append_c(buffer, unichar);
        if( unichar == '\n' ){
            if(buffer->len > 1){ //input is not an empty line
                g_debug("received new line: %s", buffer->str);
                g_string_assign(active_line, buffer->str);
                newline=TRUE;
            }
            g_string_set_size(buffer, 0);
        }
        g_io_channel_read_chars(source, &unichar, 1, &bytes_read, &error);
    }

    if(newline){
        GString * oldOverlay = g_string_new(data->currentPageData->overlay->str);
        GString * oldPrompt = g_string_new(data->currentPageData->prompt->str);
        GString * oldInput = g_string_new(data->currentPageData->input->str);
        
        blocks_mode_private_data_update_page(data);
        
        GString * newOverlay = data->currentPageData->overlay;
        GString * newPrompt = data->currentPageData->prompt;
        GString * newInput = data->currentPageData->input;

        if(!g_string_equal(oldOverlay, newOverlay)){
            RofiViewState * state = rofi_view_get_active();
            rofi_view_set_overlay(state, (newOverlay->len > 0) ? newOverlay->str : NULL);
        }
        if(!g_string_equal(oldPrompt, newPrompt)){

            if(sw->display_name != NULL){
                g_free ( sw->display_name );
            }
            sw->display_name = g_strdup  ( newPrompt->str );
            // rofi_view_reload does not update prompt, that is why this is needed
            rofi_view_switch_mode ( state, sw );
        }

        if(!g_string_equal(oldInput, newInput)){

            RofiViewState * rofiViewState = rofi_view_get_active();
            rofi_view_clear_input(rofiViewState);
            rofi_view_handle_text(rofiViewState, newInput->str);
        }


        g_string_free(oldOverlay, TRUE);
        g_string_free(oldPrompt, TRUE);
        g_string_free(oldInput, TRUE);


        g_debug("reloading rofi view");

        rofi_view_reload();

    }

    return G_SOURCE_CONTINUE;
}

// spawn watch, called when child exited
static void on_child_status (GPid pid, gint status, gpointer context)
{
    g_message ("Child %" G_PID_FORMAT " exited %s", pid,
        g_spawn_check_exit_status (status, NULL) ? "normally" : "abnormally");
    Mode *sw = (Mode *) context;
    BlocksModePrivateData *data = mode_get_private_data_extended_mode( sw );
    g_spawn_close_pid (pid);
    if(data->close_on_child_exit){
          exit(0);    
    }
}


/************************
 extended mode methods
***********************/


static int blocks_mode_init ( Mode *sw )
{
    if ( mode_get_private_data ( sw ) == NULL ) {
        BlocksModePrivateData *pd = g_malloc0 ( sizeof ( *pd ) );
        mode_set_private_data ( sw, (void *) pd );
        pd->currentPageData = page_data_new();
        pd->input_format = g_string_new("{\"name\":\"{{name_escaped}}\", \"value\":\"{{value_escaped}}\"}");
        pd->input_action = InputAction__FILTER_USING_ROFI;
        pd->close_on_child_exit = TRUE;
        pd->cmd_pid = 0;
        pd->buffer = g_string_sized_new (1024);
        pd->active_line = g_string_sized_new (1024);
        char *cmd = NULL;
        if (find_arg_str(CmdArg__BLOCKS_WRAP, &cmd)) {
            GError *error = NULL;
            int cmd_input_fd;
            int cmd_output_fd;
            char **argv = NULL;
            if ( !g_shell_parse_argv ( cmd, NULL, &argv, &error ) ){
                fprintf(stderr, "Unable to parse cmdline options: %s\n", error->message);
                g_error_free ( error );
                return 0;
            }

            pd->cmd_pid = popen2(cmd, &cmd_input_fd, &cmd_output_fd);
            if (pd->cmd_pid <= 0){
                fprintf(stderr,"Unable to exec %s\n", cmd);
                exit(1);
            }
            g_strfreev(argv);

            pd->read_channel_fd = cmd_output_fd;
            pd->write_channel_fd = cmd_input_fd;

            int retval = fcntl( pd->read_channel_fd, F_SETFL, fcntl(pd->read_channel_fd, F_GETFL) | O_NONBLOCK);
            if (retval != 0){

                fprintf(stderr,"Error setting non block on output pipe\n");
                kill(pd->cmd_pid, SIGTERM);
                exit(1);
            }
            pd->read_channel = g_io_channel_unix_new(pd->read_channel_fd);
            pd->write_channel = g_io_channel_unix_new(pd->write_channel_fd);
            g_child_watch_add (pd->cmd_pid, on_child_status, sw);

        } else {
            int retval = fcntl( STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK);
            if (retval != 0){
                fprintf(stderr,"Error setting non block on output pipe\n");
                exit(1);
            }
            pd->read_channel = g_io_channel_unix_new(STDIN_FILENO);
            pd->write_channel = g_io_channel_unix_new(STDOUT_FILENO);
        }

        pd->read_channel_watcher = g_io_add_watch(pd->read_channel, G_IO_IN, on_new_input, sw);


        pd->parser = json_parser_new ();
    }
    return TRUE;
}
static unsigned int blocks_mode_get_num_entries ( const Mode *sw )
{
    g_debug("%s", "blocks_mode_get_num_entries");
    PageData * pageData = mode_get_private_data_current_page( sw );
    return pageData->lines->len;
}

static ModeMode blocks_mode_result ( Mode *sw, int mretv, char **input, unsigned int selected_line )
{
    ModeMode           retv  = MODE_EXIT;
    BlocksModePrivateData *data = mode_get_private_data_extended_mode( sw );
    PageData * pageData = data->currentPageData;

    if ( mretv & MENU_NEXT ) {
        retv = NEXT_DIALOG;
    } else if ( mretv & MENU_PREVIOUS ) {
        retv = PREVIOUS_DIALOG;
    } else if ( mretv & MENU_QUICK_SWITCH ) {
        retv = ( mretv & MENU_LOWER_MASK );
        int custom_key = retv%20 + 1;
        char str[8];
        snprintf(str, 8, "%d", custom_key);
        blocks_mode_private_data_write_to_channel(data, Event__CUSTOM_KEY, str);
        retv = RELOAD_DIALOG;
    } else if ( ( mretv & MENU_OK ) ) {
        LineData * lineData = &g_array_index (pageData->lines, LineData, selected_line);
        blocks_mode_private_data_write_to_channel(data, Event__SELECT_ENTRY, lineData->text);
        retv = RELOAD_DIALOG;
    } else if ( ( mretv & MENU_ENTRY_DELETE ) == MENU_ENTRY_DELETE ) {
        LineData * lineData = &g_array_index (pageData->lines, LineData, selected_line);
        blocks_mode_private_data_write_to_channel(data, Event__DELETE_ENTRY, lineData->text);
        retv = RELOAD_DIALOG;
    }
     else if ( ( mretv & MENU_CUSTOM_INPUT ) ) {
        blocks_mode_private_data_write_to_channel(data, Event__EXEC_CUSTOM_INPUT, *input);
        retv = RELOAD_DIALOG;
    }
    return retv;
}

static void blocks_mode_destroy ( Mode *sw )
{
    BlocksModePrivateData *data = mode_get_private_data_extended_mode( sw );
    if ( data != NULL ) {
        if(data->cmd_pid > 0){
            kill(data->cmd_pid, SIGTERM);
        }
        if ( data->read_channel_watcher > 0 ) {
            g_source_remove ( data->read_channel_watcher );
        }
        if ( data->parser ) {
            g_object_unref ( data->parser );
        }
        page_data_free ( data->currentPageData );
        close ( data->write_channel_fd );
        close ( data->read_channel_fd );
        g_free ( data->write_channel );
        g_free ( data->read_channel );
        g_free ( data );
        mode_set_private_data ( sw, NULL );
    }
}



static char * blocks_mode_get_display_value ( const Mode *sw, unsigned int selected_line, int *state, G_GNUC_UNUSED GList **attr_list, int get_entry )
{

    if(selected_line <= 0){
        g_debug("%s", "blocks_mode_get_display_value");

        /**
         *   Mode._preprocess_input is not called when input is empty,
         * the only method called when the input changes to empty is this one
         * that is reason the following 3 lines are added.
         */
        RofiViewState * rofiViewState = rofi_view_get_active();
        if(rofiViewState != NULL){
            BlocksModePrivateData *data = mode_get_private_data_extended_mode( sw );
            blocks_mode_verify_input_change(data, rofi_view_get_user_input(rofiViewState));
        }
    }
    PageData * pageData = mode_get_private_data_current_page( sw );
    LineData * lineData = &g_array_index (pageData->lines, LineData, selected_line);
    *state |= 
        1 * lineData->urgent +
        2 * lineData->highlight +
        8 * lineData->markup;
    return get_entry ? g_strdup(lineData->text) : NULL;
}

static int blocks_mode_token_match ( const Mode *sw, rofi_int_matcher **tokens, unsigned int selected_line )
{
    if(selected_line <= 0){
        g_debug("%s", "blocks_mode_token_match");
    }
    BlocksModePrivateData *data = mode_get_private_data_extended_mode( sw );
    PageData * pageData = data->currentPageData;
    switch(data->input_action){
        case InputAction__SEND_ACTION: return TRUE;
    }
    LineData * lineData = &g_array_index (pageData->lines, LineData, selected_line);
    return helper_token_match ( tokens, lineData->text);
}

static char * blocks_mode_get_message ( const Mode *sw )
{
    g_debug("%s", "blocks_mode_get_message");
    BlocksModePrivateData *data = mode_get_private_data_extended_mode( sw );
    PageData * pageData = mode_get_private_data_current_page( sw );
    gchar* result = g_strdup(pageData->message->str);
    return result;
}

static char * blocks_mode_preprocess_input ( Mode *sw, const char *input )
{
    g_debug("%s", "blocks_mode_preprocess_input");
    BlocksModePrivateData *data = mode_get_private_data_extended_mode( sw );
    blocks_mode_verify_input_change(data, input);
    return g_strdup(input);
}

Mode mode =
{
    .abi_version        = ABI_VERSION,
    .name               = "blocks",
    .cfg_name_key       = "display-blocks",
    ._init              = blocks_mode_init,
    ._get_num_entries   = blocks_mode_get_num_entries,
    ._result            = blocks_mode_result,
    ._destroy           = blocks_mode_destroy,
    ._token_match       = blocks_mode_token_match,
    ._get_display_value = blocks_mode_get_display_value,
    ._get_message       = blocks_mode_get_message,
    ._get_completion    = NULL,
    ._preprocess_input  = blocks_mode_preprocess_input,
    .private_data       = NULL,
    .free               = NULL,
};
