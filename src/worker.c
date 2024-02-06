#include "worker.h"
#include "queue.h"
#include "event.h"
#include "image.h"

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
			nqiv_unload_image_form_raw(form)
		}
		if(options->wand) {
			nqiv_unload_image_form_wand(form);
		}
		if(options->file) {
			nqiv_unload_image_form_file(form);
		}
	} else {
		bool success = true;
		if(options->wand || options->wand_soft) {
			if(options->wand && form->wand != NULL) {
				nqiv_unload_image_form_wand(form);
				nqiv_unload_image_form_file(form);
			}
			success = nqiv_image_load_wand(image, form);
		}
		if(success && options->first_frame) {
			success = nqiv_image_form_first_frame(form);
		}
		if(success && options->next_frame) {
			success = nqiv_image_form_next_frame(form);
		}
		if(success && options->raw || options->raw_soft) {
			if(options->raw && form->raw != NULL) {
				nqiv_unload_image_form_raw(form)
			}
			success = nqiv_image_load_raw(image, form);
		}
		if(success && options->surface || options->surface_soft) {
			if(options->surface && form->surface != NULL) {
				nqiv_unload_image_form_sdl_surface(form)
			}
			nqiv_image_load_sdl_surface(image, form);
		}
	}
}

void nqiv_worker_main(nqiv_queue* queue, omp_lock_t* lock, const Uint32 event_code)
{
	bool running = true;
	while(running) {
		omp_set_lock(lock);
		nqiv_event event = {0};
		const bool event_found = nqiv_queue_pop(queue, sizeof(nqiv_event), &event)
		if(event_found) {
			omp_set_lock(event.options.image_load.image->lock);
			switch(event.type) {
				case NQIV_EVENT_WORKER_STOP:
					nqiv_log_write( queue->logger, NQIV_LOG_DEBUG, "Received stop event on thread %d.\n", omp_get_thread_num() );
					running = false;
					break;
				case NQIV_EVENT_IMAGE_LOAD:
					nqiv_log_write( queue->logger, NQIV_LOG_DEBUG, "Received image load event on thread %d.\n", omp_get_thread_num() );
					nqiv_worker_handle_image_load_form(&event.options.image_load.image_options, event.options.image_load.image, event.options.image_load.image->image);
					nqiv_worker_handle_image_load_form(&event.options.image_load.thumbnail_options, event.options.image_load.image, event.options.image_load.image->thumbnail);
					if(event.options.image_load.set_thumbnail_path) {
						if(image->thumbnail.path == NULL) {
							if( !nqiv_thumbnail_calculate_path(image, &image->thumbnail.path, false) ) {
								nqiv_log_write(image->parent->logger, NQIV_LOG_ERROR, "Failed to create thumbnail path for %s", image->image.path);
							}
						}
					}
					if(event.options.image_load.create_thumbnail) {
						if(event.options.image_load.image->parent->thumbnail.load && 
						   event.options.image_load.image->thumbnail->wand == NULL &&
						   nqiv_image_load_wand(event.options.image_load.image, event.options.image_load.image->thumbnail)
						   ) {
								nqiv_unload_image_form_wand(event.options.image_load.image->thumbnail);
						} else {
							if(event.options.image_load.image->image->wand == NULL) {
								if( nqiv_image_load_wand(event.options.image_load.image, event.options.image_load.image->image) ) {
									nqiv_thumbnail_create(event.options.image_load.image);
									nqiv_unload_image_form_wand(event.options.image_load.image->image);
								}
							} else {
								nqiv_thumbnail_create(event.options.image_load.image);
							}
						}
						event.options.image_load.image->thumbnail_attempted = true;
					}
					break;
			}
			omp_unset_lock(event.options.image_load.image->lock);
		}
		omp_unset_lock(lock);
		SDL_Event tell_finished;
		tell_finished.type = SDL_USEREVENT;
		tell_finished.user.code = event_code;
		tell_finished.user.data1 = lock;
		SDL_PushEvent(&tell_finished);
	}
}
