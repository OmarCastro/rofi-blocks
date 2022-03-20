// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2020 Omar Castro

#ifndef ROFI_BLOCKS_PAGE_DATA_H
#define ROFI_BLOCKS_PAGE_DATA_H
#include <gmodule.h>
#include <stdint.h>
#include <json-glib/json-glib.h>
typedef enum 
{
	MarkupStatus_UNDEFINED = 0,
	MarkupStatus_ENABLED = 1,
	MarkupStatus_DISABLED = 2
} MarkupStatus;

typedef struct
{
    MarkupStatus markup_default;
    GString *message;
    GString *overlay;
    GString *prompt;
    GString *input;
    GArray *lines;
} PageData;

typedef struct
{
    gchar *text;
    gchar *icon;
    gchar *data;
    gboolean urgent;
    gboolean highlight;
    gboolean markup;
    uint32_t icon_fetch_uid; //cache icon uid
} LineData;

PageData * page_data_new();

void page_data_destroy(PageData * pageData);

const char * page_data_get_message_or_empty_string(PageData * pageData);

gboolean page_data_is_message_empty(PageData * pageData);

void page_data_set_message(PageData * pageData, const char * message);

const char * page_data_get_overlay_or_empty_string(PageData * pageData);

gboolean page_data_is_overlay_empty(PageData * pageData);

void page_data_set_overlay(PageData * pageData, const char * overlay);


size_t page_data_get_number_of_lines(PageData * pageData);

LineData * page_data_get_line_by_index_or_else(PageData * pageData, unsigned int index, LineData * elseValue);

void page_data_add_line(PageData * pageData, const gchar * label, const gchar * icon, const gchar * data, gboolean urgent, gboolean highlight, gboolean markup);

void page_data_add_line_json_node(PageData * pageData, JsonNode * element);

void page_data_clear_lines(PageData * pageData);

#endif // ROFI_BLOCKS_PAGE_DATA_H
