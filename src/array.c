#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <assert.h>

#include "array.h"

int nqiv_array_get_units_count(const nqiv_array* array)
{
	assert(array != NULL);
	assert(array->position % array->unit_length == 0);
	return array->position / array->unit_length;
}

int nqiv_array_get_last_idx(const nqiv_array* array)
{
	const int units_count = nqiv_array_get_units_count(array);
	return units_count > 0 ? units_count - 1 : 0;
}

nqiv_array* nqiv_array_create(const int unit_size, const int unit_count)
{
	const int length = unit_count * unit_size;
	assert(length >= unit_size && length >= unit_count);
	nqiv_array* array = (nqiv_array*)calloc( 1, sizeof(nqiv_array) );
	if(array == NULL) {
		return array;
	}
	array->data = calloc(unit_count, unit_size);
	if(array->data == NULL) {
		free(array);
		array = NULL;
		return array;
	}
	array->data_length = length;
	array->max_data_length = length;
	array->unit_length = unit_size;
	return array;
}

void nqiv_array_inherit(nqiv_array* array, void* data, const int unit_size, const int length)
{
	memset( array, 0, sizeof(nqiv_array) );
	array->data = data;
	array->data_length = length;
	array->max_data_length = length;
	array->unit_length = unit_size;
}

void nqiv_array_unlimit_data(nqiv_array* array)
{
	array->max_data_length = 0;
}

bool nqiv_array_grow(nqiv_array* array, const int new_count, const bool force)
{
	const int length = array->unit_length * new_count;
	if(length < array->unit_length || length < new_count) {
		return false;
	}
	assert(force || array->max_data_length == 0 || array->data_length <= array->max_data_length);
	if(length <= array->data_length) {
		return true;
	}
	if(!force && array->max_data_length > 0 && length > array->max_data_length) {
		return false;
	}
	void* new_data = (void*)realloc(array->data, length);
	if(new_data == NULL) {
		return false;
	}
	array->data = new_data;
	array->data_length = length;
	array->max_data_length = array->max_data_length == 0 || array->max_data_length >= array->data_length ? array->max_data_length : array->data_length;
	assert( sizeof(void*) == sizeof(char*) );
	memset( (char*)array->data + array->position, 0, length - array->position );
	return true;
}

bool nqiv_array_make_room(nqiv_array* array, const int add_count)
{
	const int minimum_count = nqiv_array_get_units_count(array) + add_count;
	if( !nqiv_array_grow(array, minimum_count, false) ) {
		return false;
	}
	return true;
}

bool nqiv_array_insert_count(nqiv_array* array, const void* ptr, const int idx, const int count)
{
	assert(idx >= 0);
	assert( idx <= nqiv_array_get_units_count(array) );
	const int add_length = array->unit_length * count;
	if( !nqiv_array_make_room(array, count) ) {
		return false;
	}
	const int offset = idx * array->unit_length;
	const int next_offset = offset + add_length;
	assert(array->position >= offset);
	char* data = array->data;
	memcpy(&data[next_offset], &data[offset], array->position - offset);
	memcpy(&data[offset], ptr, add_length);
	array->position += add_length;
	assert(array->position <= array->data_length);
	assert(array->max_data_length <= 0 || array->data_length <= array->max_data_length);
	return true;
}

bool nqiv_array_insert(nqiv_array* array, const void* ptr, const int idx)
{
	return nqiv_array_insert_count(array, ptr, idx, 1);
}

void nqiv_array_remove_count(nqiv_array* array, const int idx, const int count)
{
	assert(idx >= 0);
	const int remove_length = array->unit_length * count;
	const int offset = idx * array->unit_length;
	if(offset >= array->position) {
		return;
	}
	assert(offset + remove_length <= array->position);
	const int next_offset = offset + remove_length;
	char* data = array->data;
	if(next_offset < array->position) {
		memcpy(&data[offset], &data[next_offset], array->position - next_offset);
	}
	array->position -= remove_length;
	memset(&data[array->position], 0, remove_length);
	assert(array->position >= 0);
}

void nqiv_array_remove(nqiv_array* array, const int idx)
{
	nqiv_array_remove_count(array, idx, 1);
}

bool nqiv_array_push_count(nqiv_array* array, const void* ptr, const int count)
{
	return nqiv_array_insert_count(array,
								   ptr,
								   nqiv_array_get_units_count(array),
								   count);
}

bool nqiv_array_push(nqiv_array* array, const void* ptr)
{
	return nqiv_array_push_count(array, ptr, 1);
}

bool nqiv_array_push_str(nqiv_array* array, const char* ptr)
{
	assert( array->unit_length == sizeof(char) );
	const size_t len = strlen(ptr);
	return len > INT_MAX || array->unit_length * len + array->position >= (size_t)array->max_data_length ?
		   false : nqiv_array_push_count(array, ptr, len);
}

bool nqiv_array_get_count(const nqiv_array* array, const int idx, void* ptr, const int count)
{
	assert(idx >= 0);
	const int offset = idx * array->unit_length;
	const int amount = count * array->unit_length;
	if(offset >= array->position) {
		return false;
	}
	assert(offset + amount <= array->position);
	const char* data = array->data;
	memcpy(ptr, &data[offset], amount);
	return true;
}

bool nqiv_array_get(const nqiv_array* array, const int idx, void* ptr)
{
	return nqiv_array_get_count(array, idx, ptr, 1);
}

bool nqiv_array_pop_count(nqiv_array* array, void* ptr, const int count)
{
	bool output = false;
	const int last_idx = nqiv_array_get_units_count(array);
	const int start_idx = last_idx > count ? last_idx - count : 0;
	if( ptr == NULL || nqiv_array_get_count(array, start_idx, ptr, count) ) {
		nqiv_array_remove_count(array, start_idx, count);
		output = true;
	}
	return output;
}

bool nqiv_array_pop(nqiv_array* array, void* ptr)
{
	return nqiv_array_pop_count(array, ptr, 1);
}

void nqiv_array_clear(nqiv_array* array)
{
	nqiv_array_remove_count( array, 0, nqiv_array_get_units_count(array) );
	memset(array->data, 0, array->data_length);
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
