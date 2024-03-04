#ifndef NQIV_STATE_H
#define NQIV_STATE_H

#include <stddef.h>
#include <stdbool.h>

#include "logging.h"
#include "image.h"
#include "keybinds.h"
#include "keyrate.h"
#include "montage.h"
#include "queue.h"

#include <SDL2/SDL.h>
#include <omp.h>

typedef struct nqiv_state
{
	bool read_from_stdin;
	nqiv_log_ctx logger;
	nqiv_image_manager images;
	nqiv_keybind_manager keybinds;
	nqiv_keyrate_manager keystates;
	nqiv_montage_state montage;
	nqiv_queue thread_queue;
	bool SDL_inited;
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* texture_background;
	SDL_Texture* texture_montage_selection;
	SDL_Texture* texture_montage_alpha_background;
	SDL_Texture* texture_montage_unloaded_background;
	SDL_Texture* texture_montage_error_background;
	SDL_Texture* texture_alpha_background;
	Uint32 thread_event_number;
	int thread_count;
	omp_lock_t* thread_locks;
	bool in_montage;
	bool stretch_images;
	char* window_title;
	size_t window_title_size;
	bool no_resample_oversized;
} nqiv_state;

#endif /* NQIV_STATE_H */
