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

void nqiv_cmd_alert_main(nqiv_cmd_manager* manager)
{
	SDL_Event e = {0};
	e.type = SDL_USEREVENT;
	e.user.code = (Sint32)manager->state->cfg_event_number;
	if(SDL_PushEvent(&e) < 0) {
		nqiv_log_write(&manager->state->logger, NQIV_LOG_ERROR,
		               "Failed to send SDL event from thread %d. SDL Error: %s\n",
		               omp_get_thread_num(), SDL_GetError());
	}
}

void nqiv_cmd_force_quit_main(nqiv_cmd_manager* manager)
{
	nqiv_key_action action = NQIV_KEY_ACTION_QUIT;
	nqiv_queue_push_force(&manager->state->key_actions, &action);
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

bool nqiv_cmd_parser_set_none(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	(void)manager;
	(void)tokens;
	return true;
}

bool nqiv_cmd_parser_set_thread_count(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->thread_count = tokens[0]->value.as_int;
	return true;
}

bool nqiv_cmd_parser_set_thread_event_interval(nqiv_cmd_manager*    manager,
                                               nqiv_cmd_arg_token** tokens)
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

bool nqiv_cmd_parser_set_zoom_down_amount_more(nqiv_cmd_manager*    manager,
                                               nqiv_cmd_arg_token** tokens)
{
	manager->state->images.zoom.pan_down_amount_more = tokens[0]->value.as_double;
	return true;
}

bool nqiv_cmd_parser_set_zoom_in_amount_more(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->images.zoom.zoom_in_amount_more = tokens[0]->value.as_double;
	return true;
}

bool nqiv_cmd_parser_set_zoom_left_amount_more(nqiv_cmd_manager*    manager,
                                               nqiv_cmd_arg_token** tokens)
{
	manager->state->images.zoom.pan_left_amount_more = tokens[0]->value.as_double;
	return true;
}

bool nqiv_cmd_parser_set_zoom_out_amount_more(nqiv_cmd_manager*    manager,
                                              nqiv_cmd_arg_token** tokens)
{
	manager->state->images.zoom.zoom_out_amount_more = tokens[0]->value.as_double;
	return true;
}

bool nqiv_cmd_parser_set_zoom_right_amount_more(nqiv_cmd_manager*    manager,
                                                nqiv_cmd_arg_token** tokens)
{
	manager->state->images.zoom.pan_right_amount_more = tokens[0]->value.as_double;
	return true;
}

bool nqiv_cmd_parser_set_zoom_up_amount_more(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->images.zoom.pan_up_amount_more = tokens[0]->value.as_double;
	return true;
}

bool nqiv_cmd_parser_set_zoom_up_coordinate_x_times(nqiv_cmd_manager*    manager,
                                                    nqiv_cmd_arg_token** tokens)
{
	manager->state->images.zoom.pan_coordinate_x_multiplier = tokens[0]->value.as_double;
	return true;
}

bool nqiv_cmd_parser_set_zoom_up_coordinate_y_times(nqiv_cmd_manager*    manager,
                                                    nqiv_cmd_arg_token** tokens)
{
	manager->state->images.zoom.pan_coordinate_y_multiplier = tokens[0]->value.as_double;
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

bool nqiv_cmd_parser_set_default_frame_time(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->images.default_frame_time = tokens[0]->value.as_int;
	return true;
}

bool nqiv_cmd_parser_set_thumbnail_zoom_amount(nqiv_cmd_manager*    manager,
                                               nqiv_cmd_arg_token** tokens)
{
	manager->state->images.zoom.thumbnail_adjust = tokens[0]->value.as_int;
	return true;
}

bool nqiv_cmd_parser_set_thumbnail_zoom_amount_more(nqiv_cmd_manager*    manager,
                                                    nqiv_cmd_arg_token** tokens)
{
	manager->state->images.zoom.thumbnail_adjust_more = tokens[0]->value.as_int;
	return true;
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
	const char data_end = nqiv_cmd_tmpterm(tokens[0]->raw, tokens[0]->length);
	manager->state->logger.level = tokens[0]->value.as_log_level;
	nqiv_cmd_tmpret(tokens[0]->raw, tokens[0]->length, data_end);
	return true;
}

bool nqiv_cmd_parser_set_log_prefix(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	const char data_end = nqiv_cmd_tmpterm(tokens[0]->raw, tokens[0]->length);
	nqiv_log_set_prefix_format(&manager->state->logger, tokens[0]->raw);
	nqiv_cmd_tmpret(tokens[0]->raw, tokens[0]->length, data_end);
	return true;
}

bool nqiv_cmd_parser_set_no_resample_oversized(nqiv_cmd_manager*    manager,
                                               nqiv_cmd_arg_token** tokens)
{
	manager->state->no_resample_oversized = tokens[0]->value.as_bool;
	return true;
}

bool nqiv_cmd_parser_set_show_loading_indicator(nqiv_cmd_manager*    manager,
                                                nqiv_cmd_arg_token** tokens)
{
	manager->state->show_loading_indicator = tokens[0]->value.as_bool;
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
		nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING, "Failed to apply color for %s.\n",
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

bool nqiv_cmd_parser_set_minimum_delay_default(nqiv_cmd_manager*    manager,
                                               nqiv_cmd_arg_token** tokens)
{
	manager->state->keystates.settings.minimum_delay = tokens[0]->value.as_Uint64;
	return true;
}

bool nqiv_cmd_parser_set_repeat_delay_default(nqiv_cmd_manager*    manager,
                                              nqiv_cmd_arg_token** tokens)
{
	manager->state->keystates.settings.consecutive_delay = tokens[0]->value.as_Uint64;
	return true;
}

bool nqiv_cmd_parser_set_send_on_down_default(nqiv_cmd_manager*    manager,
                                              nqiv_cmd_arg_token** tokens)
{
	manager->state->keystates.send_on_down = tokens[0]->value.as_bool;
	return true;
}

bool nqiv_cmd_parser_set_send_on_up_default(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->keystates.send_on_up = tokens[0]->value.as_bool;
	return true;
}

bool nqiv_cmd_parser_set_start_delay_default(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->keystates.settings.start_delay = tokens[0]->value.as_Uint64;
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
	return nqiv_queue_push(&manager->state->key_actions, &tokens[0]->value.as_key_action);
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
	fprintf(stdout, "%" PRIu64, manager->state->prune_delay);
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

void nqiv_cmd_parser_print_zoom_up_coordinate_x_times(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%f", manager->state->images.zoom.pan_coordinate_x_multiplier);
}

void nqiv_cmd_parser_print_zoom_up_coordinate_y_times(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%f", manager->state->images.zoom.pan_coordinate_y_multiplier);
}

void nqiv_cmd_parser_print_zoom_default(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%s", nqiv_zoom_default_names[manager->state->zoom_default]);
}

void nqiv_cmd_parser_print_zoom_scale_mode(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%s", nqiv_scale_mode_to_text(manager->state->texture_scale_mode));
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

void nqiv_cmd_parser_print_default_frame_time(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%d", manager->state->images.default_frame_time);
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

void nqiv_cmd_parser_print_show_loading_indicator(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%s", manager->state->show_loading_indicator ? "true" : "false");
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
	fprintf(stdout, "%s", nqiv_log_level_names[manager->state->logger.level / 10]);
}

void nqiv_cmd_parser_print_log_prefix(nqiv_cmd_manager* manager)
{
	if(strlen(manager->state->logger.prefix_format) == 0 && !manager->print_settings.dumpcfg) {
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

void nqiv_cmd_parser_print_delay_accel_default(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%" PRIu64, manager->state->keystates.settings.delay_accel);
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
	nqiv_cmd_print_str_list(manager, manager->state->logger_stream_names);
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

void nqiv_cmd_parser_print_minimum_delay_default(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%" PRIu64, manager->state->keystates.settings.minimum_delay);
}

void nqiv_cmd_parser_print_repeat_delay_default(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%" PRIu64, manager->state->keystates.settings.consecutive_delay);
}

void nqiv_cmd_parser_print_start_delay_default(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%" PRIu64, manager->state->keystates.settings.start_delay);
}

const char* nqiv_press_action_names[] = {
	"default",
	"allow",
	"deny",
};

void nqiv_cmd_parser_print_send_on_down_default(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%s", manager->state->keystates.send_on_down ? "true" : "false");
}

void nqiv_cmd_parser_print_send_on_up_default(nqiv_cmd_manager* manager)
{
	fprintf(stdout, "%s", manager->state->keystates.send_on_up ? "true" : "false");
}

nqiv_cmd_arg_desc nqiv_parser_arg_type_int_natural = {
	.type = NQIV_CMD_ARG_INT,
	.setting = {.of_int = {.min = 0, .max = INT_MAX}},
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_int_positive = {
	.type = NQIV_CMD_ARG_INT,
	.setting = {.of_int = {.min = 1, .max = INT_MAX}},
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_Uint64 = {
	.type = NQIV_CMD_ARG_UINT64,
	.setting = {.of_Uint64 = {.min = (Uint64)0, .max = (Uint64)INT_MAX}},
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_Uint8 = {
	.type = NQIV_CMD_ARG_UINT8,
	.setting = {{0}},
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_double_positive = {
	.type = NQIV_CMD_ARG_DOUBLE,
	.setting = {.of_double = {.min = NQIV_CMD_ARG_FLOAT_MIN, .max = NQIV_CMD_ARG_FLOAT_MAX}},
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_double_negative = {
	.type = NQIV_CMD_ARG_DOUBLE,
	.setting = {.of_double = {.min = -NQIV_CMD_ARG_FLOAT_MAX, .max = -NQIV_CMD_ARG_FLOAT_MIN}},
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_double_negative_one = {
	.type = NQIV_CMD_ARG_DOUBLE,
	.setting = {.of_double = {.min = -1.0, .max = -NQIV_CMD_ARG_FLOAT_MIN}},
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_double_positive_one = {
	.type = NQIV_CMD_ARG_DOUBLE,
	.setting = {.of_double = {.min = NQIV_CMD_ARG_FLOAT_MIN, .max = 1.0}},
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_double = {
	.type = NQIV_CMD_ARG_DOUBLE,
	.setting = {.of_double = {.min = -NQIV_CMD_ARG_FLOAT_MAX, .max = NQIV_CMD_ARG_FLOAT_MAX}},
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_bool = {
	.type = NQIV_CMD_ARG_BOOL,
	.setting = {{0}},
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_log_level = {
	.type = NQIV_CMD_ARG_LOG_LEVEL,
	.setting = {{0}},
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_press_action = {
	.type = NQIV_CMD_ARG_PRESS_ACTION,
	.setting = {{0}},
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_key_action = {
	.type = NQIV_CMD_ARG_KEY_ACTION,
	.setting = {{0}},
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_key_action_brief = {
	.type = NQIV_CMD_ARG_KEY_ACTION,
	.setting = {.of_key_action = {.brief = true}},
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_keybind = {
	.type = NQIV_CMD_ARG_KEYBIND,
	.setting = {{0}},
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_string = {
	.type = NQIV_CMD_ARG_STRING,
	.setting = {.of_string = {.spaceless = true}},
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_string_full = {
	.type = NQIV_CMD_ARG_STRING,
	.setting = {.of_string = {.spaceless = false}},
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_pruner = {
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
						*length = strlen(subs[sidx]);
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
		fprintf(stdout, "'lgui', 'rgui', or 'gui' for either.\n");
		print_prefix(manager);
		fprintf(stdout, "num\n");
		print_prefix(manager);
		fprintf(stdout, "caps\n");
		print_prefix(manager);
		fprintf(stdout, "mode\n");
		print_prefix(manager);
		fprintf(stdout, "scroll\n");
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
		break;
	case NQIV_CMD_ARG_STRING:
		fprintf(stdout, "STRING(%s)",
		        arg->setting.of_string.spaceless ? "spaceless" : "spaces allowed");
		break;
	case NQIV_CMD_ARG_PRUNER:
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
		fprintf(stdout, "'raw' will cause the following checks to consider raw data only.\n");
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
		break;
	}
}

void nqiv_cmd_print_args(nqiv_cmd_manager*               manager,
                         const nqiv_cmd_arg_desc* const* args,
                         void (*print_prefix)(const nqiv_cmd_manager*))
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
	if(current_node->print_value != NULL || current_node->store_value != NULL) {
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
			char*     end = NULL;
			const int tmp = strtol(data, &end, 10);
			if(errno != ERANGE && end != NULL && tmp >= desc->setting.of_int.min
			   && tmp <= desc->setting.of_int.max) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG,
				               "Cmd int arg at %d for token %s is %d for input %s\n", tidx,
				               current_node->name, tmp, data);
				token->value.as_int = tmp;
				output = end - data;
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
			if(errno != ERANGE && end != NULL && tmp >= desc->setting.of_double.min
			   && tmp <= desc->setting.of_double.max) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG,
				               "Cmd double at %d for token %s is %f for input %s\n", tidx,
				               current_node->name, tmp, data);
				token->value.as_double = tmp;
				output = end - data;
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
			char*     end = NULL;
			const int tmp = strtol(data, &end, 10);
			if(errno != ERANGE && end != NULL && (Uint64)tmp >= desc->setting.of_Uint64.min
			   && (Uint64)tmp <= desc->setting.of_Uint64.max) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG,
				               "Cmd uint64 arg at %d for token %s is %d for input %s\n", tidx,
				               current_node->name, tmp, data);
				token->value.as_Uint64 = (Uint64)tmp;
				output = end - data;
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
			char*     end = NULL;
			const int tmp = strtol(data, &end, 10);
			if(errno != ERANGE && end != NULL && tmp >= 0 && tmp <= 255) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG,
				               "Cmd uint8 arg at %d for token %s is %d for input %s\n", tidx,
				               current_node->name, tmp, data);
				token->value.as_Uint8 = (Uint8)tmp;
				output = end - data;
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
				output = strlen(nqiv_log_level_names[tmp / 10]);
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
				output = strlen(nqiv_press_action_names[tmp]);
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
				output = strlen(nqiv_keybind_action_names[tmp]);
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
		} else {
			idx += parse_result;
		}
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
	const char eolc = nqiv_cmd_tmpterm(data, eolpos);
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
				break;
			}
			child = child->peer;
		}
		if(!found_node) {
			break;
		}
	}
	if(dumpcfg) {
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

bool nqiv_cmd_consume_stream(nqiv_cmd_manager* manager, FILE* stream)
{
	while(true) {
		const int c = fgetc(stream);
		if(c == EOF) {
			break;
		}
		if(!nqiv_cmd_add_byte(manager, (char)c)) {
			return false;
		}
		if(c == '\r' || c == '\n') {
			if(!nqiv_cmd_parse(manager)) {
				return false;
			}
		}
	}
	if(ferror(stream)) {
		nqiv_log_write(&manager->state->logger, NQIV_LOG_ERROR, "Error reading config stream.\n");
		return false;
	}
	return true;
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
int nqiv_cmd_get_args_list_length(nqiv_cmd_arg_desc** args)
{
	assert(args != NULL);
	int count;
	for(count = 0; args[count] != NULL; ++count) {}
	assert(count > 0);
	return count + 1;
}

int nqiv_cmd_get_args_length(const nqiv_cmd_node* node)
{
	assert(node != NULL);
	return nqiv_cmd_get_args_list_length(node->args);
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
	memset(node->name, 0, strlen(node->name));
	free(node->name);
	memset(node->description, 0, strlen(node->description));
	free(node->description);
	if(node->args != NULL) {
		memset(node->args, 0, nqiv_cmd_get_args_length(node));
		free(node->args);
	}
	if(node->child != NULL) {
		nqiv_cmd_destroy_node(node->child);
	}
	if(node->peer != NULL) {
		nqiv_cmd_destroy_node(node->peer);
	}
	memset(node, 0, sizeof(nqiv_cmd_node));
	free(node);
}

nqiv_cmd_node* nqiv_cmd_make_base_node(bool* status, const char* name, const char* description)
{
	nqiv_cmd_node* node = (nqiv_cmd_node*)calloc(1, sizeof(nqiv_cmd_node));
	if(node == NULL) {
		*status = *status && false;
		return NULL;
	}
	node->name = (char*)calloc(strlen(name) + 1, sizeof(char));
	if(node->name == NULL) {
		free(node);
		*status = *status && false;
		return NULL;
	}
	node->description = (char*)calloc(strlen(description) + 1, sizeof(char));
	if(node->description == NULL) {
		free(node->name);
		free(node);
		*status = *status && false;
		return NULL;
	}
	memcpy(node->name, name, strlen(name));
	memcpy(node->description, description, strlen(description));
	*status = *status && true;
	return node;
}

nqiv_cmd_node* nqiv_cmd_make_leaf_node(bool*       status,
                                       const char* name,
                                       const char* description,
                                       bool (*store_value)(nqiv_cmd_manager*, nqiv_cmd_arg_token**),
                                       void (*print_value)(nqiv_cmd_manager*),
                                       nqiv_cmd_arg_desc** args)
{
	assert(store_value != NULL);
	assert(args != NULL);
	nqiv_cmd_node* node = nqiv_cmd_make_base_node(status, name, description);
	if(node == NULL) {
		*status = *status && false;
		return NULL;
	}
	assert(store_value != NULL || print_value != NULL);
	node->store_value = store_value;
	node->print_value = print_value;
	const int arg_count = nqiv_cmd_get_args_list_length(args);
	node->args = (nqiv_cmd_arg_desc**)calloc(arg_count, sizeof(nqiv_cmd_arg_desc*));
	if(node->args == NULL) {
		nqiv_cmd_destroy_node(node);
		*status = *status && false;
		return NULL;
	}
	memcpy(node->args, args, arg_count * sizeof(nqiv_cmd_arg_desc*));
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
	nqiv_cmd_node* node = nqiv_cmd_make_base_node(status, name, description);
	nqiv_cmd_add_child_or_peer(parent, node);
	return node;
}

nqiv_cmd_node* nqiv_cmd_add_child_leaf_node(bool*          status,
                                            nqiv_cmd_node* parent,
                                            const char*    name,
                                            const char*    description,
                                            bool (*store_value)(nqiv_cmd_manager*,
                                                                nqiv_cmd_arg_token**),
                                            void (*print_value)(nqiv_cmd_manager*),
                                            nqiv_cmd_arg_desc** args)
{
	if(parent == NULL || !*status) {
		return NULL;
	}
	nqiv_cmd_node* node =
		nqiv_cmd_make_leaf_node(status, name, description, store_value, print_value, args);
	nqiv_cmd_add_child_or_peer(parent, node);
	return node;
}

#define STACKLEN 16
#define SET_CURRENT                                                         \
	assert(nqiv_array_get_units_count(&stack) > 0);                         \
	assert(nqiv_array_get_units_count(&stack) < STACKLEN);                  \
	current_node = NULL;                                                    \
	nqiv_array_get(&stack, nqiv_array_get_last_idx(&stack), &current_node); \
	assert(current_node != NULL);
#define DEPRECATE                \
	assert(deprecated == false); \
	deprecated = true;
#define APPLY_DEPRECATE                \
	tmp_node->deprecated = deprecated; \
	deprecated = false;
#define B(NAME, DESCRIPTION)                                                                 \
	SET_CURRENT;                                                                             \
	assert(tmp_node == NULL);                                                                \
	tmp_node = nqiv_cmd_add_child_branch_node(&status, current_node, (NAME), (DESCRIPTION)); \
	APPLY_DEPRECATE;                                                                         \
	nqiv_array_push(&stack, &tmp_node);                                                      \
	tmp_node = NULL;
#define L(NAME, DESCRIPTION, STORE_VALUE, PRINT_VALUE, ARGS)                              \
	SET_CURRENT;                                                                          \
	assert(nqiv_cmd_get_args_list_length(ARGS) < NQIV_CMD_MAX_ARGS);                      \
	assert(tmp_node == NULL);                                                             \
	tmp_node = nqiv_cmd_add_child_leaf_node(&status, current_node, (NAME), (DESCRIPTION), \
	                                        (STORE_VALUE), (PRINT_VALUE), (ARGS));        \
	APPLY_DEPRECATE;                                                                      \
	tmp_node = NULL;
#define POP                                         \
	assert(nqiv_array_get_units_count(&stack) > 0); \
	nqiv_array_pop(&stack, NULL);                   \
	SET_CURRENT;
bool nqiv_cmd_manager_build_cmdtree(nqiv_cmd_manager* manager)
{
	nqiv_cmd_arg_desc* sendkey_args[] = {&nqiv_parser_arg_type_key_action, NULL};
	nqiv_cmd_arg_desc* idxname_args[] = {&nqiv_parser_arg_type_int_natural,
	                                     &nqiv_parser_arg_type_string_full, NULL};
	nqiv_cmd_arg_desc* natural_args[] = {&nqiv_parser_arg_type_int_natural, NULL};
	nqiv_cmd_arg_desc* positive_args[] = {&nqiv_parser_arg_type_int_positive, NULL};
	nqiv_cmd_arg_desc* uint64_args[] = {&nqiv_parser_arg_type_Uint64, NULL};
	nqiv_cmd_arg_desc* keyactionbrief_uint64_args[] = {&nqiv_parser_arg_type_key_action_brief,
	                                                   &nqiv_parser_arg_type_Uint64, NULL};
	nqiv_cmd_arg_desc* keyactionbrief_pressaction_args[] = {
		&nqiv_parser_arg_type_key_action_brief, &nqiv_parser_arg_type_press_action, NULL};
	nqiv_cmd_arg_desc* stringfull_args[] = {&nqiv_parser_arg_type_string_full, NULL};
	nqiv_cmd_arg_desc* string_args[] = {&nqiv_parser_arg_type_string, NULL};
	nqiv_cmd_arg_desc* pruner_args[] = {&nqiv_parser_arg_type_pruner, NULL};
	nqiv_cmd_arg_desc* keybind_args[] = {&nqiv_parser_arg_type_keybind, NULL};
	nqiv_cmd_arg_desc* loglevel_args[] = {&nqiv_parser_arg_type_log_level, NULL};
	nqiv_cmd_arg_desc* doublepositiveone_args[] = {&nqiv_parser_arg_type_double_positive_one, NULL};
	nqiv_cmd_arg_desc* doublenegativeone_args[] = {&nqiv_parser_arg_type_double_negative_one, NULL};
	nqiv_cmd_arg_desc* doublepositive_args[] = {&nqiv_parser_arg_type_double_positive, NULL};
	nqiv_cmd_arg_desc* doublenegative_args[] = {&nqiv_parser_arg_type_double_negative, NULL};
	nqiv_cmd_arg_desc* double_args[] = {&nqiv_parser_arg_type_double, NULL};
	nqiv_cmd_arg_desc* intpositive_args[] = {&nqiv_parser_arg_type_int_positive, NULL};
	nqiv_cmd_arg_desc* bool_args[] = {&nqiv_parser_arg_type_bool, NULL};
	nqiv_cmd_arg_desc* color_args[] = {&nqiv_parser_arg_type_Uint8, &nqiv_parser_arg_type_Uint8,
	                                   &nqiv_parser_arg_type_Uint8, &nqiv_parser_arg_type_Uint8,
	                                   NULL};
	nqiv_cmd_node*     stack_data[STACKLEN] = {0};
	nqiv_array         stack;
	nqiv_array_inherit(&stack, stack_data, sizeof(nqiv_cmd_node*), STACKLEN);
	bool           status = true;
	nqiv_cmd_node* root_node = nqiv_cmd_make_base_node(
		&status, "root",
		"Root of parsing tree. Prefix help to get help messages on commands, helptree to do the "
	    "same recursively, helpchildren to only recurse one level, or dumpcfg to dump functional "
	    "commands to set the current configuration. Lines can also be commented by prefixing with "
	    "#");
	nqiv_cmd_node* current_node;
	nqiv_cmd_node* tmp_node = NULL;
	bool           deprecated = false;
	nqiv_array_push(&stack, &root_node);

	L("sendkey", "Issue a simulated keyboard action to the program.", nqiv_cmd_parser_sendkey, NULL,
	  sendkey_args);
	B("insert", "Add a value to a particular location.");
	{
		L("image", "Insert an image path to be opened at a particular index.",
		  nqiv_cmd_parser_insert_image, NULL, idxname_args);
	}
	POP;
	B("remove", "Remove a value from a particular location.");
	{
		B("image", "Remove an image from the list to be opened.");
		{
			L("index", "Delete the image from the given index.", nqiv_cmd_parser_remove_image_index,
			  NULL, natural_args);
		}
		POP;
	}
	POP;
	B("append", "Add a value to the end of an existing series.");
	{
		B("log", "Append operations related to logging.");
		{
			L("stream", "Log to the given stream.", nqiv_cmd_parser_append_log_stream,
			  nqiv_cmd_parser_print_log_stream, stringfull_args);
		}
		POP;
		L("image", "Add an image path to the be opened.", nqiv_cmd_parser_append_image, NULL,
		  stringfull_args);
		L("pruner",
		  "Declaratively specified pruning instructions. Use help to get list of commands.",
		  nqiv_cmd_parser_append_pruner, nqiv_cmd_parser_print_pruner, pruner_args);
		DEPRECATE L("extension",
		            "(DEPRECATED nqiv will not be handling this for now) Add an image extension to "
		            "be accepted.",
					nqiv_cmd_parser_set_none, nqiv_cmd_parser_print_none, string_args);
		L("keybind", "Add a keybind.", nqiv_cmd_parser_append_keybind,
		  nqiv_cmd_parser_print_keybind, keybind_args);
	}
	POP;
	B("set", "Alter a singular value.");
	{
		B("log", "Set operations related to logging.");
		{
			L("level", "Log messages this level or lower are printed.",
			  nqiv_cmd_parser_set_log_level, nqiv_cmd_parser_print_log_level, loglevel_args);
			L("prefix",
			  "Log message format. Special messages are delimited by #. ## produces a literal #. "
			  "#time:<strftime format># prints the time. #level# prints the log level.",
			  nqiv_cmd_parser_set_log_prefix, nqiv_cmd_parser_print_log_prefix, stringfull_args);
		}
		POP;
		B("thread", "Settings related to thread behavior.");
		{
			L("count",
			  "Set the number of worker threads used by the software. Starts as the number of "
			  "threads on the machine divided by three (or one). This does not count toward VIPs "
			  "threads. See 'set vips threads' for that.",
			  nqiv_cmd_parser_set_thread_count, nqiv_cmd_parser_print_thread_count, positive_args);
			L("event_interval",
			  "Threads will update the master after processing this many events. 0 to process all.",
			  nqiv_cmd_parser_set_thread_event_interval,
			  nqiv_cmd_parser_print_thread_event_interval, natural_args);
			L("prune_delay",
			  "A pruning cycle will be allowed to run after this many milliseconds since the last "
			  "one. 0 always allow prune cycles.",
			  nqiv_cmd_parser_set_prune_delay, nqiv_cmd_parser_print_prune_delay, uint64_args);
			L("extra_wakeup_delay",
			  "In addition to an internal algorithm, wait this long to wait to let the master "
			  "thread lock a worker. Longer times might produce longer loading delays, but help "
			  "improve UI responsiveness.",
			  nqiv_cmd_parser_set_extra_wakeup_delay, nqiv_cmd_parser_print_extra_wakeup_delay,
			  natural_args);
		}
		POP;
		B("vips", "Settings related to the VIPS library.");
		{
			L("threads",
			  "Set the number of threads used by the VIPs library. The default is the number of "
			  "available threads divided by two (or one). If set to 0, it is determined by the "
			  "environment variable VIPS_CONCURRENCY, or if unset, the number of threads available "
			  "on the machine.",
			  nqiv_cmd_parser_set_vips_threads, nqiv_cmd_parser_print_vips_threads, natural_args);
		}
		POP;
		B("zoom", "Set operations related to zooming.");
		{
			L("left_amount", "Amount to pan the zoom left with each action.",
			  nqiv_cmd_parser_set_zoom_left_amount, nqiv_cmd_parser_print_zoom_left_amount,
			  doublenegativeone_args);
			L("right_amount", "Amount to pan the zoom right with each action",
			  nqiv_cmd_parser_set_zoom_right_amount, nqiv_cmd_parser_print_zoom_right_amount,
			  doublepositiveone_args);
			L("down_amount", "Amount to pan the zoom down with each action",
			  nqiv_cmd_parser_set_zoom_down_amount, nqiv_cmd_parser_print_zoom_down_amount,
			  doublepositiveone_args);
			L("up_amount", "Amount to pan the zoom up with each action",
			  nqiv_cmd_parser_set_zoom_up_amount, nqiv_cmd_parser_print_zoom_up_amount,
			  doublenegativeone_args);
			L("out_amount", "Amount to pan the zoom out with each action",
			  nqiv_cmd_parser_set_zoom_out_amount, nqiv_cmd_parser_print_zoom_out_amount,
			  doublepositive_args);
			L("in_amount", "Amount to pan the zoom in with each action",
			  nqiv_cmd_parser_set_zoom_in_amount, nqiv_cmd_parser_print_zoom_in_amount,
			  doublenegative_args);
			L("left_amount_more", "Higher amount to pan the zoom left with each action.",
			  nqiv_cmd_parser_set_zoom_left_amount_more,
			  nqiv_cmd_parser_print_zoom_left_amount_more, doublenegativeone_args);
			L("right_amount_more", "Higher amount to pan the zoom right with each action",
			  nqiv_cmd_parser_set_zoom_right_amount_more,
			  nqiv_cmd_parser_print_zoom_right_amount_more, doublepositiveone_args);
			L("down_amount_more", "Higher amount to pan the zoom down with each action",
			  nqiv_cmd_parser_set_zoom_down_amount_more,
			  nqiv_cmd_parser_print_zoom_down_amount_more, doublepositiveone_args);
			L("up_amount_more", "Higher amount to pan the zoom up with each action",
			  nqiv_cmd_parser_set_zoom_up_amount_more, nqiv_cmd_parser_print_zoom_up_amount_more,
			  doublenegativeone_args);
			L("out_amount_more", "Higher amount to pan the zoom out with each action",
			  nqiv_cmd_parser_set_zoom_out_amount_more, nqiv_cmd_parser_print_zoom_out_amount_more,
			  doublepositive_args);
			L("in_amount_more", "Higher amount to pan the zoom in with each action",
			  nqiv_cmd_parser_set_zoom_in_amount_more, nqiv_cmd_parser_print_zoom_in_amount_more,
			  doublenegative_args);
			L("up_coordinate_x_times",
			  "This is multiplied against x axis panning caused by relative motion (like mouse "
			  "panning)",
			  nqiv_cmd_parser_set_zoom_up_coordinate_x_times,
			  nqiv_cmd_parser_print_zoom_up_coordinate_x_times, double_args);
			L("up_coordinate_y_times",
			  "This is multiplied against y axis panning caused by relative motion (like mouse "
			  "panning)",
			  nqiv_cmd_parser_set_zoom_up_coordinate_y_times,
			  nqiv_cmd_parser_print_zoom_up_coordinate_y_times, double_args);
			L("default",
			  "Default zoom setting when loading an image- 'keep' old zoom, 'fit' to display, or "
			  "set 'actual_size'.",
			  nqiv_cmd_parser_set_zoom_default, nqiv_cmd_parser_print_zoom_default, string_args);
			L("scale_mode",
			  "Set scale mode used for SDL textures. Options are: 'nearest', 'linear', and 'best' "
			  "or 'anisotropic'.",
			  nqiv_cmd_parser_set_zoom_scale_mode, nqiv_cmd_parser_print_zoom_scale_mode,
			  string_args);
		}
		POP;
		B("thumbnail", "Set operations related to thumbnails.");
		{
			L("path", "Path thumbnails are stored under. This directory must exist.",
			  nqiv_cmd_parser_set_thumbnail_path, nqiv_cmd_parser_print_thumbnail_path,
			  stringfull_args);
			L("size_adjust",
			  "Number of pixels to resize thumbnails by with 'zoom' action in montage mode.",
			  nqiv_cmd_parser_set_thumbnail_zoom_amount,
			  nqiv_cmd_parser_print_thumbnail_zoom_amount, intpositive_args);
			L("size_adjust_more",
			  "Higher number of pixels to resize thumbnails by with 'zoom' action in montage mode.",
			  nqiv_cmd_parser_set_thumbnail_zoom_amount_more,
			  nqiv_cmd_parser_print_thumbnail_zoom_amount_more, intpositive_args);
			L("load", "Whether to read thumbnails from the disk.",
			  nqiv_cmd_parser_set_thumbnail_load, nqiv_cmd_parser_print_thumbnail_load, bool_args);
			L("save",
			  "Whether to save thumbnails to the disk. Note that if thumbnail_load is not set to "
			  "true, then thumbnails will always be saved, even if they are up to date on the "
			  "disk.",
			  nqiv_cmd_parser_set_thumbnail_save, nqiv_cmd_parser_print_thumbnail_save, bool_args);
			L("size", "Width and height of thumbnails are the same value.",
			  nqiv_cmd_parser_set_thumbnail_size, nqiv_cmd_parser_print_thumbnail_size,
			  intpositive_args);
		}
		POP;
		L("default_frame_time", "If an animated image does not provide a frame time, use this.",
		  nqiv_cmd_parser_set_default_frame_time, nqiv_cmd_parser_print_default_frame_time,
		  intpositive_args);
		B("keypress", "Settings for delaying and registering keypresses.");
		{
			DEPRECATE B(
				"action",
				"(DEPRECATED: These settings are essentially part of the keybind implementation, "
			    "now. Using these settings now will have no effect, nor will they print any "
			    "information. This may cause some breakage of old key actions.) Key action "
			    "specific settings for delaying and registering keypresses.");
			{
				L("start_delay", "Before a key is registered, it must be pressed for this long.",
				  nqiv_cmd_parser_set_none, nqiv_cmd_parser_print_none, keyactionbrief_uint64_args);
				L("repeat_delay", "This is the starting delay for repeating a key.",
				  nqiv_cmd_parser_set_none, nqiv_cmd_parser_print_none, keyactionbrief_uint64_args);
				L("delay_accel",
				  "The repeat delay will be reduced by this amount for each repetition.",
				  nqiv_cmd_parser_set_none, nqiv_cmd_parser_print_none, keyactionbrief_uint64_args);
				L("minimum_delay", "The delay will never be less than this.",
				  nqiv_cmd_parser_set_none, nqiv_cmd_parser_print_none, keyactionbrief_uint64_args);
				L("send_on_up", "Register releasing of the key.", nqiv_cmd_parser_set_none,
				  nqiv_cmd_parser_print_none, keyactionbrief_pressaction_args);
				L("send_on_down", "Register pressing of the key.", nqiv_cmd_parser_set_none,
				  nqiv_cmd_parser_print_none, keyactionbrief_pressaction_args);
			}
			POP;
			B("default", "Default settings for delaying and registering keypresses.");
			{
				L("start_delay", "Before a key is registered, it must be pressed for this long.",
				  nqiv_cmd_parser_set_start_delay_default,
				  nqiv_cmd_parser_print_start_delay_default, uint64_args);
				L("repeat_delay", "This is the starting delay for repeating a key.",
				  nqiv_cmd_parser_set_repeat_delay_default,
				  nqiv_cmd_parser_print_repeat_delay_default, uint64_args);
				L("delay_accel",
				  "The repeat delay will be reduced by this amount for each repetition.",
				  nqiv_cmd_parser_set_delay_accel_default,
				  nqiv_cmd_parser_print_delay_accel_default, uint64_args);
				L("minimum_delay", "The delay will never be less than this.",
				  nqiv_cmd_parser_set_minimum_delay_default,
				  nqiv_cmd_parser_print_minimum_delay_default, uint64_args);
				L("send_on_up", "Register releasing of the key.",
				  nqiv_cmd_parser_set_send_on_up_default, nqiv_cmd_parser_print_send_on_up_default,
				  bool_args);
				L("send_on_down", "Register pressing of the key.",
				  nqiv_cmd_parser_set_send_on_down_default,
				  nqiv_cmd_parser_print_send_on_down_default, bool_args);
			}
			POP;
			/*L("", "", nqiv_cmd_parser_set_, nqiv_cmd_parser_print_, _args);*/
		}
		POP;
		B("color", "Set operations related to color.");
		{
			L("background", "Color of background.", nqiv_cmd_parser_set_background_color,
			  nqiv_cmd_parser_print_background_color, color_args);
			L("error", "Color of image area when there's an error loading.",
			  nqiv_cmd_parser_set_error_color, nqiv_cmd_parser_print_error_color, color_args);
			L("loading", "Color of image area when image is still loading.",
			  nqiv_cmd_parser_set_loading_color, nqiv_cmd_parser_print_loading_color, color_args);
			L("selection", "Color of box around selected image.",
			  nqiv_cmd_parser_set_selection_color, nqiv_cmd_parser_print_selection_color,
			  color_args);
			L("mark", "Color of dashed box around marked image.", nqiv_cmd_parser_set_mark_color,
			  nqiv_cmd_parser_print_mark_color, color_args);
			L("alpha_background_one",
			  "The background of a transparent image is rendered as checkers. This is the first "
			  "color.",
			  nqiv_cmd_parser_set_alpha_background_color_one,
			  nqiv_cmd_parser_print_alpha_background_color_one, color_args);
			L("alpha_background_two",
			  "The background of a transparent image is rendered as checkers. This is the second "
			  "color.",
			  nqiv_cmd_parser_set_alpha_background_color_two,
			  nqiv_cmd_parser_print_alpha_background_color_two, color_args);
		}
		POP;
		B("preload", "Set options related to preloading images not yet in view.");
		{
			L("ahead", "This number of images ahead of the current montage are loaded.",
			  nqiv_cmd_parser_set_preload_ahead, nqiv_cmd_parser_print_preload_ahead, natural_args);
			L("behind", "This number of images behind of the current montage are loaded.",
			  nqiv_cmd_parser_set_preload_behind, nqiv_cmd_parser_print_preload_behind,
			  natural_args);
		}
		POP;
		L("no_resample_oversized",
		  "Normally, if the image is larger than 16k by 16k pixels, it will be reloaded for each "
		  "zoom. This keeps the normal behavior with the entire image downsized.",
		  nqiv_cmd_parser_set_no_resample_oversized, nqiv_cmd_parser_print_no_resample_oversized,
		  bool_args);
		B("show", "Settings related to displaying optional entities.");
		{
			L("loading_indicator",
			  "Determine whether the loading indicator is rendered in image mode (achieve the same "
			  "in montage mode by setting `set color loading` to match `set color background`).",
			  nqiv_cmd_parser_set_show_loading_indicator,
			  nqiv_cmd_parser_print_show_loading_indicator, bool_args);
		}
		POP;
		DEPRECATE L(
			"queue_size",
			"(DEPRECATED: Sizes can still be printed, but not set. For now, this is handled by an "
		    "internal algorithm.) Dynamic arrays in the software are backed by a given amount of "
		    "memory. They will expand as needed, but it may improve performance to allocate more "
		    "memory in advance. This value is the default minimum.",
			nqiv_cmd_parser_set_none, nqiv_cmd_parser_print_queue_size, intpositive_args);
		B("window", "Set operations related to the window.");
		{
			L("width", "Set the width of the program window.", nqiv_cmd_parser_set_window_width,
			  nqiv_cmd_parser_print_window_width, intpositive_args);
			L("height", "Set the height of the program window.", nqiv_cmd_parser_set_window_height,
			  nqiv_cmd_parser_print_window_height, intpositive_args);
		}
		POP;
		B("cmd", "Set operations related to the commands.");
		{
			L("parse_error_quit", "Quit if there are errors parsing commands.",
			  nqiv_cmd_parser_set_parse_error_quit, nqiv_cmd_parser_print_parse_error_quit,
			  bool_args);
			L("apply_error_quit", "Quit if there are errors applying correctly-parsed commands.",
			  nqiv_cmd_parser_set_apply_error_quit, nqiv_cmd_parser_print_apply_error_quit,
			  bool_args);
		}
		POP;
	}
	POP;

	if(status) {
		manager->root_node = root_node;
	}
	return status;
}
#undef POP
#undef B
#undef L
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
	manager->buffer = nqiv_array_create(sizeof(char), NQIV_CMD_READ_BUFFER_LENGTH);
	if(manager->buffer == NULL) {
		nqiv_log_write(&manager->state->logger, NQIV_LOG_ERROR,
		               "Failed to allocate memory to create cmd buffer of length %d\n",
		               NQIV_CMD_READ_BUFFER_LENGTH);
		return false;
	}
	manager->buffer->max_data_length = NQIV_CMD_READ_BUFFER_LENGTH_MAX;
	manager->buffer->min_add_count = NQIV_CMD_READ_BUFFER_LENGTH;
	if(!nqiv_cmd_manager_build_cmdtree(manager)) {
		nqiv_cmd_manager_destroy(manager);
		nqiv_log_write(&manager->state->logger, NQIV_LOG_ERROR,
		               "Failed to build cmd parsing tree.\n");
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
		nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING,
		               "Failed to read commands from %s\n", path);
		return false;
	}
	const bool output = nqiv_cmd_consume_stream(manager, stream);
	fclose(stream);
	return output;
}
