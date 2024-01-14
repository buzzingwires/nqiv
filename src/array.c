#include <stdbool.h>

#include "array.h"

nqiv_array* nqiv_array_create(const int starting_length)
{
	nqiv_array* array = (nqiv_array*)calloc( 1, sizeof(nqiv_array) )
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
	array->position = 0;
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
	if(new_length <= array->starting_length) {
		return false;
	}
	void* new_data = (void*)realloc(array->data, new_length);
	if(new_data == NULL) {
		return false;
	}
	array->data = new_data;
	array->data_length = new_length;
	memset(array->data + array->position, 0, new_length - array->position);
	return true;
}

bool nqiv_array_push_ptr(nqiv_array* array, void* ptr)
{
	if(array == NULL) {
		return false;
	}
	if(array->data == NULL) {
		return false;
	}
	const int minimum_length = array->position + sizeof(void*);
	if( array->data_length < minimum_length && !nqiv_array_grow(array, minimum_length) ) {
		return false;
	}
	*((void**)(array->data[array->position])) = ptr;
	array->position += sizeof(void*);
}

void* nqiv_array_get_ptr(nqiv_array* array, const int idx)
{
	if(array == NULL) {
		return NULL;
	}
	if(array->data == NULL) {
		return NULL;
	}
	if(idx < 0) {
		return NULL;
	}
	const int offset = idx * sizeof(void*);
	if(offset > array->data_length) {
		return NULL;
	}
	return ((void**)array->data)[offset];
}

void* nqiv_array_pop_ptr(nqiv_array* array)
{
	if(array == NULL) {
		return NULL;
	}
	if(array->data == NULL) {
		return NULL;
	}
	const int offset = array->position - sizeof(void*);
	if(offset < 0) {
		return NULL;
	}
	const void* output = ((void**)array->data)[offset];
	memset( array->data + offset, 0, sizeof(void*) );
	array->position = offset;
	return output;
}

bool nqiv_array_push_char_ptr(nqiv_array* array, char* ptr)
{
	return nqiv_array_push_ptr( array, (void*)ptr );
}

char* nqiv_array_get_char_ptr(nqiv_array* array, const int idx)
{
	return (char*)nqiv_array_get_ptr(array, idx);
}

void* nqiv_array_pop_char_ptr(nqiv_array* array)
{
	return (char*)nqiv_array_pop_ptr(array);
}

bool nqiv_array_push_FILE_ptr(nqiv_array* array, FILE* ptr)
{
	return nqiv_array_push_ptr( array, (void*)ptr );
}

FILE* nqiv_array_get_FILE_ptr(nqiv_array* array, const int idx)
{
	return (FILE*)nqiv_array_get_ptr(array, idx);
}

void* nqiv_array_pop_FILE_ptr(nqiv_array* array)
{
	return (FILE*)nqiv_array_pop_ptr(array);
}

void nqiv_array_destroy(nqiv_array* array)
{
	if(data == NULL) {
		return;
	}
	if(array->data != NULL) {
		memset(array->data, 0, array->data_length);
		free(array->data);
	}
	array->data_length = 0;
	array->position = 0;
}
