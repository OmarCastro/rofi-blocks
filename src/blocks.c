// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2020 Omar Castro

#define G_LOG_DOMAIN    "BlocksMode"


#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>

#include <rofi/mode.h>
#include <rofi/helper.h>
#include <rofi/mode-private.h>
#include <rofi/rofi-icon-fetcher.h>

#include <glib-object.h>
#include <json-glib/json-glib.h>

#include <stdint.h>

#include "string_utils.h"
#include "render_state.h"
#include "page_data.h"
#include "json_glib_extensions.h"
#include "blocks_mode_data.h"

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
const gchar* CmdArg__BLOCKS_PROMPT = "-blocks-prompt";
const gchar* CmdArg__MARKUP_ROWS = "-markup-rows";

static const gchar* EMPTY_STRING = "";

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

/**************
 rofi extension
****************/

unsigned int blocks_mode_rofi_view_get_current_position(RofiViewState * rofiViewState, PageData * pageData){
    unsigned int next_position = rofi_view_get_next_position(rofiViewState);
    unsigned int length = page_data_get_number_of_lines(pageData);
    if(next_position <= 0 || next_position >= UINT32_MAX - 10) {
        return length - 1;
    } else {
        return next_position - 1;
    }
}


/**************************************
  extended mode pirvate data methods
**************************************/



void blocks_mode_private_data_write_to_channel ( BlocksModePrivateData * data, Event event, const char * action_value){
        GIOChannel * write_channel = data->write_channel;
        if(data->write_channel == NULL){
            //gets here when the script exits or there was an error loading it
            return;
        }
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
    bool inputChanged = g_strcmp0(inputStr->str, new_input_value) != 0;
    if(inputChanged){
        g_string_assign(inputStr, new_input_value);
        if(data->input_action == InputAction__SEND_ACTION ){
            blocks_mode_private_data_write_to_channel(data, Event__INPUT_CHANGE, new_input_value);
        }
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
        GString * oldOverlay = g_string_new(page_data_get_overlay_or_empty_string(data->currentPageData));
        GString * prompt = data->currentPageData->prompt;
        GString * input = data->currentPageData->input;

        GString * oldPrompt = prompt == NULL ? NULL : g_string_new(prompt->str);
        GString * oldInput = input == NULL ? NULL : g_string_new(input->str);

        blocks_mode_private_data_update_page(data);
        

        GString * newOverlay = g_string_new(page_data_get_overlay_or_empty_string(data->currentPageData));
        GString * newPrompt = data->currentPageData->prompt;
        GString * newInput = data->currentPageData->input;

        bool overlayChanged = !g_string_equal(oldOverlay, newOverlay);
        bool promptChanged = newPrompt != NULL && (oldPrompt == NULL || !g_string_equal(oldPrompt, newPrompt));
        bool inputChanged = newInput != NULL && (oldInput == NULL || !g_string_equal(oldInput, newInput));


        if(overlayChanged){
            RofiViewState * state = rofi_view_get_active();
            rofi_view_set_overlay(state, (newOverlay->len > 0) ? newOverlay->str : NULL);
        }

        if(promptChanged){
            g_free ( sw->display_name );
            sw->display_name = g_strdup ( newPrompt->str );
            // rofi_view_reload does not update prompt, that is why this is needed
            rofi_view_switch_mode ( state, sw );
        }

        if(inputChanged){
            RofiViewState * rofiViewState = rofi_view_get_active();
            rofi_view_clear_input(rofiViewState);
            rofi_view_handle_text(rofiViewState, newInput->str);
        }


        g_string_free(oldOverlay, TRUE);
        g_string_free(newOverlay, TRUE);
        oldPrompt != NULL && g_string_free(oldPrompt, TRUE);
        oldInput != NULL && g_string_free(oldInput, TRUE);


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
    data->write_channel = NULL;
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
        g_debug("%s %i", "on_render.active line", blocks_mode_rofi_view_get_current_position(rofiViewState, data->currentPageData));
    

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
        BlocksModePrivateData *pd = blocks_mode_private_data_new();
        mode_set_private_data ( sw, (void *) pd );
        char *cmd = NULL;
        if (find_arg_str(CmdArg__MARKUP_ROWS, &cmd)) {
            pd->currentPageData->markup_default = MarkupStatus_ENABLED;
        }

        char *prompt = NULL;
        if (find_arg_str(CmdArg__BLOCKS_PROMPT, &prompt)) {
            sw->display_name = g_strdup ( prompt );
        }

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

            if ( ! g_spawn_async_with_pipes ( NULL, argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH, NULL, NULL, &(pd->cmd_pid), &(cmd_input_fd), &(cmd_output_fd), NULL, &error)) {
                fprintf(stderr, "Unable to exec %s\n", error->message);
                char buffer[1024];
                int result = 4;
                char * cmd_escaped = str_new_escaped_for_json_string(cmd);
                char * error_message_escaped = str_new_escaped_for_json_string(error->message);
                snprintf(buffer, sizeof(buffer), 
                    "{\"close on exit\": false, \"message\":\"Error loading %s:%s\"}\n", 
                    cmd_escaped,
                    error_message_escaped
                );
                fprintf(stderr, "message:  %s\n", buffer);

                g_string_assign(pd->active_line, buffer);
                blocks_mode_private_data_update_page(pd);
                g_error_free ( error );
                g_free(cmd_escaped);
                g_free(error_message_escaped);
                return TRUE;
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
    }
    return TRUE;
}
static unsigned int blocks_mode_get_num_entries ( const Mode *sw )
{
    g_debug("%s", "blocks_mode_get_num_entries");
    PageData * pageData = mode_get_private_data_current_page( sw );
    return page_data_get_number_of_lines(pageData);
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
        blocks_mode_private_data_update_destroy(data);
        mode_set_private_data ( sw, NULL );
    }
}

 static cairo_surface_t * blocks_mode_get_icon ( const Mode *sw, unsigned int selected_line, int height ){
    PageData * pageData = mode_get_private_data_current_page( sw );
    LineData * lineData = page_data_get_line_by_index_or_else(pageData, selected_line, NULL);
    if(lineData == NULL){
        return NULL;
    }

    const gchar * icon = lineData->icon;
    if(icon == NULL || icon[0] == '\0'){
        return NULL;
    }

    if(lineData->icon_fetch_uid <= 0){
        lineData->icon_fetch_uid = rofi_icon_fetcher_query ( icon, height );
    } 
    return rofi_icon_fetcher_get ( lineData->icon_fetch_uid );
 }



static char * blocks_mode_get_display_value ( const Mode *sw, unsigned int selected_line, int *state, G_GNUC_UNUSED GList **attr_list, int get_entry )
{
    BlocksModePrivateData *data = mode_get_private_data_extended_mode( sw );
    if(!data->waiting_for_idle){
        data->waiting_for_idle = TRUE;
        g_idle_add (on_render_callback, (void *) sw);
    }

    PageData * pageData = mode_get_private_data_current_page( sw );
    LineData * lineData = page_data_get_line_by_index_or_else(pageData, selected_line, NULL);
    if(lineData == NULL){
        *state |= 16;
        return get_entry ? g_strdup("") : NULL;
    }

    *state |= 
        1 * lineData->urgent +
        2 * lineData->highlight +
        8 * lineData->markup;
    return get_entry ? g_strdup(lineData->text) : NULL;
}

static int blocks_mode_token_match ( const Mode *sw, rofi_int_matcher **tokens, unsigned int selected_line )
{
    BlocksModePrivateData *data = mode_get_private_data_extended_mode( sw );
    PageData * pageData = data->currentPageData;
    LineData * lineData = page_data_get_line_by_index_or_else(pageData, selected_line, NULL);
    if(lineData == NULL){
        return FALSE;
    }

    if(data->input_action == InputAction__SEND_ACTION){
        return TRUE;
    }
    return helper_token_match ( tokens, lineData->text);
}

static char * blocks_mode_get_message ( const Mode *sw )
{
    g_debug("%s", "blocks_mode_get_message");
    PageData * pageData = mode_get_private_data_current_page( sw );
    gchar* result = page_data_is_message_empty(pageData) ? NULL : g_strdup(page_data_get_message_or_empty_string(pageData));
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
    ._get_icon          = blocks_mode_get_icon,
    ._get_display_value = blocks_mode_get_display_value,
    ._get_message       = blocks_mode_get_message,
    ._get_completion    = NULL,
    ._preprocess_input  = blocks_mode_preprocess_input,
    .private_data       = NULL,
    .free               = NULL,
};
