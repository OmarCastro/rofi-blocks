// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2020 Omar Castro

#include <gmodule.h>
#include "render_state.h"



struct RenderState {
	bool rendered_selected; 
	bool rendered_last; 
	unsigned int last_active;
	unsigned int current_active;
};


RenderState * render_state_new(void){
    RenderState *state = g_malloc0 ( sizeof ( *state ) );
    return state;
}

void render_state_destroy( RenderState * state){
	g_free(state);
}

bool render_state_has_selected_line_rendered( RenderState * state ){
	return state->rendered_selected;
}

bool render_state_has_last_line_rendered( RenderState * state ){
	return state->rendered_last;
}

unsigned int render_state_get_last_active( RenderState * state ){
	return state->last_active;
}

unsigned int render_state_get_current_active( RenderState * state ){
	return state->current_active;
} 

void render_state_set_current_active( RenderState * state, unsigned int active_line ){
	if( state->current_active == active_line ){
		return;
	}
	state->last_active = state->current_active;
	state->current_active = active_line;
}

void render_state_prepare_render( RenderState * state ){
	state->rendered_selected = FALSE;
	state->rendered_last = FALSE;
}
