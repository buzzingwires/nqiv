#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <float.h>

#include <SDL2/SDL.h>
#include <omp.h>

#include "logging.h"
#include "array.h"
#include "state.h"
#include "keybinds.h"
#include "keyrate.h"
#include "pruner.h"

void nqiv_cmd_alert_main(nqiv_cmd_manager* manager)
{
	SDL_Event e = {0};
	e.type = SDL_USEREVENT;
	e.user.code = (Sint32)manager->state->cfg_event_number;
	if(SDL_PushEvent(&e) < 0) {
		nqiv_log_write( &manager->state->logger, NQIV_LOG_ERROR, "Failed to send SDL event from thread %d. SDL Error: %s\n", omp_get_thread_num(), SDL_GetError() );
	}
}

void nqiv_cmd_force_quit_main(nqiv_cmd_manager* manager)
{
	nqiv_key_action action = NQIV_KEY_ACTION_QUIT;
	nqiv_queue_push_force(&manager->state->key_actions, sizeof(nqiv_key_action), &action);
	nqiv_cmd_alert_main(manager);
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

bool nqiv_cmd_parser_set_thread_count(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->thread_count = tokens[0]->value.as_int;
	return true;
}

bool nqiv_cmd_parser_set_thread_event_interval(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->thread_event_interval = tokens[0]->value.as_int;
	return true;
}

bool nqiv_cmd_parser_set_vips_threads(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->vips_threads = tokens[0]->value.as_int;
	vips_concurrency_set(tokens[0]->value.as_int);
	return true;
}

bool nqiv_cmd_parser_set_prune_delay(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->prune_delay = tokens[0]->value.as_Uint64;
	return true;
}

bool nqiv_cmd_parser_set_extra_wakeup_delay(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->extra_wakeup_delay = tokens[0]->value.as_int;
	return true;
}

bool nqiv_cmd_parser_set_zoom_down_amount(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->images.zoom.pan_down_amount = tokens[0]->value.as_double;
	return true;
}

bool nqiv_cmd_parser_set_zoom_in_amount(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->images.zoom.zoom_in_amount = tokens[0]->value.as_double;
	return true;
}

bool nqiv_cmd_parser_set_zoom_left_amount(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->images.zoom.pan_left_amount = tokens[0]->value.as_double;
	return true;
}

bool nqiv_cmd_parser_set_zoom_out_amount(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->images.zoom.zoom_out_amount = tokens[0]->value.as_double;
	return true;
}

bool nqiv_cmd_parser_set_zoom_right_amount(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->images.zoom.pan_right_amount = tokens[0]->value.as_double;
	return true;
}

bool nqiv_cmd_parser_set_zoom_up_amount(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->images.zoom.pan_up_amount = tokens[0]->value.as_double;
	return true;
}

bool nqiv_cmd_parser_set_zoom_down_amount_more(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->images.zoom.pan_down_amount_more = tokens[0]->value.as_double;
	return true;
}

bool nqiv_cmd_parser_set_zoom_in_amount_more(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->images.zoom.zoom_in_amount_more = tokens[0]->value.as_double;
	return true;
}

bool nqiv_cmd_parser_set_zoom_left_amount_more(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->images.zoom.pan_left_amount_more = tokens[0]->value.as_double;
	return true;
}

bool nqiv_cmd_parser_set_zoom_out_amount_more(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->images.zoom.zoom_out_amount_more = tokens[0]->value.as_double;
	return true;
}

bool nqiv_cmd_parser_set_zoom_right_amount_more(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->images.zoom.pan_right_amount_more = tokens[0]->value.as_double;
	return true;
}

bool nqiv_cmd_parser_set_zoom_up_amount_more(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->images.zoom.pan_up_amount_more = tokens[0]->value.as_double;
	return true;
}

bool nqiv_cmd_parser_set_zoom_default(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	const char data_end = nqiv_cmd_tmpterm(tokens[0]->raw, tokens[0]->length);
	const nqiv_zoom_default zd = nqiv_text_to_zoom_default(tokens[0]->raw);
	nqiv_cmd_tmpret(tokens[0]->raw, tokens[0]->length, data_end);
	if(zd == NQIV_ZOOM_DEFAULT_UNKNOWN) {
		return false;
	}
	manager->state->zoom_default = zd;
	return true;
}

bool nqiv_cmd_parser_set_thumbnail_load(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->images.thumbnail.load = tokens[0]->value.as_bool;
	return true;
}

bool nqiv_cmd_parser_set_thumbnail_save(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->images.thumbnail.save = tokens[0]->value.as_bool;
	return true;
}

bool nqiv_cmd_parser_set_thumbnail_size(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	const int old_size = manager->state->images.thumbnail.size;
	manager->state->images.thumbnail.size = tokens[0]->value.as_int;
	nqiv_image_manager_reattempt_thumbnails(&manager->state->images, old_size);
	return true;
}

bool nqiv_cmd_parser_set_thumbnail_zoom_amount(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->images.zoom.thumbnail_adjust = tokens[0]->value.as_int;
	return true;
}

bool nqiv_cmd_parser_set_thumbnail_zoom_amount_more(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->images.zoom.thumbnail_adjust_more = tokens[0]->value.as_int;
	return true;
}

bool nqiv_cmd_parser_set_thumbnail_path(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	const char data_end = nqiv_cmd_tmpterm(tokens[0]->raw, tokens[0]->length);
	const bool output = nqiv_image_manager_set_thumbnail_root(&manager->state->images, tokens[0]->raw);
	nqiv_cmd_tmpret(tokens[0]->raw, tokens[0]->length, data_end);
	return output;
}

bool nqiv_cmd_parser_set_log_level(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->logger.level = tokens[0]->value.as_log_level;
	return true;
}

bool nqiv_cmd_parser_set_log_prefix(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	const char data_end = nqiv_cmd_tmpterm(tokens[0]->raw, tokens[0]->length);
	nqiv_log_set_prefix_format(&manager->state->logger, tokens[0]->raw);
	nqiv_cmd_tmpret(tokens[0]->raw, tokens[0]->length, data_end);
	return true;
}

bool nqiv_cmd_parser_set_no_resample_oversized(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->no_resample_oversized = tokens[0]->value.as_bool;
	return true;
}

bool nqiv_cmd_parser_set_parse_error_quit(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->cmd_parse_error_quit = tokens[0]->value.as_bool;
	return true;
}

bool nqiv_cmd_parser_set_apply_error_quit(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->cmd_apply_error_quit = tokens[0]->value.as_bool;
	return true;
}

bool nqiv_cmd_parser_apply_color( nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens, SDL_Color* color, const char* error_message, bool apply(nqiv_state* state) )
{
	SDL_Color tmp;
	memcpy( &tmp, color, sizeof(SDL_Color) );
	color->r = tokens[0]->value.as_Uint8;
	color->g = tokens[1]->value.as_Uint8;
	color->b = tokens[2]->value.as_Uint8;
	color->a = tokens[3]->value.as_Uint8;
	if( !apply(manager->state) ) {
		nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING, "Failed to apply color for %s.\n", error_message);
		memcpy( color, &tmp, sizeof(SDL_Color) );
		return false;
	}
	return true;
}

bool nqiv_cmd_parser_set_alpha_background_color_one(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	return nqiv_cmd_parser_apply_color(manager, tokens, &manager->state->alpha_checker_color_one, "alpha background color one", nqiv_state_recreate_all_alpha_background_textures);
}

bool nqiv_cmd_parser_set_alpha_background_color_two(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	return nqiv_cmd_parser_apply_color(manager, tokens, &manager->state->alpha_checker_color_two, "alpha background color two", nqiv_state_recreate_all_alpha_background_textures);
}

bool nqiv_cmd_parser_set_background_color(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	return nqiv_cmd_parser_apply_color(manager, tokens, &manager->state->background_color, "background color", nqiv_state_recreate_background_texture);
}

bool nqiv_cmd_parser_set_error_color(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	return nqiv_cmd_parser_apply_color(manager, tokens, &manager->state->error_color, "error color", nqiv_state_recreate_error_texture);
}

bool nqiv_cmd_parser_set_loading_color(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	return nqiv_cmd_parser_apply_color(manager, tokens, &manager->state->loading_color, "loading background color", nqiv_state_recreate_loading_texture);
}

bool nqiv_cmd_parser_set_selection_color(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	return nqiv_cmd_parser_apply_color(manager, tokens, &manager->state->selection_color, "selection outline color", nqiv_state_recreate_thumbnail_selection_texture);
}

bool nqiv_cmd_parser_set_mark_color(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	return nqiv_cmd_parser_apply_color(manager, tokens, &manager->state->mark_color, "mark outline color", nqiv_state_recreate_mark_texture);
}

bool nqiv_cmd_parser_set_preload_ahead(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->montage.preload.ahead = tokens[0]->value.as_int;
	return true;
}

bool nqiv_cmd_parser_set_preload_behind(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->montage.preload.behind = tokens[0]->value.as_int;
	return true;
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

bool nqiv_cmd_parser_set_minimum_delay(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->keystates.states[tokens[0]->value.as_key_action].settings.minimum_delay = tokens[1]->value.as_Uint64;
	return true;
}

bool nqiv_cmd_parser_set_minimum_delay_default(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->keystates.settings.minimum_delay = tokens[0]->value.as_Uint64;
	return true;
}

bool nqiv_cmd_parser_set_repeat_delay(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->keystates.states[tokens[0]->value.as_key_action].settings.consecutive_delay = tokens[1]->value.as_Uint64;
	return true;
}

bool nqiv_cmd_parser_set_repeat_delay_default(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->keystates.settings.consecutive_delay = tokens[0]->value.as_Uint64;
	return true;
}

bool nqiv_cmd_parser_set_send_on_down(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->keystates.states[tokens[0]->value.as_key_action].send_on_down = tokens[1]->value.as_press_action;
	return true;
}

bool nqiv_cmd_parser_set_send_on_down_default(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->keystates.send_on_down = tokens[0]->value.as_bool;
	return true;
}

bool nqiv_cmd_parser_set_send_on_up(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->keystates.states[tokens[0]->value.as_key_action].send_on_up = tokens[1]->value.as_press_action;
	return true;
}

bool nqiv_cmd_parser_set_send_on_up_default(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->keystates.send_on_up = tokens[0]->value.as_bool;
	return true;
}

bool nqiv_cmd_parser_set_start_delay(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->keystates.states[tokens[0]->value.as_key_action].settings.start_delay = tokens[1]->value.as_Uint64;
	return true;
}

bool nqiv_cmd_parser_set_start_delay_default(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->keystates.settings.start_delay = tokens[0]->value.as_Uint64;
	return true;
}

bool nqiv_cmd_parser_set_delay_accel(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->keystates.states[tokens[0]->value.as_key_action].settings.delay_accel = tokens[1]->value.as_Uint64;
	return true;
}

bool nqiv_cmd_parser_set_delay_accel_default(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->keystates.settings.delay_accel = tokens[0]->value.as_Uint64;
	return true;
}

bool nqiv_cmd_parser_append_pruner(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	return nqiv_pruner_append(&manager->state->pruner, &tokens[0]->value.as_pruner);
}

bool nqiv_cmd_parser_append_extension(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	const char data_end = nqiv_cmd_tmpterm(tokens[0]->raw, tokens[0]->length);
	const bool output = nqiv_image_manager_add_extension(&manager->state->images, tokens[0]->raw);
	nqiv_cmd_tmpret(tokens[0]->raw, tokens[0]->length, data_end);
	return output;
}

bool nqiv_cmd_parser_append_image(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	const char data_end = nqiv_cmd_tmpterm(tokens[0]->raw, tokens[0]->length);
	bool output = false;
	if( nqiv_image_manager_has_path_extension(&manager->state->images, tokens[0]->raw) ) {
		output = nqiv_image_manager_append(&manager->state->images, tokens[0]->raw);
	}
	nqiv_cmd_tmpret(tokens[0]->raw, tokens[0]->length, data_end);
	return output;
}

bool nqiv_cmd_parser_append_keybind(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	return nqiv_keybind_add(&manager->state->keybinds, &tokens[0]->value.as_keybind.key, tokens[0]->value.as_keybind.action);
}

bool nqiv_cmd_parser_append_log_stream(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	const char data_end = nqiv_cmd_tmpterm(tokens[0]->raw, tokens[0]->length);
	const bool output = nqiv_add_logger_path(manager->state, tokens[0]->raw);
	nqiv_cmd_tmpret(tokens[0]->raw, tokens[0]->length, data_end);
	return output;
}

bool nqiv_cmd_parser_insert_image(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	const char data_end = nqiv_cmd_tmpterm(tokens[1]->raw, tokens[1]->length);
	const bool output = nqiv_image_manager_insert(&manager->state->images, tokens[1]->raw, tokens[0]->value.as_int);
	nqiv_cmd_tmpret(tokens[1]->raw, tokens[1]->length, data_end);
	return output;
}

bool nqiv_cmd_parser_remove_image_index(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	return nqiv_image_manager_remove(&manager->state->images, tokens[0]->value.as_int);
}

bool nqiv_cmd_parser_sendkey(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	return nqiv_queue_push(&manager->state->key_actions, sizeof(nqiv_key_action), &tokens[0]->value.as_key_action);
}

bool nqiv_cmd_parser_set_queue_size(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	if(tokens[0]->value.as_int < manager->state->queue_length) {
		nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING, "Can't set queue length of %d, as it's less than %d.\n", tokens[0]->value.as_int, manager->state->queue_length);
		return false;
	}
	const int old_queue_length = manager->state->queue_length;
	manager->state->queue_length = tokens[0]->value.as_int;
	if( !nqiv_state_expand_queues(manager->state) ) {
		nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING, "Failed to set queue new queue length of %d, from %d.\n", tokens[0]->value.as_int, manager->state->queue_length);
		manager->state->queue_length = old_queue_length;
		return false;
	}
	return true;
}

void nqiv_cmd_print_indent(const nqiv_cmd_manager* manager)
{
	int indent_count;
	for(indent_count = manager->print_settings.indent; indent_count > 0; --indent_count) {
		fprintf(stdout, "\t");
	}
}

void nqiv_cmd_parser_print_thread_count(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%d", manager->state->thread_count);
}

void nqiv_cmd_parser_print_thread_event_interval(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%d", manager->state->thread_event_interval);
}

void nqiv_cmd_parser_print_vips_threads(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%d", manager->state->vips_threads);
}

void nqiv_cmd_parser_print_prune_delay(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%lu", manager->state->prune_delay);
}

void nqiv_cmd_parser_print_extra_wakeup_delay(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%d", manager->state->extra_wakeup_delay);
}

void nqiv_cmd_parser_print_zoom_down_amount(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%f", manager->state->images.zoom.pan_down_amount);
}

void nqiv_cmd_parser_print_zoom_in_amount(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%f", manager->state->images.zoom.zoom_in_amount);
}

void nqiv_cmd_parser_print_zoom_left_amount(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%f", manager->state->images.zoom.pan_left_amount);
}

void nqiv_cmd_parser_print_zoom_out_amount(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%f", manager->state->images.zoom.zoom_out_amount);
}

void nqiv_cmd_parser_print_zoom_right_amount(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%f", manager->state->images.zoom.pan_right_amount);
}

void nqiv_cmd_parser_print_zoom_up_amount(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%f", manager->state->images.zoom.pan_up_amount);
}

void nqiv_cmd_parser_print_zoom_down_amount_more(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%f", manager->state->images.zoom.pan_down_amount_more);
}

void nqiv_cmd_parser_print_zoom_in_amount_more(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%f", manager->state->images.zoom.zoom_in_amount_more);
}

void nqiv_cmd_parser_print_zoom_left_amount_more(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%f", manager->state->images.zoom.pan_left_amount_more);
}

void nqiv_cmd_parser_print_zoom_out_amount_more(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%f", manager->state->images.zoom.zoom_out_amount_more);
}

void nqiv_cmd_parser_print_zoom_right_amount_more(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%f", manager->state->images.zoom.pan_right_amount_more);
}

void nqiv_cmd_parser_print_zoom_up_amount_more(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%f", manager->state->images.zoom.pan_up_amount_more);
}

void nqiv_cmd_parser_print_zoom_default(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%s", nqiv_zoom_default_names[manager->state->zoom_default]);
}

void nqiv_cmd_parser_print_thumbnail_load(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%s", manager->state->images.thumbnail.load ? "true" : "false");
}

void nqiv_cmd_parser_print_thumbnail_save(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%s", manager->state->images.thumbnail.save ? "true" : "false");
}

void nqiv_cmd_parser_print_thumbnail_size(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%d", manager->state->images.thumbnail.size);
}

void nqiv_cmd_parser_print_thumbnail_zoom_amount(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%d", manager->state->images.zoom.thumbnail_adjust);
}

void nqiv_cmd_parser_print_thumbnail_zoom_amount_more(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%d", manager->state->images.zoom.thumbnail_adjust_more);
}

void nqiv_cmd_parser_print_thumbnail_path(nqiv_cmd_manager* manager)
{
	if(manager->state->images.thumbnail.root != NULL) {
		fprintf(stdout, "%s", manager->state->images.thumbnail.root);
	} else if(!manager->print_settings.dumpcfg) {
		fprintf(stdout, "UNSET");
	}
}

void nqiv_cmd_parser_print_no_resample_oversized(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%s", manager->state->no_resample_oversized ? "true" : "false");
}

void nqiv_cmd_parser_print_queue_size(nqiv_cmd_manager* manager)
{
	if(!manager->print_settings.dumpcfg) {
		fprintf(stdout, "\n");
		nqiv_cmd_print_indent(manager);
		fprintf(stdout, "%d\n", manager->state->queue_length);
		nqiv_cmd_print_indent(manager);
		fprintf(stdout, "Keybinds: %d\n", manager->state->keybinds.lookup->data_length);
		nqiv_cmd_print_indent(manager);
		fprintf(stdout, "Images: %d\n", manager->state->images.images->data_length);
		nqiv_cmd_print_indent(manager);
		fprintf(stdout, "Image extensions: %d\n", manager->state->images.extensions->data_length);
		nqiv_cmd_print_indent(manager);
		fprintf(stdout, "Logger streams list: %d\n", manager->state->logger_stream_names->data_length);
		int idx;
		for(idx = 0; idx < manager->state->thread_queue.bin_count; ++idx) {
			nqiv_cmd_print_indent(manager);
			fprintf(stdout, "Thread Queue %d: %d\n", idx, manager->state->thread_queue.bins[idx].array->data_length);
		}
		nqiv_cmd_print_indent(manager);
		fprintf(stdout, "Key Actions: %d\n", manager->state->key_actions.array->data_length);
		nqiv_cmd_print_indent(manager);
		fprintf(stdout, "Cmd Buffer: %d\n", manager->state->cmds.buffer->data_length);
	} else {
		fprintf(stdout, "%d\n", manager->state->queue_length);
		fprintf(stdout, "#Keybinds: %d\n", manager->state->keybinds.lookup->data_length);
		fprintf(stdout, "#Images: %d\n", manager->state->images.images->data_length);
		fprintf(stdout, "#Image extensions: %d\n", manager->state->images.extensions->data_length);
		fprintf(stdout, "#Logger streams list: %d\n", manager->state->logger_stream_names->data_length);
		int idx;
		for(idx = 0; idx < manager->state->thread_queue.bin_count; ++idx) {
			fprintf(stdout, "#Thread Queue %d: %d\n", idx, manager->state->thread_queue.bins[idx].array->data_length);
		}
		fprintf(stdout, "#Key Actions: %d\n", manager->state->key_actions.array->data_length);
		fprintf(stdout, "#Cmd Buffer: %d", manager->state->cmds.buffer->data_length);
	}
}

void nqiv_cmd_parser_print_log_level(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%s", nqiv_log_level_names[manager->state->logger.level / 10]);
}

void nqiv_cmd_parser_print_log_prefix(nqiv_cmd_manager* manager)
{
	if( strlen(manager->state->logger.prefix_format) == 0 && !manager->print_settings.dumpcfg ) {
		fprintf(stdout, "UNSET");
	} else {
		fprintf(stdout, "%s", manager->state->logger.prefix_format);
	}
}

void nqiv_cmd_parser_print_parse_error_quit(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%s", manager->state->cmd_parse_error_quit ? "true" : "false");
}

void nqiv_cmd_parser_print_apply_error_quit(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%s", manager->state->cmd_parse_error_quit ? "true" : "false");
}

void nqiv_cmd_parser_print_alpha_background_color_one(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%hhu %hhu %hhu %hhu", manager->state->alpha_checker_color_one.r, manager->state->alpha_checker_color_one.g, manager->state->alpha_checker_color_one.b, manager->state->alpha_checker_color_one.a);
}

void nqiv_cmd_parser_print_alpha_background_color_two(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%hhu %hhu %hhu %hhu", manager->state->alpha_checker_color_two.r, manager->state->alpha_checker_color_two.g, manager->state->alpha_checker_color_two.b, manager->state->alpha_checker_color_two.a);
}

void nqiv_cmd_parser_print_background_color(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%hhu %hhu %hhu %hhu", manager->state->background_color.r, manager->state->background_color.g, manager->state->background_color.b, manager->state->background_color.a);
}

void nqiv_cmd_parser_print_error_color(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%hhu %hhu %hhu %hhu", manager->state->error_color.r, manager->state->error_color.g, manager->state->error_color.b, manager->state->error_color.a);
}

void nqiv_cmd_parser_print_loading_color(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%hhu %hhu %hhu %hhu", manager->state->loading_color.r, manager->state->loading_color.g, manager->state->loading_color.b, manager->state->loading_color.a);
}

void nqiv_cmd_parser_print_selection_color(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%hhu %hhu %hhu %hhu", manager->state->selection_color.r, manager->state->selection_color.g, manager->state->selection_color.b, manager->state->selection_color.a);
}

void nqiv_cmd_parser_print_mark_color(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%hhu %hhu %hhu %hhu", manager->state->mark_color.r, manager->state->mark_color.g, manager->state->mark_color.b, manager->state->mark_color.a);
}

void nqiv_cmd_parser_print_preload_ahead(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%d", manager->state->montage.preload.ahead);
}

void nqiv_cmd_parser_print_preload_behind(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%d", manager->state->montage.preload.behind);
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

void nqiv_cmd_parser_print_delay_accel(nqiv_cmd_manager* manager)
{
	nqiv_key_action action;
	for(action = NQIV_KEY_ACTION_QUIT; action <= NQIV_KEY_ACTION_MAX; ++action) {
		if(!manager->print_settings.dumpcfg) {
			if(action == NQIV_KEY_ACTION_QUIT) {
				fprintf(stdout, "\n");
			}
			nqiv_cmd_print_indent(manager);
			fprintf(stdout, "%s %lu\n", nqiv_keybind_action_names[action], manager->state->keystates.states[action].settings.delay_accel);
		} else if(action == NQIV_KEY_ACTION_QUIT) {
			fprintf(stdout, "%s %lu\n", nqiv_keybind_action_names[action], manager->state->keystates.states[action].settings.delay_accel);
		} else if(action != NQIV_KEY_ACTION_MAX) {
			fprintf(stdout, "%s%s %lu\n", manager->print_settings.prefix, nqiv_keybind_action_names[action], manager->state->keystates.states[action].settings.delay_accel);
		} else {
			fprintf(stdout, "%s%s %lu", manager->print_settings.prefix, nqiv_keybind_action_names[action], manager->state->keystates.states[action].settings.delay_accel);
		}
	}
}

void nqiv_cmd_parser_print_delay_accel_default(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%lu", manager->state->keystates.settings.delay_accel);
}

void nqiv_cmd_parser_print_minimum_delay(nqiv_cmd_manager* manager)
{
	nqiv_key_action action;
	for(action = NQIV_KEY_ACTION_QUIT; action <= NQIV_KEY_ACTION_MAX; ++action) {
		if(!manager->print_settings.dumpcfg) {
			if(action == NQIV_KEY_ACTION_QUIT) {
				fprintf(stdout, "\n");
			}
			nqiv_cmd_print_indent(manager);
			fprintf(stdout, "%s %lu\n", nqiv_keybind_action_names[action], manager->state->keystates.states[action].settings.minimum_delay);
		} else if(action == NQIV_KEY_ACTION_QUIT) {
			fprintf(stdout, "%s %lu\n", nqiv_keybind_action_names[action], manager->state->keystates.states[action].settings.minimum_delay);
		} else if(action != NQIV_KEY_ACTION_MAX) {
			fprintf(stdout, "%s%s %lu\n", manager->print_settings.prefix, nqiv_keybind_action_names[action], manager->state->keystates.states[action].settings.minimum_delay);
		} else {
			fprintf(stdout, "%s%s %lu", manager->print_settings.prefix, nqiv_keybind_action_names[action], manager->state->keystates.states[action].settings.minimum_delay);
		}
	}
}

void nqiv_cmd_print_str_list(nqiv_cmd_manager* manager, nqiv_array* list)
{
	const int list_len = list->position / sizeof(char*);
	bool taken = false;
	int idx;
	for(idx = 0; idx < list_len; ++idx) {
		char* str = nqiv_array_get_char_ptr(list, idx);
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
	nqiv_cmd_print_str_list(manager, manager->state->logger_stream_names);
}

void nqiv_cmd_parser_print_pruner(nqiv_cmd_manager* manager)
{
	const int list_len = manager->state->pruner.pruners->position / sizeof(nqiv_pruner_desc);
	bool taken = false;
	int idx;
	for(idx = 0; idx < list_len; ++idx) {
		nqiv_pruner_desc desc = {0};
		if( nqiv_array_get_bytes(manager->state->pruner.pruners, idx, sizeof(nqiv_pruner_desc), &desc) ) {
			taken = true;
			char desc_str[NQIV_PRUNER_DESC_STRLEN] = {0};
			nqiv_pruner_desc_to_string(&desc, desc_str);
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
	}
	if(!taken && manager->print_settings.dumpcfg) {
		fprintf(stdout, "#%s", manager->print_settings.prefix);
	}
}

void nqiv_cmd_parser_print_extension(nqiv_cmd_manager* manager)
{
	nqiv_cmd_print_str_list(manager, manager->state->images.extensions);
}

void nqiv_cmd_parser_print_keybind(nqiv_cmd_manager* manager)
{
	const int list_len = manager->state->keybinds.lookup->position / sizeof(nqiv_keybind_pair);
	bool taken = false;
	int idx;
	for(idx = 0; idx < list_len; ++idx) {
		nqiv_keybind_pair pair = {.key = {0}, .action = -1};
		if( nqiv_array_get_bytes(manager->state->keybinds.lookup, idx, sizeof(nqiv_keybind_pair), &pair) ) {
			taken = true;
			char keybind_str[NQIV_KEYBIND_STRLEN] = {0};
			nqiv_keybind_to_string(&pair, keybind_str);
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
	}
	if(!taken && manager->print_settings.dumpcfg) {
		fprintf(stdout, "#%s", manager->print_settings.prefix);
	}
}

void nqiv_cmd_parser_print_minimum_delay_default(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%lu", manager->state->keystates.settings.minimum_delay);
}

void nqiv_cmd_parser_print_repeat_delay(nqiv_cmd_manager* manager)
{
	nqiv_key_action action;
	for(action = NQIV_KEY_ACTION_QUIT; action <= NQIV_KEY_ACTION_MAX; ++action) {
		if(!manager->print_settings.dumpcfg) {
			if(action == NQIV_KEY_ACTION_QUIT) {
				fprintf(stdout, "\n");
			}
			nqiv_cmd_print_indent(manager);
			fprintf(stdout, "%s %lu\n", nqiv_keybind_action_names[action], manager->state->keystates.states[action].settings.consecutive_delay);
		} else if(action == NQIV_KEY_ACTION_QUIT) {
			fprintf(stdout, "%s %lu\n", nqiv_keybind_action_names[action], manager->state->keystates.states[action].settings.consecutive_delay);
		} else if(action != NQIV_KEY_ACTION_MAX) {
			fprintf(stdout, "%s%s %lu\n", manager->print_settings.prefix, nqiv_keybind_action_names[action], manager->state->keystates.states[action].settings.consecutive_delay);
		} else {
			fprintf(stdout, "%s%s %lu", manager->print_settings.prefix, nqiv_keybind_action_names[action], manager->state->keystates.states[action].settings.consecutive_delay);
		}
	}
}

void nqiv_cmd_parser_print_repeat_delay_default(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%lu", manager->state->keystates.settings.consecutive_delay);
}

void nqiv_cmd_parser_print_start_delay(nqiv_cmd_manager* manager)
{
	nqiv_key_action action;
	for(action = NQIV_KEY_ACTION_QUIT; action <= NQIV_KEY_ACTION_MAX; ++action) {
		if(!manager->print_settings.dumpcfg) {
			if(action == NQIV_KEY_ACTION_QUIT) {
				fprintf(stdout, "\n");
			}
			nqiv_cmd_print_indent(manager);
			fprintf(stdout, "%s %lu\n", nqiv_keybind_action_names[action], manager->state->keystates.states[action].settings.start_delay);
		} else if(action == NQIV_KEY_ACTION_QUIT) {
			fprintf(stdout, "%s %lu\n", nqiv_keybind_action_names[action], manager->state->keystates.states[action].settings.start_delay);
		} else if(action != NQIV_KEY_ACTION_MAX) {
			fprintf(stdout, "%s%s %lu\n", manager->print_settings.prefix, nqiv_keybind_action_names[action], manager->state->keystates.states[action].settings.start_delay);
		} else {
			fprintf(stdout, "%s%s %lu", manager->print_settings.prefix, nqiv_keybind_action_names[action], manager->state->keystates.states[action].settings.start_delay);
		}
	}
}

void nqiv_cmd_parser_print_start_delay_default(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%lu", manager->state->keystates.settings.start_delay);
}

const char* nqiv_press_action_names[] =
{
	"default",
	"allow",
	"deny"
};

void nqiv_cmd_parser_print_send_on_down(nqiv_cmd_manager* manager)
{
	nqiv_key_action action;
	for(action = NQIV_KEY_ACTION_QUIT; action <= NQIV_KEY_ACTION_MAX; ++action) {
		if(!manager->print_settings.dumpcfg) {
			if(action == NQIV_KEY_ACTION_QUIT) {
				fprintf(stdout, "\n");
			}
			nqiv_cmd_print_indent(manager);
			fprintf(stdout, "%s %s\n", nqiv_keybind_action_names[action], nqiv_press_action_names[manager->state->keystates.states[action].send_on_down]);
		} else if(action == NQIV_KEY_ACTION_QUIT) {
			fprintf(stdout, "%s %s\n", nqiv_keybind_action_names[action], nqiv_press_action_names[manager->state->keystates.states[action].send_on_down]);
		} else if(action != NQIV_KEY_ACTION_MAX) {
			fprintf(stdout, "%s%s %s\n", manager->print_settings.prefix, nqiv_keybind_action_names[action], nqiv_press_action_names[manager->state->keystates.states[action].send_on_down]);
		} else {
			fprintf(stdout, "%s%s %s", manager->print_settings.prefix, nqiv_keybind_action_names[action], nqiv_press_action_names[manager->state->keystates.states[action].send_on_down]);
		}
	}
}

void nqiv_cmd_parser_print_send_on_down_default(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%s", manager->state->keystates.send_on_down ? "true" : "false");
}

void nqiv_cmd_parser_print_send_on_up(nqiv_cmd_manager* manager)
{
	nqiv_key_action action;
	for(action = NQIV_KEY_ACTION_QUIT; action <= NQIV_KEY_ACTION_MAX; ++action) {
		if(!manager->print_settings.dumpcfg) {
			if(action == NQIV_KEY_ACTION_QUIT) {
				fprintf(stdout, "\n");
			}
			nqiv_cmd_print_indent(manager);
			fprintf(stdout, "%s %s\n", nqiv_keybind_action_names[action], nqiv_press_action_names[manager->state->keystates.states[action].send_on_up]);
		} else if(action == NQIV_KEY_ACTION_QUIT) {
			fprintf(stdout, "%s %s\n", nqiv_keybind_action_names[action], nqiv_press_action_names[manager->state->keystates.states[action].send_on_up]);
		} else if(action != NQIV_KEY_ACTION_MAX) {
			fprintf(stdout, "%s%s %s\n", manager->print_settings.prefix, nqiv_keybind_action_names[action], nqiv_press_action_names[manager->state->keystates.states[action].send_on_up]);
		} else {
			fprintf(stdout, "%s%s %s", manager->print_settings.prefix, nqiv_keybind_action_names[action], nqiv_press_action_names[manager->state->keystates.states[action].send_on_up]);
		}
	}
}

void nqiv_cmd_parser_print_send_on_up_default(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%s", manager->state->keystates.send_on_up ? "true" : "false");
}

/* TODO Setup cmds function in main */

nqiv_cmd_arg_desc nqiv_parser_arg_type_int_natural = {
	.type = NQIV_CMD_ARG_INT,
	.setting = { .of_int = {.min = 0, .max = INT_MAX} },
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_int_positive = {
	.type = NQIV_CMD_ARG_INT,
	.setting = { .of_int = {.min = 1, .max = INT_MAX} },
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_Uint64 = {
	.type = NQIV_CMD_ARG_UINT64,
	.setting = { .of_Uint64 = {.min = (Uint64)0, .max = (Uint64)INT_MAX} },
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_Uint8 = {
	.type = NQIV_CMD_ARG_UINT8,
	.setting = { {0} },
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_double_positive = {
	.type = NQIV_CMD_ARG_DOUBLE,
	.setting = { .of_double = {.min = DBL_MIN, .max = DBL_MAX} },
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_double_negative = {
	.type = NQIV_CMD_ARG_DOUBLE,
	.setting = { .of_double = {.min = -DBL_MAX, .max = -DBL_MIN} },
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_double_negative_one = {
	.type = NQIV_CMD_ARG_DOUBLE,
	.setting = { .of_double = {.min = -1.0, .max = -DBL_MIN} },
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_double_positive_one = {
	.type = NQIV_CMD_ARG_DOUBLE,
	.setting = { .of_double = {.min = DBL_MIN, .max = 1.0} },
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_bool = {
	.type = NQIV_CMD_ARG_BOOL,
	.setting = { {0} },
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_log_level = {
	.type = NQIV_CMD_ARG_LOG_LEVEL,
	.setting = { {0} },
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_press_action = {
	.type = NQIV_CMD_ARG_PRESS_ACTION,
	.setting = { {0} },
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_key_action = {
	.type = NQIV_CMD_ARG_KEY_ACTION,
	.setting = { {0} },
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_key_action_brief = {
	.type = NQIV_CMD_ARG_KEY_ACTION,
	.setting = { .of_key_action = {.brief = true} },
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_keybind = {
	.type = NQIV_CMD_ARG_KEYBIND,
	.setting = { {0} },
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_string = {
	.type = NQIV_CMD_ARG_STRING,
	.setting = { .of_string = {.spaceless = true} },
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_string_full = {
	.type = NQIV_CMD_ARG_STRING,
	.setting = { .of_string = {.spaceless = false} },
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_pruner = {
	.type = NQIV_CMD_ARG_PRUNER,
	.setting = { {0} },
};

nqiv_cmd_node nqiv_parser_nodes_root = {
	.name = "root",
	.description = "Root of parsing tree. Prefix help to get help messages on commands, helptree to do the same recursively, or dumpcfg to dump functional commands to set the current configuration. Lines can also be commented by prefixing with #",
	.store_value = NULL,
	.print_value = NULL,
	.args = {NULL},
	.children = {
		&(nqiv_cmd_node)
		{
			.name = "sendkey",
			.description = "Issue a simulated keyboard action to the program.",
			.store_value = nqiv_cmd_parser_sendkey,
			.print_value = NULL,
			.args = {&nqiv_parser_arg_type_key_action, NULL},
			.children = {NULL},
		},
		&(nqiv_cmd_node)
		{
			.name = "insert",
			.description = "Add a value to a particular location.",
			.store_value = NULL,
			.print_value = NULL,
			.args = {NULL},
			.children = {
				&(nqiv_cmd_node)
				{
					.name = "image",
					.description = "Insert an image path to be opened at a particular index.",
					.store_value = nqiv_cmd_parser_insert_image,
					.print_value = NULL,
					.args = {&nqiv_parser_arg_type_int_natural, &nqiv_parser_arg_type_string_full, NULL},
					.children = {NULL},
				},
				NULL,
			},
		},
		&(nqiv_cmd_node)
		{
			.name = "remove",
			.description = "Remove a value from a particular location.",
			.store_value = NULL,
			.print_value = NULL,
			.args = {NULL},
			.children = {
				&(nqiv_cmd_node)
				{
					.name = "image",
					.description = "Remove an image to be opened.",
					.store_value = NULL,
					.print_value = NULL,
					.args = {NULL},
					.children = {
						&(nqiv_cmd_node)
						{
							.name = "index",
							.description = "Delete the image from the given index.",
							.store_value = nqiv_cmd_parser_remove_image_index,
							.print_value = NULL,
							.args = {&nqiv_parser_arg_type_int_natural, NULL},
							.children = {NULL},
						},
						NULL,
					}
				},
				NULL,
			},
		},
		&(nqiv_cmd_node)
		{
			.name = "append",
			.description = "Add a value to the end of an existing series.",
			.store_value = NULL,
			.print_value = NULL,
			.args = {NULL},
			.children = {
				&(nqiv_cmd_node)
				{
					.name = "log",
					.description = "Append operations related to logging.",
					.store_value = NULL,
					.print_value = NULL,
					.args = {NULL},
					.children = {
						&(nqiv_cmd_node)
						{
							.name = "stream",
							.description = "Log to the given stream.",
							.store_value = nqiv_cmd_parser_append_log_stream,
							.print_value = nqiv_cmd_parser_print_log_stream,
							.args = {&nqiv_parser_arg_type_string_full, NULL},
							.children = {NULL},
						},
						NULL,
					}
				},
				&(nqiv_cmd_node)
				{
					.name = "image",
					.description = "Add an image path to the be opened.",
					.store_value = nqiv_cmd_parser_append_image,
					.print_value = NULL,
					.args = {&nqiv_parser_arg_type_string_full, NULL},
					.children = {NULL},
				},
				&(nqiv_cmd_node)
				{
					.name = "pruner",
					.description = "Declaratively specified pruning instructions. Use help to get list of commands.",
					.store_value = nqiv_cmd_parser_append_pruner,
					.print_value = nqiv_cmd_parser_print_pruner,
					.args = {&nqiv_parser_arg_type_pruner, NULL},
					.children = {NULL},
				},
				&(nqiv_cmd_node)
				{
					.name = "extension",
					.description = "Add an image extension to be accepted.",
					.store_value = nqiv_cmd_parser_append_extension,
					.print_value = nqiv_cmd_parser_print_extension,
					.args = {&nqiv_parser_arg_type_string, NULL},
					.children = {NULL},
				},
				&(nqiv_cmd_node)
				{
					.name = "keybind",
					.description = "Add a keybind.",
					.store_value = nqiv_cmd_parser_append_keybind,
					.print_value = nqiv_cmd_parser_print_keybind,
					.args = {&nqiv_parser_arg_type_keybind, NULL},
					.children = {NULL},
				},
				NULL,
			},
		},
		&(nqiv_cmd_node)
		{
			.name = "set",
			.description = "Alter a singular value.",
			.store_value = NULL,
			.print_value = NULL,
			.args = {NULL},
			.children = {
				&(nqiv_cmd_node)
				{
					.name = "log",
					.description = "Set operations related to logging.",
					.store_value = NULL,
					.print_value = NULL,
					.args = {NULL},
					.children = {
						&(nqiv_cmd_node)
						{
							.name = "level",
							.description = "Log messages this level or lower are printed.",
							.store_value = nqiv_cmd_parser_set_log_level,
							.print_value = nqiv_cmd_parser_print_log_level,
							.args = {&nqiv_parser_arg_type_log_level, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "prefix",
							.description = "Log message format. Special messages are delimited by #. ## produces a literal #. #time:<strftime format># prints the time. #level# prints the log level.",
							.store_value = nqiv_cmd_parser_set_log_prefix,
							.print_value = nqiv_cmd_parser_print_log_prefix,
							.args = {&nqiv_parser_arg_type_string_full, NULL},
							.children = {NULL},
						},
						NULL,
					}
				},
				&(nqiv_cmd_node)
				{
					.name = "thread",
					.description = "Settings related to thread behavior.",
					.store_value = NULL,
					.print_value = NULL,
					.args = {NULL},
					.children = {
						&(nqiv_cmd_node)
						{
							.name = "count",
							.description = "Set the number of worker threads used by the software. Starts as the number of threads on the machine divided by three (or one). This does not count toward VIPs threads. See 'set vips threads' for that.",
							.store_value = nqiv_cmd_parser_set_thread_count,
							.print_value = nqiv_cmd_parser_print_thread_count,
							.args = {&nqiv_parser_arg_type_int_positive, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "event_interval",
							.description = "Threads will update the master after processing this many events. 0 to process all.",
							.store_value = nqiv_cmd_parser_set_thread_event_interval,
							.print_value = nqiv_cmd_parser_print_thread_event_interval,
							.args = {&nqiv_parser_arg_type_int_natural, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "prune_delay",
							.description = "A pruning cycle will be allowed to run after this many milliseconds since the last one. 0 always allow prune cycles.",
							.store_value = nqiv_cmd_parser_set_prune_delay,
							.print_value = nqiv_cmd_parser_print_prune_delay,
							.args = {&nqiv_parser_arg_type_Uint64, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "extra_wakeup_delay",
							.description = "In addition to an internal algorithm, wait this long to wait to let the master thread lock a worker. Longer times might produce longer loading delays, but help improve UI responsiveness.",
							.store_value = nqiv_cmd_parser_set_extra_wakeup_delay,
							.print_value = nqiv_cmd_parser_print_extra_wakeup_delay,
							.args = {&nqiv_parser_arg_type_int_natural, NULL},
							.children = {NULL},
						},
						NULL
					}
				},
				&(nqiv_cmd_node)
				{
					.name = "vips",
					.description = "Settings related to the VIPS library.",
					.store_value = NULL,
					.print_value = NULL,
					.args = {NULL},
					.children = {
						&(nqiv_cmd_node)
						{
							.name = "threads",
							.description = "Set the number of threads used by the VIPs library. The default is the number of available threads divided by two (or one). If set to 0, it is determined by the environment variable VIPS_CONCURRENCY, or if unset, the number of threads available on the machine.",
							.store_value = nqiv_cmd_parser_set_vips_threads,
							.print_value = nqiv_cmd_parser_print_vips_threads,
							.args = {&nqiv_parser_arg_type_int_natural, NULL},
							.children = {NULL},
						},
						NULL
					}
				},
				&(nqiv_cmd_node)
				{
					.name = "zoom",
					.description = "Set operations related to zooming.",
					.store_value = NULL,
					.print_value = NULL,
					.args = {NULL},
					.children = {
						&(nqiv_cmd_node)
						{
							.name = "left_amount",
							.description = "Amount to pan the zoom left with each action",
							.store_value = nqiv_cmd_parser_set_zoom_left_amount,
							.print_value = nqiv_cmd_parser_print_zoom_left_amount,
							.args = {&nqiv_parser_arg_type_double_negative_one, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "right_amount",
							.description = "Amount to pan the zoom right with each action",
							.store_value = nqiv_cmd_parser_set_zoom_right_amount,
							.print_value = nqiv_cmd_parser_print_zoom_right_amount,
							.args = {&nqiv_parser_arg_type_double_positive_one, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "down_amount",
							.description = "Amount to pan the zoom down with each action",
							.store_value = nqiv_cmd_parser_set_zoom_down_amount,
							.print_value = nqiv_cmd_parser_print_zoom_down_amount,
							.args = {&nqiv_parser_arg_type_double_positive_one, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "up_amount",
							.description = "Amount to pan the zoom up with each action",
							.store_value = nqiv_cmd_parser_set_zoom_up_amount,
							.print_value = nqiv_cmd_parser_print_zoom_up_amount,
							.args = {&nqiv_parser_arg_type_double_negative_one, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "out_amount",
							.description = "Amount to pan the zoom out with each action",
							.store_value = nqiv_cmd_parser_set_zoom_out_amount,
							.print_value = nqiv_cmd_parser_print_zoom_out_amount,
							.args = {&nqiv_parser_arg_type_double_positive, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "in_amount",
							.description = "Amount to pan the zoom in with each action",
							.store_value = nqiv_cmd_parser_set_zoom_in_amount,
							.print_value = nqiv_cmd_parser_print_zoom_in_amount,
							.args = {&nqiv_parser_arg_type_double_negative, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "left_amount_more",
							.description = "Higher amount to pan the zoom left with each action",
							.store_value = nqiv_cmd_parser_set_zoom_left_amount_more,
							.print_value = nqiv_cmd_parser_print_zoom_left_amount_more,
							.args = {&nqiv_parser_arg_type_double_negative_one, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "right_amount_more",
							.description = "Higher amount to pan the zoom right with each action",
							.store_value = nqiv_cmd_parser_set_zoom_right_amount_more,
							.print_value = nqiv_cmd_parser_print_zoom_right_amount_more,
							.args = {&nqiv_parser_arg_type_double_positive_one, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "down_amount_more",
							.description = "Higher amount to pan the zoom down with each action",
							.store_value = nqiv_cmd_parser_set_zoom_down_amount_more,
							.print_value = nqiv_cmd_parser_print_zoom_down_amount_more,
							.args = {&nqiv_parser_arg_type_double_positive_one, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "up_amount_more",
							.description = "Higher amount to pan the zoom up with each action",
							.store_value = nqiv_cmd_parser_set_zoom_up_amount_more,
							.print_value = nqiv_cmd_parser_print_zoom_up_amount_more,
							.args = {&nqiv_parser_arg_type_double_negative_one, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "out_amount_more",
							.description = "Higher amount to pan the zoom out with each action",
							.store_value = nqiv_cmd_parser_set_zoom_out_amount_more,
							.print_value = nqiv_cmd_parser_print_zoom_out_amount_more,
							.args = {&nqiv_parser_arg_type_double_positive, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "in_amount_more",
							.description = "Higher amount to pan the zoom in with each action",
							.store_value = nqiv_cmd_parser_set_zoom_in_amount_more,
							.print_value = nqiv_cmd_parser_print_zoom_in_amount_more,
							.args = {&nqiv_parser_arg_type_double_negative, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "default",
							.description = "Default zoom setting when loading an image- 'keep' old zoom, 'fit' to display, or set 'actual_size'.",
							.store_value = nqiv_cmd_parser_set_zoom_default,
							.print_value = nqiv_cmd_parser_print_zoom_default,
							.args = {&nqiv_parser_arg_type_string, NULL},
							.children = {NULL},
						},
						NULL,
					}
				},
				&(nqiv_cmd_node)
				{
					.name = "thumbnail",
					.description = "Set operations related to thumbnails.",
					.store_value = NULL,
					.print_value = NULL,
					.args = {NULL},
					.children = {
						&(nqiv_cmd_node)
						{
							.name = "path",
							.description = "Path thumbnails are stored under. This directory must exist.",
							.store_value = nqiv_cmd_parser_set_thumbnail_path,
							.print_value = nqiv_cmd_parser_print_thumbnail_path,
							.args = {&nqiv_parser_arg_type_string_full, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "size_adjust",
							.description = "Number of pixels to resize thumbnails by with 'zoom' action in montage mode.",
							.store_value = nqiv_cmd_parser_set_thumbnail_zoom_amount,
							.print_value = nqiv_cmd_parser_print_thumbnail_zoom_amount,
							.args = {&nqiv_parser_arg_type_int_positive, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "size_adjust_more",
							.description = "Higher number of pixels to resize thumbnails by with 'zoom' action in montage mode.",
							.store_value = nqiv_cmd_parser_set_thumbnail_zoom_amount_more,
							.print_value = nqiv_cmd_parser_print_thumbnail_zoom_amount_more,
							.args = {&nqiv_parser_arg_type_int_positive, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "load",
							.description = "Whether to read thumbnails from the disk.",
							.store_value = nqiv_cmd_parser_set_thumbnail_load,
							.print_value = nqiv_cmd_parser_print_thumbnail_load,
							.args = {&nqiv_parser_arg_type_bool, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "save",
							.description = "Whether to save thumbnails to the disk. Note that if thumbnail_load is not set to true, then thumbnails will always be saved, even if they are up to date on the disk.",
							.store_value = nqiv_cmd_parser_set_thumbnail_save,
							.print_value = nqiv_cmd_parser_print_thumbnail_save,
							.args = {&nqiv_parser_arg_type_bool, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "size",
							.description = "Width and height of thumbnails are the same value.",
							.store_value = nqiv_cmd_parser_set_thumbnail_size,
							.print_value = nqiv_cmd_parser_print_thumbnail_size,
							.args = {&nqiv_parser_arg_type_int_positive, NULL},
							.children = {NULL},
						},
						NULL,
					}
				},
				&(nqiv_cmd_node)
				{
					.name = "keypress",
					.description = "Settings for delaying and registering keypresses.",
					.store_value = NULL,
					.print_value = NULL,
					.args = {NULL},
					.children =  {
						&(nqiv_cmd_node)
						{
							.name = "action",
							.description = "Key action specific settings for delaying and registering keypresses.",
							.store_value = NULL,
							.print_value = NULL,
							.args = {NULL},
							.children =  {
								&(nqiv_cmd_node)
								{
									.name = "start_delay",
									.description = "Before a key is registered, it must be pressed for this long.",
									.store_value = nqiv_cmd_parser_set_start_delay,
									.print_value = nqiv_cmd_parser_print_start_delay,
									.args = {&nqiv_parser_arg_type_key_action_brief, &nqiv_parser_arg_type_Uint64, NULL},
									.children = {NULL},
								},
								&(nqiv_cmd_node)
								{
									.name = "repeat_delay",
									.description = "This is the starting delay for repeating a key.",
									.store_value = nqiv_cmd_parser_set_repeat_delay,
									.print_value = nqiv_cmd_parser_print_repeat_delay,
									.args = {&nqiv_parser_arg_type_key_action_brief, &nqiv_parser_arg_type_Uint64, NULL},
									.children = {NULL},
								},
								&(nqiv_cmd_node)
								{
									.name = "delay_accel",
									.description = "The repeat delay will be reduced by this amount for each repetition.",
									.store_value = nqiv_cmd_parser_set_delay_accel,
									.print_value = nqiv_cmd_parser_print_delay_accel,
									.args = {&nqiv_parser_arg_type_key_action_brief, &nqiv_parser_arg_type_Uint64, NULL},
									.children = {NULL},
								},
								&(nqiv_cmd_node)
								{
									.name = "minimum_delay",
									.description = "The delay will never be less than this.",
									.store_value = nqiv_cmd_parser_set_minimum_delay,
									.print_value = nqiv_cmd_parser_print_minimum_delay,
									.args = {&nqiv_parser_arg_type_key_action_brief, &nqiv_parser_arg_type_Uint64, NULL},
									.children = {NULL},
								},
								&(nqiv_cmd_node)
								{
									.name = "send_on_up",
									.description = "Register releasing of the key.",
									.store_value = nqiv_cmd_parser_set_send_on_up,
									.print_value = nqiv_cmd_parser_print_send_on_up,
									.args = {&nqiv_parser_arg_type_key_action_brief, &nqiv_parser_arg_type_press_action, NULL},
									.children = {NULL},
								},
								&(nqiv_cmd_node)
								{
									.name = "send_on_down",
									.description = "Register pressing of the key.",
									.store_value = nqiv_cmd_parser_set_send_on_down,
									.print_value = nqiv_cmd_parser_print_send_on_down,
									.args = {&nqiv_parser_arg_type_key_action_brief, &nqiv_parser_arg_type_press_action, NULL},
									.children = {NULL},
								},
								NULL
							},
						},
						&(nqiv_cmd_node)
						{
							.name = "default",
							.description = "Default settings for delaying and registering keypresses.",
							.store_value = NULL,
							.print_value = NULL,
							.args = {NULL},
							.children =  {
								&(nqiv_cmd_node)
								{
									.name = "start_delay",
									.description = "Before a key is registered, it must be pressed for this long.",
									.store_value = nqiv_cmd_parser_set_start_delay_default,
									.print_value = nqiv_cmd_parser_print_start_delay_default,
									.args = {&nqiv_parser_arg_type_Uint64, NULL},
									.children = {NULL},
								},
								&(nqiv_cmd_node)
								{
									.name = "repeat_delay",
									.description = "This is the starting delay for repeating a key.",
									.store_value = nqiv_cmd_parser_set_repeat_delay_default,
									.print_value = nqiv_cmd_parser_print_repeat_delay_default,
									.args = {&nqiv_parser_arg_type_Uint64, NULL},
									.children = {NULL},
								},
								&(nqiv_cmd_node)
								{
									.name = "delay_accel",
									.description = "The repeat delay will be reduced by this amount for each repetition.",
									.store_value = nqiv_cmd_parser_set_delay_accel_default,
									.print_value = nqiv_cmd_parser_print_delay_accel_default,
									.args = {&nqiv_parser_arg_type_Uint64, NULL},
									.children = {NULL},
								},
								&(nqiv_cmd_node)
								{
									.name = "minimum_delay",
									.description = "The delay will never be less than this.",
									.store_value = nqiv_cmd_parser_set_minimum_delay_default,
									.print_value = nqiv_cmd_parser_print_minimum_delay_default,
									.args = {&nqiv_parser_arg_type_Uint64, NULL},
									.children = {NULL},
								},
								&(nqiv_cmd_node)
								{
									.name = "send_on_up",
									.description = "Register releasing of the key.",
									.store_value = nqiv_cmd_parser_set_send_on_up_default,
									.print_value = nqiv_cmd_parser_print_send_on_up_default,
									.args = {&nqiv_parser_arg_type_bool, NULL},
									.children = {NULL},
								},
								&(nqiv_cmd_node)
								{
									.name = "send_on_down",
									.description = "Register pressing of the key.",
									.store_value = nqiv_cmd_parser_set_send_on_down_default,
									.print_value = nqiv_cmd_parser_print_send_on_down_default,
									.args = {&nqiv_parser_arg_type_bool, NULL},
									.children = {NULL},
								},
								NULL
							}
						},
						NULL
					},
				},
				&(nqiv_cmd_node)
				{
					.name = "color",
					.description = "Set operations related to color.",
					.store_value = NULL,
					.print_value = NULL,
					.args = {NULL},
					.children = {
						&(nqiv_cmd_node)
						{
							.name = "background",
							.description = "Color of background.",
							.store_value = nqiv_cmd_parser_set_background_color,
							.print_value = nqiv_cmd_parser_print_background_color,
							.args = {&nqiv_parser_arg_type_Uint8, &nqiv_parser_arg_type_Uint8, &nqiv_parser_arg_type_Uint8, &nqiv_parser_arg_type_Uint8, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "error",
							.description = "Color of image area when there's an error loading.",
							.store_value = nqiv_cmd_parser_set_error_color,
							.print_value = nqiv_cmd_parser_print_error_color,
							.args = {&nqiv_parser_arg_type_Uint8, &nqiv_parser_arg_type_Uint8, &nqiv_parser_arg_type_Uint8, &nqiv_parser_arg_type_Uint8, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "loading",
							.description = "Color of image area when image is still loading.",
							.store_value = nqiv_cmd_parser_set_loading_color,
							.print_value = nqiv_cmd_parser_print_loading_color,
							.args = {&nqiv_parser_arg_type_Uint8, &nqiv_parser_arg_type_Uint8, &nqiv_parser_arg_type_Uint8, &nqiv_parser_arg_type_Uint8, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "selection",
							.description = "Color of box around selected image.",
							.store_value = nqiv_cmd_parser_set_selection_color,
							.print_value = nqiv_cmd_parser_print_selection_color,
							.args = {&nqiv_parser_arg_type_Uint8, &nqiv_parser_arg_type_Uint8, &nqiv_parser_arg_type_Uint8, &nqiv_parser_arg_type_Uint8, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "mark",
							.description = "Color of dashed box around marked image.",
							.store_value = nqiv_cmd_parser_set_mark_color,
							.print_value = nqiv_cmd_parser_print_mark_color,
							.args = {&nqiv_parser_arg_type_Uint8, &nqiv_parser_arg_type_Uint8, &nqiv_parser_arg_type_Uint8, &nqiv_parser_arg_type_Uint8, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "alpha_background_one",
							.description = "The background of a transparent image is rendered as checkers. This is the first color.",
							.store_value = nqiv_cmd_parser_set_alpha_background_color_one,
							.print_value = nqiv_cmd_parser_print_alpha_background_color_one,
							.args = {&nqiv_parser_arg_type_Uint8, &nqiv_parser_arg_type_Uint8, &nqiv_parser_arg_type_Uint8, &nqiv_parser_arg_type_Uint8, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "alpha_background_two",
							.description = "The background of a transparent image is rendered as checkers. This is the second color.",
							.store_value = nqiv_cmd_parser_set_alpha_background_color_two,
							.print_value = nqiv_cmd_parser_print_alpha_background_color_two,
							.args = {&nqiv_parser_arg_type_Uint8, &nqiv_parser_arg_type_Uint8, &nqiv_parser_arg_type_Uint8, &nqiv_parser_arg_type_Uint8, NULL},
							.children = {NULL},
						},
						NULL,
					}
				},
				&(nqiv_cmd_node)
				{
					.name = "preload",
					.description = "Set options related to preloading images not yet in view.",
					.store_value = NULL,
					.print_value = NULL,
					.args = {NULL},
					.children = {
						&(nqiv_cmd_node)
						{
							.name = "ahead",
							.description = "This number of images ahead of the current montage are loaded.",
							.store_value = nqiv_cmd_parser_set_preload_ahead,
							.print_value = nqiv_cmd_parser_print_preload_ahead,
							.args = {&nqiv_parser_arg_type_int_natural, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "behind",
							.description = "This number of images ahead of the current montage are loaded.",
							.store_value = nqiv_cmd_parser_set_preload_behind,
							.print_value = nqiv_cmd_parser_print_preload_behind,
							.args = {&nqiv_parser_arg_type_int_natural, NULL},
							.children = {NULL},
						},
						NULL,
					}
				},
				&(nqiv_cmd_node)
				{
					.name = "no_resample_oversized",
					.description = "Normally, if the image is larger than 16k by 16k pixels, it will be reloaded for each zoom. This keeps the normal behavior with the entire image downsized.",
					.store_value = nqiv_cmd_parser_set_no_resample_oversized,
					.print_value = nqiv_cmd_parser_print_no_resample_oversized,
					.args = {&nqiv_parser_arg_type_bool, NULL},
					.children = {NULL},
				},
				&(nqiv_cmd_node)
				{
					.name = "queue_size",
					.description = "Dynamic arrays in the software are backed by a given amount of memory. They will expand as needed, but it may improve performance to allocate more memory in advance. This value is the default minimum.",
					.store_value = nqiv_cmd_parser_set_queue_size,
					.print_value = nqiv_cmd_parser_print_queue_size,
					.args = {&nqiv_parser_arg_type_int_positive, NULL},
					.children = {NULL},
				},
				&(nqiv_cmd_node)
				{
					.name = "window",
					.description = "Set operations related to the window.",
					.store_value = NULL,
					.print_value = NULL,
					.args = {NULL},
					.children = {
						&(nqiv_cmd_node)
						{
							.name = "width",
							.description = "Set the width of the program window",
							.store_value = nqiv_cmd_parser_set_window_width,
							.print_value = nqiv_cmd_parser_print_window_width,
							.args = {&nqiv_parser_arg_type_int_positive, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "height",
							.description = "Set the height of the program window",
							.store_value = nqiv_cmd_parser_set_window_height,
							.print_value = nqiv_cmd_parser_print_window_height,
							.args = {&nqiv_parser_arg_type_int_positive, NULL},
							.children = {NULL},
						},
						NULL,
					}
				},
				&(nqiv_cmd_node)
				{
					.name = "cmd",
					.description = "Set operations related to the commands.",
					.store_value = NULL,
					.print_value = NULL,
					.args = {NULL},
					.children = {
						&(nqiv_cmd_node)
						{
							.name = "parse_error_quit",
							.description = "Quit if there are errors parsing commands.",
							.store_value = nqiv_cmd_parser_set_parse_error_quit,
							.print_value = nqiv_cmd_parser_print_parse_error_quit,
							.args = {&nqiv_parser_arg_type_bool, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "apply_error_quit",
							.description = "Quit if there are errors applying correctly-parsed commands.",
							.store_value = nqiv_cmd_parser_set_apply_error_quit,
							.print_value = nqiv_cmd_parser_print_apply_error_quit,
							.args = {&nqiv_parser_arg_type_bool, NULL},
							.children = {NULL},
						},
						NULL,
					}
				},
				NULL,
			}
		},
		NULL
	}
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

int nqiv_cmd_scan_subs(const char* data, const int start, const int end, const bool negated, const char** subs, int* length)
{
	int offset = -1;
	int bidx;
	for(bidx = start; bidx < end; ++bidx) {
		int sidx = 0;
		while(subs[sidx] != NULL) {
			if(!negated) {
				if(strncmp(&data[bidx], subs[sidx], strlen(subs[sidx]) ) == 0) {
					if(length != NULL) {
						*length = strlen(subs[sidx]);
					}
					goto found;
				}
			} else {
				if(strncmp(&data[bidx], subs[sidx], strlen(subs[sidx]) ) == 0) {
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

int nqiv_cmd_scan_not_whitespace_and_eol(const char* data, const int start, const int end, int* length)
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
	(void) manager;

	fprintf(stdout, "#");
}

void nqiv_cmd_print_single_arg( nqiv_cmd_manager* manager, const nqiv_cmd_arg_desc* arg, void (*print_prefix)(const nqiv_cmd_manager*) )
{
	switch(arg->type) {
	case NQIV_CMD_ARG_INT:
		fprintf(stdout, "INT(%d-%d)", arg->setting.of_int.min, arg->setting.of_int.max);
		break;
	case NQIV_CMD_ARG_DOUBLE:
		fprintf(stdout, "DOUBLE(%f-%f)", arg->setting.of_double.min, arg->setting.of_double.max);
		break;
	case NQIV_CMD_ARG_UINT64:
		fprintf(stdout, "UINT64(%lu-%lu)", arg->setting.of_Uint64.min, arg->setting.of_Uint64.max);
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
			nqiv_log_level level;
			for(level = NQIV_LOG_ANY; level <= NQIV_LOG_ERROR; level += 10) {
				fprintf(stdout, "%s", nqiv_log_level_names[level / 10]);
				if(level != NQIV_LOG_FINAL) {
					fprintf(stdout, "|");
				}
			}
		}
		fprintf(stdout, ")");
		break;
	case NQIV_CMD_ARG_PRESS_ACTION:
		fprintf(stdout, "PRESS_ACTION(");
		{
			nqiv_keyrate_press_action action;
			for(action = NQIV_KEYRATE_START; action <= NQIV_KEYRATE_END; ++action) {
				fprintf(stdout, "%s", nqiv_press_action_names[action]);
				if(action != NQIV_KEYRATE_END) {
					fprintf(stdout, "|");
				}
			}
		}
		fprintf(stdout, ")");
		break;
	case NQIV_CMD_ARG_KEY_ACTION:
		fprintf(stdout, "KEY_ACTION");
		if(!arg->setting.of_key_action.brief) {
			fprintf(stdout, "\n");
			nqiv_key_action action;
			for(action = NQIV_KEY_ACTION_QUIT; action <= NQIV_KEY_ACTION_MAX; ++action) {
				print_prefix(manager);
				fprintf(stdout, "%s\n", nqiv_keybind_action_names[action]);
			}
		}
		break;
	case NQIV_CMD_ARG_KEYBIND:
		fprintf(stdout, "<SDL_key_sequence>=<key_action>");
		break;
	case NQIV_CMD_ARG_STRING:
		fprintf(stdout, "STRING(%s)", arg->setting.of_string.spaceless ? "spaceless" : "spaces allowed");
		break;
	case NQIV_CMD_ARG_PRUNER:
		fprintf(stdout, "PRUNER\n");
		print_prefix(manager);
		fprintf(stdout, "'no' will clear the option that comes after.\n");
		print_prefix(manager);
		fprintf(stdout, "'sum' <MAX> will check the addition of all checked integer values against another specified value to determine success.\n");
		print_prefix(manager);
		fprintf(stdout, "'or' will use boolean or with the result of all checks to determine success.\n");
		print_prefix(manager);
		fprintf(stdout, "'and' will use boolean and with the result of all checks to determine success.\n");
		print_prefix(manager);
		fprintf(stdout, "'unload' will cause specified image datatypes to be unloaded in the event of a failed check.\n");
		print_prefix(manager);
		fprintf(stdout, "'hard' will cause 'unload' to always work. Otherwise, they will only be unloaded if the corresponding texture is not NULL (this can prevent prematurely unloading things needed for the texture).\n");
		print_prefix(manager);
		fprintf(stdout, "'thumbnail' will cause thumbnail images to be considered by the following checks.\n");
		print_prefix(manager);
		fprintf(stdout, "'image' will cause normal images to be considered by the following checks.\n");
		print_prefix(manager);
		fprintf(stdout, "'vips' will cause the following checks to consider vips data only.\n");
		print_prefix(manager);
		fprintf(stdout, "'raw' will cause the following checks to consider raw data only.\n");
		print_prefix(manager);
		fprintf(stdout, "'surface' will cause the following checks to consider SDL surface data only.\n");
		print_prefix(manager);
		fprintf(stdout, "'texture' will cause the following checks to consider SDL texture data only.\n");
		print_prefix(manager);
		fprintf(stdout, "'loaded_ahead' <THRESHOLD> <MAX> will check for whether the number of loaded images after the threshold is greater than max.\n");
		print_prefix(manager);
		fprintf(stdout, "'loaded_behind' <THRESHOLD> <MAX> will check for whether the number of loaded images before the threshold is greater than max.\n");
		print_prefix(manager);
		fprintf(stdout, "'bytes_ahead' <THRESHOLD> <MAX> will check for whether the number of loaded bytes after the threshold is greater than max.\n");
		print_prefix(manager);
		fprintf(stdout, "'bytes_behind' <THRESHOLD> <MAX> will check for whether the number of loaded bytes before the threshold is greater than max.\n");
		print_prefix(manager);
		fprintf(stdout, "'self_opened' will check if the currently-selected image is loaded.\n");
		print_prefix(manager);
		fprintf(stdout, "'not_animated' will check if the currently-selected image is not animated.");
		break;
	}
}

void nqiv_cmd_print_args( nqiv_cmd_manager* manager, const nqiv_cmd_arg_desc* const* args, void (*print_prefix)(const nqiv_cmd_manager*) )
{
	int idx = 0;
	while(args[idx] != NULL) {
		nqiv_cmd_print_single_arg(manager, args[idx], print_prefix);
		++idx;
		if(args[idx] != NULL) {
			fprintf(stdout, " ");
		}
	}
}

void nqiv_cmd_dumpcfg(nqiv_cmd_manager* manager, const nqiv_cmd_node* current_node, const bool recurse, const char* current_cmd)
{
	char new_cmd[NQIV_CMD_DUMPCFG_BUFFER_LENGTH] = {0};
	int new_position = strlen(current_cmd);
	memcpy(new_cmd, current_cmd, new_position);
	if(current_node != &nqiv_parser_nodes_root) {
		memcpy( new_cmd + new_position, current_node->name, strlen(current_node->name) );
		new_position += strlen(current_node->name);
		new_cmd[new_position] = ' ';
		new_position += sizeof(char);
	}
	if(current_node->print_value != NULL || current_node->store_value != NULL) {
		fprintf(stdout, "#%s\n#", current_node->description);
		nqiv_cmd_print_args(manager, (const nqiv_cmd_arg_desc* const*)(current_node->args), nqiv_cmd_print_comment_prefix);
		fprintf(stdout, "\n");
		if(current_node->print_value != NULL) {
			if(strncmp( new_cmd, "append", strlen("append") ) != 0) {
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
	int cidx = 0;
	while(current_node->children[cidx] != NULL) {
		nqiv_cmd_dumpcfg(manager, current_node->children[cidx], recurse, new_cmd);
		++cidx;
	}
}

void nqiv_cmd_print_help(nqiv_cmd_manager* manager, const nqiv_cmd_node* current_node, const bool recurse)
{
	nqiv_cmd_print_indent(manager);
	fprintf(stdout, "%s: %s", current_node->name, current_node->description);
	if(current_node->args[0] != NULL) {
		fprintf(stdout, " - ");
		nqiv_cmd_print_args(manager, (const nqiv_cmd_arg_desc* const*)(current_node->args), nqiv_cmd_print_indent);
	}
	if(current_node->print_value != NULL) {
		fprintf(stdout, " - ");
		current_node->print_value(manager);
	}
	fprintf(stdout, "\n");
	if(!recurse) {
		return;
	}
	manager->print_settings.indent += 1;
	int cidx = 0;
	while(current_node->children[cidx] != NULL) {
		nqiv_cmd_print_help(manager, current_node->children[cidx], recurse);
		++cidx;
	}
	manager->print_settings.indent -= 1;
}

int nqiv_cmd_parse_arg_token(nqiv_cmd_manager* manager, const nqiv_cmd_node* current_node, const int tidx, const int start_idx, const int eolpos, nqiv_cmd_arg_token* token)
{
	char* mutdata = manager->buffer->data;
	const char data_end = nqiv_cmd_tmpterm(mutdata, eolpos);
	int output = -1;
	char* mutdata_start = mutdata + start_idx;
	const char* data = mutdata_start;
	const nqiv_cmd_arg_desc* desc = current_node->args[tidx];
	switch(desc->type){
	case NQIV_CMD_ARG_INT:
		{
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd checking int arg at %d for token %s for input %s\n", tidx, current_node->name, data);
			char* end = NULL;
			const int tmp = strtol(data, &end, 10);
			if(errno != ERANGE && end != NULL && tmp >= desc->setting.of_int.min && tmp <= desc->setting.of_int.max) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd int arg at %d for token %s is %d for input %s\n", tidx, current_node->name, tmp, data);
				token->value.as_int = tmp;
				output = end - data;
			} else {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING, "Cmd error parsing int arg at %d for token %s with input %s\n", tidx, current_node->name, data);
			}
		}
		break;
	case NQIV_CMD_ARG_DOUBLE:
		{
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd checking double arg at %d for token %s for input %s\n", tidx, current_node->name, data);
			char* end = NULL;
			const double tmp = strtod(data, &end);
			if(errno != ERANGE && end != NULL && tmp >= desc->setting.of_double.min && tmp <= desc->setting.of_double.max) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd double at %d for token %s is %f for input %s\n", tidx, current_node->name, tmp, data);
				token->value.as_double = tmp;
				output = end - data;
			} else {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING, "Cmd error parsing double arg at %d for token %s with input %s\n", tidx, current_node->name, data);
			}
		}
		break;
	case NQIV_CMD_ARG_UINT64:
		{
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd checking uint64 arg at %d for token %s for input %s\n", tidx, current_node->name, data);
			char* end = NULL;
			const int tmp = strtol(data, &end, 10);
			if(errno != ERANGE && end != NULL && (Uint64)tmp >= desc->setting.of_Uint64.min && (Uint64)tmp <= desc->setting.of_Uint64.max) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd uint64 arg at %d for token %s is %d for input %s\n", tidx, current_node->name, tmp, data);
				token->value.as_Uint64 = (Uint64)tmp;
				output = end - data;
			} else {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING, "Cmd error parsing uint64 arg at %d for token %s with input %s\n", tidx, current_node->name, data);
			}
		}
		break;
	case NQIV_CMD_ARG_UINT8:
		{
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd checking uint8 arg at %d for token %s for input %s\n", tidx, current_node->name, data);
			char* end = NULL;
			const int tmp = strtol(data, &end, 10);
			if(errno != ERANGE && end != NULL && tmp >= 0 && tmp <= 255) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd uint8 arg at %d for token %s is %d for input %s\n", tidx, current_node->name, tmp, data);
				token->value.as_Uint8 = (Uint8)tmp;
				output = end - data;
			} else {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING, "Cmd error parsing uint8 arg at %d for token %s with input %s\n", tidx, current_node->name, data);
			}
		}
		break;
	case NQIV_CMD_ARG_BOOL:
		{
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd checking bool arg at %d for token %s for input %s\n", tidx, current_node->name, data);
			if(strcmp(data, "true") == 0) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd bool arg at %d for token %s is true for input %s\n", tidx, current_node->name, data);
				token->value.as_bool = true;
				output = strlen("true");
			} else if(strcmp(data, "false") == 0) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd bool arg at %d for token %s is false for input %s\n", tidx, current_node->name, data);
				token->value.as_bool = false;
				output = strlen("false");
			} else {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING, "Cmd error parsing bool arg at %d for token %s with input %s\n", tidx, current_node->name, data);
			}
		}
		break;
	case NQIV_CMD_ARG_LOG_LEVEL:
		{
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd checking log level arg at %d for token %s for input %s\n", tidx, current_node->name, data);
			const nqiv_log_level tmp = nqiv_log_level_from_string(data);
			if(tmp != NQIV_LOG_UNKNOWN) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd log level arg at %d for token %s is %s for input %s\n", tidx, current_node->name, nqiv_log_level_names[tmp / 10], data);
				token->value.as_log_level = tmp;
				output = strlen(nqiv_log_level_names[tmp / 10]);
			} else {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING, "Cmd error parsing log level arg at %d for token %s with input %s\n", tidx, current_node->name, data);
			}
		}
		break;
	case NQIV_CMD_ARG_PRESS_ACTION:
		{
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd checking press action arg at %d for token %s for input %s\n", tidx, current_node->name, data);
			nqiv_keyrate_press_action tmp;
			if( nqiv_keyrate_press_action_from_string(data, &tmp) ) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd press action arg at %d for token %s is %s for input %s\n", tidx, current_node->name, nqiv_press_action_names[tmp], data);
				token->value.as_press_action = tmp;
				output = strlen(nqiv_press_action_names[tmp]);
			} else {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING, "Cmd error parsing press action arg at %d for token %s with input %s\n", tidx, current_node->name, data);
			}
		}
		break;
	case NQIV_CMD_ARG_KEY_ACTION:
		{
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd checking key action arg at %d for token %s for input %s\n", tidx, current_node->name, data);
			int arg_end = nqiv_cmd_scan_whitespace(mutdata, start_idx, eolpos, NULL);
			if(arg_end == -1 || arg_end > eolpos) {
				arg_end = eolpos;
			}
			const char arg_end_char = nqiv_cmd_tmpterm(mutdata, arg_end);
			const nqiv_key_action tmp = nqiv_text_to_key_action(data);
			if(tmp != NQIV_KEY_ACTION_NONE) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd key action arg at %d for token %s is %s for input %s\n", tidx, current_node->name, nqiv_keybind_action_names[tmp], data);
				token->value.as_key_action = tmp;
				output = strlen(nqiv_keybind_action_names[tmp]);
			} else {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING, "Cmd error parsing key action arg at %d for token %s with input %s\n", tidx, current_node->name, data);
			}
			nqiv_cmd_tmpret(mutdata, arg_end, arg_end_char);
		}
		break;
	case NQIV_CMD_ARG_KEYBIND:
		{
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd checking keybind arg at %d for token %s for input %s\n", tidx, current_node->name, data);
			nqiv_keybind_pair tmp;
			const int length = nqiv_keybind_text_to_keybind(mutdata_start, &tmp);
			if(length != -1) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd keybind arg at %d for token %s for input %s\n", tidx, current_node->name, data);
				memcpy( &token->value.as_keybind, &tmp, sizeof(nqiv_keybind_pair) );
				output = length;
			} else {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING, "Cmd error parsing keybind arg at %d for token %s with input %s\n", tidx, current_node->name, data);
			}
		}
		break;
	case NQIV_CMD_ARG_PRUNER:
		{
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd checking pruner arg at %d for token %s for input %s\n", tidx, current_node->name, data);
			nqiv_pruner_desc tmp;
			if( nqiv_pruner_create_desc(&manager->state->logger, data, &tmp) ) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd pruner arg at %d for token %s for input %s\n", tidx, current_node->name, data);
				memcpy( &token->value.as_pruner, &tmp, sizeof(nqiv_pruner_desc) );
				output = eolpos - start_idx;
			} else {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING, "Cmd error parsing pruner arg at %d for token %s with input %s\n", tidx, current_node->name, data);
			}
		}
		break;
	case NQIV_CMD_ARG_STRING:
		if(desc->setting.of_string.spaceless) {
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd checking spaceless arg at %d for token %s for input %s\n", tidx, current_node->name, data);
			const int length = nqiv_cmd_scan_whitespace(data, 0, eolpos - start_idx, NULL);
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd spaceless string at %d for token %s for input %s\n", tidx, current_node->name, data);
			if(length != -1) {
				output = length;
			} else {
				output = eolpos - start_idx;
			}
		} else {
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd string at %d for token %s for input %s\n", tidx, current_node->name, data);
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

bool nqiv_cmd_parse_args(nqiv_cmd_manager* manager, const nqiv_cmd_node* current_node, const int start_idx, const int eolpos, nqiv_cmd_arg_token** tokens)
{
	bool error = false;
	int idx = start_idx;
	int tidx = 0;
	char* data = manager->buffer->data;

	while(current_node->args[tidx] != NULL) {
		const int next_text_offset = nqiv_cmd_scan_not_whitespace(data, idx, eolpos, NULL);
		if(next_text_offset != -1) {
			idx = next_text_offset;
		} else {
			idx = eolpos;
		}
		const int parse_result = nqiv_cmd_parse_arg_token(manager, current_node, tidx, idx, eolpos, tokens[tidx]);
		if(parse_result == -1) {
			error = true;
			break;
		} else {
			idx += parse_result;
		}
		++tidx;
	}
	if(error || nqiv_cmd_scan_not_whitespace(data, idx, eolpos, NULL) != -1) {
		nqiv_cmd_print_help(manager, current_node, false);
		if(manager->state->cmd_parse_error_quit) {
			nqiv_cmd_force_quit_main(manager);
			error = true;
		} else {
			error = false;
		}
	}
	return !error;
}

bool nqiv_cmd_execute_node(nqiv_cmd_manager* manager, const nqiv_cmd_node* current_node, const int idx, const int eolpos)
{
	nqiv_cmd_arg_token tokens[NQIV_CMD_MAX_ARGS] = {0};
	nqiv_cmd_arg_token* token_ptrs[NQIV_CMD_MAX_ARGS + 1] = {0};
	int count;
	for(count = 0; count < NQIV_CMD_MAX_ARGS; ++count) {
		token_ptrs[count] = &tokens[count];
	}
	if( !nqiv_cmd_parse_args(manager, current_node, idx, eolpos, token_ptrs) ) {
		if(manager->state->cmd_parse_error_quit) {
			return true;
		} else {
			return false;
		}
	}
	nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd storing value for %s (%s).\n", current_node->name, current_node->description);
	bool output = current_node->store_value(manager, token_ptrs);
	if(!output) {
		nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING, "Cmd error storing value for %s (%s).\n", current_node->name, current_node->description);
		if(manager->state->cmd_apply_error_quit) {
			nqiv_cmd_force_quit_main(manager);
			output = true;
		} else {
			output = false;
		}
	} else {
		output = false;
	}
	return output;
}

bool nqiv_cmd_parse_line(nqiv_cmd_manager* manager)
{
	memset( &manager->print_settings, 0, sizeof(nqiv_cmd_manager_print_settings) );
	char current_cmd[NQIV_CMD_DUMPCFG_BUFFER_LENGTH] = {0};
	int current_cmd_position = 0;
	bool error = false;
	bool help = false;
	bool recurse_help = false;
	bool dumpcfg = false;
	char* data = manager->buffer->data;
	int idx = nqiv_cmd_scan_not_whitespace_and_eol(data, 0, manager->buffer->position, NULL);
	if(idx == -1) {
		nqiv_array_remove_bytes(manager->buffer, 0, manager->buffer->position);
		return true; /* The entire string must be whitespace- nothing to do. */
	}
	const int eolpos = nqiv_cmd_scan_eol(data, idx, manager->buffer->position, NULL);
	if(eolpos == -1) {
		return true; /* We don't have an EOL yet. Nothing to do. */
	}
	if(data[idx] == '#') {
		const char eolc = nqiv_cmd_tmpterm(data, eolpos);
		nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd skipping input %s\n", data + idx);
		nqiv_cmd_tmpret(data, eolpos, eolc);
		nqiv_array_remove_bytes(manager->buffer, 0, eolpos);
		return true; /* This line is a comment- ignore it. */
	}
	const char eolc = nqiv_cmd_tmpterm(data, eolpos);
	nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd parsing input %s\n", data + idx);
	nqiv_cmd_tmpret(data, eolpos, eolc);
	if(strncmp( &data[idx], "helptree", strlen("helptree") ) == 0) {
		idx += strlen("helptree");
		help = true;
		recurse_help = true;
	}
	if(strncmp( &data[idx], "help", strlen("help") ) == 0) {
		idx += strlen("help");
		help = true;
	}
	if(strncmp( &data[idx], "dumpcfg", strlen("dumpcfg") ) == 0) {
		idx += strlen("dumpcfg");
		dumpcfg = true;
	}
	nqiv_cmd_node* current_node = &nqiv_parser_nodes_root;
	while(idx < eolpos) {
		const int next_text_offset = nqiv_cmd_scan_not_whitespace(data, idx, eolpos, NULL);
		if(next_text_offset != -1) {
			idx = next_text_offset;
		} else if(help || current_node != &nqiv_parser_nodes_root) {
			break;
		}
		bool found_node = false;
		int cidx = 0;
		while(current_node->children[cidx] != NULL) {
			const char tmp = nqiv_cmd_tmpterm(data, eolpos);
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd checking token %s child %s for input %s\n", current_node->name, current_node->children[cidx]->name, data + idx);
			nqiv_cmd_tmpret(data, eolpos, tmp);
			int data_end = nqiv_cmd_scan_whitespace(data, idx, eolpos, NULL);
			if(data_end == -1 || data_end > eolpos) {
				data_end = eolpos;
			}
			if(strncmp( &data[idx], current_node->children[cidx]->name, data_end - idx) == 0 && strlen(current_node->children[cidx]->name) == (size_t)(data_end - idx) ) {
				current_node = current_node->children[cidx];
				idx = data_end;
				found_node = true;
				memcpy( current_cmd + current_cmd_position, current_node->name, strlen(current_node->name) );
				current_cmd_position += strlen(current_node->name);
				current_cmd[current_cmd_position] = ' ';
				current_cmd_position += sizeof(char);
				break;
			}
			++cidx;
		}
		if(!found_node) {
			break;
		}
	}
	if(dumpcfg) {
		nqiv_cmd_dumpcfg(manager, current_node, true, current_cmd);
	} else if(help) {
		nqiv_cmd_print_help(manager, current_node, recurse_help);
	} else if(!error && current_node->store_value != NULL) {
		error = nqiv_cmd_execute_node(manager, current_node, idx, eolpos);
	} else {
		nqiv_cmd_print_help(manager, current_node, false);
		if(error && manager->state->cmd_parse_error_quit) {
			nqiv_cmd_force_quit_main(manager);
		} else {
			error = false;
		}
	}
	nqiv_array_remove_bytes(manager->buffer, 0, eolpos + 1);
	return !error;
}

bool nqiv_cmd_parse(nqiv_cmd_manager* manager)
{
	while(nqiv_cmd_scan_eol(manager->buffer->data, 0, manager->buffer->position, NULL) != -1) {
		if( !nqiv_cmd_parse_line(manager) ) {
			return false;
		}
	}
	return true;
}

bool nqiv_cmd_add_byte(nqiv_cmd_manager* manager, const char byte)
{
	if( !nqiv_array_make_room(manager->buffer, NQIV_CMD_ADD_BYTE_BUFFER_LENGTH) ) {
		return false;
	}
	char buf[NQIV_CMD_ADD_BYTE_BUFFER_LENGTH] = {byte};
	if( !nqiv_array_push_bytes(manager->buffer, buf, NQIV_CMD_ADD_BYTE_BUFFER_LENGTH) ) {
		nqiv_log_write( &manager->state->logger, NQIV_LOG_ERROR, "Failed to append byte %c to nqiv command parser of length %d.\n", byte, manager->buffer->data_length );
		nqiv_cmd_force_quit_main(manager);
		return false;
	}
	return true;
}

bool nqiv_cmd_add_string(nqiv_cmd_manager* manager, const char* str)
{
	int idx = 0;
	while(str[idx] != '\0') {
		if( !nqiv_cmd_add_byte(manager, str[idx]) ) {
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

bool nqiv_cmd_consume_stream(nqiv_cmd_manager* manager, FILE* stream)
{
	int c;
	while(true) {
		c = fgetc(stream);
		if(c == EOF) {
			break;
		}
		if( !nqiv_cmd_add_byte(manager, (char)c) ) {
			return false;
		}
		if(c == '\r' || c == '\n') {
			if( !nqiv_cmd_parse(manager) ) {
				return false;
			}
		}
	}
	if( ferror(stream) ) {
		nqiv_log_write( &manager->state->logger, NQIV_LOG_ERROR, "Error reading config stream.\n");
		return false;
	}
	return true;
}

void nqiv_cmd_manager_destroy(nqiv_cmd_manager* manager)
{
	if(manager->buffer != NULL) {
		nqiv_array_destroy(manager->buffer);
	}
	memset( manager, 0, sizeof(nqiv_cmd_manager) );
}

bool nqiv_cmd_manager_init(nqiv_cmd_manager* manager, nqiv_state* state)
{
	nqiv_cmd_manager_destroy(manager);
	manager->buffer = nqiv_array_create(NQIV_CMD_READ_BUFFER_LENGTH);
	if(manager->buffer == NULL) {
		nqiv_log_write(&manager->state->logger, NQIV_LOG_ERROR, "Failed to allocate memory to create cmd buffer of length %d\n", state->queue_length);
		return false;
	}
	manager->state = state;
	manager->state->cmd_parse_error_quit = true;
	manager->state->cmd_apply_error_quit = true;
	nqiv_log_write(&manager->state->logger, NQIV_LOG_INFO, "Initialized cmd manager.\n");
	return true;
}

bool nqiv_cmd_consume_stream_from_path(nqiv_cmd_manager* manager, const char* path)
{
	nqiv_log_write(&manager->state->logger, NQIV_LOG_INFO, "Reading commands from %s\n", path);
	FILE* stream = fopen(path, "r");
	if(stream == NULL) {
		nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING, "Failed to read commands from %s\n", path);
		return false;
	}
	const bool output = nqiv_cmd_consume_stream(manager, stream);
	fclose(stream);
	return output;
}
