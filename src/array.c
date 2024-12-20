#include "platform.h"

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
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
	assert(units_count > 0);
	return units_count - 1;
}

nqiv_array* nqiv_array_create(const int unit_size, const int unit_count)
{
	const int length = unit_count * unit_size;
	assert(length >= unit_size && length >= unit_count); /* Overflow check */
	nqiv_array* array = (nqiv_array*)calloc(1, sizeof(nqiv_array));
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

void nqiv_array_inherit(nqiv_array* array, void* data, const int unit_size, const int count)
{
	/* Overflow check. */
	assert(count * unit_size >= count);
	assert(count * unit_size >= unit_size);
	memset(array, 0, sizeof(nqiv_array));
	array->data = data;
	array->data_length = count * unit_size;
	array->max_data_length = count * unit_size;
	array->unit_length = unit_size;
}

void nqiv_array_unlimit_data(nqiv_array* array)
{
	array->max_data_length = 0;
}

int nqiv_array_calculate_potential_length(const nqiv_array* array, const int units)
{
	const int length = array->unit_length * units;
	if(length < array->unit_length || length < units || units <= 0) { /* Overflow/sanity check. */
		return -1;
	}
	return length;
}

bool nqiv_array_grow(nqiv_array* array, const int new_count, const bool force)
{
	int length = nqiv_array_calculate_potential_length(array, new_count);
	if(length < 0) {
		return false;
	}
	assert(force || array->max_data_length == 0 || array->data_length <= array->max_data_length);
	if(length <= array->data_length) {
		return true;
	}
	if(array->min_add_count > 0) {
		const int needed_units = new_count - nqiv_array_get_units_count(array);
		assert(needed_units > 0);
		if(needed_units < array->min_add_count) {
			length = nqiv_array_calculate_potential_length(array, nqiv_array_get_units_count(array)
			                                                          + array->min_add_count);
			if(length < 0) {
				return false;
			}
		}
	}
	if(!force && array->max_data_length > 0 && length > array->max_data_length) {
		return false;
	}
	void* new_data = realloc(array->data, length);
	if(new_data == NULL) {
		return false;
	}
	array->data = new_data;
	array->data_length = length;
	array->max_data_length =
		array->max_data_length == 0 || array->max_data_length >= array->data_length
			? array->max_data_length
			: array->data_length;
	assert(sizeof(void*) == sizeof(char*));
	memset((char*)array->data + array->position, 0, length - array->position);
	return true;
}

bool nqiv_array_make_room(nqiv_array* array, const int add_count)
{
	assert(add_count > 0);
	const int minimum_count = nqiv_array_get_units_count(array) + add_count;
	if(!nqiv_array_grow(array, minimum_count, false)) {
		return false;
	}
	return true;
}

bool nqiv_array_insert_count(nqiv_array* array, const void* ptr, const int idx, const int count)
{
	if(count == 0) {
		return true;
	}
	assert(idx >= 0);
	assert(idx <= nqiv_array_get_units_count(array));
	const int add_length = array->unit_length * count;
	if(!nqiv_array_make_room(array, count)) {
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
	char*     data = array->data;
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
	return nqiv_array_insert_count(array, ptr, nqiv_array_get_units_count(array), count);
}

bool nqiv_array_push(nqiv_array* array, const void* ptr)
{
	return nqiv_array_push_count(array, ptr, 1);
}

bool nqiv_array_push_str_count(nqiv_array* array, const char* ptr, const int count)
{
	assert((size_t)array->unit_length == sizeof(char));
	return nqiv_array_push_count(array, ptr, count);
}

bool nqiv_array_push_str(nqiv_array* array, const char* ptr)
{
	return nqiv_array_push_str_count(array, ptr, nqiv_strlen(ptr));
}

bool nqiv_array_push_sprintf(nqiv_array* array, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	char      output[NQIV_ARRAY_SPRINTF_BUFLEN + 1] = {0};
	const int result = vsnprintf(output, NQIV_ARRAY_SPRINTF_BUFLEN, format, args);
	va_end(args);
	return result >= 0 && result < NQIV_ARRAY_SPRINTF_BUFLEN && nqiv_array_push_str(array, output);
}

bool nqiv_array_get_count(const nqiv_array* array, const int idx, void* ptr, const int count)
{
	assert(idx >= 0);
	const int offset = idx * array->unit_length;
	const int amount = count * array->unit_length;
	if(offset >= array->position || offset < 0) {
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
	bool      output = false;
	const int last_idx = nqiv_array_get_units_count(array);
	const int start_idx = last_idx > count ? last_idx - count : 0;
	if(ptr == NULL || nqiv_array_get_count(array, start_idx, ptr, count)) {
		nqiv_array_remove_count(array, start_idx, count);
		output = true;
	}
	return output;
}

bool nqiv_array_pop(nqiv_array* array, void* ptr)
{
	return nqiv_array_pop_count(array, ptr, 1);
}

void nqiv_array_set_max_data_length(nqiv_array* array, const int count)
{
	const int length = count * array->unit_length;
	assert(length >= array->unit_length && length >= count);
	array->max_data_length = length;
}

void nqiv_array_clear(nqiv_array* array)
{
	nqiv_array_remove_count(array, 0, nqiv_array_get_units_count(array));
	memset(array->data, 0, array->data_length);
}

void nqiv_array_destroy(nqiv_array* array)
{
	assert(array != NULL);
	if(array->data != NULL) {
		memset(array->data, 0, array->data_length);
		free(array->data);
	}
	memset(array, 0, sizeof(nqiv_array));
	free(array);
}
