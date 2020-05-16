// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2020 Omar Castro
#ifndef ROFI_BLOCKS_MODE_DATA_H
#define ROFI_BLOCKS_MODE_DATA_H
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <gmodule.h>
#include <time.h>


#include <glib-object.h>
#include <json-glib/json-glib.h>

#include <stdint.h>

#include "string_utils.h"
#include "render_state.h"
#include "page_data.h"
#include "json_glib_extensions.h"

typedef enum {
    InputAction__SEND_ACTION,
    InputAction__FILTER_USING_ROFI
} InputAction;

const char *input_action_names;
const size_t NUM_OF_INPUT_ACTIONS;


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

} BlocksModePrivateData;



void blocks_mode_private_data_update_page(BlocksModePrivateData * data);

#endif // ROFI_BLOCKS_MODE_DATA_H


