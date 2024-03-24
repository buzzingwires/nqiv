#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "array.h"

nqiv_array* nqiv_array_create(const int starting_length)
{
	nqiv_array* array = (nqiv_array*)calloc( 1, sizeof(nqiv_array) );
	if(array == NULL) {
		return array;
	}
	array->data = calloc(1, starting_length);
	if(array->data == NULL) {
		free(array);
		array = NULL;
		return array;
	}
	array->data_length = starting_length;
	return array;
}

bool nqiv_array_grow(nqiv_array* array, const int new_length)
{
	if(array == NULL) {
		return false;
	}
	if(array->data == NULL) {
		return false;
	}
	if(new_length <= array->data_length) {
		return true;
	}
	void* new_data = (void*)realloc(array->data, new_length);
	if(new_data == NULL) {
		return false;
	}
	array->data = new_data;
	array->data_length = new_length;
	assert( sizeof(void*) == sizeof(char*) );
	memset( (char*)array->data + array->position, 0, new_length - array->position );
	return true;
}

bool nqiv_array_make_room(nqiv_array* array, const int size)
{
	if(array == NULL) {
		return false;
	}
	if(array->data == NULL) {
		return false;
	}
	const int minimum_length = array->position + size;
	if( array->data_length < minimum_length && !nqiv_array_grow(array, minimum_length) ) {
		return false;
	}
	return true;
}

bool nqiv_array_insert_bytes(nqiv_array* array, void* ptr, const int count, const int idx)
{
	assert(idx >= 0);
	assert(count > 0);
	if( !nqiv_array_make_room(array, count) ) {
		return false;
	}
	const int offset = idx * count;
	const int next_offset = offset + count;
	/*array->position = array->position >= offset ? array->position : offset;*/
	assert(array->position >= offset);
	char* data = array->data;
	memcpy(&data[next_offset], &data[offset], array->position - offset);
	memcpy(&data[offset], ptr, count);
	array->position += count;
	assert(array->position <= array->data_length);
	return true;
}

void nqiv_array_remove_bytes(nqiv_array* array, const int idx, const int count)
{
	assert(count > 0);
	if(idx < 0) {
		return;
	}
	const int offset = idx * count;
	if(offset >= array->position) {
		return;
	}
	const int next_offset = offset + count;
	char* data = array->data;
	if(next_offset < array->position) {
		memcpy(&data[offset], &data[next_offset], array->position - next_offset);
	}
	array->position -= count;
	memset(&data[array->position], 0, count);
	assert(array->position >= 0);
}

bool nqiv_array_push_bytes(nqiv_array* array, void* ptr, const int count)
{
	assert(array->position % count == 0);
	return nqiv_array_insert_bytes(array, ptr, count, array->position / count);
}

bool nqiv_array_get_bytes(nqiv_array* array, const int idx, const int count, void* ptr)
{
	assert(count != 0);
	if(array == NULL) {
		return false;
	}
	if(array->data == NULL) {
		return false;
	}
	if(idx < 0) {
		return false;
	}
	const int offset = idx * count;
	if(offset >= array->position) {
		return false;
	}
	char* data = array->data;
	memcpy(ptr, &data[offset], count);
	return true;
}

bool nqiv_array_pop_bytes(nqiv_array* array, const int count, void* ptr)
{
	if(array == NULL) {
		return false;
	}
	if(array->data == NULL) {
		return false;
	}
	const int offset = array->position - count;
	if(offset < 0) {
		return false;
	}
	char* data = array->data;
	memcpy(ptr, &data[offset], count);
	memset(&data[offset], 0, count);
	array->position = offset;
	return true;
}

bool nqiv_array_insert_ptr(nqiv_array* array, void* ptr, const int idx)
{
	return nqiv_array_insert_bytes(array, &ptr, sizeof(void*), idx);
}

void nqiv_array_remove_ptr(nqiv_array* array, const int idx)
{
	nqiv_array_remove_bytes( array, idx, sizeof(void*) );
}

bool nqiv_array_push_ptr(nqiv_array* array, void* ptr)
{
	assert(array->position % sizeof(void*) == 0);
	return nqiv_array_insert_ptr( array, ptr, array->position / sizeof(void*) );
}

void* nqiv_array_get_ptr(nqiv_array* array, const int idx)
{
	void* ptr = NULL;
	nqiv_array_get_bytes(array, idx, sizeof(void*), &ptr);
	return ptr;
}

void* nqiv_array_pop_ptr(nqiv_array* array)
{
	void* ptr = NULL;
	nqiv_array_pop_bytes(array, sizeof(void*), &ptr);
	return ptr;
}

bool nqiv_array_insert_char_ptr(nqiv_array* array, char* ptr, const int idx)
{
	return nqiv_array_insert_ptr(array, (void*)ptr, idx);
}

void nqiv_array_remove_char_ptr(nqiv_array* array, const int idx)
{
	nqiv_array_remove_ptr(array, idx);
}

bool nqiv_array_push_char_ptr(nqiv_array* array, char* ptr)
{
	return nqiv_array_push_ptr( array, (void*)ptr );
}

char* nqiv_array_get_char_ptr(nqiv_array* array, const int idx)
{
	return (char*)nqiv_array_get_ptr(array, idx);
}

char* nqiv_array_pop_char_ptr(nqiv_array* array)
{
	return (char*)nqiv_array_pop_ptr(array);
}

bool nqiv_array_insert_FILE_ptr(nqiv_array* array, FILE* ptr, const int idx)
{
	return nqiv_array_insert_ptr(array, (void*)ptr, idx);
}

void nqiv_array_remove_FILE_ptr(nqiv_array* array, const int idx)
{
	nqiv_array_remove_ptr(array, idx);
}

bool nqiv_array_push_FILE_ptr(nqiv_array* array, FILE* ptr)
{
	return nqiv_array_push_ptr( array, (void*)ptr );
}

FILE* nqiv_array_get_FILE_ptr(nqiv_array* array, const int idx)
{
	return (FILE*)nqiv_array_get_ptr(array, idx);
}

FILE* nqiv_array_pop_FILE_ptr(nqiv_array* array)
{
	return (FILE*)nqiv_array_pop_ptr(array);
}

void nqiv_array_clear(nqiv_array* array)
{
	if(array == NULL) {
		return;
	}
	if(array->data == NULL) {
		return;
	}
	assert( sizeof(void*) == sizeof(char*) );
	memset( (char*)array->data, 0, array->position );
	array->position = 0;
}

void nqiv_array_destroy(nqiv_array* array)
{
	if(array == NULL) {
		 return;
	}
	if(array->data != NULL) {
		memset(array->data, 0, array->data_length);
		free(array->data);
	}
	memset( array, 0, sizeof(nqiv_array) );
	free(array);
}
