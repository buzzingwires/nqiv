#ifndef NQIV_WORKER_H
#define NQIV_WORKER_H

#include <SDL2/SDL.h>
#include <omp.h>

#include "queue.h"
#include "image.h"

void nqiv_worker_main(nqiv_log_ctx* logger, nqiv_priority_queue* queue, omp_lock_t* lock, const int delay_base, const Uint32 event_code);

#endif /* NQIV_WORKER_H */
