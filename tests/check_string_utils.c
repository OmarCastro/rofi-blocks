// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2020 Omar Castro
#include "simple_tap_test_util.h"
#include "../src/string_utils.h"


int main(void)
{
	char * result;
    test_true(str_replace(NULL, NULL, NULL) == NULL);
    test_true(str_replace("aaaa", NULL, NULL) == NULL);
    test_true(str_replace("aaaa", "", "bbb") == NULL);
    
    result = str_replace("a{{aa}}a", "{{aa}}", "bb");
    test_string_equals(.result = result, .expected = "abba", .description = "str_replace(\"a{{aa}}a\", \"{{aa}}\", \"bb\") == \"abba\"");
    free(result);

    result = str_replace("a{{aa}}a", "{{aa}}", NULL);
    test_string_equals(.result = result, .expected = "aa", .description = "str_replace(\"a{{aa}}a\", \"{{aa}}\", NULL) == \"aa\"");
    free(result);



    result = str_replace("a {{aa}} {{bb}} c", "{{aa}}", "lorem");
    test_string_equals(.result = result, .expected = "a lorem {{bb}} c", .description = "(result = str_replace(\"a {{aa}} {{bb}} c\", \"{{aa}}\", \"lorem\")) == \"a lorem {{bb}} c\"");
    test_string_equals(.result = str_replace_in(&result, "{{bb}}", "ipsum"), .expected = "a lorem ipsum c", .description = "str_replace_in(&result, \"{{bb}}\", \"ipsum\") == \"a lorem ipsum c\"");
    free(result);

    result = str_replace("a {{aa}} {{bb}} c", "{{aa}}", "lorem");
    test_string_equals(.result = result, .expected = "a lorem {{bb}} c", .description = "(result = str_replace(\"a {{aa}} {{bb}} c\", \"{{aa}}\", \"lorem\")) == \"a lorem {{bb}} c\"");
    test_string_equals(.result = str_replace_in_escaped(&result, "{{bb}}", "\\ \" \t \f \b \r\n a b"), .expected = "a lorem \\\\ \\\" \\t \\f \\b \\r\\n a b c", .description = "str_replace_in(&result, \"{{bb}}\", \\\\ \\\" \\t \\f \\b \\r\\n a b\") == \"a lorem \\\\\\\\ \\\\\\\" \\\\t \\\\f \\\\b \\\\r\\\\n a b c\"");
    free(result);

    return test_finish();
}
