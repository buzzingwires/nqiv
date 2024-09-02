#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <SDL2/SDL.h>

#include "logging.h"
#include "array.h"
#include "queue.h"
#include "keyrate.h"
#include "keybinds.h"

int nqiv_findchar(const char* text, const char query, const int start, const int stop)
{
	assert(text != NULL);
	assert(stop >= -1);
	assert(start >= 0);
	const int step = stop >= start ? 1 : -1;
	int idx = start;
	char c = text[idx];
	while(c != '\0' && idx != stop) {
		c = text[idx];
		if(c == query) {
			return idx;
		}
		idx += step;
	}
	return -1;
}

const char* nqiv_keybind_action_names[] =
{
	"quit",
	"page_up",
	"page_down",
	"toggle_montage",
	"set_montage",
	"set_viewing",
	"zoom_in",
	"zoom_out",
	"pan_left",
	"pan_right",
	"pan_up",
	"pan_down",
	"zoom_in_more",
	"zoom_out_more",
	"pan_left_more",
	"pan_right_more",
	"pan_up_more",
	"pan_down_more",
	"pan_center",
	"image_previous",
	"image_next",
	"montage_left",
	"montage_right",
	"montage_up",
	"montage_down",
	"montage_start",
	"montage_end",
	"toggle_stretch",
	"stretch",
	"keep_aspect_ratio",
	"fit",
	"actual_size",
	"keep_fit",
	"keep_actual_size",
	"keep_current_zoom",
	"toggle_kept_zoom",
	"scale_mode_nearest",
	"scale_mode_linear",
	"scale_mode_anisotropic",
	"toggle_scale_mode",
	"image_mark_toggle",
	"image_mark",
	"image_unmark",
	"print_marked",
	"montage_select_at_mouse",
	"image_mark_at_mouse",
	"image_unmark_at_mouse",
	"image_mark_toggle_at_mouse",
	"start_mouse_pan",
	"end_mouse_pan",
	"image_zoom_in",
	"image_zoom_out",
	"image_zoom_in_more",
	"image_zoom_out_more",
	"reload"
};

nqiv_key_action nqiv_text_to_key_action(const char* text, const int length)
{
	nqiv_key_action action = NQIV_KEY_ACTION_NONE;
	for(action = NQIV_KEY_ACTION_QUIT; action <= NQIV_KEY_ACTION_MAX; ++action) {
		if( strncmp(text, nqiv_keybind_action_names[action], length) == 0 && (size_t)length == strlen(nqiv_keybind_action_names[action]) ) {
			return action;
		}
	}
	return NQIV_KEY_ACTION_NONE;
}

bool nqiv_text_to_key_match(char* text, const int length, nqiv_key_match* match)
{
	bool success = true;
	if( strncmp(text, "lshift", length) == 0 ) {
		match->mode |= NQIV_KEY_MATCH_MODE_KEY_MOD;
		match->data.key.mod |= KMOD_LSHIFT;
	} else if( strncmp(text, "rshift", length) == 0 ) {
		match->mode |= NQIV_KEY_MATCH_MODE_KEY_MOD;
		match->data.key.mod |= KMOD_RSHIFT;
	} else if( strncmp(text, "lctrl", length) == 0 ) {
		match->mode |= NQIV_KEY_MATCH_MODE_KEY_MOD;
		match->data.key.mod |= KMOD_LCTRL;
	} else if( strncmp(text, "rctrl", length) == 0 ) {
		match->mode |= NQIV_KEY_MATCH_MODE_KEY_MOD;
		match->data.key.mod |= KMOD_RCTRL;
	} else if( strncmp(text, "lalt", length) == 0 ) {
		match->mode |= NQIV_KEY_MATCH_MODE_KEY_MOD;
		match->data.key.mod |= KMOD_LALT;
	} else if( strncmp(text, "ralt", length) == 0 ) {
		match->mode |= NQIV_KEY_MATCH_MODE_KEY_MOD;
		match->data.key.mod |= KMOD_RALT;
	} else if( strncmp(text, "lgui", length) == 0 ) {
		match->mode |= NQIV_KEY_MATCH_MODE_KEY_MOD;
		match->data.key.mod |= KMOD_LGUI;
	} else if( strncmp(text, "rgui", length) == 0 ) {
		match->mode |= NQIV_KEY_MATCH_MODE_KEY_MOD;
		match->data.key.mod |= KMOD_RGUI;
	} else if( strncmp(text, "num", length) == 0 ) {
		match->mode |= NQIV_KEY_MATCH_MODE_KEY_MOD;
		match->data.key.mod |= KMOD_NUM;
	} else if( strncmp(text, "caps", length) == 0 ) {
		match->mode |= NQIV_KEY_MATCH_MODE_KEY_MOD;
		match->data.key.mod |= KMOD_CAPS;
	} else if( strncmp(text, "mode", length) == 0 ) {
		match->mode |= NQIV_KEY_MATCH_MODE_KEY_MOD;
		match->data.key.mod |= KMOD_MODE;
	} else if( strncmp(text, "scroll", length) == 0 ) {
		match->mode |= NQIV_KEY_MATCH_MODE_KEY_MOD;
		match->data.key.mod |= KMOD_SCROLL;
	} else if( strncmp(text, "ctrl", length) == 0 ) {
		match->mode |= NQIV_KEY_MATCH_MODE_KEY_MOD;
		match->data.key.mod |= KMOD_CTRL;
	} else if( strncmp(text, "shift", length) == 0 ) {
		match->mode |= NQIV_KEY_MATCH_MODE_KEY_MOD;
		match->data.key.mod |= KMOD_SHIFT;
	} else if( strncmp(text, "alt", length) == 0 ) {
		match->mode |= NQIV_KEY_MATCH_MODE_KEY_MOD;
		match->data.key.mod |= KMOD_ALT;
	} else if( strncmp(text, "gui", length) == 0 ) {
		match->mode |= NQIV_KEY_MATCH_MODE_KEY_MOD;
		match->data.key.mod |= KMOD_GUI;
	} else if( strncmp(text, "scroll_forward", length) == 0 ) {
		if( ( match->mode & (NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_FORWARD | NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_BACKWARD | NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_LEFT | NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_RIGHT | NQIV_KEY_MATCH_MODE_MOUSE_BUTTON | NQIV_KEY_MATCH_MODE_KEY) ) != 0 ) {
			success = false;
		} else {
			match->mode |= NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_FORWARD;
		}
	} else if( strncmp(text, "scroll_backward", length) == 0 ) {
		if( ( match->mode & (NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_FORWARD | NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_BACKWARD | NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_LEFT | NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_RIGHT | NQIV_KEY_MATCH_MODE_MOUSE_BUTTON | NQIV_KEY_MATCH_MODE_KEY) ) != 0 ) {
			success = false;
		} else {
			match->mode |= NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_BACKWARD;
		}
	} else if( strncmp(text, "scroll_left", length) == 0 ) {
		if( ( match->mode & (NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_FORWARD | NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_BACKWARD | NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_LEFT | NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_RIGHT | NQIV_KEY_MATCH_MODE_MOUSE_BUTTON | NQIV_KEY_MATCH_MODE_KEY) ) != 0 ) {
			success = false;
		} else {
			match->mode |= NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_LEFT;
		}
	} else if( strncmp(text, "scroll_right", length) == 0 ) {
		if( ( match->mode & (NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_FORWARD | NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_BACKWARD | NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_LEFT | NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_RIGHT | NQIV_KEY_MATCH_MODE_MOUSE_BUTTON | NQIV_KEY_MATCH_MODE_KEY) ) != 0 ) {
			success = false;
		} else {
			match->mode |= NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_RIGHT;
		}
	} else if( (size_t)length > strlen("mouse") && strncmp( text, "mouse", strlen("mouse") ) == 0 ) {
		if( ( match->mode & (NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_FORWARD | NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_BACKWARD | NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_LEFT | NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_RIGHT | NQIV_KEY_MATCH_MODE_MOUSE_BUTTON | NQIV_KEY_MATCH_MODE_KEY) ) != 0 ) {
			success = false;
		} else {
			char* end = NULL;
			const int tmp = strtol(text + strlen("mouse"), &end, 10);
			if(errno == ERANGE || end == NULL || tmp < 0 || tmp > 255 || end > text + length) {
				success = false;
			} else {
				match->mode |= NQIV_KEY_MATCH_MODE_MOUSE_BUTTON;
				match->data.mouse_button.button = tmp;
				match->data.mouse_button.clicks = 1;
				if(end < text + length) {
					if(end + strlen("_double") <= text + length &&
					   strncmp( end, "_double", strlen("_double") ) == 0) {
						match->data.mouse_button.clicks = 2;
					} else {
						success = false;
					}
				}
			}
		}
	} else {
		const char endchar = text[length];
		text[length] = '\0';
		const SDL_Scancode sc = SDL_GetScancodeFromName(text);
		assert(length >= 0);
		assert(strlen(text) == (size_t)length);
		text[length] = endchar;
		if(sc == SDL_SCANCODE_UNKNOWN || match->data.key.scancode != 0 || ( match->mode & (NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_FORWARD | NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_BACKWARD | NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_LEFT | NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_RIGHT | NQIV_KEY_MATCH_MODE_MOUSE_BUTTON | NQIV_KEY_MATCH_MODE_KEY) ) != 0) {
			success = false;
		} else {
			match->mode |= NQIV_KEY_MATCH_MODE_KEY;
			match->data.key.scancode = sc;
		}
	}
	return success;
}

bool nqiv_text_to_keystate_numerical(char* text, const int length, const char* prefix, Uint64* output)
{
	bool success = false;
	if(*output == 0 && (size_t)length > strlen(prefix) && strncmp( text, prefix, strlen(prefix) ) == 0) {
		char* end = NULL;
		const int tmp = strtol(text + strlen(prefix), &end, 10);
		if(errno != ERANGE && end != NULL && tmp > 0 && end <= text + length) {
			success = true;
			*output = (Uint64)tmp;
		}
	}
	return success;
}

bool nqiv_text_to_keystate(char* text, const int length, nqiv_keyrate_keystate* state)
{
	bool success = true;
	if(strncmp(text, "allow_on_up", length) == 0 && state->send_on_up == NQIV_KEYRATE_ON_MANAGER) {
		state->send_on_up = NQIV_KEYRATE_ALLOW;
	} else if(strncmp(text, "allow_on_down", length) == 0 && state->send_on_down == NQIV_KEYRATE_ON_MANAGER) {
		state->send_on_down = NQIV_KEYRATE_ALLOW;
	} else if(strncmp(text, "deny_on_up", length) == 0 && state->send_on_up == NQIV_KEYRATE_ON_MANAGER) {
		state->send_on_up = NQIV_KEYRATE_DENY;
	} else if(strncmp(text, "deny_on_down", length) == 0 && state->send_on_down == NQIV_KEYRATE_ON_MANAGER) {
		state->send_on_down = NQIV_KEYRATE_DENY;
	} else if( !nqiv_text_to_keystate_numerical( text, length, "start_delay_", &(state->settings.start_delay) ) &&
			   !nqiv_text_to_keystate_numerical( text, length, "consecutive_delay_", &(state->settings.consecutive_delay) ) &&
			   !nqiv_text_to_keystate_numerical( text, length, "delay_accel_", &(state->settings.delay_accel) ) &&
			   !nqiv_text_to_keystate_numerical( text, length, "minimum_delay_", &(state->settings.minimum_delay) ) ) {
		success = false;
	}
	return success;
}

int nqiv_keybind_text_to_keybind(char* text, nqiv_keybind_pair* pair)
{
	const size_t textlen = strlen(text);
	if(textlen == 0) {
		return -1;
	}
	const int equal_start = nqiv_findchar(text, '=', textlen - 1, -1);
	if( equal_start == -1 || textlen <= (size_t)equal_start + 1 ) {
		return -1;
	}
	nqiv_keybind_pair tmp = {0};
	tmp.action = NQIV_KEY_ACTION_NONE;
	int idx;
	int section_start;
	bool success = true;
	for(idx = 0, section_start = 0; idx <= equal_start; ++idx) {
		if(text[idx] == '+' || (idx == equal_start && idx > section_start) ) {
			int section_end = idx;
			if(idx + 1 == equal_start || text[idx + 1] == '+') {
				section_end += 1;
			}
			success = nqiv_text_to_key_match(text + section_start, section_end - section_start, &tmp.match);
			if(!success) {
				break;
			}
			section_start = section_end + 1;
		}
	}
	for(idx = equal_start + 1, section_start = idx; (size_t)idx <= textlen; ++idx) {
		if( text[idx] == '+' || ( (size_t)idx == textlen && idx > section_start ) ) {
			int section_end = idx;
			if( (size_t)idx + 1 == textlen || text[idx + 1] == '+' ) {
				section_end += 1;
			}
			const int section_length = section_end - section_start;
			if( !nqiv_text_to_keystate(text + section_start, section_length, &tmp.keyrate) ) {
				if(tmp.action != NQIV_KEY_ACTION_NONE) {
					success = false;
					break;
				}
				tmp.action = nqiv_text_to_key_action(text + section_start, section_length);
				if(tmp.action == NQIV_KEY_ACTION_NONE) {
					success = false;
					break;
				}
			}
			section_start = section_end + 1;
		}
	}
	if(tmp.match.mode == NQIV_KEY_MATCH_MODE_KEY_MOD) {
		success = false;
	}
	if(success) {
		memcpy( pair, &tmp, sizeof(nqiv_keybind_pair) );
		return textlen;
	} else {
		return -1;
	}
}

bool nqiv_keybind_create_manager(nqiv_keybind_manager* manager, nqiv_log_ctx* logger, const int starting_array_length)
{
	assert(manager != NULL);
	nqiv_log_write(logger, NQIV_LOG_INFO, "Creating keybind manager.\n");
	nqiv_array* arrayptr = nqiv_array_create(sizeof(nqiv_keybind_pair), starting_array_length);
	if(arrayptr == NULL) {
		return false;
	}
	manager->lookup = arrayptr;
	manager->logger = logger;
	return true;
}

bool nqiv_keybind_add(nqiv_keybind_manager* manager, const nqiv_keybind_pair* pair)
{
	assert(manager != NULL);
	assert(manager->lookup != NULL);
	assert(pair != NULL);
	nqiv_keybind_pair tmp = {0};
	memcpy( &tmp.match, &pair->match, sizeof(nqiv_key_match) );
	memcpy( &tmp.keyrate, &pair->keyrate, sizeof(nqiv_keyrate_keystate) );
	memset( &tmp.keyrate.ephemeral, 0, sizeof(nqiv_keyrate_keystate_ephemeral) );
	tmp.action = pair->action;
	return nqiv_array_push(manager->lookup, &tmp);
}

bool nqiv_key_match_element_to_string(nqiv_array* builder, const char* suffix)
{
	return nqiv_array_push_str(builder, suffix) &&
		   nqiv_array_push_str(builder, "+");
}

bool nqiv_keymod_to_string(const nqiv_keybind_pair* pair, nqiv_array* builder)
{
	const Uint16 mods[] =
	{
		KMOD_LSHIFT,
		KMOD_RSHIFT,
		KMOD_LCTRL,
		KMOD_RCTRL,
		KMOD_LALT,
		KMOD_RALT,
		KMOD_NUM,
		KMOD_CAPS,
		KMOD_MODE,
		KMOD_SCROLL,
		KMOD_CTRL,
		KMOD_SHIFT,
		KMOD_ALT,
		0
	};
	const Uint16 anti_mods[] =
	{
		KMOD_RSHIFT,
		KMOD_LSHIFT,
		KMOD_RCTRL,
		KMOD_LCTRL,
		KMOD_RALT,
		KMOD_LALT,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0
	};
	const char* mod_names[] =
	{
		"lshift",
		"rshift",
		"lctrl",
		"rctrl",
		"lalt",
		"ralt",
		"num",
		"caps",
		"mode",
		"scroll",
		"ctrl",
		"shift",
		"alt",
		NULL
	};
	int idx;
	bool success = true;
	for(idx = 0; mod_names[idx] != NULL; ++idx) {
		if( (pair->match.data.key.mod & mods[idx]) == mods[idx] && (pair->match.data.key.mod & anti_mods[idx]) == 0 ) {
			success = success && nqiv_key_match_element_to_string(builder, mod_names[idx]);
		}
	}
	return success;
}

bool nqiv_keyrate_to_string(nqiv_array* builder, const nqiv_keyrate_keystate* state)
{
	bool success = true;
	if(state->send_on_down == NQIV_KEYRATE_ALLOW) {
		success = success && nqiv_array_push_str(builder, "allow_on_down+");
	}
	if(state->send_on_up == NQIV_KEYRATE_ALLOW) {
		success = success && nqiv_array_push_str(builder, "allow_on_up+");
	}
	if(state->send_on_down == NQIV_KEYRATE_DENY) {
		success = success && nqiv_array_push_str(builder, "deny_on_down+");
	}
	if(state->send_on_up == NQIV_KEYRATE_DENY) {
		success = success && nqiv_array_push_str(builder, "deny_on_up+");
	}
	if(state->settings.start_delay != 0) {
		success = success &&  nqiv_array_push_sprintf(builder, "start_delay_%d+", state->settings.start_delay);
	}
	if(state->settings.consecutive_delay != 0) {
		success = success &&  nqiv_array_push_sprintf(builder, "consecutive_delay_%d+", state->settings.consecutive_delay);
	}
	if(state->settings.delay_accel != 0) {
		success = success &&  nqiv_array_push_sprintf(builder, "delay_accel_%d+", state->settings.delay_accel);
	}
	if(state->settings.minimum_delay != 0) {
		success = success &&  nqiv_array_push_sprintf(builder, "minimum_delay_%d+", state->settings.minimum_delay);
	}
	return success;
}

bool nqiv_keybind_to_string(const nqiv_keybind_pair* pair, char* buf)
{
	nqiv_array builder;
	nqiv_array_inherit(&builder, buf, sizeof(char), NQIV_KEYBIND_STRLEN);
	bool success = true;
	success = success && nqiv_keymod_to_string(pair, &builder);
	if( (pair->match.mode & NQIV_KEY_MATCH_MODE_KEY) != 0 ) {
		success = success && nqiv_key_match_element_to_string(&builder, SDL_GetScancodeName(pair->match.data.key.scancode) );
	}
	if( (pair->match.mode & NQIV_KEY_MATCH_MODE_MOUSE_BUTTON) != 0 ) {
		assert(pair->match.data.mouse_button.clicks == 1 || pair->match.data.mouse_button.clicks == 2);
		success = success && nqiv_array_push_sprintf(&builder, "mouse%d%s+", pair->match.data.mouse_button.button, pair->match.data.mouse_button.clicks == 2 ? "_double" : "");
	}
	if( (pair->match.mode & NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_FORWARD) != 0 ) {
		success = success && nqiv_key_match_element_to_string(&builder, "scroll_forward");
	}
	if( (pair->match.mode & NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_BACKWARD) != 0 ) {
		success = success && nqiv_key_match_element_to_string(&builder, "scroll_backward");
	}
	if( (pair->match.mode & NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_LEFT) != 0 ) {
		success = success && nqiv_key_match_element_to_string(&builder, "scroll_left");
	}
	if( (pair->match.mode & NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_RIGHT) != 0 ) {
		success = success && nqiv_key_match_element_to_string(&builder, "scroll_right");
	}
	assert(nqiv_array_get_units_count(&builder) > 0);
	assert(buf[nqiv_array_get_last_idx(&builder)] == '+');
	buf[nqiv_array_get_last_idx(&builder)] = '=';
	success = success && nqiv_keyrate_to_string( &builder, &(pair->keyrate) );
	success = success && nqiv_array_push_str(&builder, nqiv_keybind_action_names[pair->action]);
	return success;
}

bool nqiv_compare_mod(const Uint16 a, const Uint16 b)
{
	const Uint16 ac = a & ~KMOD_GUI & ~KMOD_SCROLL & ~KMOD_NUM;
	const Uint16 bc = b & ~KMOD_GUI & ~KMOD_SCROLL & ~KMOD_NUM;
	return ( (bool)(ac & KMOD_SHIFT)  == (bool)(bc & KMOD_SHIFT)  ) &&
		   ( (bool)(ac & KMOD_CTRL)   == (bool)(bc & KMOD_CTRL)   ) &&
		   ( (bool)(ac & KMOD_ALT)    == (bool)(bc & KMOD_ALT)    ) &&
		   ( (bool)(ac & KMOD_GUI)    == (bool)(bc & KMOD_GUI)    ) &&
		   ( (bool)(ac & KMOD_NUM)    == (bool)(bc & KMOD_NUM)    ) &&
		   ( (bool)(ac & KMOD_CAPS)   == (bool)(bc & KMOD_CAPS)   ) &&
		   ( (bool)(ac & KMOD_MODE)   == (bool)(bc & KMOD_MODE)   ) &&
		   ( (bool)(ac & KMOD_SCROLL) == (bool)(bc & KMOD_SCROLL) );
}

bool nqiv_keybind_compare_match(const nqiv_key_match* a, const nqiv_key_match* b)
{
	return (a->mode == b->mode) &&
		   ( (a->mode & NQIV_KEY_MATCH_MODE_KEY_MOD) == 0 || ( nqiv_compare_mod(a->data.key.mod, b->data.key.mod) ) ) &&
		   ( (a->mode & NQIV_KEY_MATCH_MODE_KEY) == 0 || (a->data.key.scancode == b->data.key.scancode) ) &&
		   ( (a->mode & NQIV_KEY_MATCH_MODE_MOUSE_BUTTON) == 0 || ( a->data.mouse_button.button == b->data.mouse_button.button && a->data.mouse_button.clicks == b->data.mouse_button.clicks ) );
}

nqiv_key_lookup_summary nqiv_keybind_lookup(nqiv_keybind_manager* manager, const nqiv_key_match* match, nqiv_queue* output)
{
	assert(manager != NULL);
	assert(manager->lookup != NULL);
	assert(match != NULL);

	nqiv_key_lookup_summary result = NQIV_KEY_LOOKUP_NOT_FOUND;
	const int lookup_len = nqiv_array_get_units_count(manager->lookup);
	nqiv_keybind_pair* lookup = manager->lookup->data;
	int idx;
	for(idx = 0; idx < lookup_len; ++idx) {
		const nqiv_keybind_pair* pair = &lookup[idx];
		if( nqiv_keybind_compare_match(&pair->match, match) ) {
			if( output == NULL || !nqiv_queue_push(output, &pair) ) {
				result |= NQIV_KEY_LOOKUP_FAILURE;
			} else {
				result |= NQIV_KEY_LOOKUP_FOUND;
			}
		}
	}
	return result;
}

void nqiv_keybind_destroy_manager(nqiv_keybind_manager* manager) {
	assert(manager != NULL);
	assert(manager->lookup != NULL);
	nqiv_log_write(manager->logger, NQIV_LOG_INFO, "Destroying keybind manager.\n");
	nqiv_array_destroy(manager->lookup);
	memset( manager, 0, sizeof(nqiv_keybind_manager) );
}
