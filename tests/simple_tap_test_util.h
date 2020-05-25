#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

struct test_location { const char* const file; const char* const function ; const int line; };
#define TEST_LOCATION() (struct test_location){.line=__LINE__, .file=__FILE__ , .function=__func__ } 

struct test_true { int assertion; const char* const description; };
#define test_true(assertVal, ...) test_true((struct test_true){ .assertion=assertVal, .description=#assertVal, __VA_ARGS__}, TEST_LOCATION() );

struct test_string_equals { const char* const result; const char* const expected ; const char* const description; };
#define test_string_equals(...) test_string_equals((struct test_string_equals){ .result=NULL, .expected=NULL, .description=NULL, __VA_ARGS__}, TEST_LOCATION() );





static int testNumber = 1;


static void (test_plan)(int i) {
	printf("1..%d\n", i);                                                        
}                                                                                

inline static void print_tap(bool result, struct test_location location, const char* const description){
  printf("%s %d [%s:%d] %s\n", result ?  "ok" : "nok", testNumber++, location.file, location.line,  description);
}

/* use parentheses to avoid macro subst */             
static void (test_true)(struct test_true parameters, struct test_location location) {
    const char* const description = parameters.description ? parameters.description : "";
	print_tap((bool) parameters.assertion, location,  description);                                                        
}                                                                                

/* use parentheses to avoid macro subst */             
static void (test_string_equals)(struct test_string_equals parameters, struct test_location location) {
    bool result = strcmp(parameters.result, parameters.expected) ? "ok" : "nok";
    const char* const description = parameters.description ? parameters.description : "";
	print_tap(result, location,  description);                                                        
}    
