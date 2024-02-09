#include <assert.h>

#include "../src/array.h"

#include "array_tests.h"

void array_test_bytes(void)
{
	nqiv_array* array = nqiv_array_create(1);
	assert(array != NULL);
	int times;
	for(times = 0; times < 2; ++times) {
		int idx;
		for(idx = 0; idx < 5; ++idx) {
			assert( nqiv_array_push_bytes( array, &idx, sizeof(int) ) );
			assert( nqiv_array_insert_bytes(array, &idx, sizeof(int), idx) );
		}
		int gotten = -1;
		int popped = -1;
		for(idx = 4; idx >= 0; --idx) {
			assert( nqiv_array_get_bytes(array, idx, sizeof(int), &gotten) );
			nqiv_array_remove_bytes( array, idx, sizeof(int) );
			assert( nqiv_array_pop_bytes(array, sizeof(int), &popped) );
			assert(gotten == popped);
			assert(gotten == idx);
		}
		assert( !nqiv_array_get_bytes(array, idx, sizeof(int), &gotten) );
		nqiv_array_remove_bytes( array, idx, sizeof(int) );
		assert( !nqiv_array_pop_bytes(array, sizeof(int), &popped) );
	}
	nqiv_array_destroy(array);
}

void array_test_ptr(void)
{
	nqiv_array* array = nqiv_array_create(1);
	assert(array != NULL);
	int times;
	for(times = 0; times < 2; ++times) {
		int idx;
		for(idx = 0; idx < 5; ++idx) {
			assert( nqiv_array_push_ptr(array, (char*)0 + idx) );
			assert( nqiv_array_insert_ptr(array, (char*)0 + idx, idx) );
		}
		/* XXX: We intentionally use an unsafe cast here, since the pointer should be the same. */
		int* gotten = (int*)-1;
		int* popped = (int*)-1;
		for(idx = 4; idx >= 0; --idx) {
			gotten = nqiv_array_get_ptr(array, idx);
			assert(gotten != NULL || idx == 0);
			nqiv_array_remove_ptr(array, idx);
			popped = nqiv_array_pop_ptr(array);
			assert(gotten == popped);
			assert( (char*)gotten == (char*)0 + idx );
		}
		assert(nqiv_array_get_ptr(array, idx) == NULL);
		assert(nqiv_array_pop_ptr(array) == NULL);
	}
	nqiv_array_destroy(array);
}

void array_test_char_ptr(void)
{
	nqiv_array* array = nqiv_array_create(1);
	assert(array != NULL);
	int times;
	for(times = 0; times < 2; ++times) {
		int idx;
		for(idx = 0; idx < 5; ++idx) {
			assert( nqiv_array_push_char_ptr(array, (char*)0 + idx) );
			assert( nqiv_array_insert_char_ptr(array, (char*)0 + idx, idx) );
		}
		/* XXX: We intentionally use an unsafe cast here, since the pointer should be the same. */
		char* gotten = (char*)-1;
		char* popped = (char*)-1;
		for(idx = 4; idx >= 0; --idx) {
			gotten = nqiv_array_get_char_ptr(array, idx);
			assert(gotten != NULL || idx == 0);
			nqiv_array_remove_char_ptr(array, idx);
			popped = nqiv_array_pop_char_ptr(array);
			assert(gotten == popped);
			assert(gotten == (char*)0 + idx);
		}
		assert(nqiv_array_get_char_ptr(array, idx) == NULL);
		assert(nqiv_array_pop_char_ptr(array) == NULL);
	}
	nqiv_array_destroy(array);
}

void array_test_FILE_ptr(void)
{
	nqiv_array* array = nqiv_array_create(1);
	assert(array != NULL);
	int times;
	for(times = 0; times < 2; ++times) {
		int idx;
		for(idx = 0; idx < 5; ++idx) {
			assert( nqiv_array_push_FILE_ptr(array, (FILE*)0 + idx) );
			assert( nqiv_array_insert_FILE_ptr(array, (FILE*)0 + idx, idx) );
		}
		/* XXX: We intentionally use an unsafe cast here, since the pointer should be the same. */
		FILE* gotten = (FILE*)-1;
		FILE* popped = (FILE*)-1;
		for(idx = 4; idx >= 0; --idx) {
			gotten = nqiv_array_get_FILE_ptr(array, idx);
			assert(gotten != NULL || idx == 0);
			nqiv_array_remove_FILE_ptr(array, idx);
			popped = nqiv_array_pop_FILE_ptr(array);
			assert(gotten == popped);
			assert( (FILE*)gotten == (FILE*)0 + idx );
		}
		assert(nqiv_array_get_FILE_ptr(array, idx) == NULL);
		assert(nqiv_array_pop_FILE_ptr(array) == NULL);
	}
	nqiv_array_destroy(array);
}
