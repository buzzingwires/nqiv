#include <stdio.h>
#include <errno.h>
#include <assert.h>

#include <vips/vips.h>
#include <SDL2/SDL.h>
#include <omp.h>

#include "platform.h"
#include "logging.h"
#include "image.h"
#include "worker.h"
#include "array.h"
#include "queue.h"
#include "event.h"
#include "montage.h"
#include "keybinds.h"
#include "keyrate.h"
#include "state.h"
#include "cmd.h"

#define OPTPARSE_IMPLEMENTATION
#define OPTPARSE_API static
#include "optparse.h"

void nqiv_close_log_streams(nqiv_log_ctx* logger)
{
	const int streams_len = logger->streams->position / sizeof(FILE*);
	int idx;
	FILE* ptr;
	for(idx = 0; idx < streams_len; ++idx) {
		if( nqiv_array_get_bytes(logger->streams, idx, sizeof(FILE*), &ptr) && ptr != stderr && ptr != stdout ) {
			fclose(ptr);
		}
	}
}

void nqiv_state_clear_thread_locks(nqiv_state* state)
{
	if(state->thread_locks != NULL) {
		int idx;
		for(idx = 0; idx < state->thread_count; ++idx) {
			if(state->thread_locks[idx] != NULL) {
				omp_unset_lock(state->thread_locks[idx]);
				omp_destroy_lock(state->thread_locks[idx]);
				memset( state->thread_locks[idx], 0, sizeof(omp_lock_t) );
				free(state->thread_locks[idx]);
			}
		}
		memset(state->thread_locks, 0, sizeof(omp_lock_t*) * state->thread_count);
		free(state->thread_locks);
		state->thread_locks = NULL;
	}
}

void nqiv_state_clear(nqiv_state* state)
{
	nqiv_priority_queue_destroy(&state->thread_queue);
	if(state->key_actions.array != NULL) {
		nqiv_queue_destroy(&state->key_actions);
	}
	if(state->cmds.buffer != NULL) {
		nqiv_cmd_manager_destroy(&state->cmds);
	}
	if(state->keybinds.lookup != NULL) {
		nqiv_keybind_destroy_manager(&state->keybinds);
	}
	if(state->images.images != NULL) {
		nqiv_image_manager_destroy(&state->images);
	}
	if(state->pruner.pruners != NULL) {
		nqiv_pruner_destroy(&state->pruner);
	}
	if(state->logger.streams != NULL) {
		nqiv_close_log_streams(&state->logger);
		nqiv_log_destroy(&state->logger);
	}
	if(state->texture_background != NULL) {
		SDL_DestroyTexture(state->texture_background);
	}
	if(state->texture_montage_selection != NULL) {
		SDL_DestroyTexture(state->texture_montage_selection);
	}
	if(state->texture_montage_mark != NULL) {
		SDL_DestroyTexture(state->texture_montage_mark);
	}
	if(state->texture_montage_alpha_background != NULL) {
		SDL_DestroyTexture(state->texture_montage_alpha_background);
	}
	if(state->texture_alpha_background != NULL) {
		SDL_DestroyTexture(state->texture_alpha_background);
	}
	if(state->renderer != NULL) {
		SDL_DestroyRenderer(state->renderer);
	}
	if(state->window != NULL) {
		SDL_DestroyWindow(state->window);
	}
	if(state->SDL_inited) {
		SDL_Quit();
	}
	if(state->thread_locks != NULL) {
		nqiv_state_clear_thread_locks(state);
	}
	omp_destroy_lock(&state->thread_event_transaction_group_lock);
	if(state->window_title != NULL) {
		free(state->window_title);
	}
	memset( state, 0, sizeof(nqiv_state) );
	vips_shutdown();
}

void nqiv_set_keyrate_defaults(nqiv_keyrate_manager* manager)
{
	manager->settings.start_delay = 0;
	manager->settings.consecutive_delay = 35;
	manager->settings.delay_accel = 10;
	manager->settings.minimum_delay = 5;
	manager->send_on_up = true;
	manager->send_on_down = false;
	nqiv_key_action down_actions[] = {
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
		NQIV_KEY_ACTION_PAGE_UP,
		NQIV_KEY_ACTION_PAGE_DOWN,
		NQIV_KEY_ACTION_MONTAGE_LEFT,
		NQIV_KEY_ACTION_MONTAGE_RIGHT,
		NQIV_KEY_ACTION_MONTAGE_UP,
		NQIV_KEY_ACTION_MONTAGE_DOWN,
		NQIV_KEY_ACTION_MONTAGE_START,
		NQIV_KEY_ACTION_MONTAGE_END,
		NQIV_KEY_ACTION_NONE
	};
	int idx;
	for(idx = 0; down_actions[idx] != NQIV_KEY_ACTION_NONE; ++idx) {
		nqiv_keyrate_keystate* state = &manager->states[ down_actions[idx] ];
		state->send_on_up = NQIV_KEYRATE_DENY;
		state->send_on_down = NQIV_KEYRATE_ALLOW;
	}
}

bool nqiv_setup_sdl(nqiv_state* state)
{
	if( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0 ) {
		nqiv_log_write( &state->logger, NQIV_LOG_ERROR, "Failed to init SDL. SDL Error: %s\n", SDL_GetError() );
		return false;
	}
	state->SDL_inited = true;
	state->window = SDL_CreateWindow("nqiv", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if(state->window == NULL) {
		nqiv_log_write( &state->logger, NQIV_LOG_ERROR, "Failed to create SDL Window. SDL Error: %s\n", SDL_GetError() );
		return false;
	}
	state->renderer = SDL_CreateRenderer(state->window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
	if(state->renderer == NULL) {
		nqiv_log_write( &state->logger, NQIV_LOG_ERROR, "Failed to create SDL Renderer. SDL Error: %s\n", SDL_GetError() );
		return false;
	}
	if( SDL_SetRenderDrawColor(state->renderer, 0, 0, 0, 255) != 0 ) {
		nqiv_log_write( &state->logger, NQIV_LOG_ERROR, "Failed to set SDL Renderer draw color. SDL Error: %s\n", SDL_GetError() );
		return false;
	}

	if( !nqiv_state_create_single_color_texture(state, &state->background_color, &state->texture_background) ) {
		return false;
	}
	if( !nqiv_state_create_single_color_texture(state, &state->loading_color, &state->texture_montage_unloaded_background) ) {
		return false;
	}
	if( !nqiv_state_create_single_color_texture(state, &state->error_color, &state->texture_montage_error_background) ) {
		return false;
	}
	if( !nqiv_state_create_thumbnail_selection_texture(state) ) {
		return false;
	}
	if( !nqiv_state_create_mark_texture(state) ) {
		return false;
	}

	if( !nqiv_state_create_montage_alpha_background_texture(state) ) {
		return false;
	}

	if( !nqiv_state_create_alpha_background_texture(state) ) {
		return false;
	}

	if(state->thread_event_number != 0) {
		state->thread_event_number = SDL_RegisterEvents(1);
		if(state->thread_event_number == 0xFFFFFFFF) {
			nqiv_log_write( &state->logger, NQIV_LOG_ERROR, "Failed to create SDL event for messages from threads. SDL Error: %s\n", SDL_GetError() );
			return false;
		}
	}

	if(state->cfg_event_number != 0) {
		state->cfg_event_number = SDL_RegisterEvents(1);
		if(state->cfg_event_number == 0xFFFFFFFF) {
			nqiv_log_write( &state->logger, NQIV_LOG_ERROR, "Failed to create SDL event for messages from threads. SDL Error: %s\n", SDL_GetError() );
			return false;
		}
	}

	SDL_Delay(1); /* So that we are completely guaranteed to have at least one ticks passed for SDL. */
	return true;
}

void nqiv_setup_montage(nqiv_state* state)
{
	state->montage.logger = &state->logger;
	state->montage.window = state->window;
	state->montage.images = &state->images;
	nqiv_montage_calculate_dimensions(&state->montage);
}

void nqiv_act_on_thread_locks( nqiv_state* state, void (*action)(omp_lock_t *lock) )
{
	int idx;
	for(idx = 0; idx < state->thread_count; ++idx) {
		nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Acted on worker thread %d.\n", idx);
		action(state->thread_locks[idx]);
	}
}

bool nqiv_setup_thread_info(nqiv_state* state)
{
	if(state->thread_count == 0) {
		state->thread_count = 1;
	}
	if(state->thread_locks != NULL) {
		nqiv_state_clear_thread_locks(state);
	}
	state->thread_locks = (omp_lock_t**)calloc( state->thread_count, sizeof(omp_lock_t*) );
	if(state->thread_locks == NULL) {
		nqiv_log_write(&state->logger, NQIV_LOG_ERROR, "Failed to allocate memory for %d thread locks.\n", state->thread_count);
		return false;
	}
	omp_init_lock(&state->thread_event_transaction_group_lock);
	int thread;
	for(thread = 0; thread < state->thread_count; ++thread) {
		state->thread_locks[thread] = (omp_lock_t*)calloc( 1, sizeof(omp_lock_t) );
		if(state->thread_locks[thread] == NULL) {
			nqiv_state_clear_thread_locks(state);
			return false;
		}
	}
	nqiv_act_on_thread_locks(state, omp_init_lock);
	return true;
}

bool nqiv_parse_args(char *argv[], nqiv_state* state)
{
	/*
	nqiv_log_level arg_log_level = NQIV_LOG_WARNING;
	char* arg_log_prefix = "#time:%Y-%m-%d_%H:%M:%S%z# #level#: ";
	*/
	state->queue_length = STARTING_QUEUE_LENGTH;
	state->thread_count = omp_get_num_procs() / 4;
	state->thread_event_interval = 5;
	state->prune_delay = 200;
	nqiv_log_init(&state->logger);
	if( !nqiv_cmd_manager_init(&state->cmds, state) ) {
		return false;
	}
	if( !nqiv_check_and_print_logger_error(&state->logger) ) {
		return false;
	}
	if( !nqiv_image_manager_init(&state->images, &state->logger, STARTING_QUEUE_LENGTH) ) {
		fputs("Failed to initialize image manager.\n", stderr);
		return false;
	}
	if( !nqiv_pruner_init(&state->pruner, &state->logger, STARTING_QUEUE_LENGTH) ) {
		fputs("Failed to initialize pruner.\n", stderr);
		return false;
	}
	if( !nqiv_keybind_create_manager(&state->keybinds, &state->logger, STARTING_QUEUE_LENGTH) ) {
		fputs("Failed to initialize image manager.\n", stderr);
		return false;
	}
	nqiv_set_keyrate_defaults(&state->keystates);
	if( !nqiv_priority_queue_init(&state->thread_queue, &state->logger, STARTING_QUEUE_LENGTH, THREAD_QUEUE_BIN_COUNT) ) {
		fputs("Failed to initialize thread queue.\n", stderr);
		return false;
	}
	if( !nqiv_queue_init(&state->key_actions, &state->logger, STARTING_QUEUE_LENGTH) ) {
		fputs("Failed to initialize key action queue.\n", stderr);
		return false;
	}
	nqiv_state_set_default_colors(state);
	if( !nqiv_setup_sdl(state) ) {
		nqiv_state_clear(state);
		return false;
	}
	nqiv_setup_montage(state);
	struct optparse_long longopts[] = {
		{"cmd-from-stdin", 's', OPTPARSE_NONE},
		{"cmd", 'c', OPTPARSE_REQUIRED},
		{"cfg", 'C', OPTPARSE_REQUIRED},
		{"help", 'h', OPTPARSE_NONE},
		{0}
	};
	struct optparse options;
	optparse_init(&options, argv);
	int option;
	char default_config_path[PATH_MAX + 1] = {0};
	if( !nqiv_get_default_cfg(default_config_path, PATH_MAX + 1) ) {
        fprintf(stderr, "Failed to find default config path..\n");
		return false;
	}
	if( !nqiv_cmd_consume_stream_from_path(&state->cmds, default_config_path) ) {
        fprintf(stderr, "Failed to read commands from default config path %s\n", default_config_path);
		return false;
	}
    while ((option = optparse_long(&options, longopts, NULL)) != -1) {
        switch (option) {
		case 's':
			if( !nqiv_cmd_consume_stream(&state->cmds, stdin) ) {
				return false;
			}
			break;
		case 'c':
			if( !nqiv_cmd_add_line_and_parse(&state->cmds, options.optarg) ) {
				return false;
			}
			break;
		case 'C':
			if( !nqiv_cmd_consume_stream_from_path(&state->cmds, options.optarg) ) {
				return false;
			}
			break;
		case 'h':
			fprintf(stderr, "-s/--cmd-from-stdin Read commands from stdin.\n");
			fprintf(stderr, "-c/--cmd <cmd> Issue a single command to the image viewer's command processor. Also pass help to get information about commands.\n");
			fprintf(stderr, "-C/--cfg <path> Specify a config file to be read by the image viewer's command processor.\n");
			fprintf(stderr, "-h/--help Print this help message.\n");
			return false;
        case '?':
            fprintf(stderr, "%s: %s\n", argv[0], options.errmsg);
			return false;
        }
    }
	if( !nqiv_setup_thread_info(state) ) {
		nqiv_state_clear(state);
		return false;
	}
	char* arg;
    while ( ( arg = optparse_arg(&options) ) ) {
		if( nqiv_image_manager_has_path_extension(&state->images, arg) ) {
			if( !nqiv_image_manager_append(&state->images, arg) ) {
				return false;
			}
		}
	}
	if(state->images.images->position / sizeof(nqiv_image*) > 1) {
		state->in_montage = true;
	}
	return true;
} /* parse_args */

void nqiv_unlock_threads(nqiv_state* state, const int count)
{
	nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Unlocking threads.\n");
	int idx;
	int unlocked;
	for(idx = 0, unlocked = count; idx < state->thread_count && unlocked > 0; ++idx) {
		if( !omp_test_lock(state->thread_locks[idx]) ) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Unlocking worker thread %d.\n", idx);
			--unlocked;
		} else {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Already unlocked worker thread %d.\n", idx);
		}
		omp_unset_lock(state->thread_locks[idx]);
	}
}

bool nqiv_send_thread_event_base(nqiv_state* state, const int level, nqiv_event* event, const bool force, const bool unlock_threads)
{
	nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Sending event.\n");
	bool event_sent;
	if(force) {
		nqiv_priority_queue_push_force(&state->thread_queue, level, sizeof(nqiv_event), event);
		event_sent = true;
	} else {
		event_sent = nqiv_priority_queue_push(&state->thread_queue, level, sizeof(nqiv_event), event);
	}
	nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Event sent attempted, status: %s.\n", event_sent ? "Success" : "Failure");
	if(!event_sent) {
		nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Failed to send event.\n");
		/*
		nqiv_unlock_threads(state);
		nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Unlocked threads for event.\n");
		*/
		return false;
	}
	nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Event sent successfully.\n");
	if(unlock_threads) {
		nqiv_unlock_threads(state, 1);
		nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Unlocked threads for event.\n");
	}
	return true;
}

bool nqiv_send_thread_event(nqiv_state* state, const int level, nqiv_event* event)
{
	omp_set_lock(&state->thread_event_transaction_group_lock);
	event->transaction_group = state->thread_event_transaction_group;
	omp_unset_lock(&state->thread_event_transaction_group_lock);
	return nqiv_send_thread_event_base(state, level, event, false, false);
}

bool nqiv_send_thread_event_force(nqiv_state* state, const int level, nqiv_event* event)
{
	event->transaction_group = -1;
	return nqiv_send_thread_event_base(state, level, event, true, true);
}

bool render_texture(bool* cleared, nqiv_state* state, SDL_Texture* texture, SDL_Rect* srcrect, const SDL_Rect* dstrect)
{
	if(dstrect == NULL) {
		return true;
	}
	if(!*cleared) {
		if(SDL_RenderClear(state->renderer) != 0) {
			return false;
		}
		if(SDL_RenderCopy(state->renderer, state->texture_background, NULL, NULL) != 0) {
			return false;
		}
		*cleared = true;
	}
	if( SDL_RenderCopy(state->renderer, texture, srcrect, dstrect) != 0 ) {
		return false;
	}
	return true;
}

/* TODO STEP FRAME? */
/* TODO Reset frame */
bool render_from_form(nqiv_state* state, nqiv_image* image, SDL_Texture* alpha_background, const bool is_montage, const SDL_Rect* dstrect, const bool is_thumbnail, const bool first_frame, const bool next_frame, const bool selected, const bool hard, const bool lock, const int base_priority)
{
	/* TODO Srcrect easily can make this work for both views DONE */
	/* TODO Merge load/save thumbnail, or have an short load to check for the thumbnail before saving  DONE*/
	/* TODO Use load thumbnail for is_thumbnail? */
	int pending_change_count = 0;
	bool cleared = is_montage;
	if(lock) {
		nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Locking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
		if( !omp_test_lock(&image->lock) ) {
			nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Failed to lock image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
			if( !render_texture(&cleared, state, state->texture_montage_unloaded_background, NULL, dstrect) ) {
				return false;
			}
			return true;
		}
		nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Locked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
	}
	nqiv_image_form* form = is_thumbnail ? &image->thumbnail : &image->image;
	SDL_Rect srcrect = {0};
	SDL_Rect* srcrect_ptr = &srcrect;
	SDL_Rect dstrect_zoom = {0};
	SDL_Rect* dstrect_zoom_ptr = NULL;
	bool resample_zoom = false;
	if(form->width > 0 && form->height > 0 && image->image.width > 0 && image->image.height > 0 && dstrect != NULL) {
		dstrect_zoom_ptr = &dstrect_zoom;
		if(is_thumbnail) {
			srcrect.w = image->image.width;
			srcrect.h = image->image.height;
			if(form->width > srcrect.w) {
				const double multiplier = (double)form->width / (double)srcrect.w;
				srcrect.w = (int)( (double)srcrect.w * multiplier );
				srcrect.h = (int)( (double)srcrect.h * multiplier );
			}
			if(form->height > srcrect.h) {
				const double multiplier = (double)form->height / (double)srcrect.h;
				srcrect.w = (int)( (double)srcrect.w * multiplier );
				srcrect.h = (int)( (double)srcrect.h * multiplier );
			}
		} else {
			srcrect.w = form->width;
			srcrect.h = form->height;
		}
		dstrect_zoom.w = dstrect->w;
		dstrect_zoom.h = dstrect->h;
		dstrect_zoom.x = dstrect->x;
		dstrect_zoom.y = dstrect->y;
		nqiv_image_manager_calculate_zoom_parameters(&state->images, &srcrect, dstrect_zoom_ptr);
		nqiv_image_manager_calculate_zoomrect(&state->images, !is_montage, state->stretch_images, &srcrect, dstrect_zoom_ptr); /* TODO aspect ratio */
		if(!state->no_resample_oversized && (form->height > 16000 || form->width > 16000) )  {
			if(form->srcrect.x != srcrect.x || form->srcrect.y != srcrect.y || form->srcrect.w != srcrect.w || form->srcrect.h != srcrect.h) {
				resample_zoom = true;
				nqiv_unload_image_form_texture(form);
				form->srcrect.x = srcrect.x;
				form->srcrect.y = srcrect.y;
				form->srcrect.w = srcrect.w;
				form->srcrect.h = srcrect.h;
			}
			srcrect_ptr = NULL;
		} else {
			if((form->srcrect.x != 0 || form->srcrect.y != 0 || form->srcrect.w != form->width || form->srcrect.h != form->height) || form->effective_height == 0 || form->effective_width == 0) {
				resample_zoom = true;
				nqiv_unload_image_form_texture(form);
				form->srcrect.x = 0;
				form->srcrect.y = 0;
				form->srcrect.w = form->width;
				form->srcrect.h = form->height;
			} else {
				srcrect.x = 0;
				srcrect.y = 0;
				srcrect.w = form->effective_width;
				srcrect.h = form->effective_height;
				dstrect_zoom.w = dstrect->w;
				dstrect_zoom.h = dstrect->h;
				dstrect_zoom.x = dstrect->x;
				dstrect_zoom.y = dstrect->y;
				nqiv_image_manager_calculate_zoom_parameters(&state->images, &srcrect, dstrect_zoom_ptr);
				nqiv_image_manager_calculate_zoomrect(&state->images, !is_montage, state->stretch_images, &srcrect, dstrect_zoom_ptr); /* TODO aspect ratio */
			}
		}
	}
	if(!is_thumbnail && state->images.thumbnail.save) {
		if(!image->thumbnail_attempted) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Creating thumbnail for instance that won't load it.\n");
			nqiv_event event = {0};
			event.type = NQIV_EVENT_IMAGE_LOAD;
			event.options.image_load.image = image;
			event.options.image_load.set_thumbnail_path = true;
			event.options.image_load.create_thumbnail = true;
			if( !nqiv_send_thread_event(state, base_priority + 4, &event) ) {
				nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
				omp_unset_lock(&image->lock);
				nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
				return false;
			} else {
				pending_change_count += 1;
			}
		/* TODO Signal to create thumbnail from main image, set thumbnail attempted after DONE */
		} else {
		/* TODO Signal to unload thumbnail from the image, make sure not to unload twice??? */
		}
	}
	/*
	if(!is_Thumbnail && !image->thumbnail_attempted && state->images.thumbnail.save) {
	}
	if(!is_Thumbnail && image->thumbnail_attempted && state->images.thumbnail.save) {
	}
	*/
	if(form->error) {
		/* TODO We check this because we try to load a non-existent thumbnail */
		if(is_thumbnail && !image->image.error) {
			if(!image->thumbnail_attempted && state->images.thumbnail.save) {
				nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Creating thumbnail after failing to load it.\n");
				form->error = false;
				nqiv_event event = {0};
				event.type = NQIV_EVENT_IMAGE_LOAD;
				event.options.image_load.image = image;
				event.options.image_load.set_thumbnail_path = true;
				event.options.image_load.create_thumbnail = true;
				if( !nqiv_send_thread_event(state, base_priority + 3, &event) ) {
					nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
					omp_unset_lock(&image->lock);
					nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
					return false;
				} else {
					pending_change_count += 1;
				}
				/* TODO Signal to create thumbnail from main image, set thumbnail attempted after */
				/* TODO UNSET ERROR */
			} else {
				if( !render_from_form(state, image, alpha_background, is_montage, dstrect, false, true, false, selected, hard, false, base_priority) ) {
					nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
					omp_unset_lock(&image->lock);
					nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
					return false;
				}
				/* TODO Use main image for the thumbnail */
			}
		} else {
			if( !render_texture(&cleared, state, state->texture_montage_error_background, NULL, dstrect) ) {
				nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
				omp_unset_lock(&image->lock);
				nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
				return false;
			}
		}
	} else {
		if(form->texture != NULL && !is_montage && first_frame && form->animation.exists) {
			nqiv_unload_image_form_texture(form);
		}
		if( form->texture != NULL && (!next_frame || !form->animation.frame_rendered) ) {
			/* NOOP */
		} else if( form->surface != NULL && !resample_zoom && (is_montage || !first_frame || !form->animation.exists) ) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Loading texture for image %s.\n", image->image.path);
			form->texture = SDL_CreateTextureFromSurface(state->renderer, form->surface);
			if(form->texture == NULL) {
				nqiv_log_write(&state->logger, NQIV_LOG_ERROR, "Failed to load texture for image.\n");
				nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
				omp_unset_lock(&image->lock);
				nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
				return false;
			}
		} else {
			if(first_frame || hard) {
				if( !render_texture(&cleared, state, state->texture_montage_unloaded_background, NULL, dstrect) ) {
					nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
					omp_unset_lock(&image->lock);
					nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
					return false;
				}
			}
			if(is_thumbnail) {
				nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Loading thumbnail.\n");
				nqiv_event event = {0};
				/* TODO SOFTEN EVENTS */
				event.type = NQIV_EVENT_IMAGE_LOAD;
				event.options.image_load.image = image;
				event.options.image_load.set_thumbnail_path = true;
				if(hard) {
					event.options.image_load.thumbnail_options.file = true;
					event.options.image_load.thumbnail_options.vips = true;
					event.options.image_load.borrow_thumbnail_dimension_metadata = true;
				} else {
					event.options.image_load.thumbnail_options.file_soft = true;
					event.options.image_load.thumbnail_options.vips_soft = true;
					event.options.image_load.borrow_thumbnail_dimension_metadata = true;
				}
				if( hard || next_frame || (first_frame && image->thumbnail.vips != NULL && image->thumbnail.animation.frame != 0) ) {
					event.options.image_load.thumbnail_options.raw = true;
					event.options.image_load.thumbnail_options.surface = true;
				} else {
					event.options.image_load.thumbnail_options.raw_soft = true;
					event.options.image_load.thumbnail_options.surface_soft = true;
				}
				event.options.image_load.thumbnail_options.first_frame = first_frame;
				event.options.image_load.thumbnail_options.next_frame = next_frame && !first_frame;
				if( !nqiv_send_thread_event(state, base_priority + 2, &event) ) {
					nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
					omp_unset_lock(&image->lock);
					nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
					return false;
				} else {
					pending_change_count += 1;
				}
				/* TODO Signal to load thumbnail image if it's available, otherwise use main */
				memset( &event, 0, sizeof(nqiv_event) );
				if(state->images.thumbnail.save) {
					/* TODO SOFTEN EVENTS */
					event.type = NQIV_EVENT_IMAGE_LOAD;
					event.options.image_load.image = image;
					event.options.image_load.set_thumbnail_path = true;
					event.options.image_load.create_thumbnail = true;
					event.options.image_load.borrow_thumbnail_dimension_metadata = true;
					if( !nqiv_send_thread_event(state, base_priority + 3, &event) ) {
						nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
						omp_unset_lock(&image->lock);
						nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
						return false;
					} else {
						pending_change_count += 1;
					}
				}
			} else {
				nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Loading image %s.\n", image->image.path);
				nqiv_event event = {0};
				event.type = NQIV_EVENT_IMAGE_LOAD;
				event.options.image_load.image = image;
				if(hard) {
					event.options.image_load.image_options.file = true;
					event.options.image_load.image_options.vips = true;
				} else {
					event.options.image_load.image_options.file_soft = true;
					event.options.image_load.image_options.vips_soft = true;
				}
				if( hard || next_frame || resample_zoom || (first_frame && image->image.vips != NULL && image->image.animation.frame != 0) ) {
					event.options.image_load.image_options.raw = true;
					event.options.image_load.image_options.surface = true;
				} else {
					event.options.image_load.image_options.raw_soft = true;
					event.options.image_load.image_options.surface_soft = true;
				}
				event.options.image_load.image_options.first_frame = first_frame;
				event.options.image_load.image_options.next_frame = next_frame && !first_frame;
				if( !nqiv_send_thread_event(state, base_priority + 2, &event) ) {
					nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
					omp_unset_lock(&image->lock);
					nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
					return false;
				} else {
					pending_change_count += 1;
				}
				/* TODO Signal to use main image as thumbnail */
			}
			/* TODO Else if try to save thumbnail if not attempted yet??? */
			/*
			if(is_thumbnail) {
state->images.thumbnail.load

			}
			if(state->images.thumbnail.load && state->images.thumbnail.save) {

			}
			nqiv_event event;
			event.type = NQIV_EVENT_IMAGE_LOAD;
			event.options.image_load.image = image;
			if( nqiv_send_thread_event(state, &event) ) {
				return false;
			}
			*/
		}
		if(form->texture != NULL) {
			if( !render_texture(&cleared, state, alpha_background, dstrect_zoom_ptr, dstrect_zoom_ptr) ) {
				nqiv_log_write(&state->logger, NQIV_LOG_ERROR, "Failed to draw image alpha background.\n");
				nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
				omp_unset_lock(&image->lock);
				nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
				return false;
			}
			if( !render_texture(&cleared, state, form->texture, srcrect_ptr, dstrect_zoom_ptr) ) {
				nqiv_log_write(&state->logger, NQIV_LOG_ERROR, "Failed to draw image texture.\n");
				nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
				omp_unset_lock(&image->lock);
				nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
				return false;
			}
			if(form->animation.exists && next_frame && !is_montage) {
				nqiv_unload_image_form_texture(form);
				form->animation.frame_rendered = true;
				nqiv_event event = {0};
				event.type = NQIV_EVENT_IMAGE_LOAD;
				event.options.image_load.image = image;
				if(hard) {
					event.options.image_load.image_options.file = true;
					event.options.image_load.image_options.vips = true;
				} else {
					event.options.image_load.image_options.file_soft = true;
					event.options.image_load.image_options.vips_soft = true;
				}
				if( hard || next_frame || resample_zoom || (first_frame && image->thumbnail.vips != NULL && image->thumbnail.animation.frame != 0) ) {
					event.options.image_load.image_options.raw = true;
					event.options.image_load.image_options.surface = true;
				} else {
					event.options.image_load.image_options.raw_soft = true;
					event.options.image_load.image_options.surface_soft = true;
				}
				event.options.image_load.image_options.first_frame = first_frame;
				event.options.image_load.image_options.next_frame = next_frame && !first_frame;
				if( !nqiv_send_thread_event(state, base_priority + 2, &event) ) {
					nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
					omp_unset_lock(&image->lock);
					nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
					return false;
				} else {
					pending_change_count += 1;
				}
			}
		}
	}
	if(selected) {
		if( !render_texture(&cleared, state, state->texture_montage_selection, NULL, dstrect) ) {
			nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
			omp_unset_lock(&image->lock);
			nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
			return false;
		}
	}
	if(image->marked && is_montage) {
		if( !render_texture(&cleared, state, state->texture_montage_mark, NULL, dstrect) ) {
			nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
			omp_unset_lock(&image->lock);
			nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
			return false;
		}
	}
	form->pending_change_count += pending_change_count;
	nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Pending change count Image: %d Thumbnail: %d, from main thread %d.\n", image->image.pending_change_count, image->thumbnail.pending_change_count, omp_get_thread_num() );
	if(form->pending_change_count > 0) {
		nqiv_unlock_threads(state, pending_change_count);
		nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Unlocked threads for update.\n");
	}
	nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
	omp_unset_lock(&image->lock);
	nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
	return true;
}

/*
bool render_montage_item_from_image(nqiv_state* state, nqiv_image* image, const SDL_Rect* dstrect)
{
	* TODO PRobably delete this *
}
*/

/*
bool render_montage_item(nqiv_state* state, nqiv_image* image, const SDL_Rect* dstrect)
{
	* TODO Most of this is handled from rendering from form, maybe it can even be removed *
	if(state->images->thumbnail.load) {
		if(image->thumbnail.error) {
			* TODO MAin image *
			if( SDL_RenderCopy(state->renderer, texture_montage_error_background, NULL, dstrect) != 0 ) {
				return false;
			}
		} else if(image->thumbnail.texture != NULL) {
			if( SDL_RenderCopy(state->renderer, state->texture_montage_alpha_background, NULL, dstrect) != 0 ) {
				return false;
			}
			if( SDL_RenderCopy(state->renderer, image->thumbnail.texture, NULL, dstrect) != 0 ) {
				return false;
			}
		} else if(state->images->thumbnail.save) {
			* TODO complete event and set loading indicator *
			nqiv_event event = {0};
			event.type = NQIV_EVENT_WORKER_STOP;
			if( nqiv_send_thread_event(state, &event) ) {
				return false;
			}
		} else {
			* TODO MAin image *
		}
	} else {
		* TODO Use main image, set error, or load main image *
	}
}
*/

/*2305843009213693951\0*/
#define INT_MAX_STRLEN 20
bool set_title(nqiv_state* state, nqiv_image* image)
{
	char idx_string[INT_MAX_STRLEN] = {0};
	char count_string[INT_MAX_STRLEN] = {0};
	char width_string[INT_MAX_STRLEN] = {0};
	char height_string[INT_MAX_STRLEN] = {0};
	char zoom_string[INT_MAX_STRLEN] = {0};
	snprintf(idx_string, INT_MAX_STRLEN, "%d", state->montage.positions.selection + 1);
	snprintf( count_string, INT_MAX_STRLEN, "%lu", state->images.images->position / sizeof(nqiv_image*) );
	const bool do_dimensions = image->image.width > 0 && image->image.height > 0;
	if(do_dimensions) {
		snprintf(width_string, INT_MAX_STRLEN, "%d", image->image.width);
		snprintf(height_string, INT_MAX_STRLEN, "%d", image->image.height);
	}
	if(!state->in_montage) {
		snprintf( zoom_string, INT_MAX_STRLEN, "%d", nqiv_image_manager_get_zoom_percent(&state->images) );
	}
	const char* path_components[] =
	{
		"nqiv - ",
		idx_string,
		"/",
		count_string,
		" - ",
		width_string,
		do_dimensions ? "x" : "",
		height_string,
		do_dimensions ? " - " : "",
		zoom_string,
		!state->in_montage ? "% - " : "",
		image->marked ? "MARKED -" : "",
		image->image.path,
		NULL
	};
	size_t title_len = 0;
	int idx = 0;
	while(path_components[idx] != NULL) {
		title_len += strlen(path_components[idx]);
		++idx;
	}
	title_len += 1; /* For the NUL */
	if(state->window_title == NULL) {
		state->window_title = (char*)calloc(1, title_len);
		if(state->window_title == NULL) {
			nqiv_log_write(&state->logger, NQIV_LOG_ERROR, "Failed to allocate %d bytes for new window title.\n", title_len);
			return false;
		}
		state->window_title_size = title_len;
	} else if(title_len > state->window_title_size) {
		char* new_title = (char*)realloc(state->window_title, title_len);
		if(new_title == NULL) {
			nqiv_log_write(&state->logger, NQIV_LOG_ERROR, "Failed to reallocate %d bytes for window title.\n", title_len);
			return false;
		}
		state->window_title = new_title;
		state->window_title_size = title_len;
		memset(state->window_title, 0, state->window_title_size);
	} else {
		memset(state->window_title, 0, state->window_title_size);
	}
	char* title_ptr = state->window_title;
	idx = 0;
	while(path_components[idx] != NULL) {
		const char* item = path_components[idx];
		const size_t item_len = strlen(item);
		memcpy(title_ptr, item, item_len);
		title_ptr += item_len;
		++idx;
	}
	SDL_SetWindowTitle(state->window, state->window_title);
	return true;
}
#undef INT_MAX_STRLEN

bool render_montage(nqiv_state* state, const bool hard)
{
	/*
	bool read_from_stdin;
	nqiv_log_ctx logger;
	nqiv_image_manager images;
	nqiv_keybind_manager keybinds;
	nqiv_montage_state montage;
	nqiv_queue thread_queue;
	bool SDL_inited;
	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* texture_background;
	SDL_Texture* texture_montage_selection;
	SDL_Texture* texture_montage_alpha_background;
	SDL_Texture* texture_alpha_background;
	Uint32 thread_event_number;
	int thread_count;
	omp_lock_t* thread_locks;
	void nqiv_montage_get_image_rect(nqiv_montage_state* state, const int idx, SDL_Rect* rect)

	if load thumbnail and thumbnail available, use that
	if not load thubmnail, thumbnail should not be available
	if load thumbnail and thumbnail not available, set loading indicator, send load thumbnail and quit
	if there's an error set, set error indicator, then quit
	*/
	nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Rendering montage.\n");
	if(SDL_RenderClear(state->renderer) != 0) {
		return false;
	}
	if(SDL_RenderCopy(state->renderer, state->texture_background, NULL, NULL) != 0) {
		return false;
	}
	const int images_len = state->images.images->position / sizeof(nqiv_image*);
	const int raw_start_idx = state->montage.positions.start - state->montage.preload.behind;
	const int raw_end = state->montage.positions.end + state->montage.preload.ahead;
	const int start_idx = raw_start_idx >= 0 ? raw_start_idx : 0;
	const int end = raw_end <= images_len ? raw_end : images_len;
	int idx;
	nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Preload Start: %d Preload End: %d Montage Start: %d Montage Selection: %d Montage End %d\n", start_idx, end, state->montage.positions.start, state->montage.positions.selection, state->montage.positions.end);
	for(idx = start_idx; idx < end; ++idx) {
		nqiv_image* image;
		if( !nqiv_array_get_bytes(state->images.images, idx, sizeof(nqiv_image*), &image) ) {
			return false;
		}
		if(idx >= state->montage.positions.start && idx < state->montage.positions.end) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Rendering montage image %s at %d.\n", image->image.path, idx);
			SDL_Rect dstrect;
			nqiv_montage_get_image_rect(&state->montage, idx, &dstrect);
			if( !render_from_form(state, image, state->texture_montage_alpha_background, true, &dstrect, state->images.thumbnail.load, true, false, state->montage.positions.selection == idx, hard, true, 0) ) {
				return false;
			}
			if( idx == state->montage.positions.selection && !set_title(state, image) ) {
				return false;
			}
		} else {
			if( !render_from_form(state, image, state->texture_montage_alpha_background, true, NULL, state->images.thumbnail.load, true, false, state->montage.positions.selection == idx, hard, true, 1) ) {
				return false;
			}
		}
	}
	return true;
}

bool render_image(nqiv_state* state, const bool start, const bool hard)
{
	nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Rendering selected image.\n");
	nqiv_image* image;
	if( !nqiv_array_get_bytes(state->images.images, state->montage.positions.selection, sizeof(nqiv_image*), &image) ) {
		return false;
	}
	SDL_Rect dstrect = {0};
	/* TODO RECT DONE BUT ASPECT RATIO */
	SDL_GetWindowSizeInPixels(state->window, &dstrect.w, &dstrect.h);
	if( !render_from_form(state, image, state->texture_alpha_background, false, &dstrect, false, start, true, false, hard, true, 0) ) {
		return false;
	}
	if( !set_title(state, image) ) {
		return false;
	}
	return true;
/* TODO */
}

void render_and_update(nqiv_state* state, bool* running, bool* result, const bool first_render, const bool hard)
{
	nqiv_montage_calculate_dimensions(&state->montage);
	const Uint64 new_time = SDL_GetTicks64();
	if(new_time - state->time_of_last_prune > state->prune_delay) {
		state->time_of_last_prune = new_time;
		const int prune_count = nqiv_pruner_run(&state->pruner, &state->montage, &state->images, &state->thread_queue);
		if(prune_count == -1) {
			*running = false;
			*result = false;
		} else if(prune_count > 0) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Prune count %d.\n", prune_count);
			nqiv_unlock_threads(state, prune_count);
		}
	}
	if(state->montage.range_changed) {
		omp_set_lock(&state->thread_event_transaction_group_lock);
		state->thread_event_transaction_group += 1;
		state->pruner.thread_event_transaction_group = state->thread_event_transaction_group;
		nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Increased transaction group value to %" PRIi64 " at position %d.\n", state->thread_event_transaction_group, state->montage.positions.selection);
		omp_unset_lock(&state->thread_event_transaction_group_lock);
		state->montage.range_changed = false;
	}
	if(state->in_montage) {
		if( !render_montage(state, hard) ) {
			*running = false;
			*result = false;
		}
	} else {
		if( !render_image(state, first_render, hard) ) {
			*running = false;
			*result = false;
		}
	}
	if(*result != false) {
		SDL_RenderPresent(state->renderer);
	}
}

/* TODO Won't running simulated just also run the events on non-simulated events? */
void nqiv_handle_keyactions(nqiv_state* state, bool* running, bool* result, const bool simulated, const bool key_up)
{
	nqiv_key_action action;
	while( nqiv_queue_pop_front(&state->key_actions, sizeof(nqiv_key_action), &action) ) {
		if( !simulated && !nqiv_keyrate_filter_action(&state->keystates, action, key_up) ) {
			/* NOOP */
		} else if(action == NQIV_KEY_ACTION_QUIT) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action quit.\n");
			*running = false;
			return;
		} else if(action == NQIV_KEY_ACTION_IMAGE_PREVIOUS) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action image previous.\n");
			if(!state->in_montage) {
				nqiv_montage_previous_selection(&state->montage);
				render_and_update(state, running, result, false, false);
			}
		} else if(action == NQIV_KEY_ACTION_IMAGE_NEXT) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action image next.\n");
			if(!state->in_montage) {
				nqiv_montage_next_selection(&state->montage);
				render_and_update(state, running, result, false, false);
			}
		} else if(action == NQIV_KEY_ACTION_MONTAGE_RIGHT) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action montage right.\n");
			if(state->in_montage) {
				nqiv_montage_next_selection(&state->montage);
				render_and_update(state, running, result, false, false);
			}
		} else if(action == NQIV_KEY_ACTION_MONTAGE_LEFT) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action montage left.\n");
			if(state->in_montage) {
				nqiv_montage_previous_selection(&state->montage);
				render_and_update(state, running, result, false, false);
			}
		} else if(action == NQIV_KEY_ACTION_MONTAGE_UP) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action montage up.\n");
			if(state->in_montage) {
				nqiv_montage_previous_selection_row(&state->montage);
				render_and_update(state, running, result, false, false);
			}
		} else if(action == NQIV_KEY_ACTION_MONTAGE_DOWN) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action montage down.\n");
			if(state->in_montage) {
				nqiv_montage_next_selection_row(&state->montage);
				render_and_update(state, running, result, false, false);
			}
		} else if(action == NQIV_KEY_ACTION_PAGE_UP) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action page up.\n");
			if(state->in_montage) {
				nqiv_montage_previous_selection_page(&state->montage);
				render_and_update(state, running, result, false, false);
			}
		} else if(action == NQIV_KEY_ACTION_PAGE_DOWN) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action page down.\n");
			if(state->in_montage) {
				nqiv_montage_next_selection_page(&state->montage);
				render_and_update(state, running, result, false, false);
			}
		} else if(action == NQIV_KEY_ACTION_MONTAGE_START) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action montage start.\n");
			if(state->in_montage) {
				nqiv_montage_jump_selection_start(&state->montage);
				render_and_update(state, running, result, false, false);
			}
		} else if(action == NQIV_KEY_ACTION_MONTAGE_END) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action montage end.\n");
			if(state->in_montage) {
				nqiv_montage_jump_selection_end(&state->montage);
				render_and_update(state, running, result, false, false);
			}
		} else if(action == NQIV_KEY_ACTION_TOGGLE_MONTAGE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action montage toggle.\n");
			state->in_montage = !state->in_montage;
			render_and_update(state, running, result, true, false);
		} else if(action == NQIV_KEY_ACTION_SET_MONTAGE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv montage set.\n");
			if(!state->in_montage) {
				state->in_montage = true;
				render_and_update(state, running, result, true, false);
			}
		} else if(action == NQIV_KEY_ACTION_SET_VIEWING) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action viewing set.\n");
			if(state->in_montage) {
				state->in_montage = false;
				render_and_update(state, running, result, true, false);
			}
		} else if(action == NQIV_KEY_ACTION_ZOOM_IN) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action zoom in.\n");
			if(!state->in_montage) {
				nqiv_image_manager_zoom_in(&state->images);
				render_and_update(state, running, result, false, false);
			} else if(state->montage.dimensions.count > 1) {
				nqiv_image_manager_increment_thumbnail_size(&state->images);
				render_and_update(state, running, result, false, false);
			}
		} else if(action == NQIV_KEY_ACTION_ZOOM_OUT) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action zoom out.\n");
			if(!state->in_montage) {
				nqiv_image_manager_zoom_out(&state->images);
				render_and_update(state, running, result, false, false);
			} else {
				nqiv_image_manager_decrement_thumbnail_size(&state->images);
				render_and_update(state, running, result, false, false);
			}
		} else if(action == NQIV_KEY_ACTION_PAN_LEFT) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action pan left.\n");
			if(!state->in_montage) {
				nqiv_image_manager_pan_left(&state->images);
				render_and_update(state, running, result, false, false);
			}
		} else if(action == NQIV_KEY_ACTION_PAN_RIGHT) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action pan right.\n");
			if(!state->in_montage) {
				nqiv_image_manager_pan_right(&state->images);
				render_and_update(state, running, result, false, false);
			}
		} else if(action == NQIV_KEY_ACTION_PAN_UP) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action pan up.\n");
			if(!state->in_montage) {
				nqiv_image_manager_pan_up(&state->images);
				render_and_update(state, running, result, false, false);
			}
		} else if(action == NQIV_KEY_ACTION_PAN_DOWN) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action pan down more.\n");
			if(!state->in_montage) {
				nqiv_image_manager_pan_down_more(&state->images);
				render_and_update(state, running, result, false, false);
			}
		} else if(action == NQIV_KEY_ACTION_ZOOM_IN_MORE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action zoom in more.\n");
			if(!state->in_montage) {
				nqiv_image_manager_zoom_in_more(&state->images);
				render_and_update(state, running, result, false, false);
			} else if(state->montage.dimensions.count > 1) {
				nqiv_image_manager_increment_thumbnail_size_more(&state->images);
				render_and_update(state, running, result, false, false);
			}
		} else if(action == NQIV_KEY_ACTION_ZOOM_OUT_MORE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action zoom out more.\n");
			if(!state->in_montage) {
				nqiv_image_manager_zoom_out_more(&state->images);
				render_and_update(state, running, result, false, false);
			} else {
				nqiv_image_manager_decrement_thumbnail_size_more(&state->images);
				render_and_update(state, running, result, false, false);
			}
		} else if(action == NQIV_KEY_ACTION_PAN_LEFT_MORE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action pan left more.\n");
			if(!state->in_montage) {
				nqiv_image_manager_pan_left_more(&state->images);
				render_and_update(state, running, result, false, false);
			}
		} else if(action == NQIV_KEY_ACTION_PAN_RIGHT_MORE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action pan right more.\n");
			if(!state->in_montage) {
				nqiv_image_manager_pan_right_more(&state->images);
				render_and_update(state, running, result, false, false);
			}
		} else if(action == NQIV_KEY_ACTION_PAN_UP_MORE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action pan up more.\n");
			if(!state->in_montage) {
				nqiv_image_manager_pan_up_more(&state->images);
				render_and_update(state, running, result, false, false);
			}
		} else if(action == NQIV_KEY_ACTION_PAN_DOWN_MORE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action pan down more.\n");
			if(!state->in_montage) {
				nqiv_image_manager_pan_down_more(&state->images);
				render_and_update(state, running, result, false, false);
			}
		} else if(action == NQIV_KEY_ACTION_TOGGLE_STRETCH) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action toggle stretch.\n");
			state->stretch_images = !state->stretch_images;
			render_and_update(state, running, result, false, false);
		} else if(action == NQIV_KEY_ACTION_STRETCH) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action stretch.\n");
			state->stretch_images = true;
			render_and_update(state, running, result, false, false);
		} else if(action == NQIV_KEY_ACTION_KEEP_ASPECT_RATIO) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action keep aspect ratio.\n");
			state->stretch_images = false;
			render_and_update(state, running, result, false, false);
		} else if(action == NQIV_KEY_ACTION_IMAGE_MARK_TOGGLE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action image mark.\n");
			nqiv_image* tmp_image;
			if( nqiv_array_get_bytes(state->images.images, state->montage.positions.selection, sizeof(nqiv_image*), &tmp_image) ) {
				tmp_image->marked = !tmp_image->marked;
			}
			render_and_update(state, running, result, false, false);
		} else if(action == NQIV_KEY_ACTION_IMAGE_MARK) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action image mark.\n");
			nqiv_image* tmp_image;
			if( nqiv_array_get_bytes(state->images.images, state->montage.positions.selection, sizeof(nqiv_image*), &tmp_image) ) {
				tmp_image->marked = true;
			}
			render_and_update(state, running, result, false, false);
		} else if(action == NQIV_KEY_ACTION_IMAGE_UNMARK) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action image unmark.\n");
			nqiv_image* tmp_image;
			if( nqiv_array_get_bytes(state->images.images, state->montage.positions.selection, sizeof(nqiv_image*), &tmp_image) ) {
				tmp_image->marked = false;
			}
			render_and_update(state, running, result, false, false);
		} else if(action == NQIV_KEY_ACTION_PRINT_MARKED) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action image print marked.\n");
			const int num_images = state->images.images->position / sizeof(nqiv_image*);
			int iidx;
			for(iidx = 0; iidx < num_images; ++iidx) {
				nqiv_image* image;
				if( nqiv_array_get_bytes(state->images.images, iidx, sizeof(nqiv_image*), &image) ) {
					if(image->marked) {
						fprintf(stdout, "%s\n", image->image.path);
					}
				}
			}
			render_and_update(state, running, result, false, false);
		} else if(action == NQIV_KEY_ACTION_RELOAD) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action reload.\n");
			render_and_update(state, running, result, true, true);
		}
	}
}


bool nqiv_master_thread(nqiv_state* state)
{
	bool result = true;
	bool running = true;
	while(running) {
		nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Waiting on SDL event.\n");
		SDL_PumpEvents();
		SDL_Event input_event;
		const int event_result = SDL_WaitEvent(&input_event);
		if(event_result == 0) {
			nqiv_log_write( &state->logger, NQIV_LOG_ERROR, "Failed to wait on an SDL event. SDL Error: %s\n", SDL_GetError() );
			/* TODO CHECK ME */
			running = false;
			result = false;
			break;
		}
		nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received SDL event.\n");
		/*nqiv_key_lookup_summary nqiv_keybind_lookup(nqiv_keybind_manger* manager, const SDL_Keysym* key, nqiv_array* output);*/
		/* TODO mousebuttonevent */
		switch(input_event.type) {
			case SDL_USEREVENT:
				nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received response from worker thread.\n");
				if(input_event.user.code >= 0) {
					if( (Uint32)input_event.user.code == state->thread_event_number ) {
						/*
						nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Locking thread from master.\n");
						omp_set_lock(input_event.user.data1);
						nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Locked thread from master.\n");
						*/
						nqiv_priority_queue_lock(&state->thread_queue);
						int queue_length = 0;
						int idx;
						for(idx = 0; idx < state->thread_queue.bin_count; ++idx) {
							queue_length += state->thread_queue.bins[idx].array->position / sizeof(nqiv_event*);
						}
						if(queue_length == 0) {
							if( omp_test_lock(input_event.user.data1) ) {
								nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Locked thread from master.\n");
							} else {
								nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Thread already locked from master.\n");
							}
						} else {
							nqiv_unlock_threads(state, queue_length);
						}
						nqiv_priority_queue_unlock(&state->thread_queue);
						render_and_update(state, &running, &result, false, false);
					} else if( ( Uint32)input_event.user.code == state->cfg_event_number ) {
						nqiv_handle_keyactions(state, &running, &result, false, false); /* TODO No simulated actions for now. */
						render_and_update(state, &running, &result, false, false);
					}
				}
				break;
			case SDL_WINDOWEVENT:
				if(input_event.window.event == SDL_WINDOWEVENT_RESIZED ||
				   input_event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
				   input_event.window.event == SDL_WINDOWEVENT_EXPOSED ||
				   input_event.window.event == SDL_WINDOWEVENT_MAXIMIZED ||
				   input_event.window.event == SDL_WINDOWEVENT_RESTORED ||
				   input_event.window.event == SDL_WINDOWEVENT_SHOWN ||
				   input_event.window.event == SDL_WINDOWEVENT_ICCPROF_CHANGED ||
				   input_event.window.event == SDL_WINDOWEVENT_DISPLAY_CHANGED) {
					nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received window event for redrawing.\n");
					render_and_update(state, &running, &result, false, false);
				}
				break;
			case SDL_QUIT:
				nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received window quit event.\n");
				running = false;
				break;
			case SDL_KEYDOWN:
			case SDL_KEYUP:
				assert(input_event.type == SDL_KEYDOWN || input_event.type == SDL_KEYUP);
				/*
				 * TODO
				SDL_Keysym
				nqiv_key_lookup_summary lookup_summary = nqiv_keybind_lookup(&state->keybinds, const SDL_Keysym* key, nqiv_array* output);
				*/
				nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received key up event.\n");
				{
					const nqiv_key_lookup_summary lookup_summary = nqiv_keybind_lookup(&state->keybinds, &input_event.key.keysym, &state->key_actions);
					if(lookup_summary == NQIV_KEY_LOOKUP_FAILURE) {
						running = false;
						result = false;
					} else if(lookup_summary == NQIV_KEY_LOOKUP_FOUND) {
						/*
						bool nqiv_array_insert_bytes(nqiv_array* array, void* ptr, const int count, const int idx);
						void nqiv_array_remove_bytes(nqiv_array* array, const int idx, const int count);
						bool nqiv_array_push_bytes(nqiv_array* array, void* ptr, const int count);
						bool nqiv_array_get_bytes(nqiv_array* array, const int idx, const int count, void* ptr);
						*/
						nqiv_handle_keyactions(state, &running, &result, false, input_event.type == SDL_KEYUP ? true : false);
						render_and_update(state, &running, &result, false, false);
					}
				}
				break;
		}
	}
	nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Finished waiting on events.\n");
	int idx;
	for(idx = 0; idx < state->thread_count; ++idx) {
		nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Killing thread %d.\n", idx);
		nqiv_event output_event = {0};
		output_event.type = NQIV_EVENT_WORKER_STOP;
		nqiv_send_thread_event_force(state, 0, &output_event);
		nqiv_unlock_threads(state, state->thread_count);
	}
	return result;
}

bool nqiv_run(nqiv_state* state)
{
	bool result;
	bool* result_ptr = &result;
	const int thread_count = state->thread_count;
	const int thread_event_interval = state->thread_event_interval;
	omp_lock_t** thread_locks = state->thread_locks;
	nqiv_log_ctx* logger = &state->logger;
	nqiv_priority_queue* thread_queue = &state->thread_queue;
	const int64_t* thread_event_transaction_group = &state->thread_event_transaction_group;
	omp_lock_t* thread_event_transaction_group_lock = &state->thread_event_transaction_group_lock;
	const Uint32 event_code = state->thread_event_number;
	#pragma omp parallel default(none) firstprivate(state, logger, thread_count, thread_event_interval, thread_locks, thread_queue, event_code, result_ptr, thread_event_transaction_group, thread_event_transaction_group_lock)
	{
		#pragma omp master
		{
			int thread;
			for(thread = 0; thread < thread_count; ++thread) {
				omp_lock_t* lock = thread_locks[thread];
				#pragma omp task default(none) firstprivate(logger, thread_queue, lock, thread_count, thread_event_interval, event_code, thread_event_transaction_group, thread_event_transaction_group_lock)
				nqiv_worker_main(logger, thread_queue, lock, thread_count, thread_event_interval, event_code, thread_event_transaction_group, thread_event_transaction_group_lock);
			}
			*result_ptr = nqiv_master_thread(state);
		}
		#pragma omp taskwait
	}
	return result;
}

int main(int argc, char *argv[])
{
	if(argc == 0) {
		return 0;
	}

	nqiv_state state;
	memset( &state, 0, sizeof(nqiv_state) );

	if(VIPS_INIT(argv[0]) != 0) {
		fputs("Failed to initialize vips.\n", stderr);
	}

	if( !nqiv_parse_args(argv, &state) ) {
		nqiv_state_clear(&state);
		return 1;
	}

	/* TODO
	 *
	 * In other files:
	 * Logging everywhere
	 * Synchronize functions
	 * Proper includes
	 * Appropriate asserts
	 *
	 * -Setup montage info
	 * -Setup SDL
	 * -Create templates
	 * -Create locks
	 * -Start threads
	 * -Start main event loop
	 * -changed status for image or return the image itself or just redraw all of them?
	 * -Write image to rect
	 */

	const bool result = nqiv_run(&state);
	nqiv_state_clear(&state);
	return result ? 0 : 1;
}
