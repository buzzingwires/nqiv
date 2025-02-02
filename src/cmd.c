#include "platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <float.h>
#include <inttypes.h>
#include <assert.h>

#include <SDL2/SDL.h>
#include <omp.h>

#include "logging.h"
#include "array.h"
#include "state.h"
#include "keybinds.h"
#include "keyrate.h"
#include "pruner.h"

bool nqiv_cmd_alert_main(nqiv_cmd_manager* manager)
{
	SDL_Event e = {0};
	e.type = SDL_USEREVENT;
	e.user.code = (Sint32)manager->state->cfg_event_number;
	if(SDL_PushEvent(&e) < 0) {
		nqiv_log_write(&manager->state->logger, NQIV_LOG_ERROR,
		               "Failed to send SDL event from thread %d. SDL Error: %s\n",
		               omp_get_thread_num(), SDL_GetError());
		return false;
	}
	return true;
}

void nqiv_cmd_force_quit_main(nqiv_cmd_manager* manager)
{
	nqiv_shared_var_set_op_result(&manager->state->running, NQIV_FAIL);
}

char nqiv_cmd_tmpterm(char* data, const int pos)
{
	const char value = data[pos];
	data[pos] = '\0';
	return value;
}

void nqiv_cmd_tmpret(char* data, const int pos, const char value)
{
	data[pos] = value;
}

void nqiv_cmd_set_and_flag_new_int(int* storage, const int value, bool* flag)
{
	*flag = *flag || *storage != value;
	*storage = value;
}

bool nqiv_cmd_parser_set_none(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	(void)manager;
	(void)tokens;
	return true;
}

bool nqiv_cmd_parser_set_thread_count(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	nqiv_cmd_set_and_flag_new_int(&(manager->state->pending_thread_count), tokens[0]->value.as_int,
	                              &(manager->state->restart_threads));
	return true;
}

bool nqiv_cmd_parser_set_thread_event_interval(nqiv_cmd_manager*    manager,
                                               nqiv_cmd_arg_token** tokens)
{
	nqiv_cmd_set_and_flag_new_int(&(manager->state->thread_event_interval), tokens[0]->value.as_int,
	                              &(manager->state->restart_threads));
	return true;
}

bool nqiv_cmd_parser_set_vips_threads(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->vips_threads = tokens[0]->value.as_int;
	vips_concurrency_set(tokens[0]->value.as_int);
	return true;
}

bool nqiv_cmd_parser_set_extra_wakeup_delay(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	nqiv_cmd_set_and_flag_new_int(&(manager->state->extra_wakeup_delay), tokens[0]->value.as_int,
	                              &(manager->state->restart_threads));
	return true;
}

bool nqiv_cmd_parser_set_zoom_default(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	const char              data_end = nqiv_cmd_tmpterm(tokens[0]->raw, tokens[0]->length);
	const nqiv_zoom_default zd = nqiv_text_to_zoom_default(tokens[0]->raw);
	nqiv_cmd_tmpret(tokens[0]->raw, tokens[0]->length, data_end);
	if(zd == NQIV_ZOOM_DEFAULT_UNKNOWN) {
		return false;
	}
	manager->state->zoom_default = zd;
	return true;
}

bool nqiv_cmd_parser_set_zoom_scale_mode(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	const char    data_end = nqiv_cmd_tmpterm(tokens[0]->raw, tokens[0]->length);
	SDL_ScaleMode sm;
	const bool    result = nqiv_text_to_scale_mode(tokens[0]->raw, &sm);
	nqiv_cmd_tmpret(tokens[0]->raw, tokens[0]->length, data_end);
	if(!result) {
		return false;
	}
	manager->state->texture_scale_mode = sm;
	return true;
}

bool nqiv_cmd_parser_set_thumbnail_size(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	const int old_size = manager->state->images.thumbnail.size;
	manager->state->images.thumbnail.size = tokens[0]->value.as_int;
	return nqiv_image_manager_reattempt_thumbnails(&manager->state->images, old_size);
}

bool nqiv_cmd_parser_set_thumbnail_path(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	const char data_end = nqiv_cmd_tmpterm(tokens[0]->raw, tokens[0]->length);
	const bool output =
		nqiv_image_manager_set_thumbnail_root(&manager->state->images, tokens[0]->raw);
	nqiv_cmd_tmpret(tokens[0]->raw, tokens[0]->length, data_end);
	return output;
}

bool nqiv_cmd_parser_set_log_level(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	omp_set_lock(&manager->state->logger.lock);
	const char data_end = nqiv_cmd_tmpterm(tokens[0]->raw, tokens[0]->length);
	manager->state->logger.level = tokens[0]->value.as_log_level;
	nqiv_cmd_tmpret(tokens[0]->raw, tokens[0]->length, data_end);
	omp_unset_lock(&manager->state->logger.lock);
	return true;
}

bool nqiv_cmd_parser_set_log_prefix(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	omp_set_lock(&manager->state->logger.lock);
	const char data_end = nqiv_cmd_tmpterm(tokens[0]->raw, tokens[0]->length);
	nqiv_log_set_prefix_format(&manager->state->logger, tokens[0]->raw);
	nqiv_cmd_tmpret(tokens[0]->raw, tokens[0]->length, data_end);
	omp_unset_lock(&manager->state->logger.lock);
	return true;
}

bool nqiv_cmd_parser_apply_color(nqiv_cmd_manager*    manager,
                                 nqiv_cmd_arg_token** tokens,
                                 SDL_Color*           color,
                                 const char*          error_message,
                                 bool                 apply(nqiv_state* state))
{
	SDL_Color tmp;
	memcpy(&tmp, color, sizeof(SDL_Color));
	color->r = tokens[0]->value.as_Uint8;
	color->g = tokens[1]->value.as_Uint8;
	color->b = tokens[2]->value.as_Uint8;
	color->a = tokens[3]->value.as_Uint8;
	if(!apply(manager->state)) {
		nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING, "Failed to apply color for %s\n",
		               error_message);
		memcpy(color, &tmp, sizeof(SDL_Color));
		return false;
	}
	return true;
}

bool nqiv_cmd_parser_set_alpha_background_color_one(nqiv_cmd_manager*    manager,
                                                    nqiv_cmd_arg_token** tokens)
{
	return nqiv_cmd_parser_apply_color(manager, tokens, &manager->state->alpha_checker_color_one,
	                                   "alpha background color one",
	                                   nqiv_state_recreate_all_alpha_background_textures);
}

bool nqiv_cmd_parser_set_alpha_background_color_two(nqiv_cmd_manager*    manager,
                                                    nqiv_cmd_arg_token** tokens)
{
	return nqiv_cmd_parser_apply_color(manager, tokens, &manager->state->alpha_checker_color_two,
	                                   "alpha background color two",
	                                   nqiv_state_recreate_all_alpha_background_textures);
}

bool nqiv_cmd_parser_set_background_color(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	return nqiv_cmd_parser_apply_color(manager, tokens, &manager->state->background_color,
	                                   "background color", nqiv_state_recreate_background_texture);
}

bool nqiv_cmd_parser_set_error_color(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	return nqiv_cmd_parser_apply_color(manager, tokens, &manager->state->error_color, "error color",
	                                   nqiv_state_recreate_error_texture);
}

bool nqiv_cmd_parser_set_loading_color(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	return nqiv_cmd_parser_apply_color(manager, tokens, &manager->state->loading_color,
	                                   "loading background color",
	                                   nqiv_state_recreate_loading_texture);
}

bool nqiv_cmd_parser_set_selection_color(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	return nqiv_cmd_parser_apply_color(manager, tokens, &manager->state->selection_color,
	                                   "selection outline color",
	                                   nqiv_state_recreate_thumbnail_selection_texture);
}

bool nqiv_cmd_parser_set_mark_color(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	return nqiv_cmd_parser_apply_color(manager, tokens, &manager->state->mark_color,
	                                   "mark outline color", nqiv_state_recreate_mark_texture);
}

bool nqiv_cmd_parser_set_window_height(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	int w;
	int h;
	SDL_GetWindowSizeInPixels(manager->state->window, &w, &h);
	SDL_SetWindowSize(manager->state->window, w, tokens[0]->value.as_int);
	return true;
}

bool nqiv_cmd_parser_set_window_width(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	int w;
	int h;
	SDL_GetWindowSizeInPixels(manager->state->window, &w, &h);
	SDL_SetWindowSize(manager->state->window, tokens[0]->value.as_int, h);
	return true;
}

bool nqiv_cmd_parser_append_pruner(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	return nqiv_pruner_append(&manager->state->pruner, &tokens[0]->value.as_pruner);
}

bool nqiv_cmd_parser_append_image(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	const char data_end = nqiv_cmd_tmpterm(tokens[0]->raw, tokens[0]->length);
	bool       output = true;
	output = nqiv_image_manager_append(&manager->state->images, tokens[0]->raw);
	nqiv_cmd_tmpret(tokens[0]->raw, tokens[0]->length, data_end);
	return output;
}

bool nqiv_cmd_parser_append_keybind(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	return nqiv_keybind_add(&manager->state->keybinds, &tokens[0]->value.as_keybind);
}

bool nqiv_cmd_parser_append_thread(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	if(!nqiv_array_push(manager->state->thread_specs, &tokens[0]->value.as_worker_spec)) {
		return false;
	}
	manager->state->restart_threads = true;
	return true;
}

bool nqiv_cmd_parser_append_log_stream(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	omp_set_lock(&manager->state->logger.lock);
	const char data_end = nqiv_cmd_tmpterm(tokens[0]->raw, tokens[0]->length);
	const bool output = nqiv_add_logger_path(manager->state, tokens[0]->raw);
	nqiv_cmd_tmpret(tokens[0]->raw, tokens[0]->length, data_end);
	omp_unset_lock(&manager->state->logger.lock);
	return output;
}

bool nqiv_cmd_parser_insert_image(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	const char data_end = nqiv_cmd_tmpterm(tokens[1]->raw, tokens[1]->length);
	bool       output = true;
	output =
		nqiv_image_manager_insert(&manager->state->images, tokens[1]->raw, tokens[0]->value.as_int);
	nqiv_cmd_tmpret(tokens[1]->raw, tokens[1]->length, data_end);
	return output;
}

bool nqiv_cmd_parser_remove_image_index(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	return nqiv_image_manager_remove(&manager->state->images, tokens[0]->value.as_int);
}

bool nqiv_cmd_parser_sendkey(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	const nqiv_keybind_pair* pair =
		&(manager->state->keybinds.simulated_lookup[tokens[0]->value.as_key_action]);
	return nqiv_queue_push(&manager->state->key_actions, &pair) && nqiv_cmd_alert_main(manager);
}

void nqiv_cmd_print_indent(const nqiv_cmd_manager* manager)
{
	int indent_count;
	for(indent_count = manager->print_settings.indent; indent_count > 0; --indent_count) {
		fprintf(stdout, "\t");
	}
}

void nqiv_cmd_parser_print_none(nqiv_cmd_manager* manager)
{
	(void)manager;
	fprintf(stdout, "NONE");
}

void nqiv_cmd_parser_print_log_error_message(nqiv_cmd_manager* manager)
{
	omp_set_lock(&manager->state->logger.lock);
	if(strlen(manager->state->logger.error_message) != 0) {
		fprintf(stdout, "%s", manager->state->logger.error_message);
	} else {
		fprintf(stdout, "EMPTY");
	}
	omp_unset_lock(&manager->state->logger.lock);
}

bool nqiv_cmd_parser_set_data_double(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	*((double*)manager->print_settings.current_node->data) = tokens[0]->value.as_double;
	return true;
}

bool nqiv_cmd_parser_set_data_int(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	*((int*)manager->print_settings.current_node->data) = tokens[0]->value.as_int;
	return true;
}

bool nqiv_cmd_parser_set_data_uint64(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	*((Uint64*)manager->print_settings.current_node->data) = tokens[0]->value.as_Uint64;
	return true;
}

bool nqiv_cmd_parser_set_data_bool(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	*((bool*)manager->print_settings.current_node->data) = tokens[0]->value.as_bool;
	return true;
}

void nqiv_cmd_parser_print_data_double(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%f", *((double*)manager->print_settings.current_node->data));
}

void nqiv_cmd_parser_print_data_int(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%d", *((int*)manager->print_settings.current_node->data));
}

void nqiv_cmd_parser_print_value_bool(const char* name, const bool value)
{
	fprintf(stdout, "%s: %s ", name, value ? "TRUE" : "FALSE");
}

void nqiv_cmd_parser_print_value_is_null(const char* name, const void* value)
{
	fprintf(stdout, "%s: %s ", name, value != NULL ? "SET" : "UNSET");
}

void nqiv_cmd_parser_print_value_sdlrect(const char* name, const SDL_Rect* value)
{
	fprintf(stdout, "%s: %dx%d+%d+%d ", name, value->w, value->h, value->x, value->y);
}

void nqiv_cmd_parser_print_value_image_form(nqiv_cmd_manager* manager, const nqiv_image_form* form)
{
	fprintf(stdout, "path: %s\n", form->path);
	manager->print_settings.indent += 1;
	nqiv_cmd_print_indent(manager);
	fprintf(stdout, "Dimensions %dx%d Effective Dimensions %dx%d ", form->width, form->height,
	        form->effective_width, form->effective_height);
	nqiv_cmd_parser_print_value_bool("master_dimensions_set", form->master_dimensions_set);
	nqiv_cmd_parser_print_value_bool("thumbnail_load_failed", form->thumbnail_load_failed);
	nqiv_cmd_parser_print_value_bool("error", form->error);
	fprintf(stdout, "\n");
	nqiv_cmd_print_indent(manager);
	nqiv_cmd_parser_print_value_is_null("VIPS", form->vips);
	nqiv_cmd_parser_print_value_is_null("Raw", form->data);
	nqiv_cmd_parser_print_value_is_null("Surface", form->surface);
	nqiv_cmd_parser_print_value_is_null("texture", form->texture);
	nqiv_cmd_parser_print_value_is_null("fallback_texture", form->fallback_texture);
	fprintf(stdout, "\n");
	nqiv_cmd_print_indent(manager);
	nqiv_cmd_parser_print_value_sdlrect("srcrect", &form->srcrect);
	nqiv_cmd_parser_print_value_sdlrect("master_srcrect", &form->master_srcrect);
	nqiv_cmd_parser_print_value_sdlrect("master_dstrect", &form->master_dstrect);
	fprintf(stdout, "\n");
	nqiv_cmd_print_indent(manager);
	fprintf(stdout, "ANIMATION: ");
	nqiv_cmd_parser_print_value_bool("animation.exists", form->animation.exists);
	nqiv_cmd_parser_print_value_bool("animation.frame_rendered", form->animation.frame_rendered);
	fprintf(stdout, "frame: %d ", form->animation.frame);
	fprintf(stdout, "frame_count: %d ", form->animation.frame_count);
	fprintf(stdout, "last_frame_time: %" PRIu64 " ", form->animation.last_frame_time);
	fprintf(stdout, "delay: %" PRIu32 " ", form->animation.delay);
	fprintf(stdout, "\n");
	manager->print_settings.indent -= 1;
}

void nqiv_cmd_parser_print_value_image(nqiv_cmd_manager* manager, const nqiv_image* image)
{
	nqiv_cmd_parser_print_value_bool("marked", image->thumbnail_attempted);
	nqiv_cmd_parser_print_value_bool("thumbnail_attempted", image->thumbnail_attempted);
	fprintf(stdout, "\n");
	nqiv_cmd_print_indent(manager);
	fprintf(stdout, "IMAGE FORM: ");
	nqiv_cmd_parser_print_value_image_form(manager, &image->image);
	nqiv_cmd_print_indent(manager);
	fprintf(stdout, "THUMBNAIL FORM: ");
	nqiv_cmd_parser_print_value_image_form(manager, &image->thumbnail);
}

void nqiv_cmd_parser_print_data_images(nqiv_cmd_manager* manager)
{
	assert(!manager->print_settings.dumpcfg);
	nqiv_array*  images_array = (nqiv_array*)(manager->print_settings.current_node->data);
	nqiv_image** images = images_array->data;
	const int    num_images = nqiv_array_get_units_count(images_array);
	int          idx;
	manager->print_settings.indent += 1;
	for(idx = 0; idx < num_images; ++idx) {
		fprintf(stdout, "\n");
		nqiv_cmd_print_indent(manager);
		nqiv_cmd_parser_print_value_image(manager, images[idx]);
	}
	manager->print_settings.indent -= 1;
}

void nqiv_cmd_parser_print_data_int64(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%" PRIi64, *((Sint64*)manager->print_settings.current_node->data));
}

void nqiv_cmd_parser_print_data_bool(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%s", *((bool*)manager->print_settings.current_node->data) ? "true" : "false");
}

void nqiv_cmd_parser_print_data_uint64(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%" PRIu64, *((Uint64*)manager->print_settings.current_node->data));
}

void nqiv_cmd_parser_print_data_uint32(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%" PRIu32, *((Uint32*)manager->print_settings.current_node->data));
}

void nqiv_cmd_parser_print_data_shared_op_result(nqiv_cmd_manager* manager)
{
	const nqiv_op_result op_result =
		nqiv_shared_var_get_op_result((nqiv_shared_var*)manager->print_settings.current_node->data);
	if(op_result == NQIV_SUCCESS) {
		fprintf(stdout, "SUCCESS");
	} else if(op_result == NQIV_PASS) {
		fprintf(stdout, "PASS");
	} else if(op_result == NQIV_FAIL) {
		fprintf(stdout, "FAIL");
	} else {
		assert(false);
	}
}

void nqiv_cmd_parser_print_data_shared_int64(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%" PRIi64,
	        nqiv_shared_var_get_int((nqiv_shared_var*)manager->print_settings.current_node->data));
}

void nqiv_cmd_parser_print_data_key_action_queue(nqiv_cmd_manager* manager)
{
	assert(!manager->print_settings.dumpcfg);
	nqiv_queue*         key_actions = (nqiv_queue*)(manager->print_settings.current_node->data);
	const int           num_pairs = nqiv_array_get_units_count(key_actions->array);
	nqiv_keybind_pair** pairs = key_actions->array->data;
	int                 idx;
	manager->print_settings.indent += 1;
	for(idx = 0; idx < num_pairs; ++idx) {
		fprintf(stdout, "\n");
		char       buf[NQIV_KEYBIND_STRLEN + 1] = {0};
		const bool result = nqiv_keybind_to_string(pairs[idx], buf);
		assert(result);
		(void)result;
		nqiv_cmd_print_indent(manager);
		fprintf(stdout, "%s\n", buf);
		nqiv_cmd_print_indent(manager);
		fprintf(stdout, "current_delay: %" PRIi64 " next_event_time: %" PRIi64,
		        pairs[idx]->keyrate.ephemeral.current_delay,
		        pairs[idx]->keyrate.ephemeral.next_event_time);
		if(idx != num_pairs - 1) {
			fprintf(stdout, "\n");
		}
	}
	manager->print_settings.indent -= 1;
}

void nqiv_cmd_parser_print_value_image_load_options(
	const nqiv_cmd_manager* manager, const nqiv_event_image_load_form_options* options)
{
	nqiv_cmd_parser_print_value_bool("clear_error", options->clear_error);
	nqiv_cmd_parser_print_value_bool("first_frame", options->first_frame);
	nqiv_cmd_parser_print_value_bool("next_frame", options->next_frame);
	nqiv_cmd_parser_print_value_bool("unload", options->clear_error);
	fprintf(stdout, "\n");
	nqiv_cmd_print_indent(manager);
	nqiv_cmd_parser_print_value_bool("vips", options->vips);
	nqiv_cmd_parser_print_value_bool("vips_soft", options->vips_soft);
	nqiv_cmd_parser_print_value_bool("surface", options->surface);
	nqiv_cmd_parser_print_value_bool("surface_soft", options->surface_soft);
	fprintf(stdout, "\n");
}

void nqiv_cmd_parser_print_data_event_queue(nqiv_cmd_manager* manager)
{
	assert(!manager->print_settings.dumpcfg);
	manager->print_settings.indent += 1;
	nqiv_priority_queue* queue = (nqiv_priority_queue*)(manager->print_settings.current_node->data);
	int                  bin;
	for(bin = 0; bin < queue->bin_count; ++bin) {
		fprintf(stdout, "\n");
		nqiv_cmd_print_indent(manager);
		fprintf(stdout, "BIN %d\n", bin);
		const int   num_events = nqiv_array_get_units_count(queue->bins[bin].array);
		nqiv_event* events = queue->bins[bin].array->data;
		int         idx;
		manager->print_settings.indent += 1;
		for(idx = 0; idx < num_events; ++idx) {
			const nqiv_event* e = &(events[idx]);
			switch(e->type) {
			case NQIV_EVENT_WORKER_STOP:
				nqiv_cmd_print_indent(manager);
				fprintf(stdout, "WORKER_STOP\n");
				manager->print_settings.indent += 1;
				nqiv_cmd_print_indent(manager);
				fprintf(stdout, "transaction_group: %" PRIi64, e->transaction_group);
				manager->print_settings.indent -= 1;
				break;
			case NQIV_EVENT_IMAGE_LOAD:
				nqiv_cmd_print_indent(manager);
				fprintf(stdout, "IMAGE_LOAD\n");
				manager->print_settings.indent += 1;
				nqiv_cmd_print_indent(manager);
				fprintf(stdout, "transaction_group: %" PRIi64 "\n", e->transaction_group);
				nqiv_cmd_print_indent(manager);
				fprintf(stdout,
				        "set_thumbnail_path: %s create_thubmnail: %s "
				        "borrow_thumbnail_dimension_metadata: %s\n",
				        e->options.image_load.set_thumbnail_path ? "TRUE" : "FALSE",
				        e->options.image_load.create_thumbnail ? "TRUE" : "FALSE",
				        e->options.image_load.borrow_thumbnail_dimension_metadata ? "TRUE"
				                                                                  : "FALSE");
				nqiv_cmd_print_indent(manager);
				fprintf(stdout, "IMAGE\n");
				manager->print_settings.indent += 1;
				nqiv_cmd_print_indent(manager);
				nqiv_cmd_parser_print_value_image(manager, e->options.image_load.image);
				manager->print_settings.indent -= 1;
				nqiv_cmd_print_indent(manager);
				fprintf(stdout, "IMAGE OPTIONS\n");
				manager->print_settings.indent += 1;
				nqiv_cmd_print_indent(manager);
				nqiv_cmd_parser_print_value_image_load_options(
					manager, &e->options.image_load.image_options);
				manager->print_settings.indent -= 1;
				nqiv_cmd_print_indent(manager);
				fprintf(stdout, "THUMBNAIL OPTIONS\n");
				manager->print_settings.indent += 1;
				nqiv_cmd_print_indent(manager);
				nqiv_cmd_parser_print_value_image_load_options(
					manager, &e->options.image_load.thumbnail_options);
				manager->print_settings.indent -= 1;
				manager->print_settings.indent -= 1;
				break;
			}
			if(idx != num_events - 1) {
				fprintf(stdout, "\n");
			}
		}
		manager->print_settings.indent -= 1;
	}
	manager->print_settings.indent -= 1;
}

void nqiv_cmd_parser_print_zoom_default(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%s", nqiv_zoom_default_names[manager->state->zoom_default]);
}

void nqiv_cmd_parser_print_zoom_scale_mode(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%s", nqiv_scale_mode_to_text(manager->state->texture_scale_mode));
}

void nqiv_cmd_parser_print_thumbnail_path(nqiv_cmd_manager* manager)
{
	if(manager->state->images.thumbnail.root != NULL) {
		fprintf(stdout, "%s", manager->state->images.thumbnail.root);
	} else if(!manager->print_settings.dumpcfg) {
		fprintf(stdout, "UNSET");
	}
}

void nqiv_cmd_parser_print_queue_size(nqiv_cmd_manager* manager)
{
	if(!manager->print_settings.dumpcfg) {
		fprintf(stdout, "\n");
		nqiv_cmd_print_indent(manager);
		fprintf(stdout, "Keybinds: %d\n", manager->state->keybinds.lookup->data_length);
		nqiv_cmd_print_indent(manager);
		fprintf(stdout, "Images: %d\n", manager->state->images.images->data_length);
		nqiv_cmd_print_indent(manager);
		fprintf(stdout, "Logger streams list: %d\n",
		        manager->state->logger_stream_names->data_length);
		int idx;
		for(idx = 0; idx < manager->state->thread_queue.bin_count; ++idx) {
			nqiv_cmd_print_indent(manager);
			fprintf(stdout, "Thread Queue %d: %d\n", idx,
			        manager->state->thread_queue.bins[idx].array->data_length);
		}
		nqiv_cmd_print_indent(manager);
		fprintf(stdout, "Key Actions: %d\n", manager->state->key_actions.array->data_length);
		nqiv_cmd_print_indent(manager);
		fprintf(stdout, "Cmd Buffer: %d\n", manager->state->cmds.buffer->data_length);
	} else {
		fprintf(stdout, "#Keybinds: %d\n", manager->state->keybinds.lookup->data_length);
		fprintf(stdout, "#Images: %d\n", manager->state->images.images->data_length);
		fprintf(stdout, "#Logger streams list: %d\n",
		        manager->state->logger_stream_names->data_length);
		int idx;
		for(idx = 0; idx < manager->state->thread_queue.bin_count; ++idx) {
			fprintf(stdout, "#Thread Queue %d: %d\n", idx,
			        manager->state->thread_queue.bins[idx].array->data_length);
		}
		fprintf(stdout, "#Key Actions: %d\n", manager->state->key_actions.array->data_length);
		fprintf(stdout, "#Cmd Buffer: %d", manager->state->cmds.buffer->data_length);
	}
}

void nqiv_cmd_parser_print_log_level(nqiv_cmd_manager* manager)
{
	omp_set_lock(&manager->state->logger.lock);
	fprintf(stdout, "%s", nqiv_log_level_names[manager->state->logger.level / 10]);
	omp_unset_lock(&manager->state->logger.lock);
}

void nqiv_cmd_parser_print_log_prefix(nqiv_cmd_manager* manager)
{
	omp_set_lock(&manager->state->logger.lock);
	if(strlen(manager->state->logger.prefix_format) == 0 && !manager->print_settings.dumpcfg) {
		fprintf(stdout, "UNSET");
	} else {
		fprintf(stdout, "%s", manager->state->logger.prefix_format);
	}
	omp_unset_lock(&manager->state->logger.lock);
}

void nqiv_cmd_parser_print_alpha_background_color_one(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%hhu %hhu %hhu %hhu", manager->state->alpha_checker_color_one.r,
	        manager->state->alpha_checker_color_one.g, manager->state->alpha_checker_color_one.b,
	        manager->state->alpha_checker_color_one.a);
}

void nqiv_cmd_parser_print_alpha_background_color_two(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%hhu %hhu %hhu %hhu", manager->state->alpha_checker_color_two.r,
	        manager->state->alpha_checker_color_two.g, manager->state->alpha_checker_color_two.b,
	        manager->state->alpha_checker_color_two.a);
}

void nqiv_cmd_parser_print_background_color(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%hhu %hhu %hhu %hhu", manager->state->background_color.r,
	        manager->state->background_color.g, manager->state->background_color.b,
	        manager->state->background_color.a);
}

void nqiv_cmd_parser_print_error_color(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%hhu %hhu %hhu %hhu", manager->state->error_color.r,
	        manager->state->error_color.g, manager->state->error_color.b,
	        manager->state->error_color.a);
}

void nqiv_cmd_parser_print_loading_color(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%hhu %hhu %hhu %hhu", manager->state->loading_color.r,
	        manager->state->loading_color.g, manager->state->loading_color.b,
	        manager->state->loading_color.a);
}

void nqiv_cmd_parser_print_selection_color(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%hhu %hhu %hhu %hhu", manager->state->selection_color.r,
	        manager->state->selection_color.g, manager->state->selection_color.b,
	        manager->state->selection_color.a);
}

void nqiv_cmd_parser_print_mark_color(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%hhu %hhu %hhu %hhu", manager->state->mark_color.r,
	        manager->state->mark_color.g, manager->state->mark_color.b,
	        manager->state->mark_color.a);
}

void nqiv_cmd_parser_print_window_height(nqiv_cmd_manager* manager)
{
	int w;
	int h;
	SDL_GetWindowSizeInPixels(manager->state->window, &w, &h);
	fprintf(stdout, "%d", h);
}

void nqiv_cmd_parser_print_window_width(nqiv_cmd_manager* manager)
{
	int w;
	int h;
	SDL_GetWindowSizeInPixels(manager->state->window, &w, &h);
	fprintf(stdout, "%d", w);
}

void nqiv_cmd_print_str_list(const nqiv_cmd_manager* manager, const nqiv_array* list)
{
	const int    list_len = nqiv_array_get_units_count(list);
	const char** strs = list->data;
	bool         taken = false;
	int          idx;
	for(idx = 0; idx < list_len; ++idx) {
		const char* str = strs[idx];
		if(str != NULL) {
			taken = true;
			if(!manager->print_settings.dumpcfg) {
				if(idx == 0) {
					fprintf(stdout, "\n");
				}
				nqiv_cmd_print_indent(manager);
				fprintf(stdout, "%s\n", str);
			} else if(idx == list_len - 1) {
				fprintf(stdout, "%s%s", manager->print_settings.prefix, str);
			} else {
				fprintf(stdout, "%s%s\n", manager->print_settings.prefix, str);
			}
		}
	}
	if(!taken && manager->print_settings.dumpcfg) {
		fprintf(stdout, "#%s", manager->print_settings.prefix);
	}
}

void nqiv_cmd_parser_print_log_stream(nqiv_cmd_manager* manager)
{
	omp_set_lock(&manager->state->logger.lock);
	nqiv_cmd_print_str_list(manager, manager->state->logger_stream_names);
	omp_unset_lock(&manager->state->logger.lock);
}

void nqiv_cmd_parser_print_pruner(nqiv_cmd_manager* manager)
{
	const int         list_len = nqiv_array_get_units_count(manager->state->pruner.pruners);
	nqiv_pruner_desc* pruners = manager->state->pruner.pruners->data;
	bool              taken = false;
	int               idx;
	for(idx = 0; idx < list_len; ++idx) {
		const nqiv_pruner_desc* desc = &pruners[idx];
		taken = true;
		char       desc_str[NQIV_PRUNER_DESC_STRLEN + 1] = {0};
		const bool result = nqiv_pruner_desc_to_string(desc, desc_str);
		assert(result);
		(void)result;
		if(!manager->print_settings.dumpcfg) {
			if(idx == 0) {
				fprintf(stdout, "\n");
			}
			nqiv_cmd_print_indent(manager);
			fprintf(stdout, "%s\n", desc_str);
		} else if(idx == list_len - 1) {
			fprintf(stdout, "%s%s", manager->print_settings.prefix, desc_str);
		} else {
			fprintf(stdout, "%s%s\n", manager->print_settings.prefix, desc_str);
		}
	}
	if(!taken && manager->print_settings.dumpcfg) {
		fprintf(stdout, "#%s", manager->print_settings.prefix);
	}
}

void nqiv_cmd_parser_print_keybind(nqiv_cmd_manager* manager)
{
	const int          list_len = nqiv_array_get_units_count(manager->state->keybinds.lookup);
	nqiv_keybind_pair* pairs = manager->state->keybinds.lookup->data;
	bool               taken = false;
	int                idx;
	for(idx = 0; idx < list_len; ++idx) {
		const nqiv_keybind_pair* pair = &pairs[idx];
		taken = true;
		char       keybind_str[NQIV_KEYBIND_STRLEN + 1] = {0};
		const bool result = nqiv_keybind_to_string(pair, keybind_str);
		assert(keybind_str[0] != '=' || keybind_str[1] == '=');
		assert(result);
		(void)result;
		if(!manager->print_settings.dumpcfg) {
			if(idx == 0) {
				fprintf(stdout, "\n");
			}
			nqiv_cmd_print_indent(manager);
			fprintf(stdout, "%s\n", keybind_str);
		} else if(idx == list_len - 1) {
			fprintf(stdout, "%s%s", manager->print_settings.prefix, keybind_str);
		} else {
			fprintf(stdout, "%s%s\n", manager->print_settings.prefix, keybind_str);
		}
	}
	if(!taken && manager->print_settings.dumpcfg) {
		fprintf(stdout, "#%s", manager->print_settings.prefix);
	}
}

void nqiv_cmd_parser_print_thread(nqiv_cmd_manager* manager)
{
	const int          list_len = nqiv_array_get_units_count(manager->state->thread_specs);
	nqiv_worker_spec* thread_specs = manager->state->thread_specs->data;
	bool               taken = false;
	int                idx;
	for(idx = 0; idx < list_len; ++idx) {
		const nqiv_worker_spec* spec = &thread_specs[idx];
		taken = true;
		char       spec_str[NQIV_WORKER_SPEC_STRLEN + 1] = {0};
		const bool result = nqiv_worker_spec_to_string(spec, spec_str);
		assert(result);
		(void)result;
		if(!manager->print_settings.dumpcfg) {
			if(idx == 0) {
				fprintf(stdout, "\n");
			}
			nqiv_cmd_print_indent(manager);
			fprintf(stdout, "%s\n", spec_str);
		} else if(idx == list_len - 1) {
			fprintf(stdout, "%s%s", manager->print_settings.prefix, spec_str);
		} else {
			fprintf(stdout, "%s%s\n", manager->print_settings.prefix, spec_str);
		}
	}
	if(!taken && manager->print_settings.dumpcfg) {
		fprintf(stdout, "#%s", manager->print_settings.prefix);
	}
}

const char* const nqiv_press_action_names[] = {
	"default",
	"allow",
	"deny",
};

const nqiv_cmd_arg_desc nqiv_parser_arg_type_int_natural = {
	.type = NQIV_CMD_ARG_INT,
	.setting = {.of_int = {.min = 0, .max = INT_MAX}},
};

const nqiv_cmd_arg_desc nqiv_parser_arg_type_int_positive = {
	.type = NQIV_CMD_ARG_INT,
	.setting = {.of_int = {.min = 1, .max = INT_MAX}},
};

const nqiv_cmd_arg_desc nqiv_parser_arg_type_Uint64 = {
	.type = NQIV_CMD_ARG_UINT64,
	.setting = {.of_Uint64 = {.min = (Uint64)0, .max = (Uint64)INT_MAX}},
};

const nqiv_cmd_arg_desc nqiv_parser_arg_type_Uint8 = {
	.type = NQIV_CMD_ARG_UINT8,
	.setting = {{0}},
};

const nqiv_cmd_arg_desc nqiv_parser_arg_type_double_positive = {
	.type = NQIV_CMD_ARG_DOUBLE,
	.setting = {.of_double = {.min = NQIV_CMD_ARG_FLOAT_MIN, .max = NQIV_CMD_ARG_FLOAT_MAX}},
};

const nqiv_cmd_arg_desc nqiv_parser_arg_type_double_negative = {
	.type = NQIV_CMD_ARG_DOUBLE,
	.setting = {.of_double = {.min = -NQIV_CMD_ARG_FLOAT_MAX, .max = -NQIV_CMD_ARG_FLOAT_MIN}},
};

const nqiv_cmd_arg_desc nqiv_parser_arg_type_double_negative_one = {
	.type = NQIV_CMD_ARG_DOUBLE,
	.setting = {.of_double = {.min = -1.0, .max = -NQIV_CMD_ARG_FLOAT_MIN}},
};

const nqiv_cmd_arg_desc nqiv_parser_arg_type_double_positive_one = {
	.type = NQIV_CMD_ARG_DOUBLE,
	.setting = {.of_double = {.min = NQIV_CMD_ARG_FLOAT_MIN, .max = 1.0}},
};

const nqiv_cmd_arg_desc nqiv_parser_arg_type_double = {
	.type = NQIV_CMD_ARG_DOUBLE,
	.setting = {.of_double = {.min = -NQIV_CMD_ARG_FLOAT_MAX, .max = NQIV_CMD_ARG_FLOAT_MAX}},
};

const nqiv_cmd_arg_desc nqiv_parser_arg_type_bool = {
	.type = NQIV_CMD_ARG_BOOL,
	.setting = {{0}},
};

const nqiv_cmd_arg_desc nqiv_parser_arg_type_log_level = {
	.type = NQIV_CMD_ARG_LOG_LEVEL,
	.setting = {{0}},
};

const nqiv_cmd_arg_desc nqiv_parser_arg_type_press_action = {
	.type = NQIV_CMD_ARG_PRESS_ACTION,
	.setting = {{0}},
};

const nqiv_cmd_arg_desc nqiv_parser_arg_type_key_action = {
	.type = NQIV_CMD_ARG_KEY_ACTION,
	.setting = {{0}},
};

const nqiv_cmd_arg_desc nqiv_parser_arg_type_key_action_brief = {
	.type = NQIV_CMD_ARG_KEY_ACTION,
	.setting = {.of_key_action = {.brief = true}},
};

const nqiv_cmd_arg_desc nqiv_parser_arg_type_keybind = {
	.type = NQIV_CMD_ARG_KEYBIND,
	.setting = {{0}},
};

const nqiv_cmd_arg_desc nqiv_parser_arg_type_worker_spec = {
	.type = NQIV_CMD_ARG_WORKER_SPEC,
	.setting = {{0}},
};

const nqiv_cmd_arg_desc nqiv_parser_arg_type_string = {
	.type = NQIV_CMD_ARG_STRING,
	.setting = {.of_string = {.spaceless = true}},
};

const nqiv_cmd_arg_desc nqiv_parser_arg_type_string_full = {
	.type = NQIV_CMD_ARG_STRING,
	.setting = {.of_string = {.spaceless = false}},
};

const nqiv_cmd_arg_desc nqiv_parser_arg_type_pruner = {
	.type = NQIV_CMD_ARG_PRUNER,
	.setting = {{0}},
};

bool nqiv_keyrate_press_action_from_string(const char* data, nqiv_keyrate_press_action* output)
{
	nqiv_keyrate_press_action action;
	for(action = NQIV_KEYRATE_START; action <= NQIV_KEYRATE_END; ++action) {
		if(strcmp(data, nqiv_press_action_names[action]) == 0) {
			*output = action;
			return true;
		}
	}
	return false;
}

int nqiv_cmd_scan_subs(const char*  data,
                       const int    start,
                       const int    end,
                       const bool   negated,
                       const char** subs,
                       int*         length)
{
	int offset = -1;
	int bidx;
	for(bidx = start; bidx < end; ++bidx) {
		int sidx = 0;
		while(subs[sidx] != NULL) {
			if(!negated) {
				if(strncmp(&data[bidx], subs[sidx], strlen(subs[sidx])) == 0) {
					if(length != NULL) {
						*length = nqiv_strlen(subs[sidx]);
					}
					goto found;
				}
			} else {
				if(strncmp(&data[bidx], subs[sidx], strlen(subs[sidx])) == 0) {
					break;
				}
			}
			++sidx;
		}
		if(negated && subs[sidx] == NULL) {
			goto found;
		}
	}
	goto end;
found:
	offset = bidx;
end:
	return offset;
}

int nqiv_cmd_scan_not_whitespace_and_eol(const char* data,
                                         const int   start,
                                         const int   end,
                                         int*        length)
{
	const char* whitespace_and_eol[] = {"\r\n", "\n\r", "\n", "\r", " ", "\t", NULL};
	return nqiv_cmd_scan_subs(data, start, end, true, whitespace_and_eol, length);
}

int nqiv_cmd_scan_not_whitespace(const char* data, const int start, const int end, int* length)
{
	const char* whitespace[] = {" ", "\t", NULL};
	return nqiv_cmd_scan_subs(data, start, end, true, whitespace, length);
}

int nqiv_cmd_scan_whitespace(const char* data, const int start, const int end, int* length)
{
	const char* whitespace[] = {" ", "\t", NULL};
	return nqiv_cmd_scan_subs(data, start, end, false, whitespace, length);
}

int nqiv_cmd_scan_eol(const char* data, const int start, const int end, int* length)
{
	const char* eol[] = {"\r\n", "\n\r", "\n", "\r", NULL};
	return nqiv_cmd_scan_subs(data, start, end, false, eol, length);
}

void nqiv_cmd_print_comment_prefix(const nqiv_cmd_manager* manager)
{
	(void)manager;

	fprintf(stdout, "#");
}

void nqiv_cmd_print_single_arg(nqiv_cmd_manager*        manager,
                               const nqiv_cmd_arg_desc* arg,
                               void (*print_prefix)(const nqiv_cmd_manager*))
{
	switch(arg->type) {
	case NQIV_CMD_ARG_INT:
		fprintf(stdout, "INT(%d-%d)", arg->setting.of_int.min, arg->setting.of_int.max);
		break;
	case NQIV_CMD_ARG_DOUBLE:
		fprintf(stdout, "DOUBLE(%f-%f)", arg->setting.of_double.min, arg->setting.of_double.max);
		break;
	case NQIV_CMD_ARG_UINT64:
		fprintf(stdout, "UINT64(%" PRIu64 "-%" PRIu64 ")", arg->setting.of_Uint64.min,
		        arg->setting.of_Uint64.max);
		break;
	case NQIV_CMD_ARG_UINT8:
		fprintf(stdout, "UINT8(0-255)");
		break;
	case NQIV_CMD_ARG_BOOL:
		fprintf(stdout, "BOOL(true|false)");
		break;
	case NQIV_CMD_ARG_LOG_LEVEL:
		fprintf(stdout, "LOG_LEVEL(");
		{
			manager->print_settings.indent += 1;
			nqiv_log_level level;
			for(level = NQIV_LOG_ANY; level <= NQIV_LOG_ERROR; level += 10) {
				fprintf(stdout, "%s", nqiv_log_level_names[level / 10]);
				if(level != NQIV_LOG_FINAL) {
					fprintf(stdout, "|");
				}
			}
			manager->print_settings.indent -= 1;
		}
		fprintf(stdout, ")");
		break;
	case NQIV_CMD_ARG_PRESS_ACTION:
		fprintf(stdout, "PRESS_ACTION(");
		{
			manager->print_settings.indent += 1;
			nqiv_keyrate_press_action action;
			for(action = NQIV_KEYRATE_START; action <= NQIV_KEYRATE_END; ++action) {
				fprintf(stdout, "%s", nqiv_press_action_names[action]);
				if(action != NQIV_KEYRATE_END) {
					fprintf(stdout, "|");
				}
			}
			manager->print_settings.indent -= 1;
		}
		fprintf(stdout, ")");
		break;
	case NQIV_CMD_ARG_KEY_ACTION:
		fprintf(stdout, "KEY_ACTION");
		if(!arg->setting.of_key_action.brief) {
			manager->print_settings.indent += 1;
			fprintf(stdout, "\n");
			nqiv_key_action action;
			for(action = NQIV_KEY_ACTION_QUIT; action <= NQIV_KEY_ACTION_MAX; ++action) {
				print_prefix(manager);
				fprintf(stdout, "%s\n", nqiv_keybind_action_names[action]);
			}
			manager->print_settings.indent -= 1;
		}
		break;
	case NQIV_CMD_ARG_KEYBIND:
		manager->print_settings.indent += 1;
		fprintf(stdout, "<keybind>=<key_action>\n");
		print_prefix(manager);
		fprintf(stdout, "Options are separated by +\n");
		print_prefix(manager);
		fprintf(stdout, "<keybind> options:\n");
		print_prefix(manager);
		fprintf(stdout, "SDL Scancode\n");
		print_prefix(manager);
		fprintf(stdout, "'mouse#' specifies pressing a mouse button. To specify double click, add "
		                "'double'. Example: 'mouse0_double'\n");
		print_prefix(manager);
		fprintf(stdout, "'scroll_left', 'scroll_right', 'scroll_forward', 'scroll_backward'\n");
		print_prefix(manager);
		fprintf(stdout, "Modifier keys:\n");
		print_prefix(manager);
		fprintf(stdout, "'lshift', 'rshift', or 'shift' for either.\n");
		print_prefix(manager);
		fprintf(stdout, "'lctrl', 'rctrl', or 'ctrl' for either.\n");
		print_prefix(manager);
		fprintf(stdout, "'lalt', 'ralt', or 'alt' for either.\n");
		print_prefix(manager);
		fprintf(stdout, "caps\n");
		print_prefix(manager);
		fprintf(stdout, "mode\n");
		print_prefix(manager);
		fprintf(stdout, "<key_action> options:\n");
		print_prefix(manager);
		fprintf(stdout, "Key action such as for `help sendkey`\n");
		print_prefix(manager);
		fprintf(
			stdout,
			"Normal behavior is to follow behavior specified by `helptree set keypress default`\n");
		print_prefix(manager);
		fprintf(stdout, "Overrides:\n");
		print_prefix(manager);
		fprintf(stdout, "'allow_on_up'\n");
		print_prefix(manager);
		fprintf(stdout, "'deny_on_up'\n");
		print_prefix(manager);
		fprintf(stdout, "'allow_on_down'\n");
		print_prefix(manager);
		fprintf(stdout, "'deny_on_down'\n");
		print_prefix(manager);
		fprintf(stdout, "'<int>' is 0 or greater.\n");
		print_prefix(manager);
		fprintf(stdout, "'start_delay_<int>'\n");
		print_prefix(manager);
		fprintf(stdout, "'consecutive_delay_<int>'\n");
		print_prefix(manager);
		fprintf(stdout, "'delay_accel_<int>'\n");
		print_prefix(manager);
		fprintf(stdout, "'minimum_delay_<int>'\n");
		manager->print_settings.indent -= 1;
		break;
	case NQIV_CMD_ARG_STRING:
		fprintf(stdout, "STRING(%s)",
		        arg->setting.of_string.spaceless ? "spaceless" : "spaces allowed");
		break;
	case NQIV_CMD_ARG_PRUNER:
		manager->print_settings.indent += 1;
		fprintf(stdout, "PRUNER\n");
		print_prefix(manager);
		fprintf(stdout, "'no' will clear the option that comes after.\n");
		print_prefix(manager);
		fprintf(stdout, "'sum' <MAX> will check the addition of all checked integer values against "
		                "another specified value to determine success.\n");
		print_prefix(manager);
		fprintf(stdout,
		        "'or' will use boolean or with the result of all checks to determine success.\n");
		print_prefix(manager);
		fprintf(stdout,
		        "'and' will use boolean and with the result of all checks to determine success.\n");
		print_prefix(manager);
		fprintf(stdout, "'unload' will cause specified image datatypes to be unloaded in the event "
		                "of a failed check.\n");
		print_prefix(manager);
		fprintf(stdout, "'hard' will cause 'unload' to always work. Otherwise, they will only be "
		                "unloaded if the corresponding texture is not NULL (this can prevent "
		                "prematurely unloading things needed for the texture).\n");
		print_prefix(manager);
		fprintf(
			stdout,
			"'thumbnail' will cause thumbnail images to be considered by the following checks.\n");
		print_prefix(manager);
		fprintf(stdout,
		        "'image' will cause normal images to be considered by the following checks.\n");
		print_prefix(manager);
		fprintf(stdout, "'vips' will cause the following checks to consider vips data only.\n");
		print_prefix(manager);
		fprintf(stdout,
		        "'surface' will cause the following checks to consider SDL surface data only.\n");
		print_prefix(manager);
		fprintf(stdout,
		        "'texture' will cause the following checks to consider SDL texture data only.\n");
		print_prefix(manager);
		fprintf(stdout, "'loaded_ahead' <THRESHOLD> <MAX> will check for whether the number of "
		                "loaded images after the threshold is greater than max.\n");
		print_prefix(manager);
		fprintf(stdout, "'loaded_behind' <THRESHOLD> <MAX> will check for whether the number of "
		                "loaded images before the threshold is greater than max.\n");
		print_prefix(manager);
		fprintf(stdout, "'bytes_ahead' <THRESHOLD> <MAX> will check for whether the number of "
		                "loaded bytes after the threshold is greater than max.\n");
		print_prefix(manager);
		fprintf(stdout, "'bytes_behind' <THRESHOLD> <MAX> will check for whether the number of "
		                "loaded bytes before the threshold is greater than max.\n");
		print_prefix(manager);
		fprintf(stdout, "'self_opened' will check if the currently-selected image is loaded.\n");
		print_prefix(manager);
		fprintf(stdout, "'not_animated' will check if the currently-selected image is not animated."
		                " This can be run without specifying data.");
		manager->print_settings.indent -= 1;
		break;
	case NQIV_CMD_ARG_WORKER_SPEC:
		manager->print_settings.indent += 1;
		fprintf(stdout, "WORKER_SPEC\n");
		print_prefix(manager);
		fprintf(stdout, "A worker spec specifies the settings for a worker thread.\n");
		print_prefix(manager);
		fprintf(stdout, "Each worker spec consists of a space-separated list of keys and values.\n");
		print_prefix(manager);
		fprintf(stdout, "A key is specified, then its value (or a space-separated list of values) comes after. Lists end at the end of the spec itself, or when the next key is encountered.\n");
		print_prefix(manager);
		fprintf(stdout, "Keys:\n");
		print_prefix(manager);
		fprintf(stdout, "'extra_wakeup_delay <INT(0-2147483647)>' - Extra time to wait for a worker thread to awaken. If unset, the default from 'set thread extra_wakeup_delay' is used. See this command's help for further explanation.\n");
		print_prefix(manager);
		fprintf(stdout, "'event_interval <INT(0-2147483647)>' - Number of events to process between polling cycles. If unset, the default from 'set thread event_interval' is used. See this command's help for further explanation.\n");
		print_prefix(manager);
		fprintf(stdout, "'bins <INT(0-%d)>...' - The thread will process events of the priorities specified here, in the order that they are specified. The 'natural' order of the bins is how workers handle events by default.\n", THREAD_QUEUE_BIN_COUNT - 1);
		print_prefix(manager);
		fprintf(stdout, "Priorities and their purposes:\n");
		print_prefix(manager);
		fprintf(stdout, "0 - Normal quitting is not done through the event queue, so this may be safely left out.\n");
		print_prefix(manager);
		fprintf(stdout, "1 - Load frames of an animated image.\n");
		print_prefix(manager);
		fprintf(stdout, "2 - Unload thumbnails because a new size is needed.\n");
		print_prefix(manager);
		fprintf(stdout, "3 - Perform prunes.\n");
		print_prefix(manager);
		fprintf(stdout, "4 - Load the currently-displayed image.\n");
		print_prefix(manager);
		fprintf(stdout, "5 - Load thumbnail info from image data, without creating thumbnail files.\n");
		print_prefix(manager);
		fprintf(stdout, "6 - Load thumbnail info from thumbnail files.\n");
		print_prefix(manager);
		fprintf(stdout, "7 - Create thumbnail file after attempting to load a nonexistent one.\n");
		print_prefix(manager);
		fprintf(stdout, "8 - Create thumbnail file nqiv is not expected to open.\n");
		manager->print_settings.indent -= 1;
		break;
	}
}

void nqiv_cmd_print_args(nqiv_cmd_manager*               manager,
                         const nqiv_cmd_arg_desc* const* args,
                         void (*print_prefix)(const nqiv_cmd_manager*))
{
	assert(args != NULL);
	int idx = 0;
	while(args[idx] != NULL) {
		nqiv_cmd_print_single_arg(manager, args[idx], print_prefix);
		++idx;
		if(args[idx] != NULL) {
			fprintf(stdout, " ");
		}
	}
}

void nqiv_cmd_dumpcfg(nqiv_cmd_manager*    manager,
                      const nqiv_cmd_node* current_node,
                      const bool           recurse,
                      const char*          current_cmd)
{
	if(current_node->deprecated) {
		return;
	}
	char       new_cmd[NQIV_CMD_DUMPCFG_BUFFER_LENGTH + 1] = {0};
	nqiv_array new_cmd_builder;
	nqiv_array_inherit(&new_cmd_builder, new_cmd, sizeof(char), NQIV_CMD_DUMPCFG_BUFFER_LENGTH);
	bool new_cmd_success = true;
	new_cmd_success = new_cmd_success && nqiv_array_push_str(&new_cmd_builder, current_cmd);
	if(current_node != manager->root_node) {
		new_cmd_success =
			new_cmd_success && nqiv_array_push_str(&new_cmd_builder, current_node->name);
		new_cmd_success = new_cmd_success && nqiv_array_push_str(&new_cmd_builder, " ");
	}
	assert(new_cmd_success);
	if(current_node->store_value != NULL) {
		fprintf(stdout, "#%s\n#", current_node->description);
		nqiv_cmd_print_args(manager, (const nqiv_cmd_arg_desc* const*)(current_node->args),
		                    nqiv_cmd_print_comment_prefix);
		fprintf(stdout, "\n");
		if(current_node->print_value != NULL) {
			if(strncmp(new_cmd, "append", strlen("append")) != 0) {
				fprintf(stdout, "%s", new_cmd);
			}
			manager->print_settings.prefix = new_cmd;
			manager->print_settings.dumpcfg = true;
			current_node->print_value(manager);
			fprintf(stdout, "\n");
		} else {
			fprintf(stdout, "#%s\n", new_cmd);
		}
		fprintf(stdout, "\n");
	}
	if(!recurse) {
		return;
	}
	nqiv_cmd_node* child = current_node->child;
	while(child != NULL) {
		manager->print_settings.current_node = child;
		nqiv_cmd_dumpcfg(manager, child, recurse, new_cmd);
		child = child->peer;
	}
}

void nqiv_cmd_print_help(nqiv_cmd_manager*    manager,
                         const nqiv_cmd_node* current_node,
                         const int            recurse)
{
	if(current_node->deprecated && manager->print_settings.indent > 0) {
		return;
	}
	nqiv_cmd_print_indent(manager);
	fprintf(stdout, "%s: %s", current_node->name, current_node->description);
	if(current_node->args != NULL) {
		fprintf(stdout, " - ");
		nqiv_cmd_print_args(manager, (const nqiv_cmd_arg_desc* const*)(current_node->args),
		                    nqiv_cmd_print_indent);
	}
	if(current_node->print_value != NULL) {
		fprintf(stdout, " - ");
		current_node->print_value(manager);
	}
	fprintf(stdout, "\n");
	if(recurse == 0) {
		return;
	}
	manager->print_settings.indent += 1;
	nqiv_cmd_node* child = current_node->child;
	while(child != NULL) {
		manager->print_settings.current_node = child;
		nqiv_cmd_print_help(manager, child, recurse - 1);
		child = child->peer;
	}
	manager->print_settings.indent -= 1;
}

int nqiv_cmd_parse_arg_token(nqiv_cmd_manager*    manager,
                             const nqiv_cmd_node* current_node,
                             const int            tidx,
                             const int            start_idx,
                             const int            eolpos,
                             nqiv_cmd_arg_token*  token)
{
	char*                    mutdata = manager->buffer->data;
	const char               data_end = nqiv_cmd_tmpterm(mutdata, eolpos);
	int                      output = -1;
	char*                    mutdata_start = mutdata + start_idx;
	const char*              data = mutdata_start;
	const nqiv_cmd_arg_desc* desc = current_node->args[tidx];
	switch(desc->type) {
	case NQIV_CMD_ARG_INT:
		{
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG,
			               "Cmd checking int arg at %d for token %s for input %s\n", tidx,
			               current_node->name, data);
			char*          end = NULL;
			const long int tmp = strtol(data, &end, 10);
			if(errno != ERANGE && end != NULL && data != end && tmp >= desc->setting.of_int.min
			   && tmp <= desc->setting.of_int.max) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG,
				               "Cmd int arg at %d for token %s is %ld for input %s\n", tidx,
				               current_node->name, tmp, data);
				token->value.as_int = (int)tmp;
				output = nqiv_ptrdiff(end, data);
			} else {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING,
				               "Cmd error parsing int arg at %d for token %s with input %s\n", tidx,
				               current_node->name, data);
			}
		}
		break;
	case NQIV_CMD_ARG_DOUBLE:
		{
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG,
			               "Cmd checking double arg at %d for token %s for input %s\n", tidx,
			               current_node->name, data);
			char*        end = NULL;
			const double tmp = strtod(data, &end);
			if(errno != ERANGE && end != NULL && data != end && tmp >= desc->setting.of_double.min
			   && tmp <= desc->setting.of_double.max) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG,
				               "Cmd double at %d for token %s is %f for input %s\n", tidx,
				               current_node->name, tmp, data);
				token->value.as_double = tmp;
				output = nqiv_ptrdiff(end, data);
			} else {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING,
				               "Cmd error parsing double arg at %d for token %s with input %s\n",
				               tidx, current_node->name, data);
			}
		}
		break;
	case NQIV_CMD_ARG_UINT64:
		{
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG,
			               "Cmd checking uint64 arg at %d for token %s for input %s\n", tidx,
			               current_node->name, data);
			char*                   end = NULL;
			const unsigned long int tmp = strtoul(data, &end, 10);
			if(errno != ERANGE && end != NULL && data != end
			   && (uintmax_t)tmp >= (uintmax_t)desc->setting.of_Uint64.min
			   && (uintmax_t)tmp <= (uintmax_t)desc->setting.of_Uint64.max) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG,
				               "Cmd uint64 arg at %d for token %s is %lu for input %s\n", tidx,
				               current_node->name, tmp, data);
				token->value.as_Uint64 = (Uint64)tmp;
				output = nqiv_ptrdiff(end, data);
			} else {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING,
				               "Cmd error parsing uint64 arg at %d for token %s with input %s\n",
				               tidx, current_node->name, data);
			}
		}
		break;
	case NQIV_CMD_ARG_UINT8:
		{
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG,
			               "Cmd checking uint8 arg at %d for token %s for input %s\n", tidx,
			               current_node->name, data);
			char*          end = NULL;
			const long int tmp = strtol(data, &end, 10);
			if(errno != ERANGE && end != NULL && data != end && tmp >= 0 && tmp <= 255) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG,
				               "Cmd uint8 arg at %d for token %s is %d for input %s\n", tidx,
				               current_node->name, tmp, data);
				token->value.as_Uint8 = (Uint8)tmp;
				output = nqiv_ptrdiff(end, data);
			} else {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING,
				               "Cmd error parsing uint8 arg at %d for token %s with input %s\n",
				               tidx, current_node->name, data);
			}
		}
		break;
	case NQIV_CMD_ARG_BOOL:
		{
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG,
			               "Cmd checking bool arg at %d for token %s for input %s\n", tidx,
			               current_node->name, data);
			if(strcmp(data, "true") == 0) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG,
				               "Cmd bool arg at %d for token %s is true for input %s\n", tidx,
				               current_node->name, data);
				token->value.as_bool = true;
				output = strlen("true");
			} else if(strcmp(data, "false") == 0) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG,
				               "Cmd bool arg at %d for token %s is false for input %s\n", tidx,
				               current_node->name, data);
				token->value.as_bool = false;
				output = strlen("false");
			} else {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING,
				               "Cmd error parsing bool arg at %d for token %s with input %s\n",
				               tidx, current_node->name, data);
			}
		}
		break;
	case NQIV_CMD_ARG_LOG_LEVEL:
		{
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG,
			               "Cmd checking log level arg at %d for token %s for input %s\n", tidx,
			               current_node->name, data);
			const nqiv_log_level tmp = nqiv_log_level_from_string(data);
			if(tmp != NQIV_LOG_UNKNOWN) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG,
				               "Cmd log level arg at %d for token %s is %s for input %s\n", tidx,
				               current_node->name, nqiv_log_level_names[tmp / 10], data);
				token->value.as_log_level = tmp;
				output = nqiv_strlen(nqiv_log_level_names[tmp / 10]);
			} else {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING,
				               "Cmd error parsing log level arg at %d for token %s with input %s\n",
				               tidx, current_node->name, data);
			}
		}
		break;
	case NQIV_CMD_ARG_PRESS_ACTION:
		{
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG,
			               "Cmd checking press action arg at %d for token %s for input %s\n", tidx,
			               current_node->name, data);
			nqiv_keyrate_press_action tmp;
			if(nqiv_keyrate_press_action_from_string(data, &tmp)) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG,
				               "Cmd press action arg at %d for token %s is %s for input %s\n", tidx,
				               current_node->name, nqiv_press_action_names[tmp], data);
				token->value.as_press_action = tmp;
				output = nqiv_strlen(nqiv_press_action_names[tmp]);
			} else {
				nqiv_log_write(
					&manager->state->logger, NQIV_LOG_WARNING,
					"Cmd error parsing press action arg at %d for token %s with input %s\n", tidx,
					current_node->name, data);
			}
		}
		break;
	case NQIV_CMD_ARG_KEY_ACTION:
		{
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG,
			               "Cmd checking key action arg at %d for token %s for input %s\n", tidx,
			               current_node->name, data);
			int arg_end = nqiv_cmd_scan_whitespace(mutdata, start_idx, eolpos, NULL);
			if(arg_end == -1 || arg_end > eolpos) {
				arg_end = eolpos;
			}
			const nqiv_key_action tmp = nqiv_text_to_key_action(data, arg_end - start_idx);
			if(tmp != NQIV_KEY_ACTION_NONE) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG,
				               "Cmd key action arg at %d for token %s is %s for input %s\n", tidx,
				               current_node->name, nqiv_keybind_action_names[tmp], data);
				token->value.as_key_action = tmp;
				output = nqiv_strlen(nqiv_keybind_action_names[tmp]);
			} else {
				nqiv_log_write(
					&manager->state->logger, NQIV_LOG_WARNING,
					"Cmd error parsing key action arg at %d for token %s with input %s\n", tidx,
					current_node->name, data);
			}
		}
		break;
	case NQIV_CMD_ARG_KEYBIND:
		{
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG,
			               "Cmd checking keybind arg at %d for token %s for input %s\n", tidx,
			               current_node->name, data);
			nqiv_keybind_pair tmp = {0};
			const int         length = nqiv_keybind_text_to_keybind(mutdata_start, &tmp);
			if(length != -1) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG,
				               "Cmd keybind arg at %d for token %s for input %s\n", tidx,
				               current_node->name, data);
				memcpy(&token->value.as_keybind, &tmp, sizeof(nqiv_keybind_pair));
				output = length;
			} else {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING,
				               "Cmd error parsing keybind arg at %d for token %s with input %s\n",
				               tidx, current_node->name, data);
			}
		}
		break;
	case NQIV_CMD_ARG_PRUNER:
		{
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG,
			               "Cmd checking pruner arg at %d for token %s for input %s\n", tidx,
			               current_node->name, data);
			nqiv_pruner_desc tmp;
			if(nqiv_pruner_create_desc(&manager->state->logger, data, &tmp)) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG,
				               "Cmd pruner arg at %d for token %s for input %s\n", tidx,
				               current_node->name, data);
				memcpy(&token->value.as_pruner, &tmp, sizeof(nqiv_pruner_desc));
				output = eolpos - start_idx;
			} else {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING,
				               "Cmd error parsing pruner arg at %d for token %s with input %s\n",
				               tidx, current_node->name, data);
			}
		}
		break;
	case NQIV_CMD_ARG_WORKER_SPEC:
		nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG,
		               "Cmd checking worker_spec arg at %d for token %s for input %s\n", tidx,
		               current_node->name, data);
		{
			nqiv_worker_spec tmp;
			if(nqiv_worker_string_to_spec(data, &tmp)) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG,
				               "Cmd worker_spec arg at %d for token %s for input %s\n", tidx,
				               current_node->name, data);
				memcpy(&token->value.as_worker_spec, &tmp, sizeof(nqiv_worker_spec));
				output = eolpos - start_idx;
			} else {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING,
				               "Cmd error parsing worker_spec arg at %d for token %s with input %s\n",
				               tidx, current_node->name, data);
			}
		}
		break;
	case NQIV_CMD_ARG_STRING:
		if(desc->setting.of_string.spaceless) {
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG,
			               "Cmd checking spaceless arg at %d for token %s for input %s\n", tidx,
			               current_node->name, data);
			const int length = nqiv_cmd_scan_whitespace(data, 0, eolpos - start_idx, NULL);
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG,
			               "Cmd spaceless string at %d for token %s for input %s\n", tidx,
			               current_node->name, data);
			if(length != -1) {
				output = length;
			} else {
				output = eolpos - start_idx;
			}
		} else {
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG,
			               "Cmd string at %d for token %s for input %s\n", tidx, current_node->name,
			               data);
			output = eolpos - start_idx;
		}
		break;
	}
	if(output != -1) {
		token->type = desc->type;
		token->raw = mutdata_start;
		token->length = output;
	}
	nqiv_cmd_tmpret(mutdata, eolpos, data_end);
	return output;
}

bool nqiv_cmd_parse_args(nqiv_cmd_manager*    manager,
                         const nqiv_cmd_node* current_node,
                         const int            start_idx,
                         const int            eolpos,
                         nqiv_cmd_arg_token** tokens)
{
	bool        error = false;
	int         idx = start_idx;
	int         tidx = 0;
	const char* data = manager->buffer->data;

	while(current_node->args[tidx] != NULL) {
		assert(tidx < NQIV_CMD_MAX_ARGS);
		const int next_text_offset = nqiv_cmd_scan_not_whitespace(data, idx, eolpos, NULL);
		if(next_text_offset != -1) {
			idx = next_text_offset;
		} else {
			idx = eolpos;
		}
		const int parse_result =
			nqiv_cmd_parse_arg_token(manager, current_node, tidx, idx, eolpos, tokens[tidx]);
		if(parse_result == -1) {
			error = true;
			break;
		}
		idx += parse_result;
		++tidx;
	}
	if(error || nqiv_cmd_scan_not_whitespace(data, idx, eolpos, NULL) != -1) {
		nqiv_cmd_print_help(manager, current_node, 0);
		if(manager->state->cmd_parse_error_quit) {
			nqiv_cmd_force_quit_main(manager);
			error = true;
		} else {
			error = false;
		}
	}
	return !error;
}

bool nqiv_cmd_execute_node(nqiv_cmd_manager*    manager,
                           const nqiv_cmd_node* current_node,
                           const int            idx,
                           const int            eolpos)
{
	nqiv_cmd_arg_token  tokens[NQIV_CMD_MAX_ARGS] = {0};
	nqiv_cmd_arg_token* token_ptrs[NQIV_CMD_MAX_ARGS + 1] = {0};
	int                 count;
	for(count = 0; count < NQIV_CMD_MAX_ARGS; ++count) {
		token_ptrs[count] = &tokens[count];
	}
	if(!nqiv_cmd_parse_args(manager, current_node, idx, eolpos, token_ptrs)) {
		return manager->state->cmd_parse_error_quit;
	}
	nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd storing value for %s (%s).\n",
	               current_node->name, current_node->description);
	assert(current_node->store_value != NULL);
	if(!current_node->store_value(manager, token_ptrs)) {
		nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING,
		               "Cmd error storing value for %s (%s).\n", current_node->name,
		               current_node->description);
		if(manager->state->cmd_apply_error_quit) {
			nqiv_cmd_force_quit_main(manager);
			return true;
		}
	}
	return false;
}

void nqiv_cmd_acknowledge(const nqiv_cmd_manager* manager, const char* cmd, const Uint64 time)
{
	if(manager->state->cmd_acknowledge) {
		fprintf(stdout, "Executed command '%s' in %" PRIu64 "ms\n", cmd, time);
	}
}

bool nqiv_cmd_parse_line(nqiv_cmd_manager* manager)
{
	memset(&manager->print_settings, 0, sizeof(nqiv_cmd_manager_print_settings));
	char       current_cmd[NQIV_CMD_DUMPCFG_BUFFER_LENGTH + 1] = {0};
	bool       current_cmd_success = true;
	nqiv_array current_cmd_builder;
	nqiv_array_inherit(&current_cmd_builder, current_cmd, sizeof(char),
	                   NQIV_CMD_DUMPCFG_BUFFER_LENGTH);
	bool  error = false;
	bool  help = false;
	int   help_levels = 0;
	bool  dumpcfg = false;
	char* data = manager->buffer->data;
	int   idx = nqiv_cmd_scan_not_whitespace_and_eol(data, 0, manager->buffer->position, NULL);
	if(idx == -1) {
		nqiv_array_clear(manager->buffer);
		return true; /* The entire string must be whitespace- nothing to do. */
	}
	const int eolpos = nqiv_cmd_scan_eol(data, idx, manager->buffer->position, NULL);
	if(eolpos == -1) {
		return true; /* We don't have an EOL yet. Nothing to do. */
	}
	if(data[idx] == '#') {
		const char eolc = nqiv_cmd_tmpterm(data, eolpos);
		nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd skipping input %s\n",
		               data + idx);
		nqiv_cmd_tmpret(data, eolpos, eolc);
		nqiv_array_remove_count(manager->buffer, 0, eolpos);
		return true; /* This line is a comment- ignore it. */
	}
	const Uint64 cmd_start_ticks = SDL_GetTicks64();
	const char   eolc = nqiv_cmd_tmpterm(data, eolpos);
	nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd parsing input %s\n", data + idx);
	nqiv_cmd_tmpret(data, eolpos, eolc);
	if(strncmp(&data[idx], "helptree", strlen("helptree")) == 0) {
		idx += strlen("helptree");
		help = true;
		help_levels = -1;
	} else if(strncmp(&data[idx], "helpchildren", strlen("helpchildren")) == 0) {
		idx += strlen("helpchildren");
		help = true;
		help_levels = 1;
	} else if(strncmp(&data[idx], "help", strlen("help")) == 0) {
		idx += strlen("help");
		help = true;
	} else if(strncmp(&data[idx], "dumpcfg", strlen("dumpcfg")) == 0) {
		idx += strlen("dumpcfg");
		dumpcfg = true;
	}
	nqiv_cmd_node* current_node = manager->root_node;
	assert(!manager->root_node->deprecated);
	while(idx < eolpos) {
		const int next_text_offset = nqiv_cmd_scan_not_whitespace(data, idx, eolpos, NULL);
		if(next_text_offset != -1) {
			idx = next_text_offset;
		} else if(help || current_node != manager->root_node) {
			break;
		}
		bool           found_node = false;
		nqiv_cmd_node* child = current_node->child;
		while(child != NULL) {
			const char tmp = nqiv_cmd_tmpterm(data, eolpos);
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG,
			               "Cmd checking token %s child %s for input %s\n", current_node->name,
			               child->name, data + idx);
			nqiv_cmd_tmpret(data, eolpos, tmp);
			int data_end = nqiv_cmd_scan_whitespace(data, idx, eolpos, NULL);
			if(data_end == -1 || data_end > eolpos) {
				data_end = eolpos;
			}
			if(strncmp(&data[idx], child->name, data_end - idx) == 0
			   && strlen(child->name) == (size_t)(data_end - idx)) {
				current_node = child;
				idx = data_end;
				found_node = true;
				current_cmd_success =
					current_cmd_success
					&& nqiv_array_push_str(&current_cmd_builder, current_node->name);
				current_cmd_success =
					current_cmd_success && nqiv_array_push_str(&current_cmd_builder, " ");
				if(current_node->deprecated) {
					const char eolcd = nqiv_cmd_tmpterm(data, eolpos);
					nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING,
					               "Node '%s' deprecated for input %s\n", current_node->name,
					               data + idx);
					nqiv_cmd_tmpret(data, eolpos, eolcd);
				}
				break;
			}
			child = child->peer;
		}
		if(!found_node) {
			/* We haven't found the child node and there are no arguments, either. */
			error = current_node->args == NULL;
			break;
		}
	}
	assert(current_node->store_value == NULL || current_node->args != NULL);
	manager->print_settings.current_node = current_node;
	if(dumpcfg && !error) {
		nqiv_cmd_dumpcfg(manager, current_node, true, current_cmd);
	} else if(help) {
		nqiv_cmd_print_help(manager, current_node, help_levels);
	} else if(!error && current_node->store_value != NULL) {
		error = nqiv_cmd_execute_node(manager, current_node, idx, eolpos);
	} else {
		nqiv_cmd_print_help(manager, current_node, 0);
		if(error && manager->state->cmd_parse_error_quit) {
			nqiv_cmd_force_quit_main(manager);
		} else {
			error = false;
		}
	}
	const char eola = nqiv_cmd_tmpterm(data, eolpos);
	nqiv_cmd_acknowledge(manager, data, SDL_GetTicks64() - cmd_start_ticks);
	nqiv_cmd_tmpret(data, eolpos, eola);
	nqiv_array_remove_count(manager->buffer, 0, eolpos + 1);
	assert(current_cmd_success);
	return !error;
}

bool nqiv_cmd_parse(nqiv_cmd_manager* manager)
{
	while(nqiv_cmd_scan_eol(manager->buffer->data, 0, manager->buffer->position, NULL) != -1) {
		if(!nqiv_cmd_parse_line(manager)) {
			return false;
		}
	}
	return true;
}

bool nqiv_cmd_add_byte(nqiv_cmd_manager* manager, const char byte)
{
	const char buf[NQIV_CMD_ADD_BYTE_BUFFER_LENGTH] = {byte};
	if(!nqiv_array_push_count(manager->buffer, buf, NQIV_CMD_ADD_BYTE_BUFFER_LENGTH)) {
		nqiv_log_write(&manager->state->logger, NQIV_LOG_ERROR,
		               "Failed to append byte %c to nqiv command parser of length %d.\n", byte,
		               manager->buffer->data_length);
		nqiv_cmd_force_quit_main(manager);
		return false;
	}
	return true;
}

bool nqiv_cmd_add_string(nqiv_cmd_manager* manager, const char* str)
{
	int idx = 0;
	while(str[idx] != '\0') {
		if(!nqiv_cmd_add_byte(manager, str[idx])) {
			return false;
		}
		++idx;
	}
	return true;
}

bool nqiv_cmd_add_line(nqiv_cmd_manager* manager, const char* str)
{
	return nqiv_cmd_add_string(manager, str) && nqiv_cmd_add_byte(manager, '\n');
}

bool nqiv_cmd_add_line_and_parse(nqiv_cmd_manager* manager, const char* str)
{
	return nqiv_cmd_add_line(manager, str) && nqiv_cmd_parse(manager);
}

nqiv_op_result
nqiv_cmd_add_stream_line(nqiv_cmd_manager* manager, FILE* stream, const bool nonblocking)
{
	while(true) {
		int c = -1;
		if(nonblocking) {
			c = nqiv_agetc(stream);
			if(c == -1) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_ERROR,
				               "Error reading config stream.\n");
				return NQIV_FAIL;
			} else if(c == 0) {
				return NQIV_PASS;
			}
		} else {
			c = fgetc(stream);
			if(c == EOF) {
				if(ferror(stream)) {
					nqiv_log_write(&manager->state->logger, NQIV_LOG_ERROR,
					               "Error reading config stream.\n");
					return NQIV_FAIL;
				}
				return NQIV_PASS;
			}
		}
		assert(c != -1);
		if(!nqiv_cmd_add_byte(manager, (char)c)) {
			return NQIV_FAIL;
		}
		if(c == '\r' || c == '\n') {
			return NQIV_SUCCESS;
		}
	}
}

bool nqiv_cmd_consume_stream(nqiv_cmd_manager* manager, FILE* stream)
{
	const Uint64   stream_time = SDL_GetTicks64();
	nqiv_op_result result = NQIV_SUCCESS;
	while(result == NQIV_SUCCESS) {
		result = nqiv_cmd_add_stream_line(manager, stream, false);
		if(result == NQIV_PASS || result == NQIV_FAIL) {
			break;
		} else if(!nqiv_cmd_parse(manager)) {
			result = NQIV_FAIL;
			break;
		}
	}
	nqiv_cmd_acknowledge(manager, "STREAM", SDL_GetTicks64() - stream_time);
	return result == NQIV_FAIL ? false : true;
}

nqiv_cmd_node* nqiv_cmd_get_last_peer(nqiv_cmd_node* first_peer)
{
	assert(first_peer != NULL);
	nqiv_cmd_node* current_peer;
	nqiv_cmd_node* last_peer = first_peer;
	for(current_peer = first_peer->peer; current_peer != NULL; current_peer = current_peer->peer) {
		last_peer = current_peer;
	}
	return last_peer;
}

/* TODO: Why can't we const this? */
int nqiv_cmd_get_args_list_length(const nqiv_cmd_arg_desc** args)
{
	assert(args != NULL);
	int count;
	for(count = 0; args[count] != NULL; ++count) {}
	assert(count > 0);
	return count + 1;
}

void nqiv_cmd_add_child_node(nqiv_cmd_node* parent, nqiv_cmd_node* child)
{
	assert(parent != NULL);
	if(child != NULL) {
		assert(parent->child == NULL);
		parent->child = child;
	}
}

void nqiv_cmd_add_peer_node(nqiv_cmd_node* first_peer, nqiv_cmd_node* new_peer)
{
	if(new_peer != NULL) {
		nqiv_cmd_get_last_peer(first_peer)->peer = new_peer;
	}
}

void nqiv_cmd_add_child_or_peer(nqiv_cmd_node* parent, nqiv_cmd_node* node)
{
	if(parent->child != NULL) {
		nqiv_cmd_add_peer_node(parent->child, node);
	} else {
		nqiv_cmd_add_child_node(parent, node);
	}
}

void nqiv_cmd_destroy_node(nqiv_cmd_node* node)
{
	assert(node != NULL);
	assert(node->name != NULL);
	assert(node->description != NULL);
	if(node->child != NULL) {
		nqiv_cmd_destroy_node(node->child);
	}
	if(node->peer != NULL) {
		nqiv_cmd_destroy_node(node->peer);
	}
	if(node->args != NULL) {
		memset(node->args, 0,
		       nqiv_cmd_get_args_list_length((const nqiv_cmd_arg_desc**)node->args)
		           * sizeof(nqiv_cmd_arg_desc*));
	}
	memset(node->description, 0, strlen(node->description));
	memset(node->name, 0, strlen(node->name));
	memset(node, 0, sizeof(nqiv_cmd_node));
	free(node);
}

nqiv_cmd_node* nqiv_cmd_make_base_node(bool*                     status,
                                       const char*               name,
                                       const char*               description,
                                       const nqiv_cmd_arg_desc** args)
{
	const size_t node_size = sizeof(nqiv_cmd_node);
	const size_t name_size = (strlen(name) + 1) * sizeof(char);
	const size_t description_size = (strlen(description) + 1) * sizeof(char);
	assert(name_size >= 2 * sizeof(char));
	assert(description_size >= 2 * sizeof(char));
	const size_t args_size =
		args != NULL ? nqiv_cmd_get_args_list_length(args) * sizeof(nqiv_cmd_arg_desc*) : 0;
	nqiv_cmd_node* node =
		(nqiv_cmd_node*)calloc(1, node_size + name_size + description_size + args_size);
	if(node == NULL) {
		*status = *status && false;
		return NULL;
	}
	node->name = ((char*)node) + node_size;
	node->description = node->name + name_size;
	if(args_size > 0) {
		node->args = (nqiv_cmd_arg_desc**)(node->description + description_size);
		memcpy(node->args, args, args_size);
	}
	strcpy(node->name, name);
	assert(node->name[strlen(name)] == '\0');
	assert(strcmp(node->name, name) == 0);
	strcpy(node->description, description);
	assert(node->description[strlen(description)] == '\0');
	assert(strcmp(node->description, description) == 0);
	assert(node->args == NULL || node->args[nqiv_cmd_get_args_list_length(args) - 1] == NULL);
	*status = *status && true;
	return node;
}

nqiv_cmd_node* nqiv_cmd_make_leaf_node(bool*       status,
                                       const char* name,
                                       const char* description,
                                       void*       data,
                                       bool (*store_value)(nqiv_cmd_manager*, nqiv_cmd_arg_token**),
                                       void (*print_value)(nqiv_cmd_manager*),
                                       const nqiv_cmd_arg_desc** args)
{
	nqiv_cmd_node* node = nqiv_cmd_make_base_node(status, name, description, args);
	if(node == NULL) {
		*status = *status && false;
		return NULL;
	}
	assert(store_value != NULL || print_value != NULL);
	node->store_value = store_value;
	node->print_value = print_value;
	node->data = data;
	*status = *status && true;
	return node;
}

nqiv_cmd_node* nqiv_cmd_add_child_branch_node(bool*          status,
                                              nqiv_cmd_node* parent,
                                              const char*    name,
                                              const char*    description)
{
	if(parent == NULL || !*status) {
		return NULL;
	}
	nqiv_cmd_node* node = nqiv_cmd_make_base_node(status, name, description, NULL);
	nqiv_cmd_add_child_or_peer(parent, node);
	return node;
}

nqiv_cmd_node* nqiv_cmd_add_child_leaf_node(bool*          status,
                                            nqiv_cmd_node* parent,
                                            const char*    name,
                                            const char*    description,
                                            void*          data,
                                            bool (*store_value)(nqiv_cmd_manager*,
                                                                nqiv_cmd_arg_token**),
                                            void (*print_value)(nqiv_cmd_manager*),
                                            const nqiv_cmd_arg_desc** args)
{
	if(parent == NULL || !*status) {
		return NULL;
	}
	nqiv_cmd_node* node =
		nqiv_cmd_make_leaf_node(status, name, description, data, store_value, print_value, args);
	nqiv_cmd_add_child_or_peer(parent, node);
	return node;
}

#define STACKLEN 16

void nqiv_cmd_manager_build_cmdtree_set_current(nqiv_cmd_node** current_node, const nqiv_array* stack)
{
	assert(nqiv_array_get_units_count(stack) > 0);
	assert(nqiv_array_get_units_count(stack) < STACKLEN);
	*current_node = NULL;
	nqiv_array_get(stack, nqiv_array_get_last_idx(stack), current_node);
	assert(*current_node != NULL);
}

void nqiv_cmd_manager_build_cmdtree_deprecate(bool* deprecated)
{
	assert(*deprecated == false);
	*deprecated = true;
}

void nqiv_cmd_manager_build_cmdtree_apply_deprecate(nqiv_cmd_node* tmp_node, bool* deprecated)
{
	tmp_node->deprecated = *deprecated;
	*deprecated = false;
}

void nqiv_cmd_manager_build_cmdtree_b(nqiv_cmd_node** current_node,
                                      nqiv_array*     stack,
                                      nqiv_cmd_node** tmp_node,
                                      bool*           deprecated,
                                      bool*           status,
                                      const char*     name,
                                      const char*     description)
{
	nqiv_cmd_manager_build_cmdtree_set_current(current_node, stack);
	assert(*tmp_node == NULL);
	*tmp_node = nqiv_cmd_add_child_branch_node(status, *current_node, name, description);
	nqiv_cmd_manager_build_cmdtree_apply_deprecate(*tmp_node, deprecated);
	nqiv_array_push(stack, tmp_node);
	*tmp_node = NULL;
}

void nqiv_cmd_manager_build_cmdtree_l(nqiv_cmd_node** current_node,
                                      const nqiv_array*     stack,
                                      nqiv_cmd_node** tmp_node,
                                      bool*           deprecated,
                                      bool*           status,
                                      const char*     name,
                                      const char*     description,
                                      void*           data,
                                      bool (*store_value)(nqiv_cmd_manager*, nqiv_cmd_arg_token**),
                                      void (*print_value)(nqiv_cmd_manager*),
                                      const nqiv_cmd_arg_desc** args)
{
	nqiv_cmd_manager_build_cmdtree_set_current(current_node, stack);
	assert(args == NULL || nqiv_cmd_get_args_list_length(args) < NQIV_CMD_MAX_ARGS);
	assert(*tmp_node == NULL);
	*tmp_node = nqiv_cmd_add_child_leaf_node(status, *current_node, name, description, data,
	                                         store_value, print_value, args);
	nqiv_cmd_manager_build_cmdtree_apply_deprecate(*tmp_node, deprecated);
	*tmp_node = NULL;
}

void nqiv_cmd_manager_build_cmdtree_pop(nqiv_cmd_node** current_node, nqiv_array* stack)
{
	assert(nqiv_array_get_units_count(stack) > 0);
	nqiv_array_pop(stack, NULL);
	nqiv_cmd_manager_build_cmdtree_set_current(current_node, stack);
}

#define SET_CURRENT     nqiv_cmd_manager_build_cmdtree_set_current(&current_node, &stack);
#define DEPRECATE       nqiv_cmd_manager_build_cmdtree_deprecate(&deprecated);
#define APPLY_DEPRECATE nqiv_cmd_manager_build_cmdtree_apply_deprecate(tmp_node, &deprecated);
#define B(NAME, DESCRIPTION)                                                                 \
	nqiv_cmd_manager_build_cmdtree_b(&current_node, &stack, &tmp_node, &deprecated, &status, \
	                                 (NAME), (DESCRIPTION));
#define L(NAME, DESCRIPTION, DATA, STORE_VALUE, PRINT_VALUE, ARGS)                                \
	nqiv_cmd_manager_build_cmdtree_l(&current_node, &stack, &tmp_node, &deprecated, &status,      \
	                                 (NAME), (DESCRIPTION), (DATA), (STORE_VALUE), (PRINT_VALUE), \
	                                 (ARGS));
/* Leaf node with NULL (0) data. Specialized config/legacy code. */
#define L0(NAME, DESCRIPTION, STORE_VALUE, PRINT_VALUE, ARGS) \
	L(NAME, DESCRIPTION, NULL, STORE_VALUE, PRINT_VALUE, ARGS)
/* Leaf node for inspection. No storage or args. */
#define LI(NAME, DESCRIPTION, DATA, PRINT_VALUE) L(NAME, DESCRIPTION, DATA, NULL, PRINT_VALUE, NULL)
/* Leaf node for a configuration option. Specific data with print, store, and args. */
#define LC(NAME, DESCRIPTION, DATA, STORE_VALUE, PRINT_VALUE, ARGS) \
	L(NAME, DESCRIPTION, DATA, STORE_VALUE, PRINT_VALUE, ARGS)
/* Leaf node for action. Specialized storage function and args with no printable data. */
#define LA(NAME, DESCRIPTION, STORE_VALUE, ARGS) L(NAME, DESCRIPTION, NULL, STORE_VALUE, NULL, ARGS)
/* Leaf node for specialized printing. No data or storage specified. */
#define LP(NAME, DESCRIPTION, PRINT_VALUE) L(NAME, DESCRIPTION, NULL, NULL, PRINT_VALUE, NULL)
#define POP                                nqiv_cmd_manager_build_cmdtree_pop(&current_node, &stack);
// NOLINTBEGIN(google-readability-function-size,readability-function-size)
bool nqiv_cmd_manager_build_cmdtree(nqiv_cmd_manager* manager)
{
	const nqiv_cmd_arg_desc* sendkey_args[] = {&nqiv_parser_arg_type_key_action, NULL};
	const nqiv_cmd_arg_desc* idxname_args[] = {&nqiv_parser_arg_type_int_natural,
	                                           &nqiv_parser_arg_type_string_full, NULL};
	const nqiv_cmd_arg_desc* natural_args[] = {&nqiv_parser_arg_type_int_natural, NULL};
	const nqiv_cmd_arg_desc* positive_args[] = {&nqiv_parser_arg_type_int_positive, NULL};
	const nqiv_cmd_arg_desc* uint64_args[] = {&nqiv_parser_arg_type_Uint64, NULL};
	const nqiv_cmd_arg_desc* keyactionbrief_uint64_args[] = {&nqiv_parser_arg_type_key_action_brief,
	                                                         &nqiv_parser_arg_type_Uint64, NULL};
	const nqiv_cmd_arg_desc* keyactionbrief_pressaction_args[] = {
		&nqiv_parser_arg_type_key_action_brief, &nqiv_parser_arg_type_press_action, NULL};
	const nqiv_cmd_arg_desc* stringfull_args[] = {&nqiv_parser_arg_type_string_full, NULL};
	const nqiv_cmd_arg_desc* string_args[] = {&nqiv_parser_arg_type_string, NULL};
	const nqiv_cmd_arg_desc* pruner_args[] = {&nqiv_parser_arg_type_pruner, NULL};
	const nqiv_cmd_arg_desc* keybind_args[] = {&nqiv_parser_arg_type_keybind, NULL};
	const nqiv_cmd_arg_desc* worker_spec_args[] = {&nqiv_parser_arg_type_worker_spec, NULL};
	const nqiv_cmd_arg_desc* loglevel_args[] = {&nqiv_parser_arg_type_log_level, NULL};
	const nqiv_cmd_arg_desc* doublepositiveone_args[] = {&nqiv_parser_arg_type_double_positive_one,
	                                                     NULL};
	const nqiv_cmd_arg_desc* doublenegativeone_args[] = {&nqiv_parser_arg_type_double_negative_one,
	                                                     NULL};
	const nqiv_cmd_arg_desc* doublepositive_args[] = {&nqiv_parser_arg_type_double_positive, NULL};
	const nqiv_cmd_arg_desc* doublenegative_args[] = {&nqiv_parser_arg_type_double_negative, NULL};
	const nqiv_cmd_arg_desc* double_args[] = {&nqiv_parser_arg_type_double, NULL};
	const nqiv_cmd_arg_desc* intpositive_args[] = {&nqiv_parser_arg_type_int_positive, NULL};
	const nqiv_cmd_arg_desc* bool_args[] = {&nqiv_parser_arg_type_bool, NULL};
	const nqiv_cmd_arg_desc* color_args[] = {
		&nqiv_parser_arg_type_Uint8, &nqiv_parser_arg_type_Uint8, &nqiv_parser_arg_type_Uint8,
		&nqiv_parser_arg_type_Uint8, NULL};
	nqiv_cmd_node* stack_data[STACKLEN] = {0};
	nqiv_array     stack;
	nqiv_array_inherit(&stack, stack_data, sizeof(nqiv_cmd_node*), STACKLEN);
	bool           status = true;
	nqiv_cmd_node* root_node = nqiv_cmd_make_base_node(
		&status, "root",
		"Root of parsing tree. Prefix help to get help messages on commands, helptree to do the "
		"same recursively, helpchildren to only recurse one level, or dumpcfg to dump functional "
		"commands to set the current configuration. Lines can also be commented by prefixing with "
		"#",
		NULL);
	nqiv_cmd_node* current_node;
	nqiv_cmd_node* tmp_node = NULL;
	bool           deprecated = false;
	nqiv_array_push(&stack, &root_node);

	LA("sendkey", "Issue a simulated keyboard action to the program.", nqiv_cmd_parser_sendkey,
	   sendkey_args);
	B("insert", "Add a value to a particular location.");
	{
		LA("image", "Insert an image path to be opened at a particular index.",
		   nqiv_cmd_parser_insert_image, idxname_args);
	}
	POP;
	B("remove", "Remove a value from a particular location.");
	{
		B("image", "Remove an image from the list to be opened.");
		{
			LA("index", "Delete the image from the given index.",
			   nqiv_cmd_parser_remove_image_index, natural_args);
		}
		POP;
	}
	POP;
	B("append", "Add a value to the end of an existing series.");
	{
		B("log", "Append operations related to logging.");
		{
			L0("stream", "Log to the given stream.", nqiv_cmd_parser_append_log_stream,
			   nqiv_cmd_parser_print_log_stream, stringfull_args);
		}
		POP;
		LA("image", "Add an image path to the be opened.", nqiv_cmd_parser_append_image,
		   stringfull_args);
		L0("pruner",
		   "Declaratively specified pruning instructions. Use help to get list of commands.",
		   nqiv_cmd_parser_append_pruner, nqiv_cmd_parser_print_pruner, pruner_args);
		DEPRECATE L0(
			"extension",
			"(DEPRECATED nqiv will not be handling this for now) Add an image extension to "
			"be accepted.",
			nqiv_cmd_parser_set_none, nqiv_cmd_parser_print_none, string_args);
		L0("keybind", "Add a keybind.", nqiv_cmd_parser_append_keybind,
		   nqiv_cmd_parser_print_keybind, keybind_args);
		L0("thread", "In addition to the number of threads controlled by 'set thread count' custom threads may be specially added.", nqiv_cmd_parser_append_thread,
		   nqiv_cmd_parser_print_thread, worker_spec_args);
	}
	POP;
	B("set", "Alter a singular value.");
	{
		B("log", "Set operations related to logging.");
		{
			L0("level", "Log messages this level or lower are printed.",
			   nqiv_cmd_parser_set_log_level, nqiv_cmd_parser_print_log_level, loglevel_args);
			L0("prefix",
			   "Log message format. Special messages are delimited by #. ## produces a literal #. "
			   "#time:<strftime format># prints the time. #level# prints the log level.",
			   nqiv_cmd_parser_set_log_prefix, nqiv_cmd_parser_print_log_prefix, stringfull_args);
		}
		POP;
		B("thread", "Settings related to thread behavior.");
		{
			LC("count",
			   "Set the number of worker threads used by the software. Starts as the number of "
			   "threads on the machine divided by three (or one). This does not count toward VIPs "
			   "threads. See 'set vips threads' for that. Note that there may be a delay in the "
			   "actual number of threads matching the number set here as they restart.",
			   &(manager->state->pending_thread_count), nqiv_cmd_parser_set_thread_count,
			   nqiv_cmd_parser_print_data_int, positive_args);
			LC("event_interval",
			   "After waking, worker threads will check for events and process at most this many "
			   "before waking the master and going back to sleep. Longer times might produce "
			   "longer "
			   "loading delays, but "
			   "help UI responsiveness. 0 means they will process all available events.",
			   &(manager->state->thread_event_interval), nqiv_cmd_parser_set_thread_event_interval,
			   nqiv_cmd_parser_print_data_int, natural_args);
			LC("prune_delay",
			   "During updates to nqiv's state, a pruning cycle will be allowed to run if this "
			   "many "
			   "milliseconds has passed since the last "
			   "one. 0 always allow prune cycles.",
			   &(manager->state->prune_delay), nqiv_cmd_parser_set_data_uint64,
			   nqiv_cmd_parser_print_data_uint64, uint64_args);
			LC("extra_wakeup_delay",
			   "In addition to an internal algorithm, wait this long before a worker thread "
			   "awakens "
			   "to check for events. Longer times might produce longer loading delays, but help "
			   "improve UI responsiveness.",
			   &(manager->state->extra_wakeup_delay), nqiv_cmd_parser_set_extra_wakeup_delay,
			   nqiv_cmd_parser_print_data_int, natural_args);
			LC("event_timeout",
			   "How long to wait for various events (such as inputs or updates from worker "
			   "threads) "
			   "before doing housekeeping activities (such as pruning). 0 to wait infinitely.",
			   &(manager->state->event_timeout), nqiv_cmd_parser_set_data_int,
			   nqiv_cmd_parser_print_data_int, natural_args);
		}
		POP;
		B("vips", "Settings related to the VIPS library.");
		{
			LC("threads",
			   "Set the number of threads used by the VIPs library. The default is the number of "
			   "available threads divided by two (or one). If set to 0, it is determined by the "
			   "environment variable VIPS_CONCURRENCY, or if unset, the number of threads "
			   "available "
			   "on the machine.",
			   &(manager->state->vips_threads), nqiv_cmd_parser_set_vips_threads,
			   nqiv_cmd_parser_print_data_int, natural_args);
		}
		POP;
		B("zoom", "Set operations related to zooming.");
		{
			LC("left_amount", "Amount to pan the zoom left with each action.",
			   &(manager->state->images.zoom.pan_left_amount), nqiv_cmd_parser_set_data_double,
			   nqiv_cmd_parser_print_data_double, doublenegativeone_args);
			LC("right_amount", "Amount to pan the zoom right with each action",
			   &(manager->state->images.zoom.pan_right_amount), nqiv_cmd_parser_set_data_double,
			   nqiv_cmd_parser_print_data_double, doublepositiveone_args);
			LC("down_amount", "Amount to pan the zoom down with each action",
			   &(manager->state->images.zoom.pan_down_amount), nqiv_cmd_parser_set_data_double,
			   nqiv_cmd_parser_print_data_double, doublepositiveone_args);
			LC("up_amount", "Amount to pan the zoom up with each action",
			   &(manager->state->images.zoom.pan_up_amount), nqiv_cmd_parser_set_data_double,
			   nqiv_cmd_parser_print_data_double, doublenegativeone_args);
			LC("out_amount", "Amount to pan the zoom out with each action",
			   &(manager->state->images.zoom.zoom_out_amount), nqiv_cmd_parser_set_data_double,
			   nqiv_cmd_parser_print_data_double, doublepositive_args);
			LC("in_amount", "Amount to pan the zoom in with each action",
			   &(manager->state->images.zoom.zoom_in_amount), nqiv_cmd_parser_set_data_double,
			   nqiv_cmd_parser_print_data_double, doublenegative_args);
			LC("left_amount_more", "Higher amount to pan the zoom left with each action.",
			   &(manager->state->images.zoom.pan_left_amount_more), nqiv_cmd_parser_set_data_double,
			   nqiv_cmd_parser_print_data_double, doublenegativeone_args);
			LC("right_amount_more", "Higher amount to pan the zoom right with each action",
			   &(manager->state->images.zoom.pan_right_amount_more),
			   nqiv_cmd_parser_set_data_double, nqiv_cmd_parser_print_data_double,
			   doublepositiveone_args);
			LC("down_amount_more", "Higher amount to pan the zoom down with each action",
			   &(manager->state->images.zoom.pan_down_amount_more), nqiv_cmd_parser_set_data_double,
			   nqiv_cmd_parser_print_data_double, doublepositiveone_args);
			LC("up_amount_more", "Higher amount to pan the zoom up with each action",
			   &(manager->state->images.zoom.pan_up_amount_more), nqiv_cmd_parser_set_data_double,
			   nqiv_cmd_parser_print_data_double, doublenegativeone_args);
			LC("out_amount_more", "Higher amount to pan the zoom out with each action",
			   &(manager->state->images.zoom.zoom_out_amount_more), nqiv_cmd_parser_set_data_double,
			   nqiv_cmd_parser_print_data_double, doublepositive_args);
			LC("in_amount_more", "Higher amount to pan the zoom in with each action",
			   &(manager->state->images.zoom.zoom_in_amount_more), nqiv_cmd_parser_set_data_double,
			   nqiv_cmd_parser_print_data_double, doublenegative_args);
			LC("up_coordinate_x_times",
			   "This is multiplied against x axis panning caused by relative motion (like mouse "
			   "panning)",
			   &(manager->state->images.zoom.pan_coordinate_x_multiplier),
			   nqiv_cmd_parser_set_data_double, nqiv_cmd_parser_print_data_double, double_args);
			LC("up_coordinate_y_times",
			   "This is multiplied against y axis panning caused by relative motion (like mouse "
			   "panning)",
			   &(manager->state->images.zoom.pan_coordinate_y_multiplier),
			   nqiv_cmd_parser_set_data_double, nqiv_cmd_parser_print_data_double, double_args);
			L0("default",
			   "Default zoom setting when loading an image- 'keep' old zoom, 'fit' to display, or "
			   "set 'actual_size'.",
			   nqiv_cmd_parser_set_zoom_default, nqiv_cmd_parser_print_zoom_default, string_args);
			L0("scale_mode",
			   "Set scale mode used for SDL textures. Options are: 'nearest', 'linear', and 'best' "
			   "or 'anisotropic'.",
			   nqiv_cmd_parser_set_zoom_scale_mode, nqiv_cmd_parser_print_zoom_scale_mode,
			   string_args);
		}
		POP;
		B("thumbnail", "Set operations related to thumbnails.");
		{
			L0("path", "Path thumbnails are stored under. This directory must exist.",
			   nqiv_cmd_parser_set_thumbnail_path, nqiv_cmd_parser_print_thumbnail_path,
			   stringfull_args);
			LC("size_adjust",
			   "Number of pixels to resize thumbnails by with 'zoom' action in montage mode.",
			   &(manager->state->images.zoom.thumbnail_adjust), nqiv_cmd_parser_set_data_int,
			   nqiv_cmd_parser_print_data_int, intpositive_args);
			LC("size_adjust_more",
			   "Higher number of pixels to resize thumbnails by with 'zoom' action in montage "
			   "mode.",
			   &(manager->state->images.zoom.thumbnail_adjust_more), nqiv_cmd_parser_set_data_int,
			   nqiv_cmd_parser_print_data_int, intpositive_args);
			LC("load", "Whether to read thumbnails from the disk.",
			   &(manager->state->images.thumbnail.load), nqiv_cmd_parser_set_data_bool,
			   nqiv_cmd_parser_print_data_bool, bool_args);
			LC("save",
			   "Whether to save thumbnails to the disk. Note that if thumbnail_load is not set to "
			   "true, then thumbnails will always be saved, even if they are up to date on the "
			   "disk.",
			   &(manager->state->images.thumbnail.save), nqiv_cmd_parser_set_data_bool,
			   nqiv_cmd_parser_print_data_bool, bool_args);
			LC("size", "Width and height of thumbnails are the same value.",
			   &(manager->state->images.thumbnail.size), nqiv_cmd_parser_set_thumbnail_size,
			   nqiv_cmd_parser_print_data_int, intpositive_args);
		}
		POP;
		LC("default_frame_time", "If an animated image does not provide a frame time, use this.",
		   &(manager->state->images.default_frame_time), nqiv_cmd_parser_set_data_int,
		   nqiv_cmd_parser_print_data_int, intpositive_args);
		B("keypress", "Settings for delaying and registering keypresses.");
		{
			DEPRECATE B(
				"action",
				"(DEPRECATED: These settings are essentially part of the keybind implementation, "
				"now. Using these settings now will have no effect, nor will they print any "
				"information. This may cause some breakage of old key actions.) Key action "
				"specific settings for delaying and registering keypresses.");
			{
				L0("start_delay", "Before a key is registered, it must be pressed for this long.",
				   nqiv_cmd_parser_set_none, nqiv_cmd_parser_print_none,
				   keyactionbrief_uint64_args);
				L0("repeat_delay", "This is the starting delay for repeating a key.",
				   nqiv_cmd_parser_set_none, nqiv_cmd_parser_print_none,
				   keyactionbrief_uint64_args);
				L0("delay_accel",
				   "The repeat delay will be reduced by this amount for each repetition.",
				   nqiv_cmd_parser_set_none, nqiv_cmd_parser_print_none,
				   keyactionbrief_uint64_args);
				L0("minimum_delay", "The delay will never be less than this.",
				   nqiv_cmd_parser_set_none, nqiv_cmd_parser_print_none,
				   keyactionbrief_uint64_args);
				L0("send_on_up", "Register releasing of the key.", nqiv_cmd_parser_set_none,
				   nqiv_cmd_parser_print_none, keyactionbrief_pressaction_args);
				L0("send_on_down", "Register pressing of the key.", nqiv_cmd_parser_set_none,
				   nqiv_cmd_parser_print_none, keyactionbrief_pressaction_args);
			}
			POP;
			B("default", "Default settings for delaying and registering keypresses.");
			{
				LC("start_delay", "Before a key is registered, it must be pressed for this long.",
				   &(manager->state->keystates.settings.start_delay), nqiv_cmd_parser_set_data_int,
				   nqiv_cmd_parser_print_data_int, natural_args);
				LC("repeat_delay", "This is the starting delay for repeating a key.",
				   &(manager->state->keystates.settings.consecutive_delay),
				   nqiv_cmd_parser_set_data_int, nqiv_cmd_parser_print_data_int, natural_args);
				LC("delay_accel",
				   "The repeat delay will be reduced by this amount for each repetition.",
				   &(manager->state->keystates.settings.delay_accel), nqiv_cmd_parser_set_data_int,
				   nqiv_cmd_parser_print_data_int, natural_args);
				LC("minimum_delay", "The delay will never be less than this.",
				   &(manager->state->keystates.settings.minimum_delay),
				   nqiv_cmd_parser_set_data_int, nqiv_cmd_parser_print_data_int, natural_args);
				LC("send_on_up", "Register releasing of the key.",
				   &(manager->state->keystates.send_on_up), nqiv_cmd_parser_set_data_bool,
				   nqiv_cmd_parser_print_data_bool, bool_args);
				LC("send_on_down", "Register pressing of the key.",
				   &(manager->state->keystates.send_on_down), nqiv_cmd_parser_set_data_bool,
				   nqiv_cmd_parser_print_data_bool, bool_args);
			}
			POP;
			/*L0("", "", nqiv_cmd_parser_set_, nqiv_cmd_parser_print_, _args);*/
		}
		POP;
		B("color", "Set operations related to color.");
		{
			L0("background", "Color of background.", nqiv_cmd_parser_set_background_color,
			   nqiv_cmd_parser_print_background_color, color_args);
			L0("error", "Color of image area when there's an error loading.",
			   nqiv_cmd_parser_set_error_color, nqiv_cmd_parser_print_error_color, color_args);
			L0("loading", "Color of image area when image is still loading.",
			   nqiv_cmd_parser_set_loading_color, nqiv_cmd_parser_print_loading_color, color_args);
			L0("selection", "Color of box around selected image.",
			   nqiv_cmd_parser_set_selection_color, nqiv_cmd_parser_print_selection_color,
			   color_args);
			L0("mark", "Color of dashed box around marked image.", nqiv_cmd_parser_set_mark_color,
			   nqiv_cmd_parser_print_mark_color, color_args);
			L0("alpha_background_one",
			   "The background of a transparent image is rendered as checkers. This is the first "
			   "color.",
			   nqiv_cmd_parser_set_alpha_background_color_one,
			   nqiv_cmd_parser_print_alpha_background_color_one, color_args);
			L0("alpha_background_two",
			   "The background of a transparent image is rendered as checkers. This is the second "
			   "color.",
			   nqiv_cmd_parser_set_alpha_background_color_two,
			   nqiv_cmd_parser_print_alpha_background_color_two, color_args);
		}
		POP;
		B("preload", "Set options related to preloading images not yet in view.");
		{
			LC("ahead", "This number of images ahead of the current montage are loaded.",
			   &(manager->state->montage.preload.ahead), nqiv_cmd_parser_set_data_int,
			   nqiv_cmd_parser_print_data_int, natural_args);
			LC("behind", "This number of images behind of the current montage are loaded.",
			   &(manager->state->montage.preload.behind), nqiv_cmd_parser_set_data_int,
			   nqiv_cmd_parser_print_data_int, natural_args);
		}
		POP;
		LC("no_resample_oversized",
		   "Normally, if the image is larger than the platform's maximum texture size, it will be "
		   "reloaded for each "
		   "zoom. This keeps the normal behavior with the entire image downsized.",
		   &(manager->state->no_resample_oversized), nqiv_cmd_parser_set_data_bool,
		   nqiv_cmd_parser_print_data_bool, bool_args);
		B("show", "Settings related to displaying optional entities.");
		{
			LC("loading_indicator",
			   "Determine whether the loading indicator is rendered in image mode (achieve the "
			   "same "
			   "in montage mode by setting `set color loading` to match `set color background`).",
			   &(manager->state->show_loading_indicator), nqiv_cmd_parser_set_data_bool,
			   nqiv_cmd_parser_print_data_bool, bool_args);
		}
		POP;
		DEPRECATE L0(
			"queue_size",
			"(DEPRECATED: Sizes can still be printed, but not set. For now, this is handled by an "
			"internal algorithm.) Dynamic arrays in the software are backed by a given amount of "
			"memory. They will expand as needed, but it may improve performance to allocate more "
			"memory in advance. This value is the default minimum.",
			nqiv_cmd_parser_set_none, nqiv_cmd_parser_print_queue_size, intpositive_args);
		B("window", "Set operations related to the window.");
		{
			L0("width", "Set the width of the program window.", nqiv_cmd_parser_set_window_width,
			   nqiv_cmd_parser_print_window_width, intpositive_args);
			L0("height", "Set the height of the program window.", nqiv_cmd_parser_set_window_height,
			   nqiv_cmd_parser_print_window_height, intpositive_args);
		}
		POP;
		B("cmd", "Set operations related to the commands.");
		{
			LC("parse_error_quit", "Quit if there are errors parsing commands.",
			   &(manager->state->cmd_parse_error_quit), nqiv_cmd_parser_set_data_bool,
			   nqiv_cmd_parser_print_data_bool, bool_args);
			LC("apply_error_quit", "Quit if there are errors applying correctly-parsed commands.",
			   &(manager->state->cmd_apply_error_quit), nqiv_cmd_parser_set_data_bool,
			   nqiv_cmd_parser_print_data_bool, bool_args);
			LC("acknowledge",
			   "When finished with commands (successfully or unsuccessfully), print a message to "
			   "stdout acknowledging this and offering "
			   "basic stats. Also print messages saying when stdin is or isn't available for "
			   "commands, if nqiv is set to accept commands from it.",
			   &(manager->state->cmd_acknowledge), nqiv_cmd_parser_set_data_bool,
			   nqiv_cmd_parser_print_data_bool, bool_args);
		}
		POP;
	}
	POP;
	B("internal", "Inspect internal values of the software. These won't typically allow setting.");
	{
		B("log", "Internal logging values.");
		{
			LP("error_message",
			   "Error description for logger. This is an empty string if there is none.",
			   nqiv_cmd_parser_print_log_error_message);
		}
		POP;
		B("images", "Internal image and image management data.");
		{
			B("zoom", "Internal zooming and panning data.");
			{
				LI("image_to_viewport_ratio",
				   "Current zoom level. Lower values are zoomed in more.",
				   &(manager->state->images.zoom.image_to_viewport_ratio),
				   nqiv_cmd_parser_print_data_double);
				LI("image_to_viewport_ratio_max",
				   "This is typically how far out one has to zoom to view the whole image, fit "
				   "level for larger images, actual size for ones smaller than the screen.",
				   &(manager->state->images.zoom.image_to_viewport_ratio_max),
				   nqiv_cmd_parser_print_data_double);
				LI("fit_level", "Zoom level to view whole image.",
				   &(manager->state->images.zoom.fit_level), nqiv_cmd_parser_print_data_double);
				LI("actual_size_level", "Zoom level to view actual size of image.",
				   &(manager->state->images.zoom.actual_size_level),
				   nqiv_cmd_parser_print_data_double);
				LI("viewport_horizontal_shift", "Horizontal panning offset.",
				   &(manager->state->images.zoom.viewport_horizontal_shift),
				   nqiv_cmd_parser_print_data_double);
				LI("viewport_vertical_shift", "Vertical panning offset.",
				   &(manager->state->images.zoom.viewport_vertical_shift),
				   nqiv_cmd_parser_print_data_double);
			}
			POP;
			LI("max_texture_height",
			   "Max height of a texture representable by the current hardware.",
			   &(manager->state->images.max_texture_height), nqiv_cmd_parser_print_data_int);
			LI("max_texture_width", "Max width of a texture representable by the current hardware.",
			   &(manager->state->images.max_texture_width), nqiv_cmd_parser_print_data_int);
			LI("images", "List of images and all their (recursive) attributes. Very verbose.",
			   manager->state->images.images, nqiv_cmd_parser_print_data_images);
		}
		POP;
		B("pruner", "Internal pruner data.");
		{
			/* Pruner state is wiped after running. No point in inspecting. */
			LI("thread_event_transaction_group",
			   "If the pruner needs to communicate to other threads (such as to request a prune), "
			   "it does so with this transaction group.",
			   &(manager->state->pruner.thread_event_transaction_group),
			   nqiv_cmd_parser_print_data_int64);
		}
		POP;
		/* Keybinds? Or do regular test cases cover these. */
		/* Keyrates? */
		B("montage", "Internal montage data.");
		{
			B("dimensions", "Internal montage dimension data.");
			{
				LI("window_width", "Current window width used to render montage items.",
				   &(manager->state->montage.dimensions.window_width),
				   nqiv_cmd_parser_print_data_int);
				LI("window_height", "Current window height used to render montage items.",
				   &(manager->state->montage.dimensions.window_height),
				   nqiv_cmd_parser_print_data_int);
				LI("horizontal_margin",
				   "Ratio of the height of the window to devoted to the margin at the top and "
				   "bottom. 1.0 == 100%",
				   &(manager->state->montage.dimensions.horizontal_margin),
				   nqiv_cmd_parser_print_data_double);
				LI("vertical_margin",
				   "Ratio of the height of the window to devoted to the margin at the left and "
				   "right. 1.0 == 100%",
				   &(manager->state->montage.dimensions.vertical_margin),
				   nqiv_cmd_parser_print_data_double);
				LI("column_space",
				   "Ratio of the width of the window to devoted to the space between columns. 1.0 "
				   "== 100%",
				   &(manager->state->montage.dimensions.column_space),
				   nqiv_cmd_parser_print_data_double);
				LI("row_space",
				   "Ratio of the width of the window to devoted to the space between rows. 1.0 == "
				   "100%",
				   &(manager->state->montage.dimensions.row_space),
				   nqiv_cmd_parser_print_data_double);
				LI("count_per_row", "Number of thumbnails per row.",
				   &(manager->state->montage.dimensions.count_per_row),
				   nqiv_cmd_parser_print_data_int);
				LI("count", "Max number of thumbnails per 'page' or shown on screen at once.",
				   &(manager->state->montage.dimensions.count), nqiv_cmd_parser_print_data_int);
			}
			POP;
			B("positions", "Describe what sequence of images is displayed by the montage.");
			{
				LI("start", "First index of montage.", &(manager->state->montage.positions.start),
				   nqiv_cmd_parser_print_data_int);
				LI("selection", "Selected index of montage.",
				   &(manager->state->montage.positions.selection), nqiv_cmd_parser_print_data_int);
				LI("end", "Final index of montage + 1.", &(manager->state->montage.positions.end),
				   nqiv_cmd_parser_print_data_int);
			}
			POP;
			/* Preload should be set by command options. */
		}
		POP;
		LI("thread_queue", "Print all events currently waiting in the event queue. Very verbose.",
		   &(manager->state->thread_queue), nqiv_cmd_parser_print_data_event_queue);
		LI("key_actions", "Print all currently pending key actions. Very verbose.",
		   &(manager->state->key_actions), nqiv_cmd_parser_print_data_key_action_queue);
		LI("sdl_inited", "Print whether SDL has been initialized.", &(manager->state->SDL_inited),
		   nqiv_cmd_parser_print_data_bool);
		LI("alpha_background_width", "Current width of the image background alpha texture.",
		   &(manager->state->alpha_background_width), nqiv_cmd_parser_print_data_int);
		LI("alpha_background_height", "Current height of the image background alpha texture.",
		   &(manager->state->alpha_background_height), nqiv_cmd_parser_print_data_int);
		LI("thread_event_number", "Event number for SDL events sent from worker threads.",
		   &(manager->state->thread_event_number), nqiv_cmd_parser_print_data_uint32);
		LI("cfg_event_number", "Event number for SDL events sent from the command parser",
		   &(manager->state->cfg_event_number), nqiv_cmd_parser_print_data_uint32);
		LI("running",
		   "Running status. SUCCESS means nqiv is running. PASS means it has stopped, but there is "
		   "no error. FAIL means it has stopped due to an error.",
		   &(manager->state->running), nqiv_cmd_parser_print_data_shared_op_result);
		LI("thread_count", "Current number of running threads.", &(manager->state->thread_count),
		   nqiv_cmd_parser_print_data_int);
		LI("restart_threads", "Should threads be restarted should they quit?",
		   &(manager->state->restart_threads), nqiv_cmd_parser_print_data_bool);
		LI("thread_event_transaction_group",
		   "Current transaction group used by threads. Incremented to filter out events for things "
		   "that are no longer present on the screen.",
		   &(manager->state->thread_event_transaction_group),
		   nqiv_cmd_parser_print_data_shared_int64);
		LI("time_of_last_prune", "Milliseconds since program start when last prune was performed.",
		   &(manager->state->time_of_last_prune), nqiv_cmd_parser_print_data_uint64);
		/* Active thread count should definitely be zero */
		LI("render_cleared", "Does the display need to be redrawn?",
		   &(manager->state->render_cleared), nqiv_cmd_parser_print_data_bool);
		LI("in_montage", "Are we in montage mode?", &(manager->state->in_montage),
		   nqiv_cmd_parser_print_data_bool);
		LI("stretch_images", "Should we stretch images to fill up the entire screen?",
		   &(manager->state->stretch_images), nqiv_cmd_parser_print_data_bool);
		LI("first_frame_pending",
		   "Are we still waiting for the first frame to render. Used for an edge case where the "
		   "first frame an image is requested but not yet available, preventing zoom defaults "
		   "being loaded.",
		   &(manager->state->first_frame_pending), nqiv_cmd_parser_print_data_bool);
		LI("is_mouse_panning", "Should we follow the image to the mouse movement.",
		   &(manager->state->is_mouse_panning), nqiv_cmd_parser_print_data_bool);
		LI("cmd_read_stdin", "Should we read commands from stdin.",
		   &(manager->state->cmd_read_stdin), nqiv_cmd_parser_print_data_bool);
	}
	POP;

	if(status) {
		manager->root_node = root_node;
	}
	return status;
}
// NOLINTEND(google-readability-function-size,readability-function-size)
#undef POP
#undef B
#undef L
#undef LI
#undef L0
#undef LC
#undef LA
#undef LP
#undef APPLY_DEPRECATE
#undef DEPRECATE
#undef SET_CURRENT
#undef STACKLEN

void nqiv_cmd_manager_destroy(nqiv_cmd_manager* manager)
{
	if(manager->root_node != NULL) {
		nqiv_cmd_destroy_node(manager->root_node);
	}
	if(manager->buffer != NULL) {
		nqiv_array_destroy(manager->buffer);
	}
	memset(manager, 0, sizeof(nqiv_cmd_manager));
}

bool nqiv_cmd_manager_init(nqiv_cmd_manager* manager, nqiv_state* state)
{
	nqiv_cmd_manager_destroy(manager);
	manager->state = state;
	manager->state->cmd_parse_error_quit = true;
	manager->state->cmd_apply_error_quit = true;
	manager->buffer = nqiv_array_create(sizeof(char), NQIV_CMD_READ_BUFFER_LENGTH);
	if(manager->buffer == NULL) {
		nqiv_log_write(&manager->state->logger, NQIV_LOG_ERROR,
		               "Failed to allocate memory to create cmd buffer of length %d\n",
		               NQIV_CMD_READ_BUFFER_LENGTH);
		return false;
	}
	nqiv_array_set_max_data_length(manager->buffer, NQIV_CMD_READ_BUFFER_LENGTH_MAX);
	manager->buffer->min_add_count = NQIV_CMD_READ_BUFFER_LENGTH;
	if(!nqiv_cmd_manager_build_cmdtree(manager)) {
		nqiv_cmd_manager_destroy(manager);
		nqiv_log_write(&manager->state->logger, NQIV_LOG_ERROR,
		               "Failed to build cmd parsing tree.\n");
		return false;
	}
	nqiv_log_write(&manager->state->logger, NQIV_LOG_INFO, "Initialized cmd manager.\n");
	return true;
}

bool nqiv_cmd_consume_stream_from_path(nqiv_cmd_manager* manager, const char* path)
{
	nqiv_log_write(&manager->state->logger, NQIV_LOG_INFO, "Reading commands from %s\n", path);
	FILE* stream = nqiv_fopen(path, "r");
	if(stream == NULL) {
		nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING,
		               "Failed to read commands from %s\n", path);
		return false;
	}
	const bool output = nqiv_cmd_consume_stream(manager, stream);
	fclose(stream);
	return output;
}
