// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2020 Omar Castro

#ifndef ROFI_BLOCKS_STRING_UTILS_H
#define ROFI_BLOCKS_STRING_UTILS_H

// Result is an allocated a new string
char *str_replace(const char *orig, const char *rep, const char *with) ;

char *str_replace_in(char **orig, const char *rep, const char *with);

char *str_replace_in_escaped(char **orig, const char *rep, const char *with);

char *str_new_escaped_for_json_string(const char *str_to_escape);

#endif // ROFI_BLOCKS_STRING_UTILS_H
