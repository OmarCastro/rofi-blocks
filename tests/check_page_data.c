// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2020 Omar Castro
#include "simple_tap_test_util.h"
#include "../src/page_data.h"

int main(void)
{

    PageData * page_data = page_data_new();
    
    test_true(page_data != NULL);
    test_true(page_data->markup_default == false);


    test_uint_equals(.result = page_data_get_number_of_lines(page_data), .expected = 0);


    page_data_add_line(page_data, "aaa", "any-icon", "any-data", true, true, true);
    test_string_equals(.result =  page_data_get_line_by_index_or_else(page_data, 0, NULL)->text, .expected= "aaa");
    test_true(page_data_get_line_by_index_or_else(page_data, -1, NULL) == NULL);
    test_true(page_data_get_line_by_index_or_else(NULL, 0, NULL) == NULL);
    test_uint_equals(.result = page_data_get_number_of_lines(page_data), .expected = 1);


    page_data_destroy(page_data);

    return test_finish();
}
