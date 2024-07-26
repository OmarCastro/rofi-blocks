LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) $(top_srcdir)/tap-driver.sh
EXTRA_DIST = $(TESTS)

TESTS = unit_tests
check_PROGRAMS = unit_tests

unit_tests_data_SOURCES = entrypoint.test.c ../../src/page_data.c ../../src/json_glib_extensions.c ../../src/string_utils.c
unit_tests_data_CFLAGS = @glib_CFLAGS@ --coverage
unit_tests_data_LDADD = @glib_LIBS@ -lgcov 
