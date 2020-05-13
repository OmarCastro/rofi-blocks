// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2020 Omar Castro



#include <gmodule.h>
#include <json-glib/json-glib.h>


typedef struct
{
    gboolean markup_default;
    GString *message;
    GString *overlay;
    GString *prompt;
    GString *input;
    GArray *lines;
} PageData;

typedef struct
{
    gchar *text;
    gboolean urgent;
    gboolean highlight;
    gboolean markup;
} LineData;

PageData * page_data_new();

void page_data_destroy(PageData * pageData);

void page_data_add_line(PageData * pageData, const gchar * label, gboolean urgent, gboolean highlight, gboolean markup);

void page_data_add_line_json_node(PageData * pageData, JsonNode * element);

void page_data_clear_lines(PageData * pageData);