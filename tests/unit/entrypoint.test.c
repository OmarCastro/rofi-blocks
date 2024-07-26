// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2020 Omar Castro
#include "simple_tap_test_util.h"
#include "../../src/string_utils.h"
#include "../../src/page_data.h"
 

void page_data_unit_tests(void)
{
    PageData * page_data = page_data_new();
    
    test_true(page_data != NULL);
    test_true(page_data->markup_default == false);

    test_uint_equals(.result = page_data_get_number_of_lines(page_data), .expected = 0);


    page_data_add_line(page_data, "aaa", "any-icon", "any-data", true, true, true, true);
    test_string_equals(.result =  page_data_get_line_by_index_or_else(page_data, 0, NULL)->text, .expected= "aaa");
    test_true(page_data_get_line_by_index_or_else(page_data, -1, NULL) == NULL);
    test_true(page_data_get_line_by_index_or_else(NULL, 0, NULL) == NULL);
    test_uint_equals(.result = page_data_get_number_of_lines(page_data), .expected = 1);

    page_data_destroy(page_data);
}


void string_utils_unit_tests(void)
{
	char * result;
    test_true(str_replace(NULL, NULL, NULL) == NULL);
    test_true(str_replace("aaaa", NULL, NULL) == NULL);
    test_true(str_replace("aaaa", "", "bbb") == NULL);
    
    test_autofree_string_equals(.result = str_replace("a{{aa}}a", "{{aa}}", "bb"), .expected = "abba");
    
    test_autofree_string_equals(.result = str_replace("a{{aa}}a", "{{aa}}", NULL), .expected = "aa");


    result = str_replace("a {{aa}} {{bb}} c", "{{aa}}", "lorem");
    test_string_equals(.result = result, .expected = "a lorem {{bb}} c");
    test_string_equals(.result = str_replace_in(&result, "{{bb}}", "ipsum"), .expected = "a lorem ipsum c");
    free(result);

    result = str_replace("a {{aa}} {{bb}} c", "{{aa}}", "lorem");
    test_string_equals(
        .result = result, 
        .expected = "a lorem {{bb}} c", 
    );
    test_string_equals(
        .result = str_replace_in_escaped(&result, "{{bb}}", "\\ \" \t \f \b \r\n a b"),
        .expected = "a lorem \\\\ \\\" \\t \\f \\b \\r\\n a b c", 
    );
    free(result);

}



int main(void)
{
	page_data_unit_tests();
    string_utils_unit_tests();
    return test_finish();
}
