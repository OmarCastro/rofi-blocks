// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2020 Omar Castro
#ifndef ROFI_BLOCKS_MODE_DATA_H
#define ROFI_BLOCKS_MODE_DATA_H
#include <unistd.h>
#include <gmodule.h>


#include <glib-object.h>
#include <json-glib/json-glib.h>

#include <stdint.h>

#include "render_state.h"
#include "page_data.h"

typedef enum {
    InputAction__SEND_ACTION,
    InputAction__FILTER_USING_ROFI
} InputAction;

extern const char * const input_action_names[2];

typedef struct
{
    PageData * currentPageData;
    GString * input_format;
    InputAction input_action;
    gint64 entry_to_focus;

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
    long paintNumber;

} BlocksModePrivateData;

BlocksModePrivateData * blocks_mode_private_data_new(void);

void blocks_mode_private_data_update_destroy( BlocksModePrivateData * data );

void blocks_mode_private_data_update_page(BlocksModePrivateData * data);

#endif // ROFI_BLOCKS_MODE_DATA_H


