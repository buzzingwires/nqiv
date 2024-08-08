#ifndef NQIV_WORKER_H
#define NQIV_WORKER_H

#include <stdint.h>

#include <SDL2/SDL.h>
#include <omp.h>

#include "queue.h"
#include "image.h"

void nqiv_worker_main(nqiv_log_ctx* logger, nqiv_priority_queue* queue, const int delay_base, const int event_interval, const Uint32 event_code, const int64_t* transaction_group, omp_lock_t* transaction_group_lock);

#endif /* NQIV_WORKER_H */
