#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

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
		nqiv_array_destroy(queue->array);
	}
	if(queue->lock != NULL) {
		SDL_DestroyMutex(queue->lock);
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
	queue->lock = SDL_CreateMutex();
	if(queue->lock == NULL) {
		nqiv_queue_destroy(queue);
		nqiv_log_write(logger, NQIV_LOG_ERROR,
		               "Failed to create lock for queue of %d %d-sized units\n.", unit_count,
		               unit_size);
		return false;
	}
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
	SDL_LockMutex(queue->lock);
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
	SDL_UnlockMutex(queue->lock);
	return result;
}

void nqiv_queue_push_force(nqiv_queue* queue, const void* entry)
{
	assert(entry != NULL);
	assert(queue != NULL);
	assert(queue->array != NULL);
	SDL_LockMutex(queue->lock);
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
	SDL_UnlockMutex(queue->lock);
}

bool nqiv_queue_pop(nqiv_queue* queue, void* entry)
{
	assert(entry != NULL);
	assert(queue != NULL);
	assert(queue->array != NULL);
	bool result = false;
	SDL_LockMutex(queue->lock);
	if(nqiv_array_pop(queue->array, entry)) {
		result = true;
	}
	SDL_UnlockMutex(queue->lock);
	return result;
}

bool nqiv_queue_pop_front(nqiv_queue* queue, void* entry)
{
	assert(entry != NULL);
	assert(queue != NULL);
	assert(queue->array != NULL);
	bool result = false;
	SDL_LockMutex(queue->lock);
	if(nqiv_array_get(queue->array, 0, entry)) {
		nqiv_array_remove(queue->array, 0);
		nqiv_log_write(queue->logger, NQIV_LOG_DEBUG, "Popped from queue at position 0.\n");
		result = true;
	}
	SDL_UnlockMutex(queue->lock);
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

static bool nqiv_queue_set_max_data_length(nqiv_queue* queue, void* count)
{
	nqiv_array_set_max_data_length(queue->array, (*(int*)count));
	return true;
}

static bool nqiv_queue_set_min_add_count(nqiv_queue* queue, void* count)
{
	queue->array->min_add_count = *((int*)count);
	return true;
}

static bool nqiv_queue_clear(nqiv_queue* queue, void* value)
{
	(void)value;
	SDL_LockMutex(queue->lock);
	nqiv_array_clear(queue->array);
	SDL_UnlockMutex(queue->lock);
	return true;
}

static bool nqiv_queue_lock(nqiv_queue* queue, void* value)
{
	(void)value;
	SDL_LockMutex(queue->lock);
	return true;
}

static bool nqiv_queue_unlock(nqiv_queue* queue, void* value)
{
	(void)value;
	SDL_UnlockMutex(queue->lock);
	return true;
}

static bool nqiv_priority_queue_apply(nqiv_priority_queue* queue,
                                      void*                entry,
                                      bool (*op)(nqiv_queue*, void*),
                                      const bool lazy)
{
	int idx;
	for(idx = 0; idx < queue->bin_count; ++idx) {
		/* If we are lazy, we quit immediately on success, otherwise we quit on failure, checking
		 * everything on success. */
		if(lazy) {
			if(op(&(queue->bins[idx]), entry)) {
				return true;
			}
		} else {
			if(!op(&(queue->bins[idx]), entry)) {
				return false;
			}
		}
	}
	return !lazy;
}

bool nqiv_priority_queue_pop(nqiv_priority_queue* queue, void* entry)
{
	return nqiv_priority_queue_apply(queue, entry, nqiv_queue_pop, true);
}

bool nqiv_priority_queue_pop_bins(nqiv_priority_queue* queue, const int* bins, void* entry)
{
	int idx;
	for(idx = 0; bins[idx] >= 0; ++idx) {
		assert(bins[idx] < queue->bin_count);
		if(nqiv_queue_pop(&(queue->bins[bins[idx]]), entry)) {
			return true;
		}
	}
	return false;
}

bool nqiv_priority_queue_set_max_data_length(nqiv_priority_queue* queue, const int length)
{
	int tmp_length = length;
	return nqiv_priority_queue_apply(queue, &tmp_length, nqiv_queue_set_max_data_length, false);
}

bool nqiv_priority_queue_set_min_add_count(nqiv_priority_queue* queue, const int count)
{
	int tmp_count = count;
	return nqiv_priority_queue_apply(queue, &tmp_count, nqiv_queue_set_min_add_count, false);
}

bool nqiv_priority_queue_clear(nqiv_priority_queue* queue)
{
	return nqiv_priority_queue_apply(queue, NULL, nqiv_queue_clear, false);
}

void nqiv_priority_queue_lock(nqiv_priority_queue* queue)
{
	nqiv_priority_queue_apply(queue, NULL, nqiv_queue_lock, false);
}

void nqiv_priority_queue_unlock(nqiv_priority_queue* queue)
{
	nqiv_priority_queue_apply(queue, NULL, nqiv_queue_unlock, false);
}
