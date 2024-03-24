#include <assert.h>

#include <SDL2/SDL.h>
#include <omp.h>

#include "queue.h"
#include "event.h"
#include "image.h"
#include "thumbnail.h"
#include "worker.h"

/*
typedef struct nqiv_event_image_load_form_options
{
	bool unload;
	bool file;
	bool wand;
	bool frame;
	bool raw;
	bool surface;
} nqiv_event_image_load_form_options;

typedef struct nqiv_event_image_load_options
{
	nqiv_image* image;
	bool create_thumbnail;
	nqiv_event_image_load_form_options image;
	nqiv_event_image_load_form_options thumbnail;
} nqiv_event_image_load_options;
 */

void nqiv_worker_handle_image_load_form(nqiv_event_image_load_form_options* options, nqiv_image* image, nqiv_image_form* form)
{
	if(options->unload) {
		if(options->surface) {
			nqiv_unload_image_form_surface(form);
		}
		if(options->raw) {
			nqiv_unload_image_form_raw(form);
		}
		if(options->vips) {
			nqiv_unload_image_form_vips(form);
		}
	} else {
		bool success = true;
		if(options->vips || options->vips_soft) {
			if(form->vips != NULL) {
				if(options->vips) {
					nqiv_unload_image_form_vips(form);
					success = nqiv_image_load_vips(image, form);
				}
			} else {
				success = nqiv_image_load_vips(image, form);
			}
		}
		if(success && options->first_frame) {
			success = nqiv_image_form_first_frame(image, form);
		}
		if(success && options->next_frame) {
			success = nqiv_image_form_next_frame(image, form);
		}
		if( success && (options->raw || options->raw_soft) ) {
			if(form->data != NULL) {
				if(options->raw) {
					nqiv_unload_image_form_raw(form);
					success = nqiv_image_load_raw(image, form);
				}
			} else {
				success = nqiv_image_load_raw(image, form);
			}
		}
		if( success && (options->surface || options->surface_soft) ) {
			if(form->surface != NULL) {
				if(options->surface) {
					nqiv_unload_image_form_surface(form);
					success = nqiv_image_load_surface(image, form);
				}
			} else {
				success = nqiv_image_load_surface(image, form);
			}
		}
	}
}

void nqiv_worker_main(nqiv_queue* queue, omp_lock_t* lock, const Uint32 event_code)
{
	bool running = true;
	bool last_event_found = false;
	while(running) {
		if(!last_event_found) {
			last_event_found = true;
			nqiv_log_write( queue->logger, NQIV_LOG_DEBUG, "Locking thread %d.\n", omp_get_thread_num() );
			omp_set_lock(lock);
			nqiv_log_write( queue->logger, NQIV_LOG_DEBUG, "Locked thread %d.\n", omp_get_thread_num() );
		}
		nqiv_event event = {0};
		const bool event_found = nqiv_queue_pop(queue, sizeof(nqiv_event), &event);
		if(event_found) {
			switch(event.type) {
				case NQIV_EVENT_WORKER_STOP:
					nqiv_log_write( queue->logger, NQIV_LOG_DEBUG, "Received stop event on thread %d.\n", omp_get_thread_num() );
					running = false;
					break;
				case NQIV_EVENT_IMAGE_LOAD:
					nqiv_log_write( queue->logger, NQIV_LOG_DEBUG, "Received image load event on thread %d.\n", omp_get_thread_num() );
					nqiv_log_write( queue->logger, NQIV_LOG_DEBUG, "Locking image %s, from thread %d.\n", event.options.image_load.image->image.path, omp_get_thread_num() );
					omp_set_lock(&event.options.image_load.image->lock);
					nqiv_log_write( queue->logger, NQIV_LOG_DEBUG, "Locked image %s, from thread %d.\n", event.options.image_load.image->image.path, omp_get_thread_num() );
					if(event.options.image_load.set_thumbnail_path) {
						if(event.options.image_load.image->thumbnail.path == NULL) {
							if( !nqiv_thumbnail_calculate_path(event.options.image_load.image, &event.options.image_load.image->thumbnail.path, false) ) {
								nqiv_log_write(event.options.image_load.image->parent->logger, NQIV_LOG_ERROR, "Failed to create thumbnail path for %s", event.options.image_load.image->image.path);
							}
						}
					}
					nqiv_worker_handle_image_load_form(&event.options.image_load.image_options, event.options.image_load.image, &event.options.image_load.image->image);
					if(!event.options.image_load.image->thumbnail_attempted && event.options.image_load.create_thumbnail) {
						if(event.options.image_load.image->thumbnail.vips == NULL &&
						   nqiv_image_load_vips(event.options.image_load.image, &event.options.image_load.image->thumbnail)
						   ) {
								if(event.options.image_load.image->image.vips == NULL) {
									if( nqiv_image_load_vips(event.options.image_load.image, &event.options.image_load.image->image) ) {
										if( !nqiv_thumbnail_matches_image(event.options.image_load.image) ) {
											nqiv_thumbnail_create(event.options.image_load.image);
										}
										nqiv_unload_image_form_vips(&event.options.image_load.image->image);
									}
								} else if( !nqiv_thumbnail_matches_image(event.options.image_load.image) ) {
									nqiv_thumbnail_create(event.options.image_load.image);
								}
								if(!event.options.image_load.image->parent->thumbnail.load) {
									nqiv_unload_image_form_vips(&event.options.image_load.image->thumbnail);
								}
						} else {
							if(event.options.image_load.image->image.vips == NULL) {
								if( nqiv_image_load_vips(event.options.image_load.image, &event.options.image_load.image->image) ) {
									nqiv_thumbnail_create(event.options.image_load.image);
									nqiv_unload_image_form_vips(&event.options.image_load.image->image);
								}
							} else {
								nqiv_thumbnail_create(event.options.image_load.image);
							}
						}
						event.options.image_load.image->thumbnail_attempted = true;
					}
					nqiv_worker_handle_image_load_form(&event.options.image_load.thumbnail_options, event.options.image_load.image, &event.options.image_load.image->thumbnail);
					if(event.options.image_load.borrow_thumbnail_dimension_metadata) {
						nqiv_image_borrow_thumbnail_dimensions(event.options.image_load.image);
					}
					nqiv_log_write( queue->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", event.options.image_load.image->image.path, omp_get_thread_num() );
					omp_unset_lock(&event.options.image_load.image->lock);
					nqiv_log_write( queue->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", event.options.image_load.image->image.path, omp_get_thread_num() );
					break;
			}
		} else {
			SDL_Event tell_finished = {0};
			tell_finished.type = SDL_USEREVENT;
			tell_finished.user.code = (Sint32)event_code;
			tell_finished.user.data1 = lock;
			if(SDL_PushEvent(&tell_finished) < 0) {
				nqiv_log_write( &state->logger, NQIV_LOG_ERROR, "Failed to send SDL event from thread %d. SDL Error: %s\n", omp_get_thread_num(), SDL_GetError() );
				running = false;
			}
			last_event_found = false;
			nqiv_log_write( queue->logger, NQIV_LOG_DEBUG, "Unlocking thread %d.\n", omp_get_thread_num() );
			omp_unset_lock(lock);
			nqiv_log_write( queue->logger, NQIV_LOG_DEBUG, "Unlocked thread %d.\n", omp_get_thread_num() );
			SDL_Delay(2); /* XXX: Sleep for a short time so the master thread can sync faster. Strictly speaking, it should *not* be necessary. */
		}
	}
}
