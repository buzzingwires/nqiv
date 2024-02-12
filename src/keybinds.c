#include <stdbool.h>
#include <assert.h>

#include <SDL2/SDL.h>

#include "logging.h"
#include "array.h"
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

nqiv_key_action nqiv_text_to_key_action(const char* text, const int length)
{
	if(strncmp(text, "quit", length) == 0 ) {
		return NQIV_KEY_ACTION_QUIT;
	} else if(strncmp(text, "next", length) == 0) {
		return NQIV_KEY_ACTION_NEXT;
	} else if(strncmp(text, "previous", length) == 0) {
		return NQIV_KEY_ACTION_PREVIOUS;
	} else if(strncmp(text, "page_up", length) == 0) {
		return NQIV_KEY_ACTION_PAGE_UP;
	} else if(strncmp(text, "page_down", length) == 0) {
		return NQIV_KEY_ACTION_PAGE_DOWN;
	} else if(strncmp(text, "toggle_montage", length) == 0) {
		return NQIV_KEY_ACTION_TOGGLE_MONTAGE;
	} else if(strncmp(text, "set_montage", length) == 0) {
		return NQIV_KEY_ACTION_SET_MONTAGE;
	} else if(strncmp(text, "set_viewing", length) == 0) {
		return NQIV_KEY_ACTION_SET_VIEWING;
	} else if(strncmp(text, "zoom_in", length) == 0) {
		return NQIV_KEY_ACTION_ZOOM_IN;
	} else if(strncmp(text, "zoom_out", length) == 0) {
		return NQIV_KEY_ACTION_ZOOM_OUT;
	} else if(strncmp(text, "left", length) == 0) {
		return NQIV_KEY_ACTION_LEFT;
	} else if(strncmp(text, "right", length) == 0) {
		return NQIV_KEY_ACTION_RIGHT;
	} else if(strncmp(text, "up", length) == 0) {
		return NQIV_KEY_ACTION_UP;
	} else if(strncmp(text, "down", length) == 0) {
		return NQIV_KEY_ACTION_DOWN;
	} else if(strncmp(text, "toggle_stretch", length) == 0) {
		return NQIV_KEY_ACTION_TOGGLE_STRETCH;
	} else if(strncmp(text, "stretch", length) == 0) {
		return NQIV_KEY_ACTION_STRETCH;
	} else if(strncmp(text, "keep_aspect_ratio", length) == 0) {
		return NQIV_KEY_ACTION_KEEP_ASPECT_RATIO;
	} else if(strncmp(text, "reload", length) == 0) {
		return NQIV_KEY_ACTION_RELOAD;
	} else {
		return NQIV_KEY_ACTION_NONE;
	}
}

bool nqiv_text_to_keysym(char* text, const int length, SDL_Keysym* key)
{
	bool success = true;
	if( strncmp(text, "lshift", length) == 0 ) {
		key->mod |= KMOD_LSHIFT;
	} else if( strncmp(text, "rshift", length) == 0 ) {
		key->mod |= KMOD_RSHIFT;
	} else if( strncmp(text, "lctrl", length) == 0 ) {
		key->mod |= KMOD_LCTRL;
	} else if( strncmp(text, "rctrl", length) == 0 ) {
		key->mod |= KMOD_RCTRL;
	} else if( strncmp(text, "lalt", length) == 0 ) {
		key->mod |= KMOD_LALT;
	} else if( strncmp(text, "ralt", length) == 0 ) {
		key->mod |= KMOD_RALT;
	} else if( strncmp(text, "lgui", length) == 0 ) {
		key->mod |= KMOD_LGUI;
	} else if( strncmp(text, "rgui", length) == 0 ) {
		key->mod |= KMOD_RGUI;
	} else if( strncmp(text, "num", length) == 0 ) {
		key->mod |= KMOD_NUM;
	} else if( strncmp(text, "caps", length) == 0 ) {
		key->mod |= KMOD_CAPS;
	} else if( strncmp(text, "mode", length) == 0 ) {
		key->mod |= KMOD_MODE;
	} else if( strncmp(text, "scroll", length) == 0 ) {
		key->mod |= KMOD_SCROLL;
	} else if( strncmp(text, "lctrl", length) == 0 ) {
		key->mod |= KMOD_LCTRL;
	} else if( strncmp(text, "ctrl", length) == 0 ) {
		key->mod |= KMOD_CTRL;
	} else if( strncmp(text, "shift", length) == 0 ) {
		key->mod |= KMOD_SHIFT;
	} else if( strncmp(text, "alt", length) == 0 ) {
		key->mod |= KMOD_ALT;
	} else if( strncmp(text, "gui", length) == 0 ) {
		key->mod |= KMOD_GUI;
	} else {
		const char endchar = text[length];
		text[length] = '\0';
		const SDL_Scancode sc = SDL_GetScancodeFromName(text);
		assert(length >= 0);
		assert(strlen(text) == (size_t)length);
		text[length] = endchar;
		if(sc == SDL_SCANCODE_UNKNOWN) {
			success = false;
		} else {
			key->scancode = sc;
		}
	}
	return success;
}

bool nqiv_keybind_text_to_keybind(char* text, nqiv_keybind_pair* pair)
{
	const size_t textlen = strlen(text);
	if(textlen == 0) {
		return false;
	}
	const int equal_start = nqiv_findchar(text, '=', textlen - 1, -1);
	if( equal_start == -1 || textlen <= (size_t)equal_start + 1 ) {
		return false;
	}
	const nqiv_key_action action = nqiv_text_to_key_action(text + equal_start + 1, textlen - equal_start - 1);
	if(action == NQIV_KEY_ACTION_NONE) {
		return false;
	}
	int idx;
	int section_start;
	bool success = true;
	SDL_Keysym tmpkey = {0};
	tmpkey.scancode = SDL_SCANCODE_UNKNOWN;
	SDL_Scancode sc = tmpkey.scancode;
	memset( &tmpkey, 0, sizeof(SDL_Keysym) );
	for(idx = 0, section_start = 0; idx <= equal_start; ++idx) {
		if(text[idx] == '+') {
			success = nqiv_text_to_keysym(text + section_start, idx - section_start, &tmpkey);
			if(!success) {
				break;
			}
			section_start = idx + 1;
		} else if(text[idx] == '=') {
			success = nqiv_text_to_keysym(text + section_start, idx - section_start, &tmpkey);
			break;
		}
		if(sc != SDL_SCANCODE_UNKNOWN && tmpkey.scancode != sc) {
			success = false;
			break;
		} else {
			sc = tmpkey.scancode;
		}
	}
	if(success) {
		memcpy( &pair->key, &tmpkey, sizeof(SDL_Keysym) );
		pair->action = action;
	}
	return success;
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

bool nqiv_keybind_add(nqiv_keybind_manager* manager, const SDL_Keysym* key, const nqiv_key_action action)
{
	assert(manager != NULL);
	assert(manager->lookup != NULL);
	assert(key != NULL);
	nqiv_keybind_pair pair;
	memcpy( &pair.key, key, sizeof(SDL_Keysym) );
	pair.action = action;
	return nqiv_array_push_bytes( manager->lookup, &pair, sizeof(nqiv_keybind_pair) );
}

bool nqiv_keybind_add_from_text(nqiv_keybind_manager* manager, char* text)
{
	bool success = false;
	nqiv_log_write(manager->logger, NQIV_LOG_DEBUG, "Adding keybind %s.\n", text);
	nqiv_keybind_pair pair = {0};
	if( nqiv_keybind_text_to_keybind(text, &pair) ) {
		nqiv_keybind_add(manager, &pair.key, pair.action);
		success = true;
	}
	return success;
}

bool nqiv_compare_mod(const Uint16 a, const Uint16 b)
{
	const Uint16 ac = a & ~KMOD_GUI & ~KMOD_SCROLL & ~KMOD_NUM;
	const Uint16 bc = b & ~KMOD_GUI & ~KMOD_SCROLL & ~KMOD_NUM;
	return (ac & KMOD_SHIFT && bc & KMOD_SHIFT) ||
		(ac & KMOD_CTRL && bc & KMOD_CTRL) ||
		(ac & KMOD_ALT && bc & KMOD_ALT) ||
		(ac & KMOD_GUI && bc & KMOD_GUI) ||
		(ac & KMOD_NUM && bc & KMOD_NUM) ||
		(ac & KMOD_CAPS && bc & KMOD_CAPS) ||
		(ac & KMOD_MODE && bc & KMOD_MODE) ||
		(ac & KMOD_SCROLL && bc & KMOD_SCROLL) ||
		(ac == 0 && bc == 0);
}

nqiv_key_lookup_summary nqiv_keybind_lookup(nqiv_keybind_manager* manager, const SDL_Keysym* key, nqiv_array* output)
{
	assert(manager != NULL);
	assert(manager->lookup != NULL);
	assert(key != NULL);

	nqiv_key_lookup_summary result = NQIV_KEY_LOOKUP_NOT_FOUND;
	const int lookup_len = manager->lookup->position / sizeof(nqiv_keybind_pair);
	int idx;
	for(idx = 0; idx < lookup_len; ++idx) {
		nqiv_keybind_pair pair;
		if( nqiv_array_get_bytes(manager->lookup, idx, sizeof(nqiv_keybind_pair), &pair) &&
			pair.key.scancode == key->scancode && nqiv_compare_mod(pair.key.mod, key->mod) ) {
			if( output == NULL || !nqiv_array_push_bytes( output, &pair.action, sizeof(nqiv_key_action) ) ) {
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
