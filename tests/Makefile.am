LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) $(top_srcdir)/tap-driver.sh
EXTRA_DIST = $(TESTS)

TESTS = check_string_utils check_page_data
check_PROGRAMS = check_string_utils check_page_data

check_string_utils_SOURCES = check_string_utils.c ../src/string_utils.c
check_string_utils_CFLAGS = --coverage
check_string_utils_LDADD = -lgcov 

check_page_data_SOURCES = check_page_data.c ../src/page_data.c ../src/json_glib_extensions.c
check_page_data_CFLAGS = @glib_CFLAGS@ --coverage
check_page_data_LDADD = @glib_LIBS@ -lgcov 
