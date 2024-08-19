#include <string.h>
#include <limits.h>
#include <assert.h>

#include "../src/array.h"

#include "array_tests.h"

void array_test_default(void)
{
	int idx = -1;
	nqiv_array* array = nqiv_array_create(sizeof(int), 1);
	assert(array != NULL);
	assert(array->position == 0);
	assert( array->data_length == sizeof(int) );
	assert( array->max_data_length == sizeof(int) );
	assert( array->unit_length == sizeof(int) );
	assert(nqiv_array_get_units_count(array) == 0);
	assert(nqiv_array_get_last_idx(array) == 0);
	assert( nqiv_array_push_count(array, &idx, 1) );
	assert( !nqiv_array_push_count(array, &idx, 1) );
	nqiv_array_unlimit_data(array);
	assert( nqiv_array_push_count(array, &idx, 1) );
	assert(nqiv_array_get_units_count(array) == 2);
	assert(nqiv_array_get_last_idx(array) == 1);
	nqiv_array_clear(array);
	assert(nqiv_array_get_units_count(array) == 0);
	assert(array->data_length == sizeof(int) * 2);
	assert(array->max_data_length == 0);
	int times;
	for(times = 0; times < 2; ++times) {
		for(idx = 0; idx < 5; ++idx) {
			assert( nqiv_array_push( array, &idx) );
			assert( nqiv_array_insert(array, &idx, idx) );
		}
		assert(nqiv_array_get_units_count(array) == 10);
		int to_compare[] = {0, 1, 2, 3, 4, 0, 1, 2, 3, 4};
		assert(memcmp(to_compare, array->data, 10) == 0);
		int gotten = -1;
		int popped = -1;
		for(idx = 4; idx >= 0; --idx) {
			assert( nqiv_array_get(array, idx, &gotten) );
			nqiv_array_remove(array, idx);
			assert( nqiv_array_pop(array, &popped) );
			assert(gotten == popped);
			assert(gotten == idx);
		}
	}
	nqiv_array_clear(array);
	assert(array->position == 0);
	for(idx = 0; idx < array->data_length; ++idx) {
		assert( ( (char*)(array->data) )[idx] == 0 );
	}
	nqiv_array_destroy(array);
}

void array_test_strbuild(void)
{
	char str[25 * sizeof(char)] = {0};
	nqiv_array array;
	nqiv_array_inherit(&array, str, sizeof(char), 24);
	nqiv_array_clear(&array);
	assert( nqiv_array_push_str(&array, "Hello ") );
	assert(strcmp(array.data, "Hello ") == 0);
	assert( nqiv_array_push_str(&array, "World") );
	assert(strcmp(array.data, "Hello World") == 0);
	assert( nqiv_array_push_str(&array, "!") );
	assert(strcmp(array.data, "Hello World!") == 0);
	assert(nqiv_array_get_units_count(&array) == 12);
	assert( nqiv_array_push_sprintf(&array, " ") );
	assert( nqiv_array_push_sprintf(&array, "Hello %s!", "Moon") );
	assert( !nqiv_array_push_str(&array, "!") );
	assert(strcmp(str, "Hello World! Hello Moon!") == 0);
	assert(str == array.data);
	assert(strcmp(str, array.data) == 0);
}


void array_test_alloc(void)
{
	nqiv_array* array = nqiv_array_create(1, 1);
	assert(array != NULL);
	nqiv_array_unlimit_data(array);

	int* iptr = (int*)nqiv_array_alloc( array, sizeof(int) );
	assert(iptr != NULL);
	*iptr = INT_MAX;

	char* sptr = (char*)nqiv_array_alloc( array, strlen("Hello world!") + 1 );
	assert(sptr != NULL);
	memcpy( sptr, "Hello world!", strlen("Hello world!") );

	unsigned char* ucptr = (unsigned char*)nqiv_array_alloc( array, sizeof(unsigned char) );
	assert(ucptr != NULL);
	*ucptr = 0xff;

	assert(*iptr == INT_MAX);
	assert(strcmp(sptr, "Hello world!") == 0);
	assert(*ucptr == 0xff);

	nqiv_array_destroy(array);
}
