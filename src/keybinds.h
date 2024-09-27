#ifndef NQIV_KEYBINDS_H
#define NQIV_KEYBINDS_H

#include <stdbool.h>

#include <SDL2/SDL.h>

#include "array.h"
#include "queue.h"
#include "logging.h"
#include "keyrate.h"

/*
 * Parse the keybind notation, and create an object from it that can be matched
 * to data from SDL events to translate them to key actions with certain
 * filtering/keyrate characteristics recognized by keyrate.h
 *
 * Parsing is case sensitive and retrieves match and action data separated by
 * '+' from two lists separated by '='. After successfully splitting each option
 * by '+', attempt to match them by whatever works first. Keymod, scroll, mouse
 * click, scancode for the match list. Filter/keyrate info, then action for the
 * second. See code or the 'help append keybind' command for more info. Keymods
 * must be mixed with other matches, but other matches cannot be mixed with one
 * another.
 *
 * Lookups are relatively simple. Build a match object from event info, and set
 * the mode to compare the relevant properties. If there is a match, a pointer
 * to the keybind pair will be pushed to the provided queue. Note that these
 * pointers could be broken if the list is reallocated- it shouldn't be since
 * its length is static and everything is currently handled by the master
 * thread, so they should all be processed before a potentially reallocation,
 * anyway.
 *
 */

#define NQIV_KEYBIND_STRLEN 1024 /* Keybind notation should not be longer than this. */

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
	NQIV_KEY_ACTION_SCALE_MODE_NEAREST,
	NQIV_KEY_ACTION_SCALE_MODE_LINEAR,
	NQIV_KEY_ACTION_SCALE_MODE_ANISOTROPIC,
	NQIV_KEY_ACTION_TOGGLE_SCALE_MODE,
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
	NQIV_KEY_ACTION_LENGTH,
} nqiv_key_action;

extern const char* const nqiv_keybind_action_names[];

typedef enum nqiv_key_lookup_summary
{
	NQIV_KEY_LOOKUP_NOT_FOUND = 0,
	/* If we found key actions, but failed to push them to the output queue. */
	NQIV_KEY_LOOKUP_FAILURE = 1,
	NQIV_KEY_LOOKUP_FOUND = 2,
} nqiv_key_lookup_summary;

/* Describe which data from a key match to use, or specify simple, unambiguous
 * operations like mouse wheel movement. */
typedef enum nqiv_key_match_mode
{
	NQIV_KEY_MATCH_MODE_NONE = 0,
	NQIV_KEY_MATCH_MODE_KEY = 1,
	NQIV_KEY_MATCH_MODE_KEY_MOD = 2, /* Modifier keys like shift, ctrl, etc' */
	NQIV_KEY_MATCH_MODE_MOUSE_BUTTON = 4,
	NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_FORWARD = 8,
	NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_BACKWARD = 16,
	NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_LEFT = 32,
	NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_RIGHT = 64,
} nqiv_key_match_mode;

typedef struct nqiv_key_match_data
{
	SDL_Keysym           key;
	SDL_MouseButtonEvent mouse_button;
} nqiv_key_match_data;

typedef struct nqiv_key_match
{
	nqiv_key_match_mode mode;
	nqiv_key_match_data data;
} nqiv_key_match;

typedef struct nqiv_keybind_pair
{
	nqiv_key_match        match;   /* Lookup data. */
	nqiv_keyrate_keystate keyrate; /* Key filtering data for this bind. */
	nqiv_key_action       action;  /* Action taken on match. */
} nqiv_keybind_pair;

typedef struct nqiv_keybind_manager
{
	nqiv_log_ctx* logger;
	/* Array of nqiv_keybind_pair */
	nqiv_array*   lookup;
	/* Maintain a list of nqiv_keybind_pair for simulated actions to use. */
	nqiv_keybind_pair simulated_lookup[NQIV_KEY_ACTION_LENGTH];
} nqiv_keybind_manager;

bool            nqiv_keybind_create_manager(nqiv_keybind_manager* manager,
                                            nqiv_log_ctx*         logger,
                                            const int             starting_array_length);
/*void nqiv_keybind_remove(nqiv_keybind_manager manager, const SDL_Keysym* key, const
 * nqiv_key_action action);*/
void            nqiv_key_print_actions(FILE* stream);
nqiv_key_action nqiv_text_to_key_action(const char* text, const int length);
int             nqiv_keybind_text_to_keybind(const char* original_text, nqiv_keybind_pair* pair);
bool            nqiv_keybind_add(nqiv_keybind_manager* manager, const nqiv_keybind_pair* pair);
bool            nqiv_keybind_to_string(const nqiv_keybind_pair* pair, char* buf);
nqiv_key_lookup_summary
nqiv_keybind_lookup(nqiv_keybind_manager* manager, const nqiv_key_match* match, nqiv_queue* output);
/*nqiv_key_lookup_summary nqiv_keybind_lookup_text(nqiv_keybind_manager* manager, const char*
 * key);*/
void nqiv_keybind_destroy_manager(nqiv_keybind_manager* manager);

#endif /* NQIV_KEYBINDS_H */
