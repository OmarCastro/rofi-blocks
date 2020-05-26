// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2020 Omar Castro


#include "json_glib_extensions.h"
#include "page_data.h"

static const gchar* EMPTY_STRING = "";

PageData * page_data_new(){
    PageData * pageData = g_malloc0( sizeof ( *pageData ) );
    pageData->message = g_string_sized_new(256);
    pageData->overlay = g_string_sized_new(64);
    pageData->prompt = g_string_sized_new(64);
    pageData->input = g_string_sized_new(64);
    pageData->lines = g_array_new (FALSE, TRUE, sizeof (LineData));
    return pageData;
}

void page_data_destroy(PageData * pageData){
    page_data_clear_lines(pageData);
    g_string_free(pageData->message, TRUE);
    g_string_free(pageData->overlay, TRUE);
    g_string_free(pageData->prompt, TRUE);
    g_string_free(pageData->input, TRUE);
    g_array_free (pageData->lines, TRUE);
    g_free(pageData);
}


size_t page_data_get_number_of_lines(PageData * pageData){
	return pageData->lines->len;
}


LineData * page_data_get_line_by_index_or_else(PageData * pageData, unsigned int index, LineData * elseValue){
	if(pageData == NULL || index >= pageData->lines->len){
		return elseValue;
	}
	LineData * result = &g_array_index (pageData->lines, LineData, index);
	return result;
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