#ifndef NQIV_STATE_H
#define NQIV_STATE_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "typedefs.h"
#include "logging.h"
#include "image.h"
#include "keybinds.h"
#include "keyrate.h"
#include "montage.h"
#include "queue.h"
#include "cmd.h"
#include "pruner.h"

#include <SDL2/SDL.h>
#include <omp.h>

#define STARTING_QUEUE_LENGTH 1024
#define THREAD_QUEUE_BIN_COUNT 6
#define ALPHA_BACKGROUND_CHECKER_PROPORTION 32

typedef enum nqiv_zoom_default
{
	NQIV_ZOOM_DEFAULT_UNKNOWN = -1,
	NQIV_ZOOM_DEFAULT_KEEP = 0,
	NQIV_ZOOM_DEFAULT_FIT = 1,
	NQIV_ZOOM_DEFAULT_ACTUAL = 2
} nqiv_zoom_default;

extern const char* nqiv_zoom_default_names[];

struct nqiv_state
{
	nqiv_log_ctx logger;
	nqiv_array* logger_stream_names;
	nqiv_image_manager images;
	nqiv_pruner pruner;
	nqiv_keybind_manager keybinds;
	nqiv_keyrate_manager keystates;
	nqiv_montage_state montage;
	nqiv_cmd_manager cmds;
	int queue_length;
	nqiv_priority_queue thread_queue;
	nqiv_queue key_actions;
	bool SDL_inited;
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* texture_background;
	SDL_Texture* texture_montage_selection;
	SDL_Texture* texture_montage_mark;
	SDL_Texture* texture_montage_unloaded_background;
	SDL_Texture* texture_montage_error_background;
	SDL_Texture* texture_alpha_background;
	int alpha_background_width;
	int alpha_background_height;
	Uint32 thread_event_number;
	Uint32 cfg_event_number;
	int thread_count;
	int thread_event_interval;
	int vips_threads;
	int64_t thread_event_transaction_group;
	Uint64 time_of_last_prune;
	Uint64 prune_delay;
	int extra_wakeup_delay;
	omp_lock_t thread_event_transaction_group_lock;
	omp_lock_t** thread_locks;
	bool render_cleared;
	bool in_montage;
	bool stretch_images;
	bool first_frame_pending;
	nqiv_zoom_default zoom_default;
	char* window_title;
	size_t window_title_size;
	bool no_resample_oversized;
	bool show_loading_indicator;
	SDL_Color background_color;
	SDL_Color error_color;
	SDL_Color loading_color;
	SDL_Color selection_color;
	SDL_Color mark_color;
	SDL_Color alpha_checker_color_one;
	SDL_Color alpha_checker_color_two;
	bool cmd_parse_error_quit;
	bool cmd_apply_error_quit;
};


bool nqiv_check_and_print_logger_error(nqiv_log_ctx* logger);
bool nqiv_add_logger_path(nqiv_state* state, const char* path);
void nqiv_state_set_default_colors(nqiv_state* state);
bool nqiv_state_create_thumbnail_selection_texture(nqiv_state* state);
bool nqiv_state_recreate_thumbnail_selection_texture(nqiv_state* state);
bool nqiv_state_create_mark_texture(nqiv_state* state);
bool nqiv_state_recreate_mark_texture(nqiv_state* state);
bool nqiv_state_create_alpha_background_texture(nqiv_state* state);
bool nqiv_state_recreate_all_alpha_background_textures(nqiv_state* state);
bool nqiv_state_update_alpha_background_dimensions(nqiv_state* state, const int alpha_background_width, const int alpha_background_height);
bool nqiv_state_create_single_color_texture(nqiv_state* state, const SDL_Color* color, SDL_Texture** texture);
bool nqiv_state_recreate_single_color_texture(nqiv_state* state, const SDL_Color* color, SDL_Texture** texture);
bool nqiv_state_recreate_background_texture(nqiv_state* state);
bool nqiv_state_recreate_error_texture(nqiv_state* state);
bool nqiv_state_recreate_loading_texture(nqiv_state* state);
bool nqiv_state_expand_queues(nqiv_state* state);
nqiv_zoom_default nqiv_text_to_zoom_default(const char* text);

#endif /* NQIV_STATE_H */
