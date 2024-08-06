// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2020 Omar Castro
#include <stdio.h>
#include "json_glib_extensions.h"
#include "blocks_mode_data.h"


const char * const input_action_names[2] = {
    "send",
    "filter"
};

static const size_t NUM_OF_INPUT_ACTIONS = (sizeof(input_action_names) / sizeof((input_action_names)[0]));
static const char * UNDEFINED = "";


static void blocks_mode_private_data_update_string(BlocksModePrivateData * data, GString ** str, const char * json_root_member);
static void blocks_mode_private_data_update_input_action(BlocksModePrivateData * data);
static void blocks_mode_private_data_update_message(BlocksModePrivateData * data);
static void blocks_mode_private_data_update_overlay(BlocksModePrivateData * data);
static void blocks_mode_private_data_update_prompt(BlocksModePrivateData * data);
static void blocks_mode_private_data_update_input(BlocksModePrivateData * data);
static void blocks_mode_private_data_update_input_format(BlocksModePrivateData * data);
static void blocks_mode_private_data_update_close_on_child_exit(BlocksModePrivateData * data);
static void blocks_mode_private_data_update_lines(BlocksModePrivateData * data);
static void blocks_mode_private_data_update_focus_entry(BlocksModePrivateData * data);



BlocksModePrivateData * blocks_mode_private_data_new(void){
    BlocksModePrivateData *pd = g_malloc0 ( sizeof ( *pd ) );
    pd->currentPageData = page_data_new();
    pd->currentPageData->markup_default = MarkupStatus_UNDEFINED;
    pd->input_format = g_string_new("{\"name\":\"{{name_escaped}}\", \"value\":\"{{value_escaped}}\", \"data\":\"{{data_escaped}}\"}");
    pd->entry_to_focus = -1;
    pd->input_action = InputAction__FILTER_USING_ROFI;
    pd->close_on_child_exit = TRUE;
    pd->cmd_pid = 0;
    pd->buffer = g_string_sized_new (1024);
    pd->active_line = g_string_sized_new (1024);
    pd->waiting_for_idle = FALSE;
    pd->parser = json_parser_new ();
    pd->paintNumber = -1;
    return pd;
}

void blocks_mode_private_data_update_destroy( BlocksModePrivateData * data ){
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
}




void blocks_mode_private_data_update_page(BlocksModePrivateData * data){
    GError * error = NULL;

    if(! json_parser_load_from_data(data->parser,data->active_line->str,data->active_line->len,&error)){
        fprintf(stderr, "Unable to parse line: %s\n", error->message);
        g_error_free ( error );
        return;
    }
    

    data->root = json_node_get_object(json_parser_get_root(data->parser));

    blocks_mode_private_data_update_input_action(data);
    blocks_mode_private_data_update_message(data);
    blocks_mode_private_data_update_overlay(data);
    blocks_mode_private_data_update_input(data);
    blocks_mode_private_data_update_prompt(data);
    blocks_mode_private_data_update_close_on_child_exit(data);
    blocks_mode_private_data_update_input_format(data);
    blocks_mode_private_data_update_lines(data);
    blocks_mode_private_data_update_focus_entry(data);
    data->paintNumber = 0;
}


static void blocks_mode_private_data_update_string(BlocksModePrivateData * data, GString ** str, const char * json_root_member){
    const gchar* memberVal = json_object_get_string_member_or_else(data->root, json_root_member, NULL);
    if(memberVal != NULL){
        if(*str == NULL){
            *str = g_string_sized_new(64);
        }
        g_string_assign(*str,memberVal);
    }
}

static void blocks_mode_private_data_update_input_action(BlocksModePrivateData * data){
    const gchar* input_action = json_object_get_string_member_or_else(data->root, "input action", NULL);
    if(input_action != NULL){
        for (size_t i = 0; i < NUM_OF_INPUT_ACTIONS; ++i)
        {
            if(g_strcmp0(input_action, input_action_names[i]) == 0){
                data->input_action = (InputAction) i;
            }
        }
    }
}


static void blocks_mode_private_data_update_message(BlocksModePrivateData * data){
    const gchar* memberVal = json_object_get_nullable_string_member_or_else(data->root, "message", UNDEFINED);
    if(memberVal != UNDEFINED){
        page_data_set_message(data->currentPageData, memberVal);
    }
}

static void blocks_mode_private_data_update_overlay(BlocksModePrivateData * data){
    const gchar* memberVal = json_object_get_nullable_string_member_or_else(data->root, "overlay", UNDEFINED);
    if(memberVal != UNDEFINED){
        page_data_set_overlay(data->currentPageData, memberVal);
    }
}

static void blocks_mode_private_data_update_prompt(BlocksModePrivateData * data){
    blocks_mode_private_data_update_string(data, &data->currentPageData->prompt, "prompt");
}

static void blocks_mode_private_data_update_input(BlocksModePrivateData * data){
    blocks_mode_private_data_update_string(data, &data->currentPageData->input, "input");
}

static void blocks_mode_private_data_update_input_format(BlocksModePrivateData * data){
    blocks_mode_private_data_update_string(data, &data->input_format, "event format");
}

static void blocks_mode_private_data_update_focus_entry(BlocksModePrivateData * data){
    data->entry_to_focus = json_object_get_int_member_or_else(data->root, "active entry", -1);
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
        guint len = json_array_get_length(lines);
        for(guint index = 0; index < len; ++index){
            page_data_add_line_json_node(pageData, json_array_get_element(lines, index));
        }
    }
}
