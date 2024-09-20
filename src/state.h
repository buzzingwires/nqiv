#ifndef NQIV_STATE_H
#define NQIV_STATE_H

#include "platform.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <limits.h>

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

/* Common, sufficient queue length in respective units. */
#define STARTING_QUEUE_LENGTH 512
/* Bins for thread priority queue. This is one bigger than necessary so preload events can be placed
 * after events for currently-displayed images. See nqiv_event_priority. */
#define THREAD_QUEUE_BIN_COUNT 10
/* min_add_count for thread queue bin arrays. It's pretty big to prevent a lot of reallocations. */
#define THREAD_QUEUE_ADD_COUNT 10000
/* Max length in units for thread queue bin arrays. Will influence max_data_length. If a queue
 * reaches this size, there's probably something wrong. */
#define THREAD_QUEUE_MAX_LENGTH 1000000
/* Used to calculate the dimensions of alpha background checks. Divided against average of window
 * width and height */
#define ALPHA_BACKGROUND_CHECKER_PROPORTION 32
/* Sufficient for PATH_MAX and other info. */
#define WINDOW_TITLE_LEN (1024 + PATH_MAX)

typedef enum nqiv_event_priority
{
	/* When told to quit, do so immediately */
	NQIV_EVENT_PRIORITY_QUIT = 0,
	/* Fulfill animation frames early for low latency. */
	NQIV_EVENT_PRIORITY_IMAGE_LOAD_ANIMATION = 1,
	/* Unload old thumbnails right away when a new size is needed. */
	NQIV_EVENT_PRIORITY_REATTEMPT_THUMBNAIL = 2,
	/* Prune old stuff before loading new. */
	NQIV_EVENT_PRIORITY_PRUNE = 3,
	/* Loading a displayed image should be higher priority than thumbnails. */
	NQIV_EVENT_PRIORITY_IMAGE_LOAD = 4,
	/* Loading thumbnails from image data is slower than thumbnail files. Do it first. */
	NQIV_EVENT_PRIORITY_THUMBNAIL_LOAD_EPHEMERAL = 5,
	/* First attempt to load a normal thumbnail file. */
	NQIV_EVENT_PRIORITY_THUMBNAIL_LOAD = 6,
	/* Then try to create the file if it's not available. */
	NQIV_EVENT_PRIORITY_THUMBNAIL_SAVE_LOAD_FAIL = 7,
	/* Finally save a thumbnail we'll never even try to use. */
	NQIV_EVENT_PRIORITY_THUMBNAIL_SAVE_LOAD_NO = 8,
} nqiv_event_priority;

/* When we load an image, we can decide how to adjust the zoom for it. */
typedef enum nqiv_zoom_default
{
	NQIV_ZOOM_DEFAULT_UNKNOWN = -1,
	NQIV_ZOOM_DEFAULT_KEEP = 0,  /* Keep zoom settings from last image. (Don't adjust.) */
	NQIV_ZOOM_DEFAULT_FIT = 1,   /* Fit image to screen. */
	NQIV_ZOOM_DEFAULT_ACTUAL = 2 /* Show image in actual size. */
} nqiv_zoom_default;

extern const char* const   nqiv_zoom_default_names[];

extern const char* const   nqiv_texture_scale_mode_names[];
extern const SDL_ScaleMode nqiv_texture_scale_modes[];

/* The central object. Contains everything else and some extra state used by the same loop. */
struct nqiv_state
{
	nqiv_log_ctx logger;
	/* Just used for record keeping so we can print the names of logging streams. */
	nqiv_array*          logger_stream_names;
	nqiv_image_manager   images;
	nqiv_pruner          pruner;
	nqiv_keybind_manager keybinds;
	nqiv_keyrate_manager keystates;
	nqiv_montage_state   montage;
	nqiv_cmd_manager     cmds;
	/* Communicates events to workers. */
	nqiv_priority_queue thread_queue;
	/* Pending keyboard actions. */
	nqiv_queue key_actions;
	/* Have we initialized SDL. If so, do special cleanup stuff. */
	bool          SDL_inited;
	SDL_Window*   window;
	SDL_Renderer* renderer;
	SDL_Texture*  texture_background;
	/* Box around selected montage thumbnail. */
	SDL_Texture* texture_montage_selection;
	/* Dashed box around marked montage thumbnail. */
	SDL_Texture* texture_montage_mark;
	/* While an image load is pending, show this in its place unless show_loading_indicator is
	 * false. */
	SDL_Texture* texture_montage_unloaded_background;
	/* Shown in place of an image that fails to load. */
	SDL_Texture* texture_montage_error_background;
	/* Background shown behind transparent image- may be checkered. */
	SDL_Texture*  texture_alpha_background;
	SDL_ScaleMode texture_scale_mode;
	/* Track dimensions of alpha background, since they may need to be updated. */
	int alpha_background_width;
	int alpha_background_height;
	/* SDL events returned to master from workers. */
	Uint32 thread_event_number;
	/* SDL events returned to master from configuration. */
	Uint32 cfg_event_number;
	/* Number of worker threads. */
	int thread_count;
	/* Threads will update the master after processing this many events. 0 to process all. */
	int thread_event_interval;
	int vips_threads;
	/* The transaction group of an event is compared against this, protected by a mutex lock. If the
	 * event is less than the current number, it is considered out of date and discarded. An event
	 * with a transaction group of -1 is never out of date. This feature primarily exists to solve
	 * the problem of events still being queued for images that are no longer visible. */
	int64_t thread_event_transaction_group;
	/* In SDL ticks (milliseconds) Check if prune_delay has passed for each render_and_update */
	Uint64 time_of_last_prune;
	Uint64 prune_delay;
	/* Base amount worker threads sleep between updates. */
	int        extra_wakeup_delay;
	omp_lock_t thread_event_transaction_group_lock;
	/* Used to tell when the display needs to be redrawn. */
	bool render_cleared;
	/* Is montage mode? Otherwise image mode. */
	bool in_montage;
	/* Try to fill entire display area with image, disregarding aspect ratio. */
	bool stretch_images;
	/* Used to handle edge case of first frame being passed, but not being rendered yet. Otherwise,
	 * zoom defaults may not be set. */
	bool              first_frame_pending;
	nqiv_zoom_default zoom_default;
	/* Should we pan the image according to mouse motion right now? */
	bool is_mouse_panning;
	/* If we get an image bigger than the max texture size, just shrink the entire thing once, and
	 * use that. */
	bool no_resample_oversized;
	bool show_loading_indicator;
	/* Are we currently waiting for an image to load or render? */
	bool      is_loading;
	SDL_Color background_color;
	SDL_Color error_color;
	SDL_Color loading_color;
	SDL_Color selection_color;
	SDL_Color mark_color;
	SDL_Color alpha_checker_color_one;
	SDL_Color alpha_checker_color_two;
	/* Whether to quit if the command parser has an error parsing or storing values. Otherwise
	 * attempt to continue. */
	bool cmd_parse_error_quit;
	bool cmd_apply_error_quit;
};

/* Check if logger has error message. If it does, print it and return false. */
bool nqiv_check_and_print_logger_error(const nqiv_log_ctx* logger);
/* Open stream if necessary, add it to logger and its path (or stderr/stderr) to logger_stream_names
 */
bool nqiv_add_logger_path(nqiv_state* state, const char* path);
void nqiv_state_set_default_colors(nqiv_state* state);
/* Recreation will check for existence, remove it if present, then create. */
bool              nqiv_state_create_thumbnail_selection_texture(nqiv_state* state);
bool              nqiv_state_recreate_thumbnail_selection_texture(nqiv_state* state);
bool              nqiv_state_create_mark_texture(nqiv_state* state);
bool              nqiv_state_recreate_mark_texture(nqiv_state* state);
bool              nqiv_state_create_alpha_background_texture(nqiv_state* state);
bool              nqiv_state_recreate_all_alpha_background_textures(nqiv_state* state);
bool              nqiv_state_update_alpha_background_dimensions(nqiv_state* state,
                                                                const int   alpha_background_width,
                                                                const int   alpha_background_height);
bool              nqiv_state_create_single_color_texture(nqiv_state*      state,
                                                         const SDL_Color* color,
                                                         SDL_Texture**    texture);
bool              nqiv_state_recreate_single_color_texture(nqiv_state*      state,
                                                           const SDL_Color* color,
                                                           SDL_Texture**    texture);
bool              nqiv_state_recreate_background_texture(nqiv_state* state);
bool              nqiv_state_recreate_error_texture(nqiv_state* state);
bool              nqiv_state_recreate_loading_texture(nqiv_state* state);
nqiv_zoom_default nqiv_text_to_zoom_default(const char* text);
bool              nqiv_text_to_scale_mode(const char* text, SDL_ScaleMode* sm);
const char*       nqiv_scale_mode_to_text(const SDL_ScaleMode sm);

#endif /* NQIV_STATE_H */
