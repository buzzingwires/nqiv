#include <stdbool.h>
#include <errno.h>
#include <assert.h>

#include <SDL2/SDL.h>

#include "logging.h"
#include "array.h"
#include "queue.h"
#include "keybinds.h"

/*
typedef enum nqiv_key_action
{
	NQIV_KEY_ACTION_QUIT = 0,
	NQIV_KEY_ACTION_NEXT,
	NQIV_KEY_ACTION_PREVIOUS,
	NQIV_KEY_ACTION_PAGE_UP,
	NQIV_KEY_ACTION_PAGE_DOWN,
	NQIV_KEY_ACTION_TOGGLE_MONTAGE,
	NQIV_KEY_ACTION_SET_MONTAGE,
	NQIV_KEY_ACTION_SET_VIEWING,
	NQIV_KEY_ACTION_ZOOM_IN,
	NQIV_KEY_ACTION_ZOOM_OUT,
	NQIV_KEY_ACTION_LEFT,
	NQIV_KEY_ACTION_RIGHT,
	NQIV_KEY_ACTION_UP,
	NQIV_KEY_ACTION_DOWN,
	NQIV_KEY_ACTION_TOGGLE_STRETCH,
	NQIV_KEY_ACTION_STRETCH,
	NQIV_KEY_ACTION_KEEP_ASPECT_RATIO,
	NQIV_KEY_ACTION_RELOAD,
} nqiv_key_action;

typedef struct nqiv_keybind_pair
{
	SDL_Keysym key;
	nqiv_key_action action;
} nqiv_keybind_pair;

typedef struct nqiv_keybind_manager
{
	nqiv_log_ctx* logger;
	nqiv_array* lookup;
} nqiv_keybind_manager;


bool nqiv_keybind_add_from_text(nqiv_keybind_manager* manager, const char* key, const char* action);
nqiv_key_action nqiv_keybind_lookup_text(nqiv_keybind_manager* manager, const char* key);

*/

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

nqiv_key_action nqiv_text_to_key_action(const char* text)
{
	nqiv_key_action action = NQIV_KEY_ACTION_NONE;
	for(action = NQIV_KEY_ACTION_QUIT; action <= NQIV_KEY_ACTION_MAX; ++action) {
		if( strncmp( text, nqiv_keybind_action_names[action], strlen(nqiv_keybind_action_names[action]) ) == 0 && strlen(text) == strlen(nqiv_keybind_action_names[action]) ) {
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
					if(end + strlen("_double") > text + length) {
						success = false;
					} else if(strncmp( end, "_double", strlen("_double") ) == 0) {
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
	const nqiv_key_action action = nqiv_text_to_key_action(text + equal_start + 1);
	if(action == NQIV_KEY_ACTION_NONE) {
		return -1;
	}
	int idx;
	int section_start;
	bool success = true;
	nqiv_key_match match = {0};
	for(idx = 0, section_start = 0; idx <= equal_start; ++idx) {
		if(text[idx] == '+') {
			int plus_start = idx;
			if(idx + 1 == equal_start || text[idx + 1] == '+') {
				plus_start += 1;
			}
			success = nqiv_text_to_key_match(text + section_start, plus_start - section_start, &match);
			if(!success) {
				break;
			}
			section_start = plus_start + 1;
		} else if(idx == equal_start && idx > section_start) {
			success = nqiv_text_to_key_match(text + section_start, idx - section_start, &match);
			break;
		}
	}
	if(match.mode == NQIV_KEY_MATCH_MODE_KEY_MOD) {
		success = false;
	}
	if(success) {
		memcpy( &pair->match, &match, sizeof(nqiv_key_match) );
		pair->action = action;
		return equal_start + strlen(nqiv_keybind_action_names[action]) + 1;
	} else {
		return -1;
	}
}

bool nqiv_keybind_create_manager(nqiv_keybind_manager* manager, nqiv_log_ctx* logger, const int starting_array_length)
{
	assert(manager != NULL);
	nqiv_log_write(logger, NQIV_LOG_INFO, "Creating keybind manager.\n");
	nqiv_array* arrayptr = nqiv_array_create( starting_array_length * sizeof(nqiv_keybind_pair) );
	if(arrayptr == NULL) {
		return false;
	}
	manager->lookup = arrayptr;
	manager->logger = logger;
	return true;
}

bool nqiv_keybind_add(nqiv_keybind_manager* manager, const nqiv_key_match* match, const nqiv_key_action action)
{
	assert(manager != NULL);
	assert(manager->lookup != NULL);
	assert(match != NULL);
	nqiv_keybind_pair pair;
	memcpy( &pair.match, match, sizeof(nqiv_key_match) );
	pair.action = action;
	return nqiv_array_push_bytes( manager->lookup, &pair, sizeof(nqiv_keybind_pair) );
}

int nqiv_key_match_element_to_string(char* buf, const char* suffix)
{
	int pos = 0;
	strncpy( buf, suffix, strlen(suffix) );
	pos += strlen(suffix);
	buf[pos] = '+';
	pos += 1;
	return pos;
}

int nqiv_keymod_to_string(nqiv_keybind_pair* pair, char* buf)
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
	int pos = 0;
	for(idx = 0; mod_names[idx] != NULL; ++idx) {
		if( (pair->match.data.key.mod & mods[idx]) == mods[idx] && (pair->match.data.key.mod & anti_mods[idx]) == 0 ) {
			pos += nqiv_key_match_element_to_string(buf + pos, mod_names[idx]);
		}
	}
	return pos;
}

void nqiv_keybind_to_string(nqiv_keybind_pair* pair, char* buf)
{
	int pos = 0;
	pos = nqiv_keymod_to_string(pair, buf + pos);
	if( (pair->match.mode & NQIV_KEY_MATCH_MODE_KEY) != 0 ) {
		pos += nqiv_key_match_element_to_string(buf + pos, SDL_GetScancodeName(pair->match.data.key.scancode) );
	}
	if( (pair->match.mode & NQIV_KEY_MATCH_MODE_MOUSE_BUTTON) != 0 ) {
		assert(pair->match.data.mouse_button.clicks == 1 || pair->match.data.mouse_button.clicks == 2);
		pos += snprintf(buf + pos, NQIV_KEYBIND_STRLEN - pos, "mouse%d%s", pair->match.data.mouse_button.button, pair->match.data.mouse_button.clicks == 2 ? "double" : "");
	}
	if( (pair->match.mode & NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_FORWARD) != 0 ) {
		pos += nqiv_key_match_element_to_string(buf + pos, "scroll_forward");
	}
	if( (pair->match.mode & NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_BACKWARD) != 0 ) {
		pos += nqiv_key_match_element_to_string(buf + pos, "scroll_backward");
	}
	if( (pair->match.mode & NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_LEFT) != 0 ) {
		pos += nqiv_key_match_element_to_string(buf + pos, "scroll_left");
	}
	if( (pair->match.mode & NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_RIGHT) != 0 ) {
		pos += nqiv_key_match_element_to_string(buf + pos, "scroll_right");
	}
	strncpy( buf + pos, nqiv_keybind_action_names[pair->action], strlen(nqiv_keybind_action_names[pair->action]) );
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
		   ( (a->mode & NQIV_KEY_MATCH_MODE_KEY_MOD) == 0 || ( nqiv_compare_mod(a->data.key.mod, a->data.key.mod) ) ) &&
		   ( (a->mode & NQIV_KEY_MATCH_MODE_KEY) == 0 || (a->data.key.scancode == b->data.key.scancode) ) &&
		   ( (a->mode & NQIV_KEY_MATCH_MODE_MOUSE_BUTTON) == 0 || ( a->data.mouse_button.button == b->data.mouse_button.button && a->data.mouse_button.clicks == b->data.mouse_button.clicks ) );
}

nqiv_key_lookup_summary nqiv_keybind_lookup(nqiv_keybind_manager* manager, const nqiv_key_match* match, nqiv_queue* output)
{
	assert(manager != NULL);
	assert(manager->lookup != NULL);
	assert(match != NULL);

	nqiv_key_lookup_summary result = NQIV_KEY_LOOKUP_NOT_FOUND;
	const int lookup_len = manager->lookup->position / sizeof(nqiv_keybind_pair);
	int idx;
	for(idx = 0; idx < lookup_len; ++idx) {
		nqiv_keybind_pair pair = {.match = {0}, .action = -1};
		if( nqiv_array_get_bytes(manager->lookup, idx, sizeof(nqiv_keybind_pair), &pair) &&
			nqiv_keybind_compare_match(&pair.match, match) ) {
			if( output == NULL || !nqiv_queue_push(output, sizeof(nqiv_key_action), &pair.action) ) {
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
