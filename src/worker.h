#ifndef NQIV_WORKER_H
#define NQIV_WORKER_H

#include <stdint.h>

#include <SDL2/SDL.h>
#include <omp.h>

#include "queue.h"
#include "image.h"
#include "event.h"

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

#define NQIV_WORKER_SPEC_STRLEN 512

typedef struct nqiv_worker_spec
{
	/* Basic polling rate of the queue. -1 to use global setting */
	int delay_base;
	/* Max number of events to handle each polling cycle. -1 to use global setting */
	int event_interval;
	/* Which priority bins to query. First index -1 to use global setting */
	int queue_bins[THREAD_QUEUE_BIN_COUNT + 1];
} nqiv_worker_spec;

bool nqiv_worker_spec_to_string(const nqiv_worker_spec* spec, char* string);
bool nqiv_worker_string_to_spec(const char* string, nqiv_worker_spec* spec);

void nqiv_worker_main(nqiv_log_ctx*        logger,
                      nqiv_priority_queue* queue,
                      const int            delay_base,
                      const int            event_interval,
                      const int*           queue_bins,
                      const Uint32         event_code,
                      nqiv_shared_var*     transaction_group,
                      nqiv_shared_var*     active_count,
                      nqiv_shared_var*     running);

#endif /* NQIV_WORKER_H */
