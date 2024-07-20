
#ifndef SIMPLE_TAP_TEST_UTIL_H
#define SIMPLE_TAP_TEST_UTIL_H


#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

struct test_location { const char* const file; const char* const function ; const int line; };
#define TEST_LOCATION() (struct test_location){.line=__LINE__, .file=__FILE__ , .function=__func__ } 

struct test_true { int assertion; const char* const description; };
#define test_true(assertVal, ...) test_true((struct test_true){ .assertion=assertVal, .description=#assertVal, __VA_ARGS__}, TEST_LOCATION() );

struct test_string_equals { const char* result; const char* const expected ; const char* const description; };
#define test_string_equals(...) test_string_equals((struct test_string_equals){ .description=#__VA_ARGS__, __VA_ARGS__ }, TEST_LOCATION() );

#define test_autofree_string_equals(...) test_autofree_string_equals((struct test_string_equals){ .description=#__VA_ARGS__, __VA_ARGS__ }, TEST_LOCATION() );


struct test_uint_equals { unsigned int result; unsigned int expected ; const char* const description; };
#define test_uint_equals(...) test_uint_equals((struct test_uint_equals){ .description=#__VA_ARGS__, __VA_ARGS__ }, TEST_LOCATION() );


struct test_execution
{
	char* text;
	struct test_execution * next;
};

struct test_suite
{
	unsigned int testNumber;
	struct test_execution * first;
	struct test_execution * last;
};



static struct test_suite suite = { .testNumber = 0, .first = NULL, .last = NULL };
                                                                    

static void print_tap(bool result, struct test_location location, const char* const description){
	char * text;
	const char * resultText = result ?  "ok" : "not ok" ;
	suite.testNumber++;
  	asprintf(&text, "%s %d [%s:%d] %s", resultText ,suite.testNumber, location.file, location.line,  description);
	struct test_execution * new_text_execution  = (struct test_execution*) malloc(sizeof(struct test_execution));
	new_text_execution->text = text;
	new_text_execution->next = NULL;
	if( suite.first == NULL ){
		suite.first = new_text_execution;
		suite.last = new_text_execution;
	} else {
		suite.last->next = new_text_execution;
		suite.last = new_text_execution;
	}

}

/* use parentheses to avoid macro subst */             
static void (test_true)(struct test_true parameters, struct test_location location) {
    const char* const description = parameters.description ? parameters.description : "";
	print_tap((bool) parameters.assertion, location,  description);                                                        
}                                                                                

/* use parentheses to avoid macro subst */             
static void (test_string_equals)(struct test_string_equals parameters, struct test_location location) {
    bool result = strcmp(parameters.result, parameters.expected) == 0 ? true : false;
    const char* description = parameters.description ? parameters.description : "";
    if(!result){
        char * text;
        asprintf(&text, "%s --- \n result: %s\n expected: %s\n ---", description ,parameters.result, parameters.expected);
		print_tap(result, location,  text);
		free(text);                                                        
    } else {
		print_tap(result, location,  description);                                                        
    }
}

/* use parentheses to avoid macro subst */             
static void (test_autofree_string_equals)(struct test_string_equals parameters, struct test_location location) {
    (test_string_equals)(parameters, location);
	free( (char*) parameters.result);
}

/* use parentheses to avoid macro subst */             
static void (test_uint_equals)(struct test_uint_equals parameters, struct test_location location) {
    bool result = parameters.result == parameters.expected;
    const char* description = parameters.description ? parameters.description : "";
    if(!result){
        char * text;
        asprintf(&text, "%s --- \n result: %d\n expected: %d\n ---", description ,parameters.result, parameters.expected);
		print_tap(result, location,  text); 
		free(text);                                                                                                          
    } else {
		print_tap(result, location,  description);                                                        
    }
}        

static int test_finish(){
	printf("1..%d\n", suite.testNumber);                                                        
	struct test_execution * current = suite.first;
	while(current != NULL){
		struct test_execution * next = current -> next;
		printf("%s\n", current->text);
		free(current->text);
		free(current);
		current = next;
	}
	return 0;
}

#endif // SIMPLE_TAP_TEST_UTIL_H
