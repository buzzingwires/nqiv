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
	if(queue->array != NULL) {
		omp_destroy_lock(&queue->lock);
		nqiv_array_destroy(queue->array);
		queue->array = NULL;
	}
	nqiv_log_write(logger, NQIV_LOG_INFO, "Destroyed queue of length %d\n.", queue->array->data_size);
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
	queue->array = nqiv_array_create( starting_length * sizeof(void*) );
	if(queue->array == NULL) {
		nqiv_log_write(logger, NQIV_LOG_ERROR, "Failed to allocate memory to create queue of length %d\n.", starting_length);
		return false;
	}
	omp_init_lock(&queue->lock);
	queue->logger = logger;
	nqiv_log_write(logger, NQIV_LOG_INFO, "Initialized queue of length %d\n." starting_length);
}

bool nqiv_queue_push(nqiv_queue* queue, void* entry)
{
	if(queue == NULL) {
		return false;
	}
	if(queue->array == NULL) {
		return false;
	}
	omp_set_lock(&queue->lock);
	if( !nqiv_array_push_ptr(queue->array, entry) ) {
		nqiv_log_write(logger, NQIV_LOG_WARNING, "Failed to push to array of length.\n", queue->array->data_length);
	} else {
		nqiv_log_write(logger, NQIV_LOG_DEBUG, "Pushed to queue of length %d at position %d.\n", queue->array->data_length, queue->array->position - 1);
	}
	omp_unset_lock(&queue->lock);
	return true;
}

void* nqiv_queue_pop(nqiv_queue* queue)
{
	if(queue == NULL) {
		return NULL;
	}
	if(queue->array == NULL) {
		return NULL;
	}
	omp_set_lock(&queue->lock);
	void* entry = nqiv_array_pop_ptr(queue->array);
	if(entry != NULL) {
		nqiv_log_write(logger, NQIV_LOG_DEBUG, "Popped from queue at position %d.\n", queue->array->position + 1);
	} else {
		nqiv_log_write(logger, NQIV_LOG_DEBUG, "Queue is already empty. Nothing to pop.\n");
	}
	omp_unset_lock(&queue->lock);
	return entry;
}
