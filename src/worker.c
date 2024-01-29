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
		if(options->wand) {
			if(form->wand != NULL) {
				nqiv_unload_image_form_wand(form);
				nqiv_unload_image_form_file(form);
			}
			success = nqiv_image_load_wand(image, form);
		}
		if(success && options->frame) {
			success = nqiv_image_form_next_frame(form);
		}
		if(success && options->raw) {
			if(form->raw != NULL) {
				nqiv_unload_image_form_raw(form)
			}
			success = nqiv_image_load_raw(image, form);
		}
		if(success && options->surface) {
			if(form->surface != NULL) {
				nqiv_unload_image_form_sdl_surface(form)
			}
			nqiv_image_load_sdl_surface(image, form);
		}
	}
}

void nqiv_worker_main(nqiv_queue* queue, omp_lock_t* lock)
{
	bool running = true;
	while(running) {
		omp_set_lock(lock);
		nqiv_event event;
		const bool event_found = nqiv_queue_pop(queue, sizeof(nqiv_event), &event)
		if(event_found) {
			switch(event.type) {
				case NQIV_EVENT_WORKER_STOP:
					running = false;
					break;
				case NQIV_EVENT_IMAGE_LOAD:
					nqiv_worker_handle_image_load_form(&event.options.image_load, event.options.image_load.image, event.options.image_load.image->image);
					nqiv_worker_handle_image_load_form(&event.options.image_load, event.options.image_load.image, event.options.image_load.image->thumbnail);
					if(event.options.image_load.create_thumbnail) {
						if(event.options.image_load.image->image->wand == NULL) {
							if( nqiv_image_load_wand(event.options.image_thumbnail_create.image, event.options.image_thumbnail_create.image->image) ) {
								nqiv_thumbnail_create(event.options.image_thumbnail_create.image);
								nqiv_unload_image_form_wand(event.options.image_thumbnail_create.image->image);
							}
						} else {
							nqiv_thumbnail_create(event.options.image_thumbnail_create.image);
						}
					}
					break;
			}
		}
		omp_unset_lock(lock);
		SDL_Event tell_finished;
		tell_finished.type = SDL_USEREVENT;
		tell_finished.user.code = NQIV_SDL_EVENT_WORKER_FINISHED;
		tell_finished.user.data1 = lock;
		SDL_PushEvent(&tell_finished);
	}
}
