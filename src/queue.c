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
		nqiv_log_write(queue->logger, NQIV_LOG_INFO, "Destroyed queue of length %d.\n",
		               array_length);
	}
	memset(queue, 0, sizeof(nqiv_queue));
}

bool nqiv_queue_init(nqiv_queue*   queue,
                     nqiv_log_ctx* logger,
                     const int     unit_size,
                     const int     unit_count)
{
	if(queue == NULL) {
		return false;
	}
	if(logger == NULL) {
		return false;
	}
	nqiv_queue_destroy(queue); /* WE MUST ALWAYS INITIALIZE WHAT WE PASS TO THIS TO ZERO WHAT
	                              HAPPENS IF WE DESTROY AN UNINITIALIZED QUEUE */
	queue->array = nqiv_array_create(unit_size, unit_count);
	if(queue->array == NULL) {
		nqiv_log_write(logger, NQIV_LOG_ERROR,
		               "Failed to allocate memory to create queue of %d %d-sized units\n.",
		               unit_count, unit_size);
		return false;
	}
	omp_init_lock(&queue->lock);
	queue->logger = logger;
	nqiv_log_write(logger, NQIV_LOG_INFO, "Initialized queue of %d %d-sized units.\n", unit_count,
	               unit_size);
	return true;
}

bool nqiv_queue_push(nqiv_queue* queue, const void* entry)
{
	assert(entry != NULL);
	assert(queue != NULL);
	assert(queue->array != NULL);
	bool result = true;
	omp_set_lock(&queue->lock);
	const int old_length = queue->array->data_length;
	if(!nqiv_array_push(queue->array, entry)) {
		nqiv_log_write(queue->logger, NQIV_LOG_WARNING, "Failed to push to array of length %d.\n",
		               queue->array->data_length);
		result = false;
	}
	if(old_length != queue->array->data_length) {
		nqiv_log_write(queue->logger, NQIV_LOG_DEBUG, "Expanded queue from length %d to %d.\n",
		               old_length, queue->array->data_length);
	}
	omp_unset_lock(&queue->lock);
	return result;
}

void nqiv_queue_push_force(nqiv_queue* queue, const void* entry)
{
	assert(entry != NULL);
	assert(queue != NULL);
	assert(queue->array != NULL);
	omp_set_lock(&queue->lock);
	if(!nqiv_array_push(queue->array, entry)) {
		nqiv_log_write(
			queue->logger, NQIV_LOG_WARNING,
			"Failed to push to array of length %d, so, we are forcibly overwriting it.\n",
			queue->array->data_length);
		nqiv_array_remove(queue->array, nqiv_array_get_last_idx(queue->array));
		const bool result = nqiv_array_push(queue->array, entry);
		assert(result);
		(void)result;
	} else {
		nqiv_log_write(queue->logger, NQIV_LOG_DEBUG,
		               "Pushed to queue of length %d at position %d.\n", queue->array->data_length,
		               queue->array->position - 1);
	}
	omp_unset_lock(&queue->lock);
}

bool nqiv_queue_pop(nqiv_queue* queue, void* entry)
{
	assert(entry != NULL);
	assert(queue != NULL);
	assert(queue->array != NULL);
	bool result = false;
	omp_set_lock(&queue->lock);
	if(nqiv_array_pop(queue->array, entry)) {
		result = true;
	}
	omp_unset_lock(&queue->lock);
	return result;
}

bool nqiv_queue_pop_front(nqiv_queue* queue, void* entry)
{
	assert(entry != NULL);
	assert(queue != NULL);
	assert(queue->array != NULL);
	bool result = false;
	omp_set_lock(&queue->lock);
	if(nqiv_array_get(queue->array, 0, entry)) {
		nqiv_array_remove(queue->array, 0);
		nqiv_log_write(queue->logger, NQIV_LOG_DEBUG, "Popped from queue at position 0.\n");
		result = true;
	}
	omp_unset_lock(&queue->lock);
	return result;
}

void nqiv_priority_queue_destroy(nqiv_priority_queue* queue)
{
	if(queue->bins != NULL) {
		int idx;
		for(idx = 0; idx < queue->bin_count; ++idx) {
			nqiv_queue_destroy(&(queue->bins[idx]));
		}
		free(queue->bins);
	}
	memset(queue, 0, sizeof(nqiv_priority_queue));
}

bool nqiv_priority_queue_init(nqiv_priority_queue* queue,
                              nqiv_log_ctx*        logger,
                              const int            unit_size,
                              const int            unit_count,
                              const int            bin_count)
{
	if(queue == NULL) {
		return false;
	}
	if(bin_count == 0) {
		return false;
	}
	nqiv_priority_queue tmp;
	tmp.bins = (nqiv_queue*)calloc(bin_count, sizeof(nqiv_queue));
	if(tmp.bins == NULL) {
		return false;
	}
	tmp.bin_count = bin_count;
	int idx;
	for(idx = 0; idx < tmp.bin_count; ++idx) {
		if(!nqiv_queue_init(&(tmp.bins[idx]), logger, unit_size, unit_count)) {
			nqiv_priority_queue_destroy(&tmp);
			return false;
		}
	}
	memcpy(queue, &tmp, sizeof(nqiv_priority_queue));
	return true;
}

bool nqiv_priority_queue_push(nqiv_priority_queue* queue, const int level, const void* entry)
{
	assert(queue != NULL);
	assert(level >= 0);
	assert(level < queue->bin_count);
	return nqiv_queue_push(&(queue->bins[level]), entry);
}

void nqiv_priority_queue_push_force(nqiv_priority_queue* queue, const int level, const void* entry)
{
	assert(queue != NULL);
	assert(level >= 0);
	assert(level < queue->bin_count);
	nqiv_queue_push_force(&(queue->bins[level]), entry);
}

bool nqiv_priority_queue_pop_op(nqiv_priority_queue* queue,
                                void*                entry,
                                bool (*op)(nqiv_queue*, void*))
{
	int idx;
	for(idx = 0; idx < queue->bin_count; ++idx) {
		if(op(&(queue->bins[idx]), entry)) {
			return true;
		}
	}
	return false;
}

bool nqiv_priority_queue_pop(nqiv_priority_queue* queue, void* entry)
{
	return nqiv_priority_queue_pop_op(queue, entry, nqiv_queue_pop);
}

bool nqiv_queue_set_max_data_length(nqiv_queue* queue, const int count)
{
	nqiv_array_set_max_data_length(queue->array, count);
	return true;
}

bool nqiv_queue_set_min_add_count(nqiv_queue* queue, const int count)
{
	queue->array->min_add_count = count;
	return true;
}

bool nqiv_priority_queue_apply_int(nqiv_priority_queue* queue,
                                   bool (*op)(nqiv_queue*, int),
                                   const int value)
{
	int idx;
	for(idx = 0; idx < queue->bin_count; ++idx) {
		if(!op(&(queue->bins[idx]), value)) {
			return false;
		}
	}
	return true;
}

bool nqiv_priority_queue_set_max_data_length(nqiv_priority_queue* queue, const int length)
{
	return nqiv_priority_queue_apply_int(queue, nqiv_queue_set_max_data_length, length);
}

bool nqiv_priority_queue_set_min_add_count(nqiv_priority_queue* queue, const int count)
{
	return nqiv_priority_queue_apply_int(queue, nqiv_queue_set_min_add_count, count);
}
