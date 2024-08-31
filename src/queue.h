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
bool nqiv_queue_init(nqiv_queue* queue, nqiv_log_ctx* logger, const int unit_size, const int unit_count);
bool nqiv_queue_push(nqiv_queue* queue, const void* entry);
void nqiv_queue_push_force(nqiv_queue* queue, const void* entry);
bool nqiv_queue_pop(nqiv_queue* queue, void* entry);
bool nqiv_queue_pop_front(nqiv_queue* queue, void* entry);

void nqiv_priority_queue_destroy(nqiv_priority_queue* queue);
bool nqiv_priority_queue_init(nqiv_priority_queue* queue, nqiv_log_ctx* logger, const int unit_size, const int unit_count, const int bin_count);
bool nqiv_priority_queue_push(nqiv_priority_queue* queue, const int level, const void* entry);
void nqiv_priority_queue_push_force(nqiv_priority_queue* queue, const int level, const void* entry);
bool nqiv_priority_queue_pop(nqiv_priority_queue* queue, void* entry);
bool nqiv_priority_queue_set_max_data_length(nqiv_priority_queue* queue, const int length);
bool nqiv_priority_queue_set_min_add_count(nqiv_priority_queue* queue, const int count);

#endif /* NQIV_QUEUE_H */
