#ifndef NQIV_QUEUE_H
#define NQIV_QUEUE_H

#include <stdbool.h>

#include <omp.h>

#include "logging.h"

typedef struct nqiv_queue
{
	omp_lock_t lock;
	nqiv_array* array;
	nqiv_log_ctx* logger;
} nqiv_queue;

typedef struct nqiv_priority_queue
{
	nqiv_queue* bins;
	int bin_count;
} nqiv_priority_queue;

void nqiv_queue_destroy(nqiv_queue* queue);
bool nqiv_queue_init(nqiv_queue* queue, nqiv_log_ctx* logger, const int starting_length);
bool nqiv_queue_push(nqiv_queue* queue, const int count, void* entry);
void nqiv_queue_push_force(nqiv_queue* queue, const int count, void* entry);
bool nqiv_queue_pop(nqiv_queue* queue, const int count, void* entry);
bool nqiv_queue_pop_front(nqiv_queue* queue, const int count, void* entry);

void nqiv_priority_queue_destroy(nqiv_priority_queue* queue);
bool nqiv_priority_queue_init(nqiv_priority_queue* queue, nqiv_log_ctx* logger, const int starting_bin_length, const int bin_count);
bool nqiv_priority_queue_push(nqiv_priority_queue* queue, const int level, const int count, void* entry);
void nqiv_priority_queue_push_force(nqiv_priority_queue* queue, const int level, const int count, void* entry);
bool nqiv_priority_queue_pop(nqiv_priority_queue* queue, const int count, void* entry);
void nqiv_priority_queue_lock(nqiv_priority_queue* queue);
void nqiv_priority_queue_unlock(nqiv_priority_queue* queue);
bool nqiv_priorty_queue_grow(nqiv_priority_queue* queue, const int queue_length);

#endif /* NQIV_QUEUE_H */
