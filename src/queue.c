#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include <omp.h>

#include "logging.h"
#include "array.h"
#include "queue.h"

void nqiv_queue_destroy(nqiv_queue* queue)
{
	if(queue == NULL) {
		return;
	}
	int array_length = 0;
	if(queue->array != NULL) {
		array_length = queue->array->data_length;
		omp_destroy_lock(&queue->lock);
		nqiv_array_destroy(queue->array);
	}
	if(queue->logger != NULL) {
		nqiv_log_write(queue->logger, NQIV_LOG_INFO, "Destroyed queue of length %d.\n", array_length);
	}
	memset( queue, 0, sizeof(nqiv_queue) );
}

bool nqiv_queue_init(nqiv_queue* queue, nqiv_log_ctx* logger, const int starting_length)
{
	if(queue == NULL) {
		return false;
	}
	if(logger == NULL) {
		return false;
	}
	nqiv_queue_destroy(queue); /* WE MUST ALWAYS INITIALIZE WHAT WE PASS TO THIS TO ZERO WHAT HAPPENS IF WE DESTROY AN UNINITIALIZED QUEUE */
	queue->array = nqiv_array_create( starting_length * sizeof(void*) );
	if(queue->array == NULL) {
		nqiv_log_write(logger, NQIV_LOG_ERROR, "Failed to allocate memory to create queue of length %d\n.", starting_length);
		return false;
	}
	omp_init_lock(&queue->lock);
	queue->logger = logger;
	nqiv_log_write(logger, NQIV_LOG_INFO, "Initialized queue of length %d\n.", starting_length);
	return true;
}

/*
bool nqiv_array_push_bytes(nqiv_array* array, void* ptr, const int count);
bool nqiv_array_get_bytes(nqiv_array* array, const int idx, const int count, void* ptr);
bool nqiv_array_pop_bytes(nqiv_array* array, const int count, void* ptr);
*/

bool nqiv_queue_push(nqiv_queue* queue, const int count, void* entry)
{
	assert(entry != NULL);
	assert(queue != NULL);
	assert(queue->array != NULL);
	bool result = false;
	omp_set_lock(&queue->lock);
	if( !nqiv_array_push_bytes(queue->array, entry, count) ) {
		nqiv_log_write(queue->logger, NQIV_LOG_WARNING, "Failed to push to array of length.\n", queue->array->data_length);
	} else {
		nqiv_log_write(queue->logger, NQIV_LOG_DEBUG, "Pushed to queue of length %d at position %d.\n", queue->array->data_length, queue->array->position - 1);
		result = true;
	}
	omp_unset_lock(&queue->lock);
	return result;
}

void nqiv_queue_push_force(nqiv_queue* queue, const int count, void* entry)
{
	assert(entry != NULL);
	assert(queue != NULL);
	assert(queue->array != NULL);
	assert(queue->array->data_length >= count);
	omp_set_lock(&queue->lock);
	if( !nqiv_array_push_bytes(queue->array, entry, count) ) {
		nqiv_log_write(queue->logger, NQIV_LOG_WARNING, "Failed to push to array of length %d, so, we are forcibly overwriting it.\n", queue->array->data_length);
		nqiv_array_clear(queue->array);
		const bool result = nqiv_array_push_bytes(queue->array, entry, count);
		assert(result);
		(void) result;
	} else {
		nqiv_log_write(queue->logger, NQIV_LOG_DEBUG, "Pushed to queue of length %d at position %d.\n", queue->array->data_length, queue->array->position - 1);
	}
	omp_unset_lock(&queue->lock);
}

bool nqiv_queue_pop(nqiv_queue* queue, const int count, void* entry)
{
	assert(entry != NULL);
	assert(queue != NULL);
	assert(queue->array != NULL);
	bool result = false;
	omp_set_lock(&queue->lock);
	if( nqiv_array_pop_bytes(queue->array, count, entry) ) {
		nqiv_log_write(queue->logger, NQIV_LOG_DEBUG, "Popped from queue at position %d.\n", queue->array->position + 1);
		result = true;
	} else {
		nqiv_log_write(queue->logger, NQIV_LOG_DEBUG, "Queue is already empty. Nothing to pop.\n");
	}
	omp_unset_lock(&queue->lock);
	return result;
}

bool nqiv_queue_pop_front(nqiv_queue* queue, const int count, void* entry)
{
	assert(entry != NULL);
	assert(queue != NULL);
	assert(queue->array != NULL);
	bool result = false;
	omp_set_lock(&queue->lock);
	if( nqiv_array_get_bytes(queue->array, 0, count, entry) ) {
		nqiv_array_remove_bytes(queue->array, 0, count);
		nqiv_log_write(queue->logger, NQIV_LOG_DEBUG, "Popped from queue at position 0.\n");
		result = true;
	} else {
		nqiv_log_write(queue->logger, NQIV_LOG_DEBUG, "Queue is already empty. Nothing to pop.\n");
	}
	omp_unset_lock(&queue->lock);
	return result;
}
