#ifndef NQIV_ARRAY_H
#define NQIV_ARRAY_H

#include <stdio.h>
#include <stdbool.h>

typedef struct nqiv_array
{
	void* data;
	int data_length;
	int position;
} nqiv_array;

nqiv_array* nqiv_array_create(const int starting_length);
bool nqiv_array_grow(nqiv_array* array, const int new_length);
bool nqiv_array_make_room(nqiv_array* array, const int size);
bool nqiv_array_insert_bytes(nqiv_array* array, void* ptr, const int count, const int idx);
void nqiv_array_remove_bytes(nqiv_array* array, const int idx, const int count);
bool nqiv_array_push_bytes(nqiv_array* array, void* ptr, const int count);
bool nqiv_array_get_bytes(nqiv_array* array, const int idx, const int count, void* ptr);
bool nqiv_array_pop_bytes(nqiv_array* array, const int count, void* ptr);
bool nqiv_array_insert_ptr(nqiv_array* array, void* ptr, const int idx);
void nqiv_array_remove_ptr(nqiv_array* array, const int idx);
bool nqiv_array_push_ptr(nqiv_array* array, void* ptr);
void* nqiv_array_get_ptr(nqiv_array* array, const int idx);
void* nqiv_array_pop_ptr(nqiv_array* array);
bool nqiv_array_insert_char_ptr(nqiv_array* array, char* ptr, const int idx);
void nqiv_array_remove_char_ptr(nqiv_array* array, const int idx);
bool nqiv_array_push_char_ptr(nqiv_array* array, char* ptr);
char* nqiv_array_get_char_ptr(nqiv_array* array, const int idx);
char* nqiv_array_pop_char_ptr(nqiv_array* array);
bool nqiv_array_insert_FILE_ptr(nqiv_array* array, FILE* ptr, const int idx);
void nqiv_array_remove_FILE_ptr(nqiv_array* array, const int idx);
bool nqiv_array_push_FILE_ptr(nqiv_array* array, FILE* ptr);
FILE* nqiv_array_get_FILE_ptr(nqiv_array* array, const int idx);
FILE* nqiv_array_pop_FILE_ptr(nqiv_array* array);
void nqiv_array_destroy(nqiv_array* array);

#endif /* NQIV_ARRAY_H */
