#ifndef NQIV_WORKER_H
#define NQIV_WORKER_H

#include <stdint.h>

#include <SDL2/SDL.h>
#include <omp.h>

#include "queue.h"
#include "image.h"

/*
 * Worker threads function by polling their queue by a certain wait time. They
 * will grab events until they find one with an event interval greater than the
 * current, or -1. Others will be discarded. Then they will dispatch the
 * appropriate code to handle that event. (See event.h for an overview of
 * events)
 *
 * When the event interval is met or there are no more events, they will send an
 * SDL event for the master to update its display, then sleep to repeat the
 * polling process.
 */

void nqiv_worker_main(nqiv_log_ctx*        logger,
                      nqiv_priority_queue* queue,
                      const int            delay_base,
                      const int            event_interval,
                      const Uint32         event_code,
                      const int64_t*       transaction_group,
                      omp_lock_t*          transaction_group_lock);

#endif /* NQIV_WORKER_H */
