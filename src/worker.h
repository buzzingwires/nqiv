#ifndef NQIV_WORKER_H
#define NQIV_WORKER_H

#include <stdint.h>

#include <SDL2/SDL.h>

#include "queue.h"
#include "image.h"
#include "event.h"

/*
 * When worker threads are woken up, they will optionally wait a specified time before polling the queue (to reduce resource contention and give the queue more time to fill).
 * They will poll for events until they find one with an transaction group greater than or equal to the
 * current, or -1. Others will be discarded. Then they will dispatch the
 * appropriate code to handle that event. (See event.h for an overview of
 * events)
 *
 * The number of events is tracked and compared against the event interval. When the event interval is met or there are no more events, they will send an
 * SDL event for the master to update its display, then wait to be woken up again.
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

typedef struct nqiv_worker_main_args
{
	nqiv_log_ctx*        logger;
	nqiv_priority_queue* queue;
	const Uint32 delay;
	nqiv_cond*           wakeup;
	const int            event_interval;
	const int*           queue_bins;
	const Uint32         event_code;
	SDL_atomic_t*     transaction_group;
	SDL_atomic_t*     dormant_count;
	SDL_atomic_t*     running;
} nqiv_worker_main_args;

void nqiv_worker_main(nqiv_log_ctx*        logger,
                      nqiv_priority_queue* queue,
					  const Uint32 delay,
                      nqiv_cond*           wakeup,
                      const int            event_interval,
                      const int*           queue_bins,
                      const Uint32         event_code,
                      SDL_atomic_t*     transaction_group,
                      SDL_atomic_t*     dormant_count,
                      SDL_atomic_t*     running);

/* SDL interface for nqiv_worker_main. */
int nqiv_worker_main_sdl(void* args_ptr);

#endif /* NQIV_WORKER_H */
