/**
 * rofi-top
 *
 * MIT/X11 License
 * Copyright (c) 2017 Qball Cow <qball@gmpclient.org>
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

G_MODULE_EXPORT Mode mode;

const gchar* EXTENDED_SCRIPT_OPTION = "extended-script-file";
const gchar* EMPTY_STRING = "";


typedef struct
{
    const gchar *text;
    gboolean urgent;
    gboolean highlight;
} LineData;

typedef struct
{
    RofiDistance windowWidth;
    GString *message;
    GString *overlay;
    GString *prompt;
    GArray *lines;
} PageData;

typedef struct
{
    PageData * currentPageData;

    JsonParser *parser;
    JsonObject *root;
    GError *error;
    GString * active_line;
    GString * buffer;
    
    char *cmd;
    pid_t cmd_pid;
    int cmd_input_fd;
    int cmd_output_fd;
    GIOChannel * cmd_input_fd_io_channel;
    GIOChannel * cmd_output_fd_io_channel;
    guint cmd_output_fd_io_channel_watcher;


} ExtendedModePrivateData;

/**************
  utils
***************/

typedef struct RofiViewState RofiViewState;
void rofi_view_switch_mode ( RofiViewState *state, Mode *mode );
RofiViewState * rofi_view_get_active ( void );
extern void rofi_view_set_overlay(RofiViewState * state, const char *text);
extern void rofi_view_reload ( void );


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
        printf("{\"message\":\"Error loading %s:%s\"}\n", command, strerror(errno));
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

/***********************
    Page data methods
************************/

PageData * page_data_new(){
    PageData * pageData = g_malloc0( sizeof ( *pageData ) );
    pageData->message = g_string_sized_new(256);
    pageData->overlay = g_string_sized_new(64);
    pageData->prompt = g_string_sized_new(64);
    pageData->lines = g_array_new (FALSE, TRUE, sizeof (LineData));
    return pageData;
}

void page_data_free(PageData * pageData){
    g_string_free(pageData->message, TRUE);
    g_string_free(pageData->overlay, TRUE);
    g_string_free(pageData->prompt, TRUE);
    g_free(pageData);
}

void page_data_add_line(PageData * pageData, const gchar * label, gboolean urgent, gboolean highlight){
    LineData line = { .text = label, .urgent = urgent, .highlight = highlight };
    g_array_append_val(pageData->lines, line);
}

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

void page_data_add_line_json_node(PageData * pageData, JsonNode * element){
    if(JSON_NODE_HOLDS_VALUE(element) && json_node_get_value_type(element) == G_TYPE_STRING){
        page_data_add_line(pageData, json_node_get_string(element), FALSE, FALSE);
    } else if(JSON_NODE_HOLDS_OBJECT(element)){
        JsonObject * line_obj = json_node_get_object(element);
        JsonNode * text_node = json_object_get_member(line_obj, "text");
        JsonNode * urgent_node = json_object_get_member(line_obj, "urgent");
        JsonNode * highlight_node = json_object_get_member(line_obj, "highlight");
        const gchar * text = json_node_get_string_or_else(text_node, EMPTY_STRING);
        gboolean urgent = json_node_get_boolean_or_else(urgent_node, FALSE);
        gboolean highlight = json_node_get_boolean_or_else(highlight_node, FALSE);
        page_data_add_line(pageData, text, urgent, highlight);
    }
}

void page_data_clear_lines(PageData * pageData){
    g_array_set_size(pageData->lines, 0);
}

/**************************************
  extended mode pirvate data methods
**************************************/

static void extended_mode_private_data_assign_string_to_root_member(ExtendedModePrivateData * data, GString * str, const char * member){
    JsonNode* memberNode = json_object_get_member(data->root, member);
    const gchar* memberVal = json_node_get_string_or_else(memberNode, EMPTY_STRING);
    g_string_assign(str,memberVal);

}

static void extended_mode_private_data_update_page(ExtendedModePrivateData * data){
    GError * error = NULL;
    json_parser_load_from_data(data->parser,data->active_line->str,data->active_line->len,&error);
    data->root = json_node_get_object(json_parser_get_root(data->parser));
    JsonObject *root = data->root;
    PageData * pageData = data->currentPageData;

    extended_mode_private_data_assign_string_to_root_member(data, pageData->message, "message");
    extended_mode_private_data_assign_string_to_root_member(data, pageData->overlay, "overlay");
    extended_mode_private_data_assign_string_to_root_member(data, pageData->prompt, "prompt");

    const char * OVERLAY_PROP = "overlay";
    JsonNode* overlayNode = json_object_get_member(data->root, OVERLAY_PROP);
    const gchar* overlay = json_node_get_string_or_else(overlayNode, EMPTY_STRING);
    g_string_assign(pageData->overlay,overlay);


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

/**************************
  mode extension methods
**************************/

static ExtendedModePrivateData * mode_get_private_data_extended_mode(const Mode *sw){
    return (ExtendedModePrivateData *) mode_get_private_data( sw );
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

    ExtendedModePrivateData *data = mode_get_private_data_extended_mode( sw );

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
        
        extended_mode_private_data_update_page(data);
        
        GString * newOverlay = data->currentPageData->overlay;
        GString * newPrompt = data->currentPageData->prompt;

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
        g_string_free(oldOverlay, TRUE);
        g_string_free(oldPrompt, TRUE);
    }
    rofi_view_reload();

    return G_SOURCE_CONTINUE;
}



/************************
 extended mode methods
***********************/

static int extended_mode_init ( Mode *sw )
{
    if ( mode_get_private_data ( sw ) == NULL ) {
        ExtendedModePrivateData *pd = g_malloc0 ( sizeof ( *pd ) );
        mode_set_private_data ( sw, (void *) pd );
        pd->currentPageData = page_data_new();

        char *cmd = NULL;
        if (find_arg_str(EXTENDED_SCRIPT_OPTION, &cmd)) {
            pd->cmd = g_strdup(cmd);
            pd->cmd_pid = popen2(pd->cmd, &pd->cmd_input_fd, &pd->cmd_output_fd);
             if (pd->cmd_pid <= 0){
                printf("Unable to exec %s\n", cmd);
                exit(1);
            }
            int retval = fcntl( pd->cmd_output_fd, F_SETFL, fcntl(pd->cmd_output_fd, F_GETFL) | O_NONBLOCK);
            if (retval != 0){
                printf("Error setting non block on output pipe\n");
                kill(pd->cmd_pid, SIGTERM);
                exit(1);
            }
            pd->cmd_input_fd_io_channel = g_io_channel_unix_new(pd->cmd_input_fd);
            pd->cmd_output_fd_io_channel = g_io_channel_unix_new(pd->cmd_output_fd);
            pd->cmd_output_fd_io_channel_watcher = g_io_add_watch(pd->cmd_output_fd_io_channel, G_IO_IN, on_new_input, sw);

            pd->buffer = g_string_sized_new (1024);
            pd->active_line = g_string_sized_new (1024);
        }

        pd->parser = json_parser_new ();
    }
    return TRUE;
}
static unsigned int extended_mode_get_num_entries ( const Mode *sw )
{
    PageData * pageData = mode_get_private_data_current_page( sw );
    return pageData->lines->len;
}

static ModeMode extended_mode_result ( Mode *sw, int mretv, char **input, unsigned int selected_line )
{
    ModeMode           retv  = MODE_EXIT;
    ModeMode           resetDialog  = RESET_DIALOG;
    ExtendedModePrivateData *rmpd = mode_get_private_data_extended_mode( sw );

    if ( mretv & MENU_NEXT ) {
        retv = NEXT_DIALOG;
    } else if ( mretv & MENU_PREVIOUS ) {
        retv = PREVIOUS_DIALOG;
    } else if ( mretv & MENU_QUICK_SWITCH ) {
        retv = RESET_DIALOG;
    } else if ( ( mretv & MENU_OK ) ) {

        retv = RESET_DIALOG;
    } else if ( ( mretv & MENU_ENTRY_DELETE ) == MENU_ENTRY_DELETE ) {
        retv = RESET_DIALOG;
    }
    printf("%d\n", retv);
    return retv;
}

static void extended_mode_destroy ( Mode *sw )
{
    ExtendedModePrivateData *data = mode_get_private_data_extended_mode( sw );
    if ( data != NULL ) {
        g_source_remove ( data->cmd_output_fd_io_channel_watcher );

        g_object_unref ( data->parser );
        close( data->cmd_input_fd );
        close( data->cmd_output_fd );
        page_data_free ( data->currentPageData );
        g_free ( data->cmd_input_fd_io_channel );
        g_free ( data->cmd_output_fd_io_channel );

        g_free ( data );
        mode_set_private_data ( sw, NULL );
        kill(data->cmd_pid, SIGTERM);
    }
}



static char * extended_mode_get_display_value ( const Mode *sw, unsigned int selected_line, int *state, G_GNUC_UNUSED GList **attr_list, int get_entry )
{
    PageData * pageData = mode_get_private_data_current_page( sw );
    LineData * lineData = &g_array_index (pageData->lines, LineData, selected_line);
    *state |= 
        1 * lineData->urgent +
        2 * lineData->highlight;
    return get_entry ? g_strdup_printf("%s",lineData->text) : NULL;
}

static int extended_mode_token_match ( const Mode *sw, rofi_int_matcher **tokens, unsigned int selected_line )
{
    PageData * pageData = mode_get_private_data_current_page( sw );
    LineData * lineData = &g_array_index (pageData->lines, LineData, selected_line);
    return helper_token_match ( tokens, lineData->text);
}

static char * extended_mode_get_message ( const Mode *sw )
{
    PageData * pageData = mode_get_private_data_current_page( sw );
    gchar* result = g_strdup_printf("%s",pageData->message->str);
    return result;
}

Mode mode =
{
    .abi_version        = ABI_VERSION,
    .name               = "extended-script",
    .cfg_name_key       = "display-extended",
    ._init              = extended_mode_init,
    ._get_num_entries   = extended_mode_get_num_entries,
    ._result            = extended_mode_result,
    ._destroy           = extended_mode_destroy,
    ._token_match       = extended_mode_token_match,
    ._get_display_value = extended_mode_get_display_value,
    ._get_message       = extended_mode_get_message,
    ._get_completion    = NULL,
    ._preprocess_input  = NULL,
    .private_data       = NULL,
    .free               = NULL,
};