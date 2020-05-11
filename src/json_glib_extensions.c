/**
 * rofi-blocks
 *
 * MIT/X11 License
 * Copyright (c) 2020 Omar Castro <omar.castro.360@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <stdlib.h>
#include <gmodule.h>
#include <json-glib/json-glib.h>

gboolean json_node_get_boolean_or_else(JsonNode * node, gboolean else_value){
    return node != NULL &&
           json_node_get_value_type(node) == G_TYPE_BOOLEAN ?
           json_node_get_boolean(node) : else_value;
}

const gchar * json_node_get_string_or_else(JsonNode * node, const gchar * else_value){
    return node != NULL &&
           json_node_get_value_type(node) == G_TYPE_STRING ?
           json_node_get_string(node) : else_value;
}


gboolean json_object_get_boolean_member_or_else(JsonObject * node, const gchar * member, gboolean else_value){
    return json_node_get_boolean_or_else(json_object_get_member(node, member), else_value);
}

const gchar * json_object_get_string_member_or_else(JsonObject * node, const gchar * member, const gchar * else_value){
    return json_node_get_string_or_else(json_object_get_member(node, member), else_value);
}