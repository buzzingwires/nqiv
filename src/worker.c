#include <stdint.h>
#include <assert.h>

#include <SDL2/SDL.h>
#include <omp.h>

#include "queue.h"
#include "event.h"
#include "image.h"
#include "thumbnail.h"
#include "worker.h"

void nqiv_worker_handle_image_load_form(const nqiv_event_image_load_form_options* options,
                                        nqiv_image*                               image,
                                        nqiv_image_form*                          form)
{
	if(options->unload) {
		if(options->surface || (options->surface_soft && form->texture != NULL)) {
			nqiv_unload_image_form_surface(form);
		}
		if(options->raw || (options->raw_soft && form->texture != NULL)) {
			nqiv_unload_image_form_raw(form);
		}
		if(options->vips || (options->vips_soft && form->texture != NULL)) {
			nqiv_unload_image_form_vips(form);
		}
	} else {
		if(form->texture != NULL) {
			return;
		}
		bool success = true;
		if(options->vips || options->vips_soft) {
			if((form->vips != NULL && options->vips) || form->vips == NULL) {
				if(form->vips != NULL) {
					assert(options->vips);
					nqiv_unload_image_form_vips(form);
				}
				/* Load thumbnail if allowed. If not, or if fail, we create the thumbnail VIPS data
				 * from the image for rendering, without actually generating a full on-disk
				 * thumbnail. */
				if(form == &image->thumbnail) {
					if(image->parent->thumbnail.load && !form->thumbnail_load_failed) {
						success = nqiv_image_load_vips(image, form);
						form->thumbnail_load_failed = !success;
					} else {
						success = false;
					}
					if(!success) {
						if(image->image.vips != NULL) {
							success = nqiv_thumbnail_create_vips(image);
						} else {
							if(nqiv_image_load_vips(image, &image->image)) {
								success = nqiv_thumbnail_create_vips(image);
							} else {
								success = false;
							}
						}
						form->error = !success;
					}
				} else {
					success = nqiv_image_load_vips(image, form);
				}
			}
		}
		if(success && options->first_frame) {
			success = nqiv_image_form_first_frame(image, form);
		}
		if(success && options->next_frame) {
			success = nqiv_image_form_next_frame(image, form);
		}
		if(success && (options->raw || options->raw_soft)) {
			if(form->data != NULL) {
				if(options->raw) {
					nqiv_unload_image_form_raw(form);
					success = nqiv_image_load_raw(image, form);
				}
			} else {
				success = nqiv_image_load_raw(image, form);
			}
		}
		if(success && (options->surface || options->surface_soft)) {
			if(form->surface != NULL) {
				if(options->surface) {
					nqiv_unload_image_form_surface(form);
					success = nqiv_image_load_surface(image, form);
				}
			} else {
				success = nqiv_image_load_surface(image, form);
			}
		}
		(void)success;
	}
}

void nqiv_worker_handle_image_load_form_clear_error(
	const nqiv_event_image_load_form_options* options, nqiv_image_form* form)
{
	if(options->clear_error) {
		form->error = false;
	}
}

void nqiv_worker_main(nqiv_log_ctx*        logger,
                      nqiv_priority_queue* queue,
                      const int            delay_base,
                      const int            event_interval,
                      const Uint32         event_code,
                      const int64_t*       transaction_group,
                      omp_lock_t*          transaction_group_lock)
{
	/* Stagger events by their thread num to prevent stampeding herd problems. */
	int  wait_time = delay_base + omp_get_thread_num();
	bool running = true;
	int  events_processed = 0;
	while(running) {
		nqiv_event event = {0};
		bool       event_found = false;
		/* Find valid events */
		if(events_processed < event_interval || event_interval == 0) {
			while(true) {
				event_found = nqiv_priority_queue_pop(queue, &event);
				if(!event_found || event.transaction_group == -1) {
					break;
				}
				omp_set_lock(transaction_group_lock);
				if(event.transaction_group >= *transaction_group) {
					omp_unset_lock(transaction_group_lock);
					break;
					/* NOOP */
				}
				omp_unset_lock(transaction_group_lock);
			}
		}
		if(event_found) {
			events_processed += 1;
			switch(event.type) {
			case NQIV_EVENT_WORKER_STOP:
				nqiv_log_write(logger, NQIV_LOG_DEBUG, "Received stop event on thread %d.\n",
				               omp_get_thread_num());
				running = false;
				break;
			case NQIV_EVENT_IMAGE_LOAD:
				{
					nqiv_event_image_load_options* image_load = &event.options.image_load;
					nqiv_image*                    image = image_load->image;
					nqiv_log_write(logger, NQIV_LOG_DEBUG,
					               "Received image load event on thread %d.\n",
					               omp_get_thread_num());
					nqiv_image_lock(image);
					nqiv_worker_handle_image_load_form_clear_error(&image_load->thumbnail_options,
					                                               &image->thumbnail);
					nqiv_worker_handle_image_load_form_clear_error(&image_load->image_options,
					                                               &image->image);
					if((!image->thumbnail_attempted && image_load->set_thumbnail_path
					    && image->parent->thumbnail.root != NULL)
					   && image->thumbnail.path == NULL
					   && !nqiv_thumbnail_calculate_path(image, &image->thumbnail.path, false)) {
						nqiv_log_write(image->parent->logger, NQIV_LOG_ERROR,
						               "Failed to create thumbnail path for %s\n",
						               image->image.path);
						image->thumbnail_attempted = true;
					}
					nqiv_worker_handle_image_load_form(&image_load->image_options, image,
					                                   &image->image);
					if(!image->thumbnail_attempted && image_load->create_thumbnail) {
						/* If we can load the thumbnail and are allowed to create it, then make sure
						 * it also is up to date. This involves loading the image form, as well. */
						if(image->thumbnail.vips == NULL
						   && nqiv_image_load_vips(image, &image->thumbnail)) {
							if(image->image.vips == NULL) {
								if(nqiv_image_load_vips(image, &image->image)
								   && !nqiv_thumbnail_matches_image(image)) {
									image->thumbnail.thumbnail_load_failed =
										!nqiv_thumbnail_create(image)
										&& image->thumbnail.thumbnail_load_failed;
								}
							} else if(!nqiv_thumbnail_matches_image(image)) {
								image->thumbnail.thumbnail_load_failed =
									!nqiv_thumbnail_create(image)
									&& image->thumbnail.thumbnail_load_failed;
							}
							/* Otherwise, load the image vips and create the thumbnail from scratch.
							 */
						} else {
							if(image->image.vips == NULL) {
								if(nqiv_image_load_vips(image, &image->image)) {
									image->thumbnail.thumbnail_load_failed =
										!nqiv_thumbnail_create(image)
										&& image->thumbnail.thumbnail_load_failed;
								}
							} else {
								image->thumbnail.thumbnail_load_failed =
									!nqiv_thumbnail_create(image)
									&& image->thumbnail.thumbnail_load_failed;
							}
						}
						image->thumbnail_attempted = true;
					}
					nqiv_worker_handle_image_load_form(&image_load->thumbnail_options, image,
					                                   &image->thumbnail);
					if(image_load->borrow_thumbnail_dimension_metadata) {
						nqiv_image_borrow_thumbnail_dimensions(image);
					}
					nqiv_image_unlock(image);
					break;
				}
			}
		} else {
			/* No more events? Inform master if any events have been processed. Otherwise, sleep for
			 * polling interval. */
			if(events_processed > 0) {
				nqiv_log_write(logger, NQIV_LOG_DEBUG,
				               "Thread %d waking master after processing %d events\n",
				               omp_get_thread_num(), events_processed);
				events_processed = 0;
				SDL_Event tell_finished = {0};
				tell_finished.type = SDL_USEREVENT;
				tell_finished.user.code = (Sint32)event_code;
				if(SDL_PushEvent(&tell_finished) < 0) {
					nqiv_log_write(logger, NQIV_LOG_ERROR,
					               "Failed to send SDL event from thread %d. SDL Error: %s\n",
					               omp_get_thread_num(), SDL_GetError());
					running = false;
				}
			} else {
				SDL_Delay(wait_time);
			}
		}
	}
}
