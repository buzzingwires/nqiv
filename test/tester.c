#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "logging_tests.h"
#include "array_tests.h"
#include "pruner_tests.h"

#define TEST_NAME_LEN 50

typedef struct test_set test_set;

struct test_set
{
	char name[TEST_NAME_LEN];
	int number;
	test_set** subsets;
	int subsets_len;
	void (*test_ptr)(void);
};

void print_test_set_designation(const test_set* tests)
{
	assert(tests->number >= 0);
	fprintf(stderr, "%d %s", tests->number, tests->name);
}

void destroy_test_set(test_set* tests)
{
	assert(tests != NULL);
	memset(tests->name, 0, TEST_NAME_LEN);
	if(tests->subsets != NULL) {
		assert(tests->subsets_len > 0);
		int idx;
		for(idx = 0; idx < tests->subsets_len; ++idx) {
			assert(tests->subsets[idx] != NULL);
			destroy_test_set(tests->subsets[idx]);
		}
		free(tests->subsets);
		tests->subsets_len = 0;
	}
	assert(tests->subsets_len == 0);
	memset( tests, 0, sizeof(test_set) );
	free(tests);
}

test_set* new_test_set(const char* name, const int number)
{
	assert(name != NULL);
	assert(number >= 0);
	test_set* tests = (test_set*)calloc( 1, sizeof(test_set) );
	if(tests == NULL) {
		fprintf(stderr, "Failed to create new test set %d %s\n", number, name);
		return tests;
	}
	strncpy(tests->name, name, TEST_NAME_LEN - 1);
	tests->number = number;
	return tests;
}

test_set* new_test_root(void)
{
	return new_test_set("all", 0);
}

test_set* add_test_subset(test_set* tests, int* test_counter, const char* name)
{
	assert(tests != NULL);
	assert(test_counter != NULL);
	assert(*test_counter > 0);
	assert(name != NULL);
	assert(tests->number >= 0);
	if(tests->subsets == NULL) {
		assert(tests->subsets_len == 0);
		tests->subsets = (test_set**)calloc( 1, sizeof(test_set*) );
		if(tests->subsets == NULL) {
			print_test_set_designation(tests);
			fprintf(stderr, ": Failed to create test subset array for new test set %d %s\n", *test_counter, name);
			return NULL;
		}
		tests->subsets_len = 1;
	}
	assert(tests->subsets_len > 0);
	if(tests->subsets[tests->subsets_len - 1] != NULL) {
		const int new_subsets_len = tests->subsets_len + 1;
		test_set** new_subsets = (test_set**)realloc(tests->subsets,
			sizeof(test_set*) * new_subsets_len);
		if(new_subsets == NULL) {
			print_test_set_designation(tests);
			fprintf(stderr, ": Failed to expand test subset array for new test set %d %s\n", *test_counter, name);
			return NULL;
		}
		tests->subsets_len = new_subsets_len;
		tests->subsets = new_subsets;
	}
	test_set* new_tests = new_test_set(name, *test_counter);
	if(new_tests != NULL) {
		tests->subsets[tests->subsets_len - 1] = new_tests;
		*test_counter += 1;
	}
	return new_tests;
}

test_set* add_test( test_set* tests, int* test_counter, const char* name, void (*test_ptr)(void) )
{
	assert(test_ptr != NULL);
	test_set* test = add_test_subset(tests, test_counter, name);
	if(test != NULL) {
		test->test_ptr = test_ptr;
	}
	return test;
}

bool check_name(char** array, const int count, const char* string)
{
	assert(array != NULL);
	assert(string != NULL);
	assert(count >= 0);
	if(count == 0) {
		return true;
	}
	int idx;
	for(idx = 0; idx < count; ++idx) {
		if(strcmp(array[idx], string) == 0) {
			return true;
		}
	}
	return false;
}

void run_tests_step(const test_set* tests, const bool parent_allowed, char** names_to_run, const int names_to_run_len)
{
	assert(tests != NULL);
	const bool allowed = parent_allowed || check_name(names_to_run, names_to_run_len, tests->name);
	if(tests->subsets != NULL) {
		assert(tests->subsets_len > 0);
		int idx;
		for(idx = 0; idx < tests->subsets_len; ++idx) {
			assert(tests->subsets[idx] != NULL);
			run_tests_step(tests->subsets[idx], allowed, names_to_run, names_to_run_len);
		}
	}
	if(tests->test_ptr != NULL && allowed) {
		fprintf(stderr, "RUNNING: ");
		print_test_set_designation(tests);
		fprintf(stderr, "\n");
		tests->test_ptr();
	}
}

void run_tests(const test_set* tests, char** names_to_run, const int names_to_run_count)
{
	run_tests_step(tests, false, names_to_run, names_to_run_count);
}

test_set* create_tests(void)
{
	int test_counter = 1;
	test_set* root = new_test_root(); if(root == NULL) { goto done; }

	test_set* logging = add_test_subset(root, &test_counter, "logging"); if(logging == NULL) { goto cleanup; }
	if(add_test(logging, &test_counter, "logging_general", logging_test_general) == NULL) { goto cleanup; }
	if(add_test(logging, &test_counter, "logging_nulls", logging_test_nulls) == NULL) { goto cleanup; }

	test_set* array = add_test_subset(root, &test_counter, "array"); if(array == NULL) { goto cleanup; }
	if(add_test(array, &test_counter, "array_test_default", array_test_default) == NULL) { goto cleanup; }
	if(add_test(array, &test_counter, "array_test_strbuild", array_test_strbuild) == NULL) { goto cleanup; }
	if(add_test(array, &test_counter, "array_test_alloc", array_test_alloc) == NULL) { goto cleanup; }

	test_set* pruner = add_test_subset(root, &test_counter, "pruner"); if(pruner == NULL) { goto cleanup; }
	if(add_test(pruner, &test_counter, "pruner_default", pruner_test_default) == NULL) { goto cleanup; }

	goto done;
	cleanup:
		destroy_test_set(root);
		root = NULL;
	done:
		return root;
}

int main(int argc, char *argv[])
{
	test_set* tests = create_tests();
	if(tests == NULL) {
		return 1;
	}
	assert(argc >= 1);
	run_tests(tests, &argv[1], argc - 1);
	destroy_test_set(tests);
	return 0;
}
