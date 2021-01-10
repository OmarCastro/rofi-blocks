// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2020 Omar Castro


#include "json_glib_extensions.h"
#include "page_data.h"

static const gchar* EMPTY_STRING = "";

PageData * page_data_new(){
    PageData * pageData = g_malloc0( sizeof ( *pageData ) );
    pageData->message = NULL;
    pageData->overlay = NULL;
    pageData->prompt = NULL;
    pageData->input = g_string_sized_new(256);
    pageData->lines = g_array_new (FALSE, TRUE, sizeof (LineData));
    return pageData;
}

void page_data_destroy(PageData * pageData){
    page_data_clear_lines(pageData);
    pageData->message != NULL && g_string_free(pageData->message, TRUE);
    pageData->overlay != NULL && g_string_free(pageData->overlay, TRUE);
    pageData->prompt != NULL && g_string_free(pageData->prompt, TRUE);
    g_string_free(pageData->input, TRUE);
    g_array_free (pageData->lines, TRUE);
    g_free(pageData);
}



static gboolean is_page_data_string_member_empty(GString *string_member);
static const char * get_page_data_string_member_or_empty_string(GString *string_member);
static void set_page_data_string_member(GString **string_member, const char * new_string);



gboolean page_data_is_message_empty(PageData * pageData){
    return pageData == NULL ? TRUE : is_page_data_string_member_empty(pageData->message);
}

const char * page_data_get_message_or_empty_string(PageData * pageData){
    return pageData == NULL ? EMPTY_STRING : get_page_data_string_member_or_empty_string(pageData->message);
}

void page_data_set_message(PageData * pageData, const char * message){
    set_page_data_string_member(&pageData->message, message);
}

gboolean page_data_is_overlay_empty(PageData * pageData){
    return pageData == NULL ? TRUE : is_page_data_string_member_empty(pageData->overlay);
}

const char * page_data_get_overlay_or_empty_string(PageData * pageData){
    return pageData == NULL ? EMPTY_STRING : get_page_data_string_member_or_empty_string(pageData->overlay);
}

void page_data_set_overlay(PageData * pageData, const char * overlay){
    set_page_data_string_member(&pageData->overlay, overlay);
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


void page_data_add_line(PageData * pageData, const gchar * label, const gchar * icon, const gchar * data, gboolean urgent, gboolean highlight, gboolean markup){
    LineData line = {
        .text = g_strdup(label),
        .icon = g_strdup(icon),
        .data = g_strdup(data),
        .urgent = urgent,
        .highlight = highlight,
        .markup = markup
    };
    g_array_append_val(pageData->lines, line);
}

void page_data_add_line_json_node(PageData * pageData, JsonNode * element){
    if(JSON_NODE_HOLDS_VALUE(element) && json_node_get_value_type(element) == G_TYPE_STRING){
        page_data_add_line(pageData, json_node_get_string(element), EMPTY_STRING, EMPTY_STRING, FALSE, FALSE, pageData->markup_default == MarkupStatus_ENABLED);
    } else if(JSON_NODE_HOLDS_OBJECT(element)){
        JsonObject * line_obj = json_node_get_object(element);
        const gchar * text = json_object_get_string_member_or_else(line_obj, "text", EMPTY_STRING);
        const gchar * icon = json_object_get_string_member_or_else(line_obj, "icon", EMPTY_STRING);
        const gchar * data = json_object_get_string_member_or_else(line_obj, "data", EMPTY_STRING);
        gboolean urgent = json_object_get_boolean_member_or_else(line_obj, "urgent", FALSE);
        gboolean highlight = json_object_get_boolean_member_or_else(line_obj, "highlight", FALSE);
        gboolean markup = json_object_get_boolean_member_or_else(line_obj, "markup", pageData->markup_default == MarkupStatus_ENABLED);
        page_data_add_line(pageData, text, icon, data, urgent, highlight, markup);
    }
}

void page_data_clear_lines(PageData * pageData){
    GArray * lines = pageData->lines;
    int lines_length = lines->len;
    for (int i = 0; i < lines_length; i++){
        LineData line = g_array_index (lines, LineData, i);
        g_free(line.text);
        g_free(line.icon);
        g_free(line.data);
    }
    g_array_set_size(pageData->lines, 0);
}



static gboolean is_page_data_string_member_empty(GString *string_member){
    return string_member == NULL || string_member->len <= 0;
}

static const char * get_page_data_string_member_or_empty_string(GString *string_member){
    return string_member == NULL ?  EMPTY_STRING : string_member->str;
}

static void set_page_data_string_member(GString **string_member, const char * new_string){
    gboolean isDefined = *string_member != NULL;
    gboolean willDefine = new_string != NULL;

    if( isDefined && willDefine ){
        g_string_assign(*string_member, new_string);
    } else if( isDefined && !willDefine ){
        g_string_free(*string_member, TRUE);
        *string_member = NULL;
    } else if ( !isDefined && willDefine ){
        *string_member = g_string_new (new_string);
    } 
    //else do nothing, *string_member is already NULL

}