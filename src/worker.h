#ifndef NQIV_WORKER_H
#define NQIV_WORKER_H

#include "queue.h"

typedef struct nqiv_queue
{
	omp_lock_t lock;
	nqiv_array* array;
	nqiv_log_ctx* logger;
} nqiv_queue;

void nqiv_worker_main(nqiv_queue* queue);

#endif /* NQIV_WORKER_H */
