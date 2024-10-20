#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "logging_tests.h"
#include "array_tests.h"
#include "pruner_tests.h"
#include "queue_tests.h"
#include "keybind_tests.h"
#include "keyrate_tests.h"

/*
 * Run automated, self-contained tests.
 *
 * Tests are organized in a tree structure with branch nodes (test sets) that contain a list of
 * other nodes, and leaf nodes (actual tests) which contain a valid pointer to a function that
 * performs the test.
 *
 * To run tests, take a list of names from the command line. If it is empty, run them all.
 * Otherwise, only run tests that are named, or the parent of which are named.
 *
 * To add tests:
 *
 *   - For each header in src/ make a corresponding header suffixed with _tests (Example: logging.h
 * -> logging_tests.h)
 *   - Your test function may be written in the corresponding C file. It should return void and take
 * no parameters. Checks are done using assert()
 *   - See create_tests() for instructions on adding your test.
 */

#define TEST_NAME_LEN 50

typedef struct test_set test_set;

struct test_set
{
	char       name[TEST_NAME_LEN];
	int        number;
	test_set** subsets;
	int        subsets_len;
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
	memset(tests, 0, sizeof(test_set));
	free(tests);
}

test_set* new_test_set(const char* name, const int number)
{
	assert(name != NULL);
	assert(number >= 0);
	test_set* tests = (test_set*)calloc(1, sizeof(test_set));
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
		tests->subsets = (test_set**)calloc(1, sizeof(test_set*));
		if(tests->subsets == NULL) {
			print_test_set_designation(tests);
			fprintf(stderr, ": Failed to create test subset array for new test set %d %s\n",
			        *test_counter, name);
			return NULL;
		}
		tests->subsets_len = 1;
	}
	assert(tests->subsets_len > 0);
	if(tests->subsets[tests->subsets_len - 1] != NULL) {
		const int  new_subsets_len = tests->subsets_len + 1;
		test_set** new_subsets =
			(test_set**)realloc(tests->subsets, sizeof(test_set*) * new_subsets_len);
		if(new_subsets == NULL) {
			print_test_set_designation(tests);
			fprintf(stderr, ": Failed to expand test subset array for new test set %d %s\n",
			        *test_counter, name);
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

test_set* add_test(test_set* tests, int* test_counter, const char* name, void (*test_ptr)(void))
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

void run_tests_step(const test_set* tests,
                    const bool      parent_allowed,
                    char**          names_to_run,
                    const int       names_to_run_len)
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

#define FAIL                \
	destroy_test_set(root); \
	return NULL;
/* Specify a set or category of tests. These are added to the root set (so no nested sets for now),
 * and one must be specified before adding a test. */
#define S(name)                                                 \
	current_set = add_test_subset(root, &test_counter, (name)); \
	if(current_set == NULL) {                                   \
		FAIL;                                                   \
	}
/* Specify a specific test for the last-specified set. */
#define T(name, func)                                                  \
	assert(current_set != NULL);                                       \
	if(add_test(current_set, &test_counter, (name), (func)) == NULL) { \
		FAIL;                                                          \
	}
test_set* create_tests(void)
{
	test_set* current_set;
	int       test_counter = 1;

	test_set* root = new_test_root();
	if(root == NULL) {
		return NULL;
	}

	S("array");
	T("array_test_default", array_test_default);
	T("array_test_inherit", array_test_inherit);
	T("array_test_strbuild", array_test_strbuild);

	S("queue");
	T("queue_test_default", queue_test_default);
	T("queue_test_priority_default", queue_test_priority_default);

	S("logging");
	T("logging_general", logging_test_general);
	T("logging_nulls", logging_test_nulls);

	S("pruner");
	T("pruner_default", pruner_test_default);
	T("pruner_check", pruner_test_check);

	S("keybind");
	T("keybind_parse_print", keybind_test_parse_print);
	T("keybind_lookup", keybind_test_lookup);

	S("keyrate");
	T("keyrate_default", keyrate_test_default);

	return root;
}
#undef FAIL
#undef S
#undef T

int main(int argc, char* argv[])
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
