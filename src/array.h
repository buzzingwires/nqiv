#ifndef NQIV_ARRAY_H
#define NQIV_ARRAY_H

#include <stdio.h>
#include <stdbool.h>

/*
 * nqiv_array is a multipurpose datastructure that may variously function as an
 * array, a stack, and a string builder. Its correctness and performance (in
 * that order) are considered especially important to nqiv.
 *
 * The array object contains a pointer to a certain amount of memory. This
 * memory may be dynamically allocated by nqiv_array_create (which also
 * dynamically allocates the array object itself) and freed by
 * nqiv_array_destroy. nqiv_array may also be backed by an existing (often
 * static) section of memory specified by nqiv_array_inherit
 *
 * Normally, the data reserved for the array will be capped at the starting
 * length. This cap must be manually changed and will be expanded in intervals
 * of min_add_count. Expanding max_data_length on static memory is to be
 * avoided.
 *
 * Within the allocated memory, position specifies the actual end of the array.
 * When position surpasses the length of the data, it may be reallocated as
 * discussed, or the operation will fail.
 *
 * Operations on the array will typically be done by reading or writing memory
 * of unit_length. Operations followed by _count will work on a certain number
 * of units, while other operations will just work on one.
 */

/* Intermediate bufsize for nqiv_array_push_sprintf The operation will fail if
 * this is exceeded. */
#define NQIV_ARRAY_SPRINTF_BUFLEN 4096

typedef struct nqiv_array
{
	void* data;
	int   data_length;
	int   max_data_length; /* 0 for limitless */
	int   unit_length;
	int   min_add_count; /* 0 to only allocate what's needed */
	int   position;
} nqiv_array;

int nqiv_array_get_units_count(const nqiv_array* array); /* Current length of array in units. */
int nqiv_array_get_last_idx(const nqiv_array* array);
nqiv_array* nqiv_array_create(const int unit_size, const int unit_count);
/* In units. */
void        nqiv_array_inherit(nqiv_array* array, void* data, const int unit_size, const int count);
void        nqiv_array_unlimit_data(nqiv_array* array);
/* In units! force ignores max data length and sets the new length as max. */
bool        nqiv_array_grow(nqiv_array* array, const int new_count, const bool force);
bool nqiv_array_insert_count(nqiv_array* array, const void* ptr, const int idx, const int count);
bool nqiv_array_insert(nqiv_array* array, const void* ptr, const int idx);
void nqiv_array_remove_count(nqiv_array* array, const int idx, const int count);
void nqiv_array_remove(nqiv_array* array, const int idx);
bool nqiv_array_push_count(nqiv_array* array, const void* ptr, const int count);
bool nqiv_array_push(nqiv_array* array, const void* ptr);

/* Push a specific number of characters. Unit length must be of a char. */
bool nqiv_array_push_str_count(nqiv_array* array, const char* ptr, const int count);

/* Push characters in a NULL_terminated string to the array. Unit length must be
 * of a char */
bool nqiv_array_push_str(nqiv_array* array, const char* ptr);

/* Basically, do sprintf, then push it. */
bool nqiv_array_push_sprintf(nqiv_array* array, const char* format, ...);

/* Store at ptr */
bool nqiv_array_get_count(const nqiv_array* array, const int idx, void* ptr, const int count);
bool nqiv_array_get(const nqiv_array* array, const int idx, void* ptr);
bool nqiv_array_pop_count(nqiv_array* array, void* ptr, const int count);
bool nqiv_array_pop(nqiv_array* array, void* ptr);

void nqiv_array_set_max_data_length(nqiv_array* array, const int count); /* By unit size! */
void nqiv_array_clear(nqiv_array* array); /* Remove everything and zero data. */
void nqiv_array_destroy(nqiv_array* array);

#endif /* NQIV_ARRAY_H */
