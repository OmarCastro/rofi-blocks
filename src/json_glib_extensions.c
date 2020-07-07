// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2020 Omar Castro

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

const gchar * json_node_get_nullable_string_or_else(JsonNode * node, const gchar * else_value){
    return node != NULL && json_node_is_null(node) ? NULL : json_node_get_string_or_else(node, else_value);
}


gboolean json_object_get_boolean_member_or_else(JsonObject * node, const gchar * member, gboolean else_value){
    return json_node_get_boolean_or_else(json_object_get_member(node, member), else_value);
}

const gchar * json_object_get_string_member_or_else(JsonObject * node, const gchar * member, const gchar * else_value){
    return json_node_get_string_or_else(json_object_get_member(node, member), else_value);
}

const gchar * json_object_get_nullable_string_member_or_else(JsonObject * node, const gchar * member, const gchar * else_value){
    return json_node_get_nullable_string_or_else(json_object_get_member(node, member), else_value);
}