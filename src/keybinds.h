#ifndef NQIV_KEYBINDS_H
#define NQIV_KEYBINDS_H

#include <stdbool.h>

#include <SDL.h>

typedef enum nqiv_key_action
{
	NQIV_KEY_ACTION_NONE = -1,
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

typedef enum nqiv_key_lookup_summary
{
	NQIV_KEY_LOOKUP_NOT_FOUND = 0,
	NQIV_KEY_LOOKUP_FAILURE = 1,
	NQIV_KEY_LOOKUP_FOUND = 2,
} nqiv_key_lookup_summary;

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

bool nqiv_keybind_create_manager(nqiv_keybind_manger* manager, nqiv_log_ctx* logger, const int starting_array_length);
/*void nqiv_keybind_remove(nqiv_keybind_manger manager, const SDL_Keysym* key, const nqiv_key_action action);*/
bool nqiv_keybind_add(nqiv_keybind_manger* manager, const SDL_Keysym* key, const nqiv_key_action action);
bool nqiv_keybind_add_from_text(nqiv_keybind_manger* manager, const char* text);
nqiv_key_lookup_summary nqiv_keybind_lookup(nqiv_keybind_manger* manager, const SDL_Keysym* key, nqiv_array* output);
/*nqiv_key_lookup_summary nqiv_keybind_lookup_text(nqiv_keybind_manger* manager, const char* key);*/
void nqiv_keybind_destroy_manager(nqiv_keybind_manger* manager);

#endif /* NQIV_KEYBINDS_H */
