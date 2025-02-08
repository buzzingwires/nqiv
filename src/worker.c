#include "platform.h"

#include <stdint.h>
#include <errno.h>
#include <assert.h>

#include <SDL2/SDL.h>
#include <omp.h>

#include "queue.h"
#include "event.h"
#include "image.h"
#include "thumbnail.h"
#include "state.h"
#include "worker.h"

bool nqiv_worker_bins_kv_to_string(const char* key, const int* values, nqiv_array* builder)
{
	bool started = false;
	bool success = true;
	int  idx;
	for(idx = 0; values[idx] >= 0; ++idx) {
		assert(values[idx] != 0);
		if(!started) {
			success = success && nqiv_array_push_str(builder, key);
			success = success && nqiv_array_push_str(builder, " ");
			started = true;
		}
		success = success && nqiv_array_push_str(builder, nqiv_event_priority_names[values[idx]]);
		success = success && nqiv_array_push_str(builder, ",");
	}
	return success;
}

bool nqiv_worker_int_kv_to_string(const char* key, const int value, nqiv_array* builder)
{
	bool success = true;
	if(value >= 0) {
		success = success && nqiv_array_push_str(builder, key);
		success = success && nqiv_array_push_str(builder, " ");
		success = success && nqiv_array_push_sprintf(builder, "%d ", value);
	}
	return success;
}

bool nqiv_worker_spec_to_string(const nqiv_worker_spec* spec, char* string)
{
	bool       success = true;
	nqiv_array builder;
	nqiv_array_inherit(&builder, string, sizeof(char), NQIV_WORKER_SPEC_STRLEN);
	success =
		success && nqiv_worker_int_kv_to_string("extra_wakeup_delay", spec->delay_base, &builder);
	success =
		success && nqiv_worker_int_kv_to_string("event_interval", spec->event_interval, &builder);
	success =
		success && nqiv_worker_bins_kv_to_string("priorities", &(spec->queue_bins[1]), &builder);
	if(success
	   && (string[nqiv_array_get_last_idx(&builder)] == ' '
	       || string[nqiv_array_get_last_idx(&builder)] == ',')) {
		string[nqiv_array_get_last_idx(&builder)] = '\0';
	}
	return success;
}

int nqiv_worker_string_to_int(
	const char* string, const int idx, const int value_min, const int value_max, int* output)
{
	const char* start = string + idx;
	char*       end = NULL;
	const int   tmp = nqiv_strtoi(start, &end, 10);
	if(errno != ERANGE && end != NULL && end != start && tmp >= value_min && tmp <= value_max) {
		*output = tmp;
		assert(end - start < NQIV_WORKER_SPEC_STRLEN);
		return (int)(idx + (end - start));
	}
	return -1;
}

int nqiv_cmd_scan_comma_list_sep(const char* data, const int start, const int end, int* length)
{
	const char* sep[] = {" ", "\t", ",", NULL};
	return nqiv_cmd_scan_subs(data, start, end, false, sep, length);
}

int nqiv_worker_string_to_bin_list(const char* string,
                                   const int   idx,
                                   const int   end_idx,
                                   int*        output)
{
	int oidx = 1;
	int cidx = idx;
	while(true) {
		assert(oidx != 0);
		int nidx = nqiv_cmd_scan_not_whitespace(string, cidx, end_idx, NULL);
		if(nidx == -1) {
			return cidx; /* Nothing more to parse. Caller's responsibility. */
		}
		if(cidx != idx) {
			if(string[nidx] != ',') {
				return cidx; /* Did not find a comma marking the next option. End of list */
			}
			nidx += 1;
		}
		nidx = nqiv_cmd_scan_not_whitespace(string, nidx, end_idx, NULL);
		if(nidx == -1) {
			return -1; /* Trailing comma. Not allowed. */
		}

		int seg_end_idx = nqiv_cmd_scan_comma_list_sep(string, nidx, end_idx, NULL);
		if(seg_end_idx == -1) {
			seg_end_idx = end_idx;
		}

		const nqiv_event_priority priority =
			nqiv_text_to_event_priority(string + nidx, seg_end_idx - nidx);
		if(priority == NQIV_EVENT_PRIORITY_UNKNOWN) {
			return cidx; /* Let caller decide what to do with unknown name. */
		}

		output[oidx] = priority;
		cidx = seg_end_idx;
		++oidx;

		if(oidx > THREAD_QUEUE_BIN_COUNT) {
			return -1;
		}
	}
}

bool nqiv_worker_string_to_spec(const char* string, nqiv_worker_spec* spec)
{
	nqiv_worker_spec new_spec = {.delay_base = -1, .event_interval = -1, .queue_bins = {0}};
	int              idx;
	for(idx = 1; idx < THREAD_QUEUE_BIN_COUNT + 1; ++idx) {
		new_spec.queue_bins[idx] = -1;
	}
	assert(new_spec.queue_bins[0] == 0);
	bool      success = true;
	const int end_idx = nqiv_strlen(string);
	if(end_idx >= NQIV_WORKER_SPEC_STRLEN) {
		return false;
	}
	char key[NQIV_WORKER_SPEC_STRLEN] = {0};
	idx = 0;
	while(success) {
		assert(idx <= end_idx);
		const int nidx = nqiv_cmd_scan_not_whitespace(string, idx, end_idx, NULL);
		if(nidx == -1) {
			break;
		}
		if(idx != 0 && idx == nidx) {
			/* Make sure there are spaces between items. */
			success = false;
			break;
		}
		idx = nidx;
		if(key[0] == '\0') {
			int seg_end_idx = nqiv_cmd_scan_whitespace(string, idx, end_idx, NULL);
			if(seg_end_idx == -1) {
				seg_end_idx = end_idx;
			}
			memcpy(key, string + idx, seg_end_idx - idx);
			idx = seg_end_idx;
		} else {
			if(strcmp(key, "extra_wakeup_delay") == 0) {
				idx = nqiv_worker_string_to_int(string, idx, 0, INT_MAX, &new_spec.delay_base);
			} else if(strcmp(key, "event_interval") == 0) {
				idx = nqiv_worker_string_to_int(string, idx, 0, INT_MAX, &new_spec.event_interval);
			} else if(strcmp(key, "priorities") == 0) {
				idx = nqiv_worker_string_to_bin_list(string, idx, end_idx, new_spec.queue_bins);
			} else {
				success = false;
				break;
			}
			if(idx == -1) {
				success = false;
				break;
			}
			memset(key, 0, NQIV_WORKER_SPEC_STRLEN);
		}
	}
	if(success && key[0] == '\0') {
		assert(new_spec.queue_bins[THREAD_QUEUE_BIN_COUNT] == -1);
		memcpy(spec, &new_spec, sizeof(nqiv_worker_spec));
		return true;
	}
	return false;
}

void nqiv_worker_handle_image_load_form(const nqiv_event_image_load_form_options* options,
                                        nqiv_image*                               image,
                                        nqiv_image_form*                          form)
{
	if(options->unload) {
		if(options->surface || (options->surface_soft && form->texture != NULL)) {
			nqiv_unload_image_form_surface(form);
		}
		if(options->vips || (options->vips_soft && form->texture != NULL)) {
			nqiv_unload_image_form_vips(form);
		}
	} else {
		if(form->texture != NULL && !(options->vips || options->surface)) {
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
                      const int*           queue_bins,
                      const Uint32         event_code,
                      nqiv_shared_var*     transaction_group,
                      nqiv_shared_var*     active_count,
                      nqiv_shared_var*     running)
{
	/* Stagger events by their thread num to prevent stampeding herd problems. */
	int wait_time = delay_base + omp_get_thread_num();
	int events_processed = 0;
	while(nqiv_shared_var_get_op_result(running) == NQIV_SUCCESS) {
		nqiv_event event = {0};
		bool       event_found = false;
		/* Find valid events */
		if(events_processed < event_interval || event_interval == 0) {
			while(true) {
				event_found = nqiv_priority_queue_pop_bins(queue, queue_bins, &event);
				if(!event_found || event.transaction_group == -1) {
					break;
				}
				if(event.transaction_group >= nqiv_shared_var_get_int(transaction_group)) {
					break;
					/* NOOP */
				}
			}
		}
		if(event_found) {
			events_processed += 1;
			switch(event.type) {
			case NQIV_EVENT_WORKER_STOP:
				nqiv_log_write(logger, NQIV_LOG_DEBUG, "Received stop event on thread %d.\n",
				               omp_get_thread_num());
				break;
			case NQIV_EVENT_IMAGE_LOAD:
				{
					nqiv_shared_var_inc_int(active_count);
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
					nqiv_shared_var_dec_int(active_count);
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
					nqiv_shared_var_set_op_result(running, NQIV_FAIL);
				}
			} else {
				SDL_Delay(wait_time);
			}
		}
	}
}
