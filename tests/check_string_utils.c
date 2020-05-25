// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2020 Omar Castro
#include "../src/string_utils.h"
#include "simple_tap_test_util.h"


int main(void)
{

    test_plan(3);
    test_true(str_replace(NULL, NULL, NULL) == NULL);
    test_true(str_replace("aaaa", NULL, NULL) == NULL);

    char * result = str_replace("a{{aa}}a", "{{aa}}", "bb");
    test_string_equals(.result = result, .expected = "abba", .description = "str_replace(\"a{{aa}}a\", \"{{aa}}\", \"bb\") == \"abba\"");
    free(result);
    return 0;
}
