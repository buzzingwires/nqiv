#include <string.h>
#include <limits.h>
#include <assert.h>

#include "../src/array.h"

#include "array_tests.h"

#define ARRAY_TEST_STANDARD_SIZE 10

void array_test_standard(nqiv_array* array,
                         const int   start_length,
                         const int   end_length,
                         const int   max_length,
                         const int   push_count,
                         const bool  unlimit)
{
	assert(unlimit || max_length >= 10);
	int idx = -1;
	int c;
	assert(push_count > 0);
	assert(array != NULL);
	assert(array->position == 0);
	assert(array->data_length == start_length);
	assert(array->max_data_length == max_length);
	assert(array->unit_length == sizeof(int));
	assert(nqiv_array_get_units_count(array) == 0);
	for(c = push_count; c > 0; --c) {
		assert(nqiv_array_push_count(array, &idx, 1));
	}
	assert(!nqiv_array_push_count(array, &idx, 1));
	assert(nqiv_array_get_units_count(array) == push_count);
	assert(nqiv_array_get_last_idx(array) == push_count - 1);
	if(unlimit) {
		nqiv_array_unlimit_data(array);
		assert(nqiv_array_push_count(array, &idx, 1));
		const int last_length = array->data_length;
		assert(!nqiv_array_push_count(array, &idx, INT_MAX));
		assert(array->max_data_length == 0);
		assert(array->data_length == last_length);
		assert(nqiv_array_push_count(array, &idx, 0));
		assert(array->data_length == last_length);
		assert(!nqiv_array_get_count(array, INT_MAX, &idx, INT_MAX));
	}
	nqiv_array_clear(array);
	assert(nqiv_array_get_units_count(array) == 0);
	assert(array->data_length == end_length);
	int times;
	for(times = 0; times < 2; ++times) {
		for(idx = 0; idx < 5; ++idx) {
			assert(nqiv_array_push(array, &idx));
			assert(nqiv_array_insert(array, &idx, idx));
		}
		assert(nqiv_array_get_units_count(array) == 10);
		const int to_compare[] = {0, 1, 2, 3, 4, 0, 1, 2, 3, 4};
		assert(memcmp(to_compare, array->data, 10) == 0);
		int gotten = -1;
		int popped = -1;
		for(idx = 4; idx >= 0; --idx) {
			assert(nqiv_array_get(array, idx, &gotten));
			nqiv_array_remove(array, idx);
			assert(nqiv_array_pop(array, &popped));
			assert(gotten == popped);
			assert(gotten == idx);
		}
	}
	nqiv_array_clear(array);
	assert(array->position == 0);
	for(idx = 0; idx < array->data_length; ++idx) {
		assert(((char*)(array->data))[idx] == 0);
	}
}

void array_test_default(void)
{
	nqiv_array* array = nqiv_array_create(sizeof(int), 1);
	array->min_add_count = 5;
	nqiv_array_set_max_data_length(array, 6);
	array_test_standard(array, sizeof(int) * 1, sizeof(int) * (5 * 2 + 1), array->max_data_length,
	                    6, true);
	nqiv_array_destroy(array);
}

void array_test_inherit(void)
{
	char       buf[ARRAY_TEST_STANDARD_SIZE * sizeof(int)] = {0};
	nqiv_array array;
	nqiv_array_inherit(&array, buf, sizeof(int), ARRAY_TEST_STANDARD_SIZE);
	array_test_standard(&array, ARRAY_TEST_STANDARD_SIZE * sizeof(int),
	                    ARRAY_TEST_STANDARD_SIZE * sizeof(int),
	                    ARRAY_TEST_STANDARD_SIZE * sizeof(int), ARRAY_TEST_STANDARD_SIZE, false);
}

void array_test_strbuild(void)
{
	char       str[25 * sizeof(char)] = {0};
	nqiv_array array;
	nqiv_array_inherit(&array, str, sizeof(char), 24);
	nqiv_array_clear(&array);
	assert(nqiv_array_push_str(&array, "Hello "));
	assert(strcmp(array.data, "Hello ") == 0);
	assert(nqiv_array_push_str(&array, "World"));
	assert(strcmp(array.data, "Hello World") == 0);
	assert(nqiv_array_push_str(&array, "!"));
	assert(strcmp(array.data, "Hello World!") == 0);
	assert(nqiv_array_get_units_count(&array) == 12);
	assert(nqiv_array_push_sprintf(&array, " "));
	assert(nqiv_array_push_sprintf(&array, "Hello %s!", "Moon"));
	assert(!nqiv_array_push_str(&array, "!"));
	assert(strcmp(str, "Hello World! Hello Moon!") == 0);
	assert(str == array.data);
	assert(strcmp(str, array.data) == 0);
}
