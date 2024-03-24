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

void nqiv_queue_destroy(nqiv_queue* queue);
bool nqiv_queue_init(nqiv_queue* queue, nqiv_log_ctx* logger, const int starting_length);
bool nqiv_queue_push(nqiv_queue* queue, const int count, void* entry);
bool nqiv_queue_push_force(nqiv_queue* queue, const int count, void* entry);
bool nqiv_queue_pop(nqiv_queue* queue, const int count, void* entry);
bool nqiv_queue_pop_front(nqiv_queue* queue, const int count, void* entry);

#endif /* NQIV_QUEUE_H */
