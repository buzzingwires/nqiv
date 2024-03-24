#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <float.h>

#include "logging.h"
#include "array.h"
#include "state.h"
#include "keybinds.h"
#include "keyrate.h"

void nqiv_cmd_force_quit_main(nqiv_cmd_manager* manager)
{
	const nqiv_key_action action = NQIV_KEY_ACTION_QUIT;
	nqiv_queue_push_force(&manager->state->key_actions, sizeof(nqiv_key_action), action);
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

bool nqiv_cmd_parser_set_thumbnail_path(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	const char data_end = nqiv_cmd_tmpterm(tokens[0]->raw, tokens[0]->length);
	const bool output = nqiv_image_manager_set_thumbnail_root(&manager->state->images, tokens[0]->raw);
	nqiv_cmd_tmpret(tokens[0]->raw, tokens[0]->length, data_end);
	return true;
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
	manager->state->cmd_parse_error_quit; = tokens[0]->value.as_bool;
	return true;
}

bool nqiv_cmd_parser_set_apply_error_quit(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->cmd_apply_error_quit; = tokens[0]->value.as_bool;
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

bool nqiv_cmd_parser_set_window_height(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	int w;
	int h;
	SDL_GetWindowSizeInPixels(manager->state->window, &w, &h);
	SDL_SetWindowSize(manager->state->window, &w, tokens[0]->value.as_int);
	return true;
}

bool nqiv_cmd_parser_set_window_width(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	int w;
	int h;
	SDL_GetWindowSizeInPixels(manager->state->window, &w, &h);
	SDL_SetWindowSize(manager->state->window, tokens[0]->value.as_int, &h);
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
	manager->state->keystates.settings.send_on_down = tokens[0]->value.as_bool;
	return true;
}

bool nqiv_cmd_parser_set_send_on_up(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->keystates.states[tokens[0]->value.as_key_action].send_on_up = tokens[1]->value.as_press_action;
	return true;
}

bool nqiv_cmd_parser_set_send_on_up_default(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	manager->state->keystates.settings.send_on_up = tokens[0]->value.as_bool;
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
	const bool output = nqiv_image_manager_append(&manager->state->images, tokens[0]->raw);
	nqiv_cmd_tmpret(tokens[0]->raw, tokens[0]->length, data_end);
	return output;
}

bool nqiv_cmd_parser_append_keybind(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	return nqiv_keybind_add(&manager->state->keybinds, tokens[0]->value.as_keybind.key, tokens[0]->value.as_keybind.action);
}

bool nqiv_cmd_parser_append_log_stream(nqiv_cmd_manager* manager, nqiv_cmd_arg_token** tokens)
{
	const char data_end = nqiv_cmd_tmpterm(tokens[0]->raw, tokens[0]->length);
	const bool output = nqiv_add_logger_path(&manager->state->logger, tokens[0]->raw);
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
	return nqiv_queue_push(&manager->state->key_actions, sizeof(nqiv_key_action), tokens[0]->value.as_key_action);
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

void nqiv_cmd_print_indent(const int indent)
{
	int indent_count;
	for(indent_count = indent; indent_count > 0; --indent_count) {
		fprintf(stdout, "\t");
	}
}

void nqiv_cmd_parser_print_zoom_down_amount(nqiv_cmd_manager* manager, const int indent)
{
	fprintf(stdout, "%f", manager->state->images.zoom.pan_down_amount);
}

void nqiv_cmd_parser_print_zoom_in_amount(nqiv_cmd_manager* manager, const int indent)
{
	fprintf(stdout, "%f", manager->state->images.zoom.zoom_in_amount);
}

void nqiv_cmd_parser_print_zoom_left_amount(nqiv_cmd_manager* manager, const int indent)
{
	fprintf(stdout, "%f", manager->state->images.zoom.pan_left_amount);
}

void nqiv_cmd_parser_print_zoom_out_amount(nqiv_cmd_manager* manager, const int indent)
{
	fprintf(stdout, "%f", manager->state->images.zoom.zoom_out_amount);
}

void nqiv_cmd_parser_print_zoom_right_amount(nqiv_cmd_manager* manager, const int indent)
{
	fprintf(stdout, "%f", manager->state->images.zoom.pan_right_amount);
}

void nqiv_cmd_parser_print_zoom_up_amount(nqiv_cmd_manager* manager, const int indent)
{
	fprintf(stdout, "%f", manager->state->images.zoom.pan_up_amount);
}

void nqiv_cmd_parser_print_thumbnail_load(nqiv_cmd_manager* manager, const int indent)
{
	fprintf(stdout, "%s", manager->state->images.thumbnail.load ? "true" : "false");
}

void nqiv_cmd_parser_print_thumbnail_save(nqiv_cmd_manager* manager, const int indent)
{
	fprintf(stdout, "%s", manager->state->images.thumbnail.save ? "true" : "false");
}

void nqiv_cmd_parser_print_thumbnail_size(nqiv_cmd_manager* manager, const int indent)
{
	fprintf(stdout, "%d", manager->state->images.thumbnail.size);
}

void nqiv_cmd_parser_print_thumbnail_zoom_amount(nqiv_cmd_manager* manager, const int indent)
{
	fprintf(stdout, "%d", manager->state->images.zoom.thumbnail_adjust);
}

void nqiv_cmd_parser_print_thumbnail_path(nqiv_cmd_manager* manager, const int indent)
{
	fprintf(stdout, "%s", manager->state->images.thumbnail.root);
}

void nqiv_cmd_parser_print_no_resample_oversized(nqiv_cmd_manager* manager, const int indent)
{
	fprintf(stdout, "%s", manager->state->no_resample_oversized ? "true" : "false");
}

void nqiv_cmd_parser_print_queue_size(nqiv_cmd_manager* manager, const int indent)
{
	fprintf(stdout, "\n");
	fprintf(stdout, "%d\n", manager->state->queue_length);
	nqiv_cmd_print_indent(indent);
	fprintf(stdout, "Keybinds: %d\n", state->keybinds.lookup->data_length);
	nqiv_cmd_print_indent(indent);
	fprintf(stdout, "Images: %d\n", state->images.images->data_length);
	nqiv_cmd_print_indent(indent);
	fprintf(stdout, "Image extensions: %d\n", state->images.extensions->data_length);
	nqiv_cmd_print_indent(indent);
	fprintf(stdout, "Thread Queue: %d\n", state->thread_queue.array->data_length);
	nqiv_cmd_print_indent(indent);
	fprintf(stdout, "Key Actions: %d\n", state->key_actions.array->data_length);
	nqiv_cmd_print_indent(indent);
	fprintf(stdout, "Cmd Buffer: %d\n", state->buffer->data_length);
}

void nqiv_cmd_parser_print_log_level(nqiv_cmd_manager* manager, const int indent)
{
	fprintf(stdout, "%s", nqiv_log_level_names[manager->state->logger.level / 10]);
}

void nqiv_cmd_parser_print_log_prefix(nqiv_cmd_manager* manager, const int indent)
{
	fprintf(stdout, "%s", manager->state->logger.prefix_format);
}

void nqiv_cmd_parser_print_parse_error_quit(nqiv_cmd_manager* manager, const int indent)
{
	fprintf(stdout, "%s", manager->state->cmd_parse_error_quit ? "true" : "false");
}

void nqiv_cmd_parser_print_apply_error_quit(nqiv_cmd_manager* manager, const int indent)
{
	fprintf(stdout, "%s", manager->state->cmd_parse_error_quit ? "true" : "false");
}

void nqiv_cmd_parser_print_alpha_background_color_one(nqiv_cmd_manager* manager, const int indent)
{
	fprintf(stdout, "%hhu %hhu %hhu %hhu", manager->state->alpha_checker_color_one.r, manager->state->alpha_checker_color_one.g, manager->state->alpha_checker_color_one.b, manager->state->alpha_checker_color_one.a);
}

void nqiv_cmd_parser_print_alpha_background_color_two(nqiv_cmd_manager* manager, const int indent)
{
	fprintf(stdout, "%hhu %hhu %hhu %hhu", manager->state->alpha_checker_color_two.r, manager->state->alpha_checker_color_two.g, manager->state->alpha_checker_color_two.b, manager->state->alpha_checker_color_two.a);
}

void nqiv_cmd_parser_print_background_color(nqiv_cmd_manager* manager, const int indent)
{
	fprintf(stdout, "%hhu %hhu %hhu %hhu", manager->state->background_color.r, manager->state->background_color.g, manager->state->background_color.b, manager->state->background_color.a);
}

void nqiv_cmd_parser_print_error_color(nqiv_cmd_manager* manager, const int indent)
{
	fprintf(stdout, "%hhu %hhu %hhu %hhu", manager->state->error_color.r, manager->state->error_color.g, manager->state->error_color.b, manager->state->error_color.a);
}

void nqiv_cmd_parser_print_loading_color(nqiv_cmd_manager* manager, const int indent)
{
	fprintf(stdout, "%hhu %hhu %hhu %hhu", manager->state->loading_color.r, manager->state->loading_color.g, manager->state->loading_color.b, manager->state->loading_color.a);
}

void nqiv_cmd_parser_print_selection_color(nqiv_cmd_manager* manager, const int indent)
{
	fprintf(stdout, "%hhu %hhu %hhu %hhu", manager->state->selection_color.r, manager->state->selection_color.g, manager->state->selection_color.b, manager->state->selection_color.a);
}

void nqiv_cmd_parser_print_window_height(nqiv_cmd_manager* manager, const int indent)
{
	int w;
	int h;
	SDL_GetWindowSizeInPixels(manager->state->window, &w, &h);
	fprintf(stdout, "%d", h);
}

void nqiv_cmd_parser_print_window_width(nqiv_cmd_manager* manager, const int indent)
{
	int w;
	int h;
	SDL_GetWindowSizeInPixels(manager->state->window, &w, &h);
	fprintf(stdout, "%d", w);
}

void nqiv_cmd_parser_print_delay_accel(nqiv_cmd_manager* manager, const int indent)
{
	fprintf(stdout, "\n");
	nqiv_key_action action;
	for(action = NQIV_KEY_ACTION_QUIT; action <= NQIV_KEY_ACTION_MAX; ++action) {
		nqiv_cmd_print_indent(indent);
		fprintf(stdout, "%s %llu\n", nqiv_keybind_action_names[action], manager->state->keystates.states[action].settings.delay_accel);
	}
}

void nqiv_cmd_parser_print_delay_accel_default(nqiv_cmd_manager* manager, const int indent)
{
	fprintf(stdout, "%llu", manager->state->keystates.settings.delay_accel);
}

void nqiv_cmd_parser_print_minimum_delay(nqiv_cmd_manager* manager, const int indent)
{
	fprintf(stdout, "\n");
	nqiv_key_action action;
	for(action = NQIV_KEY_ACTION_QUIT; action <= NQIV_KEY_ACTION_MAX; ++action) {
		nqiv_cmd_print_indent(indent);
		fprintf(stdout, "%s %llu\n", nqiv_keybind_action_names[action], manager->state->keystates.states[action].settings.minimum_delay);
	}
}

void nqiv_cmd_parser_print_minimum_delay_default(nqiv_cmd_manager* manager, const int indent)
{
	fprintf(stdout, "%llu", manager->state->keystates.settings.minimum_delay);
}

void nqiv_cmd_parser_print_consecutive_delay(nqiv_cmd_manager* manager, const int indent)
{
	fprintf(stdout, "\n");
	nqiv_key_action action;
	for(action = NQIV_KEY_ACTION_QUIT; action <= NQIV_KEY_ACTION_MAX; ++action) {
		nqiv_cmd_print_indent(indent);
		fprintf(stdout, "%s %llu\n", nqiv_keybind_action_names[action], manager->state->keystates.states[action].settings.consecutive_delay);
	}
}

void nqiv_cmd_parser_print_consecutive_delay_default(nqiv_cmd_manager* manager, const int indent)
{
	fprintf(stdout, "%llu", manager->state->keystates.settings.consecutive_delay);
}

void nqiv_cmd_parser_print_start_delay(nqiv_cmd_manager* manager, const int indent)
{
	fprintf(stdout, "\n");
	nqiv_key_action action;
	for(action = NQIV_KEY_ACTION_QUIT; action <= NQIV_KEY_ACTION_MAX; ++action) {
		nqiv_cmd_print_indent(indent);
		fprintf(stdout, "%s %llu\n", nqiv_keybind_action_names[action], manager->state->keystates.states[action].settings.start_delay);
	}
}

void nqiv_cmd_parser_print_start_delay_default(nqiv_cmd_manager* manager, const int indent)
{
	fprintf(stdout, "%llu", manager->state->keystates.settings.start_delay);
}

const char* nqiv_press_action_names[] =
{
	"default",
	"allow",
	"deny"
};

void nqiv_cmd_parser_print_send_on_down(nqiv_cmd_manager* manager, const int indent)
{
	fprintf(stdout, "\n");
	nqiv_key_action action;
	for(action = NQIV_KEY_ACTION_QUIT; action <= NQIV_KEY_ACTION_MAX; ++action) {
		nqiv_cmd_print_indent(indent);
		fprintf(stdout, "%s %s\n", nqiv_keybind_action_names[action], nqiv_press_action_names[manager->state->keystates.states[action].settings.send_on_down]);
	}
}

void nqiv_cmd_parser_print_send_on_down_default(nqiv_cmd_manager* manager, const int indent)
{
	fprintf(stdout, "%s", manager->state->keystates.send_on_down ? "true" : "false");
}

void nqiv_cmd_parser_print_send_on_up(nqiv_cmd_manager* manager, const int indent)
{
	fprintf(stdout, "\n");
	nqiv_key_action action;
	for(action = NQIV_KEY_ACTION_QUIT; action <= NQIV_KEY_ACTION_MAX; ++action) {
		nqiv_cmd_print_indent(indent);
		fprintf(stdout, "%s %s\n", nqiv_keybind_action_names[action], nqiv_press_action_names[manager->state->keystates.states[action].settings.send_on_up]);
	}
}

void nqiv_cmd_parser_print_send_on_up_default(nqiv_cmd_manager* manager, const int indent)
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
	.setting = {0},
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
	.setting = {0},
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_log_level = {
	.type = NQIV_CMD_ARG_LOG_LEVEL,
	.setting = {0},
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_press_action = {
	.type = NQIV_CMD_ARG_PRESS_ACTION,
	.setting = {0},
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_key_action = {
	.type = NQIV_CMD_ARG_KEY_ACTION,
	.setting = {0},
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_key_action_brief = {
	.type = NQIV_CMD_ARG_KEY_ACTION,
	.setting = { .of_key_action = {.brief = true} },
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_keybind = {
	.type = NQIV_CMD_ARG_KEYBIND,
	.setting = {0},
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_string = {
	.type = NQIV_CMD_ARG_STRING,
	.setting = { .of_string = {.spaceless = true} },
};

nqiv_cmd_arg_desc nqiv_parser_arg_type_string_full = {
	.type = NQIV_CMD_ARG_STRING,
	.setting = { .of_string = {.spaceless = false} },
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
			.args = {nqiv_parser_arg_type_key_action, NULL},
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
					.args = {nqiv_parser_arg_type_int_natural, nqiv_parser_arg_type_string_full, NULL},
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
							.args = {nqiv_parser_arg_type_int_natural, NULL},
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
							.print_value = NULL,
							.args = {nqiv_parser_arg_type_string_full, NULL},
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
					.args = {nqiv_parser_arg_type_string_full, NULL},
					.children = {NULL},
				},
				&(nqiv_cmd_node)
				{
					.name = "extension",
					.description = "Add an image extension to be accepted.",
					.store_value = nqiv_cmd_parser_append_extension,
					.print_value = NULL,
					.args = {nqiv_parser_arg_type_string, NULL},
					.children = {NULL},
				},
				&(nqiv_cmd_node)
				{
					.name = "keybind",
					.description = "Add a keybind.",
					.store_value = nqiv_cmd_parser_append_keybind,
					.print_value = NULL,
					.args = {nqiv_parser_arg_type_keybind, NULL},
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
							.args = {nqiv_parser_arg_type_log_level, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "prefix",
							.description = "Log message format. Special messages are delimited by #. ## produces a literal #. #time:<strftime format># prints the time. #level# prints the log level.",
							.store_value = nqiv_cmd_parser_set_log_prefix,
							.print_value = nqiv_cmd_parser_print_log_prefix,
							.args = {nqiv_parser_arg_type_string_full, NULL},
							.children = {NULL},
						},
						NULL,
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
							.args = {nqiv_parser_arg_type_double_negative_one, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "right_amount",
							.description = "Amount to pan the zoom right with each action",
							.store_value = nqiv_cmd_parser_set_zoom_right_amount,
							.print_value = nqiv_cmd_parser_print_zoom_right_amount,
							.args = {nqiv_parser_arg_type_double_positive_one, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "down_amount",
							.description = "Amount to pan the zoom down with each action",
							.store_value = nqiv_cmd_parser_set_zoom_down_amount,
							.print_value = nqiv_cmd_parser_print_zoom_down_amount,
							.args = {nqiv_parser_arg_type_double_positive_one, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "up_amount",
							.description = "Amount to pan the zoom up with each action",
							.store_value = nqiv_cmd_parser_set_zoom_up_amount,
							.print_value = nqiv_cmd_parser_print_zoom_up_amount,
							.args = {nqiv_parser_arg_type_double_negative_one, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "out_amount",
							.description = "Amount to pan the zoom out with each action",
							.store_value = nqiv_cmd_parser_set_zoom_out_amount,
							.print_value = nqiv_cmd_parser_print_zoom_out_amount,
							.args = {nqiv_parser_arg_type_double_positive, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "in_amount",
							.description = "Amount to pan the zoom in with each action",
							.store_value = nqiv_cmd_parser_set_zoom_in_amount,
							.print_value = nqiv_cmd_parser_print_zoom_in_amount,
							.args = {nqiv_parser_arg_type_double_negative, NULL},
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
							.args = {nqiv_parser_arg_type_string_full, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "size_adjust",
							.description = "Number of pixels to resize thumbnails by with 'zoom' action in montage mode.",
							.store_value = nqiv_cmd_parser_set_thumbnail_zoom_amount,
							.print_value = nqiv_cmd_parser_print_thumbnail_zoom_amount,
							.args = {nqiv_parser_arg_type_int_positive, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "load",
							.description = "Whether to read thumbnails from the disk.",
							.store_value = nqiv_cmd_parser_set_thumbnail_load,
							.print_value = nqiv_cmd_parser_print_thumbnail_load,
							.args = {nqiv_parser_arg_type_bool, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "save",
							.description = "Whether to save thumbnails to the disk. Note that if thumbnail_load is not set to true, then thumbnails will always be saved, even if they are up to date on the disk.",
							.store_value = nqiv_cmd_parser_set_thumbnail_save,
							.print_value = nqiv_cmd_parser_print_thumbnail_save,
							.args = {nqiv_parser_arg_type_bool, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "size",
							.description = "Width and height of thumbnails are the same value.",
							.store_value = nqiv_cmd_parser_set_thumbnail_size,
							.print_value = nqiv_cmd_parser_print_thumbnail_size,
							.args = {nqiv_parser_arg_type_int_positive, NULL},
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
									.args = {nqiv_parser_arg_type_key_action_brief, nqiv_parser_arg_type_Uint64, NULL},
									.children = {NULL},
								},
								&(nqiv_cmd_node)
								{
									.name = "repeat_delay",
									.description = "This is the starting delay for repeating a key.",
									.store_value = nqiv_cmd_parser_set_repeat_delay,
									.print_value = nqiv_cmd_parser_print_repeat_delay,
									.args = {nqiv_parser_arg_type_key_action_brief, nqiv_parser_arg_type_Uint64, NULL},
									.children = {NULL},
								},
								&(nqiv_cmd_node)
								{
									.name = "delay_accel",
									.description = "The repeat delay will be reduced by this amount for each repetition.",
									.store_value = nqiv_cmd_parser_set_delay_accel,
									.print_value = nqiv_cmd_parser_print_delay_accel,
									.args = {nqiv_parser_arg_type_key_action_brief, nqiv_parser_arg_type_Uint64, NULL},
									.children = {NULL},
								},
								&(nqiv_cmd_node)
								{
									.name = "minimum_delay",
									.description = "The delay will never be less than this.",
									.store_value = nqiv_cmd_parser_set_minimum_delay,
									.print_value = nqiv_cmd_parser_print_minimum_delay,
									.args = {nqiv_parser_arg_type_key_action_brief, nqiv_parser_arg_type_Uint64, NULL},
									.children = {NULL},
								},
								&(nqiv_cmd_node)
								{
									.name = "send_on_up",
									.description = "Register releasing of the key.",
									.store_value = nqiv_cmd_parser_set_send_on_up,
									.print_value = nqiv_cmd_parser_print_send_on_up,
									.args = {nqiv_parser_arg_type_key_action_brief, nqiv_parser_arg_type_action, NULL},
									.children = {NULL},
								},
								&(nqiv_cmd_node)
								{
									.name = "send_on_down",
									.description = "Register pressing of the key.",
									.store_value = nqiv_cmd_parser_set_send_on_down,
									.print_value = nqiv_cmd_parser_print_send_on_down,
									.args = {nqiv_parser_arg_type_key_action_brief, nqiv_parser_arg_type_press_action, NULL},
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
									.args = {nqiv_parser_arg_type_Uint64, NULL},
									.children = {NULL},
								},
								&(nqiv_cmd_node)
								{
									.name = "repeat_delay",
									.description = "This is the starting delay for repeating a key.",
									.store_value = nqiv_cmd_parser_set_repeat_delay_default,
									.print_value = nqiv_cmd_parser_print_repeat_delay_default,
									.args = {nqiv_parser_arg_type_Uint64, NULL},
									.children = {NULL},
								},
								&(nqiv_cmd_node)
								{
									.name = "delay_accel",
									.description = "The repeat delay will be reduced by this amount for each repetition.",
									.store_value = nqiv_cmd_parser_set_delay_accel_default,
									.print_value = nqiv_cmd_parser_print_delay_accel_default,
									.args = {nqiv_parser_arg_type_Uint64, NULL},
									.children = {NULL},
								},
								&(nqiv_cmd_node)
								{
									.name = "minimum_delay",
									.description = "The delay will never be less than this.",
									.store_value = nqiv_cmd_parser_set_minimum_delay_default,
									.print_value = nqiv_cmd_parser_print_minimum_delay_default,
									.args = {nqiv_parser_arg_type_Uint64, NULL},
									.children = {NULL},
								},
								&(nqiv_cmd_node)
								{
									.name = "send_on_up",
									.description = "Register releasing of the key.",
									.store_value = nqiv_cmd_parser_set_send_on_up_default,
									.print_value = nqiv_cmd_parser_print_send_on_up_default,
									.args = {nqiv_parser_arg_type_bool, NULL},
									.children = {NULL},
								},
								&(nqiv_cmd_node)
								{
									.name = "send_on_down",
									.description = "Register pressing of the key.",
									.store_value = nqiv_cmd_parser_set_send_on_down_default,
									.print_value = nqiv_cmd_parser_print_send_on_down_default,
									.args = {nqiv_parser_arg_type_bool, NULL},
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
							.args = {nqiv_parser_arg_type_Uint8, nqiv_parser_arg_type_Uint8, nqiv_parser_arg_type_Uint8, nqiv_parser_arg_type_Uint8, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "error",
							.description = "Color of image area when there's an error loading.",
							.store_value = nqiv_cmd_parser_set_error_color,
							.print_value = nqiv_cmd_parser_print_error_color,
							.args = {nqiv_parser_arg_type_Uint8, nqiv_parser_arg_type_Uint8, nqiv_parser_arg_type_Uint8, nqiv_parser_arg_type_Uint8, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "loading",
							.description = "Color of image area when image is still loading.",
							.store_value = nqiv_cmd_parser_set_loading_color,
							.print_value = nqiv_cmd_parser_print_loading_color,
							.args = {nqiv_parser_arg_type_Uint8, nqiv_parser_arg_type_Uint8, nqiv_parser_arg_type_Uint8, nqiv_parser_arg_type_Uint8, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "selection",
							.description = "Color of box around selected image.",
							.store_value = nqiv_cmd_parser_set_selection_color,
							.print_value = nqiv_cmd_parser_print_selection_color,
							.args = {nqiv_parser_arg_type_Uint8, nqiv_parser_arg_type_Uint8, nqiv_parser_arg_type_Uint8, nqiv_parser_arg_type_Uint8, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "alpha_background_one",
							.description = "The background of a transparent image is rendered as checkers. This is the first color.",
							.store_value = nqiv_cmd_parser_set_alpha_background_color_one,
							.print_value = nqiv_cmd_parser_print_alpha_background_color_one,
							.args = {nqiv_parser_arg_type_Uint8, nqiv_parser_arg_type_Uint8, nqiv_parser_arg_type_Uint8, nqiv_parser_arg_type_Uint8, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "alpha_background_two",
							.description = "The background of a transparent image is rendered as checkers. This is the second color.",
							.store_value = nqiv_cmd_parser_set_alpha_background_color_two,
							.print_value = nqiv_cmd_parser_print_alpha_background_color_two,
							.args = {nqiv_parser_arg_type_Uint8, nqiv_parser_arg_type_Uint8, nqiv_parser_arg_type_Uint8, nqiv_parser_arg_type_Uint8, NULL},
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
					.args = {nqiv_parser_arg_type_bool, NULL},
					.children = {NULL},
				},
				&(nqiv_cmd_node)
				{
					.name = "queue_size",
					.description = "Dynamic arrays in the software are backed by a given amount of memory. They will expand as needed, but it may improve performance to allocate memory all at once in advance.",
					.store_value = nqiv_cmd_parser_set_queue_size,
					.print_value = nqiv_cmd_parser_print_queue_size,
					.args = {nqiv_parser_arg_type_int_positive, NULL},
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
							.args = {nqiv_parser_arg_type_int_positive, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "height",
							.description = "Set the height of the program window",
							.store_value = nqiv_cmd_parser_set_window_height,
							.print_value = nqiv_cmd_parser_print_window_height,
							.args = {nqiv_parser_arg_type_int_positive, NULL},
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
							.args = {nqiv_parser_arg_type_bool, NULL},
							.children = {NULL},
						},
						&(nqiv_cmd_node)
						{
							.name = "apply_error_quit",
							.description = "Quit if there are errors applying correctly-parsed commands.",
							.store_value = nqiv_cmd_parser_set_apply_error_quit,
							.print_value = nqiv_cmd_parser_print_apply_error_quit,
							.args = {nqiv_parser_arg_type_bool, NULL},
							.children = {NULL},
						},
						NULL,
					}
				},
				NULL,
			},
			NULL
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
	for(bidx = start; bidx < end; ++bidx) {
		int sidx;
		while(subs[sidx] != NULL) {
			if( (strcmp(subs[sidx], &data[bidx]) != 0) == negated ) {
				offset = bidx;
				if(length != NULL) {
					*length = strlen(subs[sidx]);
				}
				goto end;
			}
		}
	}
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

void nqiv_cmd_print_args(nqiv_cmd_manager* manager, const nqiv_cmd_arg_desc** args, const int indent)
{
	int idx = 0;
	while(args[idx] != NULL) {
		const nqiv_cmd_arg_desc* arg = args[idx];
		fprintf(stdout, " ");
		switch(arg->type) {
		case NQIV_CMD_ARG_INT:
			fprintf(stdout, "INT(%d-%d)", arg->setting.of_int.min, arg->setting.of_int.max);
			break;
		case NQIV_CMD_ARG_DOUBLE:
			fprintf(stdout, "DOUBLE(%f-%f)", arg->setting.of_double.min, arg->setting.of_double.max);
			break;
		case NQIV_CMD_ARG_UINT64:
			fprintf(stdout, "UINT64(%llu-%llu)", arg->setting.of_Uint64.min, arg->setting.of_Uint64.max);
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
				for(level = NQIV_LOG_LEVEL_ANY; level <= NQIV_LOG_ERROR; level += 10) {
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
				for(action = NQIV_KEYRATE_START; level <= NQIV_KEYRATE_END; ++action) {
					fprintf(stdout, "%s", nqiv_press_action_names[action]);
					if(level != NQIV_KEYRATE_END) {
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
					nqiv_cmd_print_indent(indent);
					fprintf(stdout, "%s\n", nqiv_keybind_action_names[action]);
				}
			}
			break;
		case NQIV_CMD_ARG_KEYBIND:
			fprintf(stdout, "<SDL_key_sequence>=<key_action>")
			break;
		case NQIV_CMD_ARG_STRING:
			fprintf(stdout, "STRING(%s)", arg->setting.of_string.spaceless ? "spaceless" : "spaces allowed");
			break;
		}
		++idx;
	}
}

void nqiv_cmd_dumpcfg(nqiv_cmd_manager* manager, const nqiv_cmd_node* current_node, const bool recurse, const int indent, const char* current_cmd)
{
	char new_cmd[NQIV_CMD_DUMPCFG_BUFFER_LENGTH] = {0};
	int new_position = strlen(current_cmd);
	memcpy(new_cmd, current_cmd, new_position);
	memcpy( new_cmd + new_position, current_node->name, strlen(current_node->name) );
	new_position += strlen(current_node->name);
	new_cmd[new_position] = ' ';
	new_position += sizeof(char);
	if(current_node->print_value != NULL || current_node->set_value != NULL) {
		nqiv_cmd_print_indent(indent);
		fprintf(stdout, "#%s\n", current_node->description);
		nqiv_cmd_print_indent(indent);
		if(current_node->print_value != NULL) {
			fprintf(stdout, "%s", new_cmd);
			current_node->print_value(manager, indent);
			fprintf(stdout, "\n");
		} else {
			fprintf(stdout, "#%s\n", new_cmd);
		}
	}
	if(!recurse) {
		return;
	}
	int cidx = 0;
	while(current_node->children[cidx] != NULL) {
		nqiv_cmd_dumpcfg(manager, current_node->children[cidx], recurse, indent + 1, new_cmd);
		++cidx;
	}
}

void nqiv_cmd_print_help(nqiv_cmd_manager* manager, const nqiv_cmd_node* current_node, const bool recurse, const int indent)
{
	nqiv_cmd_print_indent(indent);
	fprintf(stdout, "%s: %s - ", current_node->name, current_node->description);
	if(current_node->print_value != NULL) {
		current_node->print_value(manager, indent);
	}
	nqiv_cmd_print_args(manager, current_node->args, indent)
	fprintf(stdout, "\n");
	if(!recurse) {
		return;
	}
	int cidx = 0;
	while(current_node->children[cidx] != NULL) {
		nqiv_cmd_print_help(manager, current_node->children[cidx], recurse, indent + 1);
		++cidx;
	}
}

void nqiv_cmd_alert_main(nqiv_cmd_manager* manager)
{
	SDL_Event e = {0};
	e.type = SDL_USEREVENT;
	e.user.code = (Sint32)manager->state->cfg_event_number;
	if(SDL_PushEvent(&tell_finished) < 0) {
		nqiv_log_write( &state->logger, NQIV_LOG_ERROR, "Failed to send SDL event from thread %d. SDL Error: %s\n", omp_get_thread_num(), SDL_GetError() );
		running = false;
	}
}

int nqiv_cmd_parse_arg_token(nqiv_cmd_manager* manager, const nqiv_cmd_node* current_node, const int tidx, const int start_idx, const int eolpos, nqiv_cmd_arg_token* token)
{
	char* mutdata = manager->buffer->data;
	const char data_end = nqiv_cmd_tmpterm(mutdata, eolpos);
	int output = -1;
	const char* data = mutdata + start_idx;
	const nqiv_cmd_arg_desc* desc = current_node->args[tidx];
	switch(desc->type){
	case NQIV_CMD_ARG_INT:
		{
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd checking int arg at %d for token %s for input %s.\n", tidx, current_node->name, data);
			const char** end = NULL;
			const int tmp = strtol(data, &end, 10);
			if(errno != ERANGE && end != NULL && tmp >= desc->setting.of_int.min && tmp <= desc->setting.of_int.max) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd int arg at %d for token %s is %d for input %s.\n", tidx, current_node->name, tmp, data);
				token->value.as_int = tmp;
				output = end - data;
			} else {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING, "Cmd error parsing int arg at %d for token %s with input %s.\n", tidx, current_node->name, data);
			}
		}
		break;
	case NQIV_CMD_ARG_DOUBLE:
		{
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd checking double arg at %d for token %s for input %s.\n", tidx, current_node->name, data);
			const char** end = NULL;
			const int tmp = strtod(data, &end);
			if(errno != ERANGE && end != NULL && tmp >= desc->setting.of_double.min && tmp <= desc->setting.of_double.max) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd double at %d for token %s is %f for input %s.\n", tidx, current_node->name, tmp, data);
				token->value.as_double = tmp;
				output = end - data;
			} else {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING, "Cmd error parsing double arg at %d for token %s with input %s.\n", tidx, current_node->name, data);
			}
		}
		break;
	case NQIV_CMD_ARG_UINT64:
		{
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd checking uint64 arg at %d for token %s for input %s.\n", tidx, current_node->name, data);
			const char** end = NULL;
			const int tmp = strtol(data, &end, 10);
			if(errno != ERANGE && end != NULL && (Uint64)tmp >= desc->setting.of_Uint64.min && (Uint64)tmp <= desc->setting.of_Uint64.max) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd uint64 arg at %d for token %s is %d for input %s.\n", tidx, current_node->name, tmp, data);
				token->value.as_Uint64 = (Uint64)tmp;
				output = end - data;
			} else {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING, "Cmd error parsing uint64 arg at %d for token %s with input %s.\n", tidx, current_node->name, data);
			}
		}
		break;
	case NQIV_CMD_ARG_UINT8:
		{
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd checking uint8 arg at %d for token %s for input %s.\n", tidx, current_node->name, data);
			const char** end = NULL;
			const int tmp = strtol(data, &end, 10);
			if(errno != ERANGE && end != NULL && tmp >= 0 && tmp <= 255) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd uint8 arg at %d for token %s is %d for input %s.\n", tidx, current_node->name, tmp, data);
				token->value.as_Uint8 = (Uint8)tmp;
				output = end - data;
			} else {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING, "Cmd error parsing uint8 arg at %d for token %s with input %s.\n", tidx, current_node->name, data);
			}
		}
		break;
	case NQIV_CMD_ARG_BOOL:
		{
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd checking bool arg at %d for token %s for input %s.\n", tidx, current_node->name, data);
			if(strcmp(data, "true") == 0) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd bool arg at %d for token %s is true for input %s.\n", tidx, current_node->name, data);
				token->value.as_bool = true;
				output = strlen("true");
			} else if(strcmp(data, "false") == 0) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd bool arg at %d for token %s is false for input %s.\n", tidx, current_node->name, data);
				token->value.as_bool = false;
				output = strlen("false");
			} else {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING, "Cmd error parsing bool arg at %d for token %s with input %s.\n", tidx, current_node->name, data);
			}
		}
		break;
	case NQIV_CMD_ARG_LOG_LEVEL:
		{
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd checking log level arg at %d for token %s for input %s.\n", tidx, current_node->name, data);
			const nqiv_log_level tmp = nqiv_log_level_from_string(data);
			if(tmp != LOG_LEVEL_UNKNOWN) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd log level arg at %d for token %s is %s for input %s.\n", tidx, current_node->name, nqiv_log_level_names[tmp / 10], data);
				token->value.as_log_level = tmp;
				output = strlen(nqiv_log_level_names[tmp / 10]);
			} else {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING, "Cmd error parsing log level arg at %d for token %s with input %s.\n", tidx, current_node->name, data);
			}
		}
		break;
	case NQIV_CMD_ARG_PRESS_ACTION:
		{
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd checking press action arg at %d for token %s for input %s.\n", tidx, current_node->name, data);
			nqiv_keyrate_press_action tmp;
			if( nqiv_keyrate_press_action_from_string(data, &tmp) ) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd press action arg at %d for token %s is %s for input %s.\n", tidx, current_node->name, nqiv_press_action_names[tmp], data);
				token->value.as_press_action = tmp;
				output = strlen(nqiv_press_action_names[tmp]);
			} else {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING, "Cmd error parsing press action arg at %d for token %s with input %s.\n", tidx, current_node->name, data);
			}
		}
		break;
	case NQIV_CMD_ARG_KEY_ACTION:
		{
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd checking key action arg at %d for token %s for input %s.\n", tidx, current_node->name, data);
			const nqiv_key_action tmp = nqiv_text_to_key_action(data, eolpos - start_idx);
			if(tmp != NQIV_KEY_ACTION_NONE) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd key action arg at %d for token %s is %s for input %s.\n", tidx, current_node->name, nqiv_keybind_action_names[tmp], data);
				token->value.as_key_action = tmp;
				output = strlen(nqiv_keybind_action_names[tmp]);
			} else {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING, "Cmd error parsing key action arg at %d for token %s with input %s.\n", tidx, current_node->name, data);
			}
		}
		break;
	case NQIV_CMD_ARG_KEYBIND:
		{
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd checking keybind arg at %d for token %s for input %s.\n", tidx, current_node->name, data);
			nqiv_keybind_pair tmp;
			const int length = nqiv_keybind_text_to_keybind(data, &tmp);
			if(length != -1) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd keybind arg at %d for token %s for input %s.\n", tidx, current_node->name, data);
				memcpy( &token->value.as_keybind, &tmp, sizeof(nqiv_keybind_pair) );
				output = length;
			} else {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING, "Cmd error parsing keybind arg at %d for token %s with input %s.\n", tidx, current_node->name, data);
			}
		}
		break;
	case NQIV_CMD_ARG_STRING:
		if(desc->setting.of_string.spaceless) {
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd checking spaceless arg at %d for token %s for input %s.\n", tidx, current_node->name, data);
			const length = nqiv_cmd_scan_whitespace(data, 0, eolpos - start_idx, NULL);
			if(length != -1) {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd spaceless string at %d for token %s for input %s.\n", tidx, current_node->name, data);
				output = length;
			} else {
				nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING, "Cmd error parsing spaceless string arg at %d for token %s with input %s.\n", tidx, current_node->name, data);
			}
		} else {
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd string at %d for token %s for input %s.\n", tidx, current_node->name, data);
			output = eolpos - start_idx;
		}
		break;
	}
	if(output != -1) {
		token->type = desc->type
		token->raw = data;
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
		} else if(idx != start_idx || idx >= eolpos) {
			error = true;
			break;
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
		nqiv_cmd_print_help(manager, current_node, false, 0);
		if(manager->state->parse_error_quit) {
			nqiv_cmd_force_quit_main(nqiv_cmd_manager* manager);
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
	if( !nqiv_cmd_parse_args(manager, current_node, start_idx, eolpos, token_ptrs) ) {
		if(manager->state->parse_error_quit) {
			return false
		} else {
			return true;
		}
	}
	nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd storing value for %s (%s).\n", current_node->name, current_node->description);
	const bool output = current_node->store_value(manager, token_ptrs);
	if(!output) {
		nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING, "Cmd error storing value for %s (%s).\n", current_node->name, current_node->description);
		if(manager->state->apply_error_quit) {
			nqiv_cmd_force_quit_main(nqiv_cmd_manager* manager);
		} else {
			output = true;
		}
	} else {
		nqiv_cmd_alert_main(nqiv_cmd_manager* manager);
	}
	return output;
}

bool nqiv_cmd_parse_line(nqiv_cmd_manager* manager)
{
	char current_cmd[NQIV_CMD_DUMPCFG_BUFFER_LENGTH];
	int current_cmd_position = 0;
	bool error = false;
	bool help = false;
	bool recurse_help = false;
	bool dumpcfg = false;
	char* data = manager->buffer->data;
	int idx = nqiv_cmd_scan_not_whitespace_and_eol(data, 0, manager->buffer->position, NULL);
	if(idx == -1) {
		return true; /* The entire string must be whitespace- nothing to do. */
	}
	const int eolpos = nqiv_cmd_scan_eol(data, idx, manager->buffer->position, NULL);
	if(eolpos == -1) {
		return true; /* We don't have an EOL yet. Nothing to do. */
	}
	if(data[idx] == '#') {
		const char eolc = nqiv_cmd_tmpterm(data, eolpos);
		nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd skipping input %s.\n", data + idx);
		nqiv_cmd_tmpret(data, eolpos, eolc);
		nqiv_array_remove_bytes(manager->buffer, 0, eolpos);
		return true; /* This line is a comment- ignore it. */
	}
	const char eolc = nqiv_cmd_tmpterm(data, eolpos);
	nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd parsing input %s.\n", data + idx);
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
		} else if(help || current_node != nqiv_parser_nodes_root) {
			const int tmp = data[eolpos];
			const char tmp = nqiv_cmd_tmpterm(data, eolpos);
			nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING, "Cmd parse error: expected whitespace for input %s.\n", data + idx);
			nqiv_cmd_tmpret(data, eolpos, tmp);
			error = true;
			break;
		}
		bool found_node = false;
		int cidx = 0;
		while(current_node->children[cidx] != NULL) {
			const char tmp = nqiv_cmd_tmpterm(data, eolpos);
			nqiv_log_write(&manager->state->logger, NQIV_LOG_DEBUG, "Cmd checking token %s child %s for input %s.\n", current_node->name, current_node->children[cidx]->name, data + idx);
			nqiv_cmd_tmpret(data, eolpos, tmp);
			if(strncmp( &data[idx], current_node->children[cidx]->name, strlen(current_node->children[cidx]->name) ) == 0) {
				current_node = current_node->children[cidx];
				idx += strlen(current_node->name);
				found_node = true;
				memcpy( current_cmd + current_cmd_position, current_node->name, strlen(current_node->name) );
				current_cmd_position += strlen(current_node->name);
				new_cmd[new_position] = ' ';
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
		nqiv_cmd_dumpcfg(manager, current_node, true, 0, current_cmd);
	} else if(help) {
		nqiv_cmd_print_help(manager, current_node, recurse_help, 0);
	} else if(!error && current_node->store_value != NULL) {
		error = nqiv_cmd_execute_node(manager, current_node, idx, eolpos);
	} else {
		nqiv_cmd_print_help(manager, current_node, false, 0);
		if(error && manager->state->parse_error_quit) {
			nqiv_cmd_force_quit_main(nqiv_cmd_manager* manager);
		} else {
			error = false;
		}
	}
	nqiv_array_remove_bytes(manager->buffer, 0, eolpos);
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
	const int buf_len = sizeof(char) * 2;
	if( !nqiv_array_make_room(manager->buffer, buf_len) ) {
		return false;
	}
	const char buf[buf_len] = {byte, '\0'};
	if( !nqiv_array_push_bytes(manager->buffer, buf, buf_len) ) {
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
	if(!manager->buffer) {
		nqiv_array_destroy(manager->buffer);
	}
	memset( manager, 0, sizeof(nqiv_cmd_manager) );
}

bool nqiv_cmd_manager_init(nqiv_cmd_manager* manager, nqiv_state* state)
{
	nqiv_cmd_manager_destroy(manager);
	manager->array = nqiv_array_create(state->queue_length);
	if(manager->array == NULL) {
		nqiv_log_write(&manager->state->logger, NQIV_LOG_ERROR, "Failed to allocate memory to create cmd buffer of length %d\n.", state->queue_length);
		return false;
	}
	manager->state = state;
	manager->state->parse_error_quit = true;
	manager->state->apply_error_quit = true;
	nqiv_log_write(&manager->state->logger, NQIV_LOG_INFO, "Initialized cmd manager.\n.");
	return true;
}

bool nqiv_cmd_consume_stream_from_path(nqiv_cmd_manager* manager, const char* path)
{
	nqiv_log_write(&manager->state->logger, NQIV_LOG_INFO, "Reading commands from %s\n.", path);
	FILE* stream = fopen(path, "r");
	if(stream == NULL) {
		nqiv_log_write(&manager->state->logger, NQIV_LOG_WARNING, "Failed to read commands from %s\n.", path);
		return false;
	}
	const bool output = nqiv_cmd_consume_stream(manager, stream);
	fclose(stream);
	return output;
}
