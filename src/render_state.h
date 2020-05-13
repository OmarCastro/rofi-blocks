// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2020 Omar Castro


#include <stdbool.h>

typedef struct RenderState RenderState;


RenderState * render_state_new();

void render_state_destroy( RenderState * state);

bool render_state_has_selected_line_rendered( RenderState * state ); 

bool render_state_has_last_line_rendered( RenderState * state ); 

unsigned int render_state_get_last_active( RenderState * state ); 

unsigned int render_state_get_current_active( RenderState * state ); 

void render_state_set_current_active( RenderState * state, unsigned int active_line ); 

void render_state_prepare_render( RenderState * state ); 