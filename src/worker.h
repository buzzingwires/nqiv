#ifndef NQIV_WORKER_H
#define NQIV_WORKER_H

#include "queue.h"

void nqiv_worker_main(nqiv_queue* queue, omp_lock_t* lock);

#endif /* NQIV_WORKER_H */
