#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>

#include "logging.h"
#include "array.h"
#include "state.h"
#include "keybinds.h"
#include "keyrate.h"

/* TODO Clear consumed */

/*
struct nqiv_cmd_node
{
	char* name;
	char* description;
	nqiv_cmd_node* children;
	bool (*parser)(nqiv_cmd_manager*);
	bool (*parameter_grabber)(nqiv_cmd_manager*, void*);
	void* circumstantial_data;
};
*/

const nqiv_cmd_node nqiv_parser_nodes_root = {
	.name = "root",
	.description = "Root of parsing tree.",
	.parser = nqiv_cmd_parser_choose_child,
	.children = {
		{
			.name = "showtree",
			.description = "Recursively list children of given command sequence, or current one.",
			.parser = nqiv_cmd_parser_showtree,
			.children = NULL,
			.parameter_grabber = NULL,
		},
		{
			.name = "help",
			.description = "List children of given command sequence, or current one.",
			.parser = nqiv_cmd_parser_help,
			.children = NULL,
			.parameter_grabber = NULL,
		},
		{
			.name = "sendkey",
			.description = "Issue a simulated keyboard action to the program.",
			.parser = nqiv_cmd_parser_sendkey,
			.children = NULL,
			.parameter_grabber = nqiv_cmd_parser_grab_key_action,
		},
		{
			.name = "insert",
			.description = "Add a value to a particular location.",
			.parser = nqiv_cmd_parser_choose_child,
			.children = {
				{
					.name = "image",
					.description = "Insert an image path to be opened at a particular index.",
					.children = NULL,
					.parser = nqiv_cmd_parser_insert_image,
					.parameter_grabber = nqiv_cmd_parser_grab_indexed_string,
				},
				NULL,
			},
			.parameter_grabber = nqiv_cmd_parser_grab_key_action,
		},
		{
			.name = "remove",
			.description = "Remove a value from a particular location.",
			.parser = nqiv_cmd_parser_choose_child,
			.children = {
				{
					.name = "image",
					.description = "Remove an image to be opened.",
					.children = {
						{
							.name = "index",
							.description = "Delete the image from the given index.",
							.children = NULL,
							.parser = nqiv_cmd_parser_remove_image_index,
							.parameter_grabber = nqiv_cmd_grab_positive_integer,
						},
						NULL,
					},
					.parser = nqiv_cmd_parser_choose_child,
					.parameter_grabber = NULL,
				},
				NULL,
			},
			.parameter_grabber = NULL,
		},
		{
			.name = "append",
			.description = "Add a value to the end of an existing series.",
			.parser = nqiv_cmd_parser_choose_child,
			.children = {
				{
					.name = "log_stream",
					.description = "Log to the given stream.",
					.children = NULL,
					.parser = nqiv_cmd_parser_append_log_stream,
					.parameter_grabber = nqiv_cmd_parser_grab_log_stream,
				},
				{
					.name = "image",
					.description = "Add an image path to the be opened.",
					.children = NULL,
					.parser = nqiv_cmd_parser_append_image,
					.parameter_grabber = nqiv_cmd_parser_grab_string,
				},
				{
					.name = "extension",
					.description = "Add an image extension to be accepted.",
					.children = NULL,
					.parser = nqiv_cmd_parser_append_extension,
					.parameter_grabber = nqiv_cmd_parser_grab_string_spaceless,
				},
				{
					.name = "keybind",
					.description = "Add a keybind.",
					.children = NULL,
					.parser = nqiv_cmd_parser_append_keybind,
					.parameter_grabber = nqiv_cmd_parser_grab_keybind,
				},
				NULL,
			},
			.parameter_grabber = NULL,
		},
		{
			.name = "set",
			.description = "Alter a singular value.",
			.parser = nqiv_cmd_parser_choose_child,
			.children = {
				{
					.name = "log_level",
					.description = "Log messages this level or lower are printed.",
					.children = NULL,
					.parser = nqiv_cmd_parser_set_log_level,
					.parameter_grabber = nqiv_cmd_parser_grab_log_level,
				},
				{
					.name = "log_prefix",
					.description = "Log message format. Special messages are delimited by #. ## produces a literal #. #time:<strftime format># prints the time. #level# prints the log level.",
					.children = NULL,
					.parser = nqiv_cmd_parser_set_log_prefix,
					.parameter_grabber = nqiv_cmd_parser_grab_string,
				},
				{
					.name = "zoom_left_amount",
					.description = "Amount to pan the zoom left with each action",
					.children = NULL,
					.parser = nqiv_cmd_parser_set_zoom_left_amount,
					.parameter_grabber = nqiv_cmd_parser_grab_negative_double,
				},
				{
					.name = "zoom_right_amount",
					.description = "Amount to pan the zoom right with each action",
					.children = NULL,
					.parser = nqiv_cmd_parser_set_zoom_right_amount,
					.parameter_grabber = nqiv_cmd_parser_grab_positive_double,
				},
				{
					.name = "zoom_down_amount",
					.description = "Amount to pan the zoom down with each action",
					.children = NULL,
					.parser = nqiv_cmd_parser_set_zoom_down_amount,
					.parameter_grabber = nqiv_cmd_parser_grab_positive_double,
				},
				{
					.name = "zoom_up_amount",
					.description = "Amount to pan the zoom up with each action",
					.children = NULL,
					.parser = nqiv_cmd_parser_set_zoom_up_amount,
					.parameter_grabber = nqiv_cmd_parser_grab_negative_double,
				},
				{
					.name = "zoom_out_amount",
					.description = "Amount to pan the zoom out with each action",
					.children = NULL,
					.parser = nqiv_cmd_parser_set_zoom_out_amount,
					.parameter_grabber = nqiv_cmd_parser_grab_positive_double,
				},
				{
					.name = "zoom_in_amount",
					.description = "Amount to pan the zoom in with each action",
					.children = NULL,
					.parser = nqiv_cmd_parser_set_zoom_in_amount,
					.parameter_grabber = nqiv_cmd_parser_grab_negative_double,
				},
				{
					.name = "thumbnail_zoom_amount",
					.description = "Number of pixels to resize thumbnails by with each action.",
					.children = NULL,
					.parser = nqiv_cmd_parser_set_thumbnail_zoom_amount,
					.parameter_grabber = nqiv_cmd_parser_grab_positive_integer,
				},
				{
					.name = "thumbnail_load",
					.description = "Whether to read thumbnails to the disk.",
					.children = NULL,
					.parser = nqiv_cmd_parser_set_thumbnail_load,
					.parameter_grabber = nqiv_cmd_grab_bool,
				},
				{
					.name = "thumbnail_load",
					.description = "Whether to save thumbnails to the disk. Note that if thumbnail_load is not set to true, then thumbnails will always be saved, even if they are up to date on the disk.",
					.children = NULL,
					.parser = nqiv_cmd_parser_set_thumbnail_save,
					.parameter_grabber = nqiv_cmd_grab_bool,
				},
				{
					.name = "thumbnail_size",
					.description = "Width and height of thumbnails are the same value.",
					.children = NULL,
					.parser = nqiv_cmd_parser_set_thumbnail_size,
					.parameter_grabber = nqiv_cmd_parser_grab_positive_integer,
				},
				{
					.name = "keypress_start_delay",
					.description = "When a key is first pressed, this delay must be passed before it will register.",
					.children = NULL,
					.parser = nqiv_cmd_parser_set_keypress_start_delay,
					.parameter_grabber = nqiv_cmd_parser_grab_keypress_uint64,
				},
				{
					.name = "keypress_repeat_delay",
					.description = "After the keypress is first registered, it will subsequently register, starting at this interval.",
					.children = NULL,
					.parser = nqiv_cmd_parser_set_keypress_repeat_delay,
					.parameter_grabber = nqiv_cmd_parser_grab_keypress_uint64,
				},
				{
					.name = "keypress_delay_accel",
					.description = "After the keypress is first registered, this will be subtracted from each subsequent delay until the minimum is met.",
					.children = NULL,
					.parser = nqiv_cmd_parser_set_keypress_delay_accel,
					.parameter_grabber = nqiv_cmd_parser_grab_keypress_uint64,
				},
				{
					.name = "keypress_minimum_delay",
					.description = "This is the absolute smallest delay for a keypress, and higher values will be clamped to this.",
					.children = NULL,
					.parser = nqiv_cmd_parser_set_keypress_minimum_delay,
					.parameter_grabber = nqiv_cmd_parser_grab_keypress_uint64,
				},
				{
					.name = "keypress_send_on_up",
					.description = "Register keypress on keyup events?",
					.children = NULL,
					.parser = nqiv_cmd_parser_set_keypress_send_on_up,
					.parameter_grabber = nqiv_cmd_parser_grab_keypress_bool,
				},
				{
					.name = "keypress_send_on_down",
					.description = "Register keypress on keydown events?",
					.children = NULL,
					.parser = nqiv_cmd_parser_set_keypress_send_on_down,
					.parameter_grabber = nqiv_cmd_parser_grab_keypress_bool,
				},
				{
					.name = "background_color",
					.description = "Color of background.",
					.children = NULL,
					.parser = nqiv_cmd_parser_set_background_color,
					.parameter_grabber = nqiv_cmd_parser_grab_quad_Uint8,
				},
				{
					.name = "error_color",
					.description = "Color of image area when there's an error loading.",
					.children = NULL,
					.parser = nqiv_cmd_parser_set_error_color,
					.parameter_grabber = nqiv_cmd_parser_grab_quad_Uint8,
				},
				{
					.name = "loading_color",
					.description = "Color of image area when image is still loading.",
					.children = NULL,
					.parser = nqiv_cmd_parser_set_loading_color,
					.parameter_grabber = nqiv_cmd_parser_grab_quad_Uint8,
				},
				{
					.name = "selection_color",
					.description = "Color of box around selected image.",
					.children = NULL,
					.parser = nqiv_cmd_parser_set_selection_color,
					.parameter_grabber = nqiv_cmd_parser_grab_quad_Uint8,
				},
				{
					.name = "alpha_background_color_one",
					.description = "The background of a transparent image is rendered as checkers. This is the first color.",
					.children = NULL,
					.parser = nqiv_cmd_parser_set_alpha_background_color_one,
					.parameter_grabber = nqiv_cmd_parser_grab_quad_Uint8,
				},
				{
					.name = "alpha_background_color_two",
					.description = "The background of a transparent image is rendered as checkers. This is the second color.",
					.children = NULL,
					.parser = nqiv_cmd_parser_set_alpha_background_color_two,
					.parameter_grabber = nqiv_cmd_parser_grab_quad_Uint8,
				},
				{
					.name = "no_resample_oversized",
					.description = "Normally, if the image is larger than 16k by 16k pixels, it will be reloaded for each zoom. This keeps the normal behavior with the entire image downsized.",
					.children = NULL,
					.parser = nqiv_cmd_parser_set_no_resample_oversized,
					.parameter_grabber = nqiv_cmd_parser_grab_bool,
				},
				{
					.name = "queue_size",
					.description = "Dynamic arrays in the software are backed by a given amount of memory. They will expand as needed, but it may improve performance to allocate memory all at once in advance.",
					.children = NULL,
					.parser = nqiv_cmd_parser_set_queue_size,
					.parameter_grabber = nqiv_cmd_parser_grab_positive_integer,
				},
				{
					.name = "window_width",
					.description = "Set the width of the program window",
					.children = NULL,
					.parser = nqiv_cmd_parser_set_window_width,
					.parameter_grabber = nqiv_cmd_parser_grab_positive_integer,
				},
				{
					.name = "window_height",
					.description = "Set the height of the program window",
					.children = NULL,
					.parser = nqiv_cmd_parser_set_window_width,
					.parameter_grabber = nqiv_cmd_parser_grab_positive_integer,
				},
				NULL,
			},
			.parameter_grabber = NULL,
		},
	}
	.parameter_grabber = NULL,
};

void nqiv_cmd_release_safe_attempted_string(nqiv_cmd_manager* manager)
{
	if(manager->buffer.replaced_byte_index != -1) {
		(char*)(manager->buffer.array->data)[manager->buffer.replaced_byte_index] =
			manager->buffer.replaced_byte;
		manager->buffer.replaced_byte = '\0';
		manager->buffer.replaced_byte_index = -1;
	}
}

char* nqiv_cmd_get_safe_attempted_string(nqiv_cmd_manager* manager)
{
	char* output = NULL;
	nqiv_cmd_release_safe_attempted_string(manager);
	output = (char*)(manager->buffer.array->data) + manager->buffer.bytes_consumed;
	if(manager->buffer.bytes_attempted < manager->buffer.array->position) {
		manager->buffer.replaced_byte =
			(char*)(manager->buffer.array->data)[manager->buffer.bytes_attempted];
		manager->buffer.replaced_index = manager->buffer.bytes_attempted;
		(char*)(manager->buffer.array->data)[manager->buffer.bytes_attempted] = '\0';
	}
	return output;
}

bool nqiv_cmd_consume_byte(nqiv_cmd_manager* manager, char* byte)
{
	if(manager->buffer.bytes_attempted >= manager->buffer.array->position) {
		return false;
	}
	nqiv_cmd_release_safe_attempted_string(manager);
	if(byte != NULL) {
		nqiv_array_get_bytes(manager->buffer.array, manager->buffer.bytes_attempted, sizeof(char), byte);
	}
	manager->buffer.bytes_attempted	+= 1;
	return true;
}

bool nqiv_cmd_consume_all_bytes(nqiv_cmd_manager* manager)
{
	if(manager->buffer.bytes_attempted >= manager->buffer.array->position) {
		return false;
	}
	nqiv_cmd_release_safe_attempted_string(manager);
	manager->buffer.bytes_attempted	= manager->buffer.array->position;
	return true;
}

bool nqiv_cmd_consume_to_pointer(nqiv_cmd_manager* manager, const char* ptr)
{
	char* attempted_ptr = manager->buffer.array->data + manager->buffer.bytes_attempted;
	char* final_ptr = manager->buffer.array->data + manager->buffer.array->position;
	if(ptr <= attempted_ptr || ptr > final_ptr) {
		return false;
	}
	const int diff = ptr - attempted_ptr;
	nqiv_cmd_release_safe_attempted_string(manager);
	manager->buffer.bytes_attempted	+= diff;
	return true;
}

bool nqiv_cmd_consume_count(nqiv_cmd_manager* manager, const int count)
{
	if(manager->buffer.bytes_attempted + count > manager->buffer.array->position) {
		return false;
	}
	nqiv_cmd_release_safe_attempted_string(manager);
	manager->buffer.bytes_attempted	+= count;
	return true;
}

void nqiv_cmd_apply_consume(nqiv_cmd_manager* manager)
{
	nqiv_cmd_release_safe_attempted_string(manager);
	manager->buffer.bytes_consumed = manager->buffer.bytes_attempted;
}

void nqiv_cmd_reset_consume(nqiv_cmd_manager* manager)
{
	nqiv_cmd_release_safe_attempted_string(manager);
	manager->buffer.bytes_attempted = manager->buffer.bytes_consumed;
}

bool nqiv_cmd_set_consume(nqiv_cmd_manager* manager, const int offset)
{
	if(offset > manager->buffer.array->position) {
		return false;
	}
	nqiv_cmd_release_safe_attempted_string(manager);
	manager->buffer.bytes_attempted = offset;
	manager->buffer.bytes_consumed = offset;
	return true;
}

void nqiv_cmd_clear_buffer(nqiv_cmd_manager* manager)
{
	nqiv_cmd_release_safe_attempted_string(manager);
	nqiv_array_clear(manager->buffer.array);
	manager->buffer.bytes_consumed = 0;
	manager->buffer.bytes_attempted = 0;
}

void nqiv_cmd_destroy(nqiv_cmd_manager* manager)
{
	if(manager->buffer.array != NULL) {
		nqiv_array_destroy(manager->buffer.array);
	}
	memset( manager, 0, sizeof(nqiv_cmd_manager) );
}

bool nqiv_cmd_consume_char_sequence(nqiv_cmd_manager* manager, const char* sequence)
{
	const int length = strlen(sequence);
	int idx;
	for(idx = 0; idx < length; ++idx) {
		char byte;
		if( !nqiv_cmd_consume_byte(manager, &byte) || byte != sequence[idx] ) {
			nqiv_reset_consume(manager);
			return false;
		}
	}
	nqiv_apply_consume(manager);
	return true;
}

int nqiv_cmd_scan_char_until(nqiv_cmd_manager* manager, const bool negated, const bool consumed, char** matches)
{
	const int start_offset = manager->buffer.bytes_consumed;
	int count = 0;
	while( nqiv_cmd_consume_all_bytes(manager) ) {
		const char* str = nqiv_cmd_get_safe_attempted_string(manager);
		char* longest_match = NULL
		int longest_match_length = 0;
		int match_index = 0;
		while(matches[match_idx] != NULL) {
			if(negated) {
				if(strncmp( str, matches[match_idx], strlen(matches[match_idx]) ) == 0) {
					goto end;
				}
			} else {
				if( strncmp( str, matches[match_idx], strlen(matches[match_idx]) ) == 0 &&
					strlen(matches[match_idx]) > longest_match_length ) {
					longest_match = matches[match_idx];
					longest_match_length = strlen(matches[match_idx]);
				}
			}
			match_index += 1;
		}
		if(!negated && longest_match == NULL) {
			goto end;
		}
		count += longest_match_length;
		nqiv_cmd_reset_consume(manager);
		nqiv_cmd_consume_count(manager, longest_match_length);
		nqiv_cmd_apply_consume(manager, count);
	}
	end:
	if(!consumed) {
		nqiv_cmd_set_consume(manager, start_offset);
	}
	return count;
}

bool nqiv_cmd_consume_whitespace(nqiv_cmd_manager* manager)
{
	return nqiv_cmd_scan_char_until(manager, false, true, {" ", "\t", NULL}) > 0;
}

bool nqiv_cmd_consume_whitespace_and_eol(nqiv_cmd_manager* manager)
{
	return nqiv_cmd_scan_char_until(manager, false, true, {" ", "\t", "\r\n", "\n", "\r", NULL}) > 0;
}

bool nqiv_cmd_consume_char_sequence_and_whitespace_and_eol(nqiv_cmd_manager* manager, const char* sequence)
{
	const int start_offset = manager->buffer.bytes_consumed;
	if( !nqiv_cmd_consume_char_sequence(manager, sequence) ||
		!nqiv_cmd_consume_whitespace_and_eol(manager) ) {
		nqiv_cmd_set_consume(manager, start_offset);
		return false;
	}
	return true;
}

bool nqiv_cmd_consume_char_sequence_and_whitespace(nqiv_cmd_manager* manager, const char* sequence)
{
	const int start_offset = manager->buffer.bytes_consumed;
	if( !nqiv_cmd_consume_char_sequence(manager, sequence) ||
		!nqiv_cmd_consume_whitespace(manager) ) {
		nqiv_cmd_set_consume(manager, start_offset);
		return false;
	}
	return true;
}

char* nqiv_cmd_get_non_whitespace_and_eol(nqiv_cmd_manager* manager)
{
	nqiv_consume_count(manager, nqiv_cmd_scan_char_until(manager, true, false, {" ", "\t", "\r\n", "\n", "\r", NULL}) );
	return nqiv_cmd_get_safe_attempted_string(manager);
}

char* nqiv_cmd_get_non_eol(nqiv_cmd_manager* manager)
{
	nqiv_consume_count(manager, nqiv_cmd_scan_char_until(manager, true, false, {"\r\n", "\n", "\r", NULL}) );
	return nqiv_cmd_get_safe_attempted_string(manager);
}

bool nqiv_cmd_consume_double(nqiv_cmd_manager* manager, double* value, const double minimum, const double maximum)
{
	nqiv_cmd_consume_all_bytes(manager);
	const char* str = nqiv_cmd_get_safe_attempted_string(manager);
	const char* end = NULL;
	const double tmp = strtod(str, &end);
	nqiv_cmd_reset_consume(manager);
	if(errno == ERANGE || tmp == HUGE_VAL || end == NULL || tmp < minimum || tmp > maximum) {
		return false;
	}
	if(value != NULL) {
		*value = tmp;
	}
	nqiv_cmd_consume_to_pointer(manager, end);
	nqiv_cmd_apply_consume(manager);
	return true;
}

bool nqiv_cmd_consume_int(nqiv_cmd_manager* manager, int* value, const int minimum, const int maximum)
{
	nqiv_cmd_consume_all_bytes(manager);
	const char* str = nqiv_cmd_get_safe_attempted_string(manager);
	const char* end = NULL;
	const double tmp = strtol(str, &end, 10);
	nqiv_cmd_reset_consume(manager);
	if(errno == ERANGE || tmp == LONG_MAX || tmp == LONG_MIN || end == NULL || tmp < minimum || tmp > maximum) {
		return false;
	}
	if(value != NULL) {
		*value = tmp;
	}
	nqiv_cmd_consume_to_pointer(manager, end);
	nqiv_cmd_apply_consume(manager);
	return true;
}

bool nqiv_cmd_consume_Uint64(nqiv_cmd_manager* manager, Uint64* value, const int minimum, const int maximum)
{
	if(minimum < 0) {
		return false;
	}
	int tmp;
	if( !nqiv_cmd_consume_int(manager, &tmp, minimum, maximum) ) {
		return false;
	}
	if(value != NULL) {
		*value = (Uint64)tmp; /* TODO Check conversion. */
	}
	return true;
}

bool nqiv_cmd_consume_Uint8(nqiv_cmd_manager* manager, Uint8* value, const int minimum, const int maximum)
{
	if(minimum < 0 || maximum > 255) {
		return false;
	}
	int tmp;
	if( !nqiv_cmd_consume_int(manager, &tmp, minimum, maximum) ) {
		return false;
	}
	if(value != NULL) {
		*value = (Uint8)tmp; /* TODO Check conversion. */
	}
	return true;
}

bool nqiv_cmd_consume_log_level(nqiv_cmd_manager* manager, nqiv_log_level* value)
{
	const char* str = nqiv_cmd_get_non_whitespace_and_eol(manager);
	const nqiv_log_level tmp = nqiv_log_level_from_string(str);
	if(tmp == NQIV_LOG_UNKNOWN) {
		nqiv_reset_consume(manager);
		return false;
	}
	nqiv_apply_consume(manager);
	if(value != NULL) {
		*value = tmp;
	}
	return true;
}

bool nqiv_cmd_consume_key_action(nqiv_cmd_manager* manager, nqiv_key_action* value)
{
	const char* str = nqiv_cmd_get_non_whitespace_and_eol(manager);
	const nqiv_key_action tmp = nqiv_text_to_key_action( str, strlen(str) );
	if(tmp == NQIV_KEY_ACTION_NONE) {
		nqiv_reset_consume(manager);
		return false;
	}
	nqiv_apply_consume(manager);
	if(value != NULL) {
		*value = tmp;
	}
	return true;
}

bool nqiv_cmd_consume_keybind(nqiv_cmd_manager* manager, nqiv_keybind_pair* value)
{
	const char* str = nqiv_cmd_get_non_eol(manager);
	nqiv_keybind_pair tmp;
	if( !nqiv_text_to_keybind(str, &tmp) ) {
		nqiv_reset_consume(manager);
		return false;
	}
	nqiv_apply_consume(manager);
	if(value != NULL) {
		memcpy( value, tmp, sizeof(nqiv_keybind_pair) );
	}
	return true;
}

bool nqiv_cmd_consume_press_action(nqiv_cmd_manager* manager, nqiv_keyrate_press_action* value)
{
	bool success = true;
	nqiv_keyrate_press_action tmp;
	if( nqiv_cmd_consume_char_sequence(manager, "default") ) {
		tmp = NQIV_KEYRATE_ON_MANAGER;
	} else if( nqiv_cmd_consume_char_sequence(manager, "allow") ) {
		tmp = NQIV_KEYRATE_ALLOW;
	} else if( nqiv_cmd_consume_char_sequence(manager, "deny") ) {
		tmp = NQIV_KEYRATE_DENY;
	} else {
		success = false;
	}
	if(success && value != NULL) {
		*value = tmp;
	}
	return success;
}

bool nqiv_cmd_consume_bool(nqiv_cmd_manager* manager, bool* value)
{
	bool success = true;
	bool tmp;
	if( nqiv_cmd_consume_char_sequence(manager, "true") ) {
		tmp = true;
	} else if( nqiv_cmd_consume_char_sequence(manager, "false") ) {
		tmp = false;
	} else {
		success = false;
	}
	if(success && value != NULL) {
		*value = tmp;
	}
	return success;
}

/* TODO Grabbers */
/* TODO Parsers */

bool nqiv_cmd_init(nqiv_cmd_manager* manager, nqiv_state* state)
{
	nqiv_cmd_destroy(manager);
	manager->buffer.array = nqiv_array_create(state->queue_length);
	if(manager->buffer.array == NULL) {
		/* We don't need to destroy here again, but if we do more allocations, this could change. */
		nqiv_log_write(&state->logger, NQIV_LOG_ERROR, "Failed to create buffer for nqiv command parser.\n");
		return false;
	}
	nqiv_cmd_clear_buffer(manager);
	manager->state = state;
	manager->current_node = nqiv_parser_nodes_root;
	return true;
}

bool nqiv_cmd_add_byte(nqiv_cmd_manager* manager, const char byte)
{
	const int buf_len = sizeof(char) * 2;
	if( !nqiv_array_make_room(manager->buffer.array, buf_len) ) {
		return false;
	}
	nqiv_cmd_release_safe_attempted_string(manager);
	char buf[sizeof(char) * 2] = {byte, '\0'};
	if( !nqiv_array_push_bytes(manager->buffer.array, buf, buf_len) ) {
		nqiv_log_write( &state->logger, NQIV_LOG_ERROR, "Failed to append byte %c to nqiv command parser of length %d.\n", byte, manager->buffer.array->data_length );
		return false;
	}
	return true;
}

bool nqiv_cmd_parse(nqiv_cmd_manager* manager)
{
	return manager->current_node->parser(manager);
}

bool nqiv_cmd_add_then_parse(nqiv_cmd_manager* manager, const char byte)
{
	return nqiv_cmd_add_byte(manager, byte) && nqiv_cmd_parse(manager);
}
