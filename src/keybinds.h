#ifndef NQIV_KEYBINDS_H
#define NQIV_KEYBINDS_H

#include <stdbool.h>

#include <SDL2/SDL.h>

#include "array.h"
#include "queue.h"
#include "logging.h"

#define NQIV_KEYBIND_STRLEN 1024

typedef enum nqiv_key_action
{
	NQIV_KEY_ACTION_NONE = -1,
	NQIV_KEY_ACTION_QUIT = 0,
	NQIV_KEY_ACTION_PAGE_UP,
	NQIV_KEY_ACTION_PAGE_DOWN,
	NQIV_KEY_ACTION_TOGGLE_MONTAGE,
	NQIV_KEY_ACTION_SET_MONTAGE,
	NQIV_KEY_ACTION_SET_VIEWING,
	NQIV_KEY_ACTION_ZOOM_IN,
	NQIV_KEY_ACTION_ZOOM_OUT,
	NQIV_KEY_ACTION_PAN_LEFT,
	NQIV_KEY_ACTION_PAN_RIGHT,
	NQIV_KEY_ACTION_PAN_UP,
	NQIV_KEY_ACTION_PAN_DOWN,
	NQIV_KEY_ACTION_ZOOM_IN_MORE,
	NQIV_KEY_ACTION_ZOOM_OUT_MORE,
	NQIV_KEY_ACTION_PAN_LEFT_MORE,
	NQIV_KEY_ACTION_PAN_RIGHT_MORE,
	NQIV_KEY_ACTION_PAN_UP_MORE,
	NQIV_KEY_ACTION_PAN_DOWN_MORE,
	NQIV_KEY_ACTION_PAN_CENTER,
	NQIV_KEY_ACTION_IMAGE_PREVIOUS,
	NQIV_KEY_ACTION_IMAGE_NEXT,
	NQIV_KEY_ACTION_MONTAGE_LEFT,
	NQIV_KEY_ACTION_MONTAGE_RIGHT,
	NQIV_KEY_ACTION_MONTAGE_UP,
	NQIV_KEY_ACTION_MONTAGE_DOWN,
	NQIV_KEY_ACTION_MONTAGE_START,
	NQIV_KEY_ACTION_MONTAGE_END,
	NQIV_KEY_ACTION_TOGGLE_STRETCH,
	NQIV_KEY_ACTION_STRETCH,
	NQIV_KEY_ACTION_KEEP_ASPECT_RATIO,
	NQIV_KEY_ACTION_FIT,
	NQIV_KEY_ACTION_ACTUAL_SIZE,
	NQIV_KEY_ACTION_KEEP_FIT,
	NQIV_KEY_ACTION_KEEP_ACTUAL_SIZE,
	NQIV_KEY_ACTION_KEEP_CURRENT_ZOOM,
	NQIV_KEY_ACTION_TOGGLE_KEPT_ZOOM,
	NQIV_KEY_ACTION_IMAGE_MARK_TOGGLE,
	NQIV_KEY_ACTION_IMAGE_MARK,
	NQIV_KEY_ACTION_IMAGE_UNMARK,
	NQIV_KEY_ACTION_PRINT_MARKED,
	NQIV_KEY_ACTION_MONTAGE_SELECT_AT_MOUSE,
	NQIV_KEY_ACTION_IMAGE_MARK_AT_MOUSE,
	NQIV_KEY_ACTION_IMAGE_UNMARK_AT_MOUSE,
	NQIV_KEY_ACTION_IMAGE_MARK_TOGGLE_AT_MOUSE,
	NQIV_KEY_ACTION_START_MOUSE_PAN,
	NQIV_KEY_ACTION_END_MOUSE_PAN,
	NQIV_KEY_ACTION_IMAGE_ZOOM_IN,
	NQIV_KEY_ACTION_IMAGE_ZOOM_OUT,
	NQIV_KEY_ACTION_IMAGE_ZOOM_IN_MORE,
	NQIV_KEY_ACTION_IMAGE_ZOOM_OUT_MORE,
	NQIV_KEY_ACTION_RELOAD,
	NQIV_KEY_ACTION_MAX = NQIV_KEY_ACTION_RELOAD,
} nqiv_key_action;

extern const char* nqiv_keybind_action_names[];

typedef enum nqiv_key_lookup_summary
{
	NQIV_KEY_LOOKUP_NOT_FOUND = 0,
	NQIV_KEY_LOOKUP_FAILURE = 1,
	NQIV_KEY_LOOKUP_FOUND = 2,
} nqiv_key_lookup_summary;

typedef enum nqiv_key_match_mode
{
	NQIV_KEY_MATCH_MODE_NONE = 0,
	NQIV_KEY_MATCH_MODE_KEY = 1,
	NQIV_KEY_MATCH_MODE_KEY_MOD = 2,
	NQIV_KEY_MATCH_MODE_MOUSE_BUTTON = 4,
	NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_FORWARD = 8,
	NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_BACKWARD = 16,
	NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_LEFT = 32,
	NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_RIGHT = 64,
} nqiv_key_match_mode;

typedef struct nqiv_key_match_data
{
	SDL_Keysym key;
	SDL_MouseButtonEvent mouse_button;
} nqiv_key_match_data;

typedef struct nqiv_key_match
{
	nqiv_key_match_mode mode;
	nqiv_key_match_data data;
} nqiv_key_match;

typedef struct nqiv_keybind_pair
{
	nqiv_key_match match;
	nqiv_key_action action;
} nqiv_keybind_pair;

typedef struct nqiv_keybind_manager
{
	nqiv_log_ctx* logger;
	nqiv_array* lookup;
} nqiv_keybind_manager;

bool nqiv_keybind_create_manager(nqiv_keybind_manager* manager, nqiv_log_ctx* logger, const int starting_array_length);
/*void nqiv_keybind_remove(nqiv_keybind_manager manager, const SDL_Keysym* key, const nqiv_key_action action);*/
void nqiv_key_print_actions(FILE* stream);
nqiv_key_action nqiv_text_to_key_action(const char* text);
int nqiv_keybind_text_to_keybind(char* text, nqiv_keybind_pair* pair);
bool nqiv_keybind_add(nqiv_keybind_manager* manager, const nqiv_key_match* key, const nqiv_key_action action);
void nqiv_keybind_to_string(nqiv_keybind_pair* pair, char* buf);
nqiv_key_lookup_summary nqiv_keybind_lookup(nqiv_keybind_manager* manager, const nqiv_key_match* match, nqiv_queue* output);
/*nqiv_key_lookup_summary nqiv_keybind_lookup_text(nqiv_keybind_manager* manager, const char* key);*/
void nqiv_keybind_destroy_manager(nqiv_keybind_manager* manager);

#endif /* NQIV_KEYBINDS_H */
