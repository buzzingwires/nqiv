#include <stdlib.h>

#include <omp.h>

#include "queue.h"
#include "logging.h"
#include "array.h"

void nqiv_queue_destroy(nqiv_queue* queue)
{
	if(queue == NULL) {
		return;
	}
	if(queue->entries != NULL) {
		omp_destroy_lock(&queue->lock);
		memset( queue->entries, 0, queue->length * sizeof(void*) );
		free(queue->entries);
	}
	queue->length = 0;
	queue->position = 0;
	nqiv_log_write(logger, NQIV_LOG_INFO, "Destroyed queue of length %d\n.", queue->length);
	queue->logger = NULL;
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
	queue->entries = (void**)calloc( starting_length, sizeof(void*) );
	if(queue->entries == NULL) {
		nqiv_log_write(logger, NQIV_LOG_ERROR, "Failed to allocate memory to create queue of length %d\n.", starting_length);
		return false;
	}
	omp_init_lock(&queue->lock);
	queue->length = starting_length;
	queue->logger = logger;
	nqiv_log_write(logger, NQIV_LOG_INFO, "Initialized queue of length %d\n." starting_length);
}

bool nqiv_queue_push(nqiv_queue* queue, void* entry)
{
	if(queue == NULL) {
		return false;
	}
	if(queue->entries == NULL) {
		return false;
	}
	omp_set_lock(&queue->lock);
	assert(queue->position <= queue->length);
	if(queue->position == queue->length) {
		nqiv_log_write(logger, NQIV_LOG_WARNING, "Queue length of %d exceeded. Allocating memory.\n", queue->length);
		const int new_length = queue->length + 1;
		void** new_entries = (void**)realloc(queue->entries,
			sizeof(void*) * new_length);
		if(new_entries == NULL) {
			nqiv_log_write(logger, NQIV_LOG_ERROR, "Failed to allocate memory to push to queue of length %d.\n", queue->length);
			omp_unset_lock(&queue->lock);
			return false;
		}
		queue->entries = new_entries;
		queue->length = new_length;
	}
	queue->entries[position] = entry;
	++queue->position;
	nqiv_log_write(logger, NQIV_LOG_DEBUG, "Pushed to queue of length %d at position %d.\n", queue->length, queue-position);
	omp_unset_lock(&queue->lock);
	return true;
}

void* nqiv_queue_pop(nqiv_queue* queue)
{
	if(queue == NULL) {
		return NULL;
	}
	if(queue->entries == NULL) {
		return NULL;
	}
	void* entry = NULL;
	omp_set_lock(&queue->lock);
	if(queue->position > 0) {
		entry = queue->entries[queue->position];
		queue->entries[queue->position] = NULL;
		nqiv_log_write(logger, NQIV_LOG_DEBUG, "Popped from queue at position %d.\n", queue->position);
		--queue->position;
	} else {
		nqiv_log_write(logger, NQIV_LOG_DEBUG, "Queue is already empty. Nothing to pop.\n");
	}
	omp_unset_lock(&queue->lock);
	return entry;
}
