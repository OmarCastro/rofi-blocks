// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2020 Omar Castro

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

#include "string_utils.h"
#include "render_state.h"
#include "page_data.h"
#include "json_glib_extensions.h"

typedef struct RofiViewState RofiViewState;
void rofi_view_switch_mode ( RofiViewState *state, Mode *mode );
RofiViewState * rofi_view_get_active ( void );
extern void rofi_view_set_overlay(RofiViewState * state, const char *text);
extern void rofi_view_reload ( void );
const char * rofi_view_get_user_input ( const RofiViewState *state );
unsigned int rofi_view_get_selected_line ( const RofiViewState *state );
void rofi_view_set_selected_line ( const RofiViewState *state, unsigned int selected_line );
unsigned int rofi_view_get_next_position ( const RofiViewState *state );
void rofi_view_handle_text ( RofiViewState *state, char *text );
void rofi_view_clear_input ( RofiViewState *state );
G_MODULE_EXPORT Mode mode;


const gchar* CmdArg__BLOCKS_WRAP = "-blocks-wrap";
const gchar* CmdArg__MARKUP_ROWS = "-markup-rows";

static const gchar* EMPTY_STRING = "";

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
    Event__ACTIVE_ENTRY,
    Event__SELECT_ENTRY, 
    Event__DELETE_ENTRY, 
    Event__EXEC_CUSTOM_INPUT
} Event;

static const char *event_enum_labels[] = {
    "INPUT_CHANGE",
    "CUSTOM_KEY",
    "ACTIVE_ENTRY", 
    "SELECT_ENTRY", 
    "DELETE_ENTRY", 
    "EXEC_CUSTOM_INPUT"
};

static const char *event_labels[] = {
    "input change",
    "custom key",
    "active entry",
    "select entry",
    "delete entry",
    "execute custom input"
};

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
    gboolean waiting_for_idle;

    RenderState * render_state;

    guint previous_active_line;

} BlocksModePrivateData;


/**************
 rofi extension
****************/

unsigned int blocks_mode_rofi_view_get_active_line(RofiViewState * rofiViewState, BlocksModePrivateData * data){
    unsigned int selected_line = rofi_view_get_selected_line(rofiViewState);
    unsigned int next_position = rofi_view_get_next_position(rofiViewState);
    unsigned int previous_active_line = data->previous_active_line;
    unsigned int length = data->currentPageData->lines->len;


    if(next_position == 0) {
        data->previous_active_line = length - 1;
    } else {
        data->previous_active_line = next_position - 1;
    }
    return data->previous_active_line; 
}

/**************
  utils
***************/


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
        execl(command, (char *) NULL);
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


/***********************
    Page data methods
************************/



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
    gunichar unichar;
    GIOStatus status;

    status = g_io_channel_read_unichar(source, &unichar, &error);

    //when there is nothing to read, status is G_IO_STATUS_AGAIN
    while(status == G_IO_STATUS_NORMAL) {
        g_string_append_unichar(buffer, unichar);
        if( unichar == '\n' ){
            if(buffer->len > 1){ //input is not an empty line
                g_debug("received new line: %s", buffer->str);
                g_string_assign(active_line, buffer->str);
                newline=TRUE;
            }
            g_string_set_size(buffer, 0);
        }
        status = g_io_channel_read_unichar(source, &unichar, &error);
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

// idle, called after rendering
static void on_render(gpointer context){
    g_debug("%s", "calling on_render");

    Mode *sw = (Mode *) context;
    BlocksModePrivateData *data = mode_get_private_data_extended_mode( sw );
    RofiViewState * rofiViewState = rofi_view_get_active();

    /**
     *   Mode._preprocess_input is not called when input is empty,
     * the only method called when the input changes to empty is blocks_mode_get_display_value
     * which later this method is called, that is reason the following 3 lines are added.
     */
    if(rofiViewState != NULL){
        blocks_mode_verify_input_change(data, rofi_view_get_user_input(rofiViewState));
        //if()

        g_debug("%s %i", "on_render.selected line", rofi_view_get_selected_line(rofiViewState));
        g_debug("%s %i", "on_render.next pos", rofi_view_get_next_position(rofiViewState));
        g_debug("%s %i", "on_render.active line", blocks_mode_rofi_view_get_active_line(rofiViewState, data));
    

    }

}

// function used on g_idle_add, it is here to guarantee that this is called once
// each time the mode content is rendered
static gboolean on_render_callback(gpointer context){
    on_render(context);
    Mode *sw = (Mode *) context;
    BlocksModePrivateData *data = mode_get_private_data_extended_mode( sw );
    data->waiting_for_idle = FALSE;
    return FALSE;
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
        pd->currentPageData->markup_default = find_arg(CmdArg__MARKUP_ROWS) >= 0 ? TRUE : FALSE;
        pd->input_format = g_string_new("{\"name\":\"{{name_escaped}}\", \"value\":\"{{value_escaped}}\"}");
        pd->input_action = InputAction__FILTER_USING_ROFI;
        pd->close_on_child_exit = TRUE;
        pd->cmd_pid = 0;
        pd->buffer = g_string_sized_new (1024);
        pd->active_line = g_string_sized_new (1024);
        pd->waiting_for_idle = FALSE;
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
        if(selected_line >= pageData->lines->len){ return RELOAD_DIALOG; }

        retv = ( mretv & MENU_LOWER_MASK );
        int custom_key = retv%20 + 1;
        char str[8];
        snprintf(str, 8, "%d", custom_key);

        LineData * lineData = &g_array_index (pageData->lines, LineData, selected_line);
        blocks_mode_private_data_write_to_channel(data, Event__ACTIVE_ENTRY, lineData->text);
        blocks_mode_private_data_write_to_channel(data, Event__CUSTOM_KEY, str);

        retv = RELOAD_DIALOG;
    } else if ( ( mretv & MENU_OK ) ) {
        if(selected_line >= pageData->lines->len){ return RELOAD_DIALOG; }
        LineData * lineData = &g_array_index (pageData->lines, LineData, selected_line);
        blocks_mode_private_data_write_to_channel(data, Event__SELECT_ENTRY, lineData->text);
        retv = RELOAD_DIALOG;
    } else if ( ( mretv & MENU_ENTRY_DELETE ) == MENU_ENTRY_DELETE ) {
        if(selected_line >= pageData->lines->len){ return RELOAD_DIALOG; }
        LineData * lineData = &g_array_index (pageData->lines, LineData, selected_line);
        blocks_mode_private_data_write_to_channel(data, Event__DELETE_ENTRY, lineData->text);
        retv = RELOAD_DIALOG;
    } else if ( ( mretv & MENU_CUSTOM_INPUT ) ) {
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
        page_data_destroy ( data->currentPageData );
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
    BlocksModePrivateData *data = mode_get_private_data_extended_mode( sw );
    if(!data->waiting_for_idle){
        data->waiting_for_idle = TRUE;
        g_idle_add (on_render_callback, sw);
    }

    PageData * pageData = mode_get_private_data_current_page( sw );
    if(pageData->lines->len <= selected_line){
        *state |= 16;
        return get_entry ? g_strdup("") : NULL;
    }


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
    
    if(pageData->lines->len <= selected_line){
        return FALSE;
    }

    if(data->input_action == InputAction__SEND_ACTION){
        return TRUE;
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
