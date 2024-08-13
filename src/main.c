#include "platform.h"

#include <stdio.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <assert.h>

#include <vips/vips.h>
#include <SDL2/SDL.h>
#include <omp.h>

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

void nqiv_close_log_streams(nqiv_state* state)
{
	const int streams_len = nqiv_array_get_units_count(state->logger.streams);
	assert( streams_len == nqiv_array_get_units_count(state->logger_stream_names) );
	FILE** fileptrs = state->logger.streams->data;
	char** nameptrs = state->logger_stream_names->data;
	int idx;
	for(idx = 0; idx < streams_len; ++idx) {
		fclose(fileptrs[idx]);
		free(nameptrs[idx]);
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
		nqiv_close_log_streams(state);
		nqiv_log_destroy(&state->logger);
	}
	if(state->logger_stream_names != NULL) {
		nqiv_array_destroy(state->logger_stream_names);
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
	if(state->thread_event_transaction_group > 0) {
		omp_destroy_lock(&state->thread_event_transaction_group_lock);
	}
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
		NQIV_KEY_ACTION_IMAGE_NEXT,
		NQIV_KEY_ACTION_IMAGE_PREVIOUS,
		NQIV_KEY_ACTION_MONTAGE_LEFT,
		NQIV_KEY_ACTION_MONTAGE_RIGHT,
		NQIV_KEY_ACTION_MONTAGE_UP,
		NQIV_KEY_ACTION_MONTAGE_DOWN,
		NQIV_KEY_ACTION_MONTAGE_START,
		NQIV_KEY_ACTION_MONTAGE_END,
		NQIV_KEY_ACTION_IMAGE_MARK,
		NQIV_KEY_ACTION_IMAGE_UNMARK,
		NQIV_KEY_ACTION_START_MOUSE_PAN,
		NQIV_KEY_ACTION_IMAGE_ZOOM_IN,
		NQIV_KEY_ACTION_IMAGE_ZOOM_OUT,
		NQIV_KEY_ACTION_IMAGE_ZOOM_IN_MORE,
		NQIV_KEY_ACTION_IMAGE_ZOOM_OUT_MORE,
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

bool nqiv_setup_thread_info(nqiv_state* state)
{
	if(state->thread_count == 0) {
		state->thread_count = 1;
	}
	omp_init_lock(&state->thread_event_transaction_group_lock);
	assert(state->thread_event_transaction_group == 0);
	state->thread_event_transaction_group = 1;
	return true;
}

bool nqiv_load_builtin_config(nqiv_state* state, const char* default_config_path)
{
	const char* cmds =
	"set log prefix #level# #time:%Y-%m-%d %T%z# \n"
	"append log stream stderr\n"
	"append extension png\n"
	"append extension mng\n"
	"append extension jpg\n"
	"append extension jpeg\n"
	"append extension webp\n"
	"append extension gif\n"
	"append extension webp\n"
	"append extension tiff\n"
	"append extension svg\n"
	"append extension PNG\n"
	"append extension MNG\n"
	"append extension JPG\n"
	"append extension JPEG\n"
	"append extension WEBP\n"
	"append extension GIF\n"
	"append extension WEBP\n"
	"append extension TIFF\n"
	"append extension SVG\n"
	"append keybind Q=quit\n"
	"append keybind Home=montage_start\n"
	"append keybind End=montage_end\n"
	"append keybind PageUp=page_up\n"
	"append keybind PageDown=page_down\n"
	"append keybind Backspace=image_previous\n"
	"append keybind Backspace=montage_left\n"
	"append keybind Left=montage_left\n"
	"append keybind Right=montage_right\n"
	"append keybind Up=montage_up\n"
	"append keybind Down=montage_down\n"
	"append keybind Space=image_next\n"
	"append keybind Space=montage_right\n"
	"append keybind Return=set_viewing\n"
	"append keybind M=toggle_montage\n"
	"append keybind Left=pan_left\n"
	"append keybind Up=pan_up\n"
	"append keybind Right=pan_right\n"
	"append keybind Down=pan_down\n"
	"append keybind Z=zoom_in\n"
	"append keybind Z+shift=zoom_out\n"
	"append keybind ctrl+Z=zoom_in_more\n"
	"append keybind ctrl+Z+shift=zoom_out_more\n"
	"append keybind Keypad +=zoom_in\n"
	"append keybind Keypad -=zoom_out\n"
	"append keybind shift+Keypad +=zoom_in_more\n"
	"append keybind shift+Keypad -=zoom_out_more\n"
	"append keybind ctrl+Left=pan_left_more\n"
	"append keybind ctrl+Up=pan_up_more\n"
	"append keybind ctrl+Right=pan_right_more\n"
	"append keybind ctrl+Down=pan_down_more\n"
	"append keybind C=pan_center\n"
	"append keybind S=toggle_stretch\n"
	"append keybind '=image_mark_toggle\n"
	"append keybind shift+'=print_marked\n"
	"append keybind ;=image_mark\n"
	"append keybind ;=montage_right\n"
	"append keybind ;=image_next\n"
	"append keybind R=reload\n"
	"append keybind F=fit\n"
	"append keybind A=actual_size\n"
	"append keybind D=toggle_kept_zoom\n"
	"append keybind T=toggle_scale_mode\n"
	"append keybind shift+F=keep_fit\n"
	"append keybind shift+A=keep_actual_size\n"
	"append keybind shift+D=keep_current_zoom\n"
	"append keybind mouse1=montage_select_at_mouse\n"
	"append keybind mouse1_double=set_viewing\n"
	"append keybind shift+mouse1=image_mark_toggle_at_mouse\n"
	"append keybind mouse1=start_mouse_pan\n"
	"append keybind mouse1=end_mouse_pan\n"
	"append keybind scroll_forward=image_zoom_in\n"
	"append keybind scroll_backward=image_zoom_out\n"
	"append keybind scroll_forward=montage_up\n"
	"append keybind scroll_backward=montage_down\n"
	"append keybind shift+scroll_forward=zoom_in\n"
	"append keybind shift+scroll_backward=zoom_out\n"
	"append pruner or thumbnail no image texture self_opened unload surface raw vips\n"
	"append pruner and no thumbnail image texture self_opened not_animated unload surface raw vips\n"
	"append pruner or no thumbnail image texture self_opened unload surface raw\n"
	"append pruner and thumbnail no image texture self_opened image no thumbnail not_animated hard unload image thumbnail surface raw vips\n"
	"append pruner or thumbnail image texture loaded_behind 0 0 loaded_ahead 0 0 surface loaded_behind 0 0 loaded_ahead 0 0 raw loaded_behind 0 0 loaded_ahead 0 0 vips loaded_behind 0 0 loaded_ahead 0 0 hard unload texture surface raw vips\n"
	"append pruner or sum 0 thumbnail image texture bytes_ahead 0 0 bytes_behind 0 0 surface bytes_ahead 0 0 bytes_behind 0 0 raw bytes_ahead 0 0 bytes_behind 0 0 vips bytes_ahead 0 0 bytes_behind 0 0 hard unload texture surface raw vips\n";
	if( !nqiv_cmd_add_string(&state->cmds, cmds) || !nqiv_cmd_parse(&state->cmds) ) {
		return false;
	}
	fprintf(stderr, "Failed to load default config file path. Consider `nqiv -c \"dumpcfg\" > %s` to create it?\n", default_config_path);
	return true;
}

bool nqiv_parse_args(char *argv[], nqiv_state* state)
{
	/*
	nqiv_log_level arg_log_level = NQIV_LOG_WARNING;
	char* arg_log_prefix = "#time:%Y-%m-%d_%H:%M:%S%z# #level#: ";
	*/
	state->queue_length = STARTING_QUEUE_LENGTH;
	state->zoom_default = NQIV_ZOOM_DEFAULT_FIT;
	state->texture_scale_mode = SDL_ScaleModeBest;
	state->no_resample_oversized = true;
	state->show_loading_indicator = true;
	state->thread_count = omp_get_num_procs() / 3;
	state->thread_count = state->thread_count > 0 ? state->thread_count : 1;
	state->vips_threads = omp_get_num_procs() / 2;
	state->vips_threads = state->vips_threads > 0 ? state->vips_threads : 1;
	state->thread_event_interval = state->thread_count * 3;
	state->extra_wakeup_delay = state->thread_count * 20;
	state->prune_delay = 50000 / state->extra_wakeup_delay;
	vips_concurrency_set(state->vips_threads);
	state->logger_stream_names = nqiv_array_create(sizeof(char*), state->queue_length);
	if(state->logger_stream_names == NULL) {
		fputs("Failed to initialize logger stream names list.\n", stderr);
		return false;
	}
	nqiv_log_init(&state->logger);
	if( !nqiv_cmd_manager_init(&state->cmds, state) ) {
		return false;
	}
	state->logger.level = NQIV_LOG_ERROR;
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
	if( !nqiv_priority_queue_init(&state->thread_queue, &state->logger, sizeof(nqiv_event), STARTING_QUEUE_LENGTH, THREAD_QUEUE_BIN_COUNT) ) {
		fputs("Failed to initialize thread queue.\n", stderr);
		return false;
	}
	if( !nqiv_queue_init(&state->key_actions, &state->logger, sizeof(nqiv_key_action), STARTING_QUEUE_LENGTH) ) {
		fputs("Failed to initialize key action queue.\n", stderr);
		return false;
	}
	nqiv_state_set_default_colors(state);
	if( !nqiv_setup_sdl(state) ) {
		nqiv_state_clear(state);
		return false;
	}
	nqiv_setup_montage(state);
	const struct optparse_long longopts[] = {
		{"cmd-from-stdin", 's', OPTPARSE_NONE},
		{"no-default-cfg", 'N', OPTPARSE_NONE},
		{"cmd", 'c', OPTPARSE_REQUIRED},
		{"cfg", 'C', OPTPARSE_REQUIRED},
		{"help", 'h', OPTPARSE_NONE},
		{0}
	};
	bool load_default = true;
	struct optparse options;
	int option;
	optparse_init(&options, argv);
    while ((option = optparse_long(&options, longopts, NULL)) != -1) {
        switch (option) {
		case 'N':
			load_default = false;
			break;
		case 'h':
			fprintf(stderr, "-s/--cmd-from-stdin Read commands from stdin.\n");
			fprintf(stderr, "-N/--no-default-cfg Do not try to load the default config file (or settings if it does not exist).\n");
			fprintf(stderr, "-c/--cmd <cmd> Issue a single command to the image viewer's command processor. Also pass help to get information about commands.\n");
			fprintf(stderr, "-C/--cfg <path> Specify a config file to be read by the image viewer's command processor.\n");
			fprintf(stderr, "-h/--help Print this help message.\n");
			return false;
        case '?':
            fprintf(stderr, "%s: %s\n", argv[0], options.errmsg);
			return false;
        }
    }
	if(load_default) {
		char default_config_path[PATH_MAX + 1] = {0};
		if( nqiv_get_default_cfg(default_config_path, PATH_MAX + 1) ) {
			FILE* stream = fopen(default_config_path, "r");
			if(stream != NULL) {
				const int c = fgetc(stream);
				if(c == EOF) {
					if( !nqiv_load_builtin_config(state, default_config_path) ) {
						return false;
					}
				} else {
				    ungetc(c, stream);
					if( !nqiv_cmd_consume_stream(&state->cmds, stream) ) {
						fprintf(stderr, "Failed to read commands from default config path %s\n", default_config_path);
						fclose(stream);
						return false;
					}
				}
				fclose(stream);
			} else {
				if( !nqiv_load_builtin_config(state, default_config_path) ) {
					return false;
				}
			}
		}
	}
	optparse_init(&options, argv);
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
			fprintf(stderr, "-N/--no-default-cfg Do not try to load the default config file.\n");
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
	const char* arg;
    while ( ( arg = optparse_arg(&options) ) ) {
		if( nqiv_image_manager_has_path_extension(&state->images, arg) ) {
			if( !nqiv_image_manager_append(&state->images, arg) ) {
				return false;
			}
		}
	}
	state->first_frame_pending = true;
	if(nqiv_array_get_units_count(state->images.images) > 1) {
		state->in_montage = true;
	}
	return true;
} /* parse_args */

bool nqiv_send_thread_event_base(nqiv_state* state, const int level, const nqiv_event* event, const bool force)
{
	nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Sending event.\n");
	bool event_sent;
	if(force) {
		nqiv_priority_queue_push_force(&state->thread_queue, level, event);
		event_sent = true;
	} else {
		event_sent = nqiv_priority_queue_push(&state->thread_queue, level, event);
	}
	nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Event sent attempted, status: %s.\n", event_sent ? "Success" : "Failure");
	if(!event_sent) {
		nqiv_log_write(&state->logger, NQIV_LOG_ERROR, "Failed to send event.\n");
		/*
		nqiv_unlock_threads(state);
		nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Unlocked threads for event.\n");
		*/
		return false;
	}
	nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Event sent successfully.\n");
	return true;
}

bool nqiv_send_thread_event(nqiv_state* state, const int level, nqiv_event* event)
{
	omp_set_lock(&state->thread_event_transaction_group_lock);
	event->transaction_group = state->thread_event_transaction_group;
	omp_unset_lock(&state->thread_event_transaction_group_lock);
	return nqiv_send_thread_event_base(state, level, event, false);
}

bool nqiv_send_thread_event_force(nqiv_state* state, const int level, nqiv_event* event)
{
	event->transaction_group = -1;
	return nqiv_send_thread_event_base(state, level, event, true);
}

bool render_texture(bool* cleared, const SDL_Rect* cleardst, nqiv_state* state, SDL_Texture* texture, SDL_Rect* srcrect, const SDL_Rect* dstrect)
{
	if(dstrect == NULL) {
		return true;
	}
	if(!*cleared) {
		if(SDL_RenderClear(state->renderer) != 0) {
			nqiv_log_write(&state->logger, NQIV_LOG_ERROR, "Failed to clear renderer.\n");
			return false;
		}
		if(SDL_RenderCopy(state->renderer, state->texture_background, NULL, NULL) != 0) {
			nqiv_log_write(&state->logger, NQIV_LOG_ERROR, "Failed to copy texture background.\n");
			return false;
		}
		*cleared = true;
		state->render_cleared = true;
	}
	if(cleardst != NULL && SDL_RenderCopy(state->renderer, state->texture_background, NULL, cleardst) != 0) {
		nqiv_log_write(&state->logger, NQIV_LOG_ERROR, "Failed to clear rendering space using texture background.\n");
		return false;
	}
	if(SDL_SetTextureScaleMode(texture, state->texture_scale_mode) != 0) {
		nqiv_log_write(&state->logger, NQIV_LOG_ERROR, "Failed to set texture scale mode.\n");
		return false;
	}
	if( SDL_RenderCopy(state->renderer, texture, srcrect, dstrect) != 0 ) {
		nqiv_log_write(&state->logger, NQIV_LOG_ERROR, "Failed to copy texture.\n");
		return false;
	}
	return true;
}

void nqiv_apply_zoom_default(nqiv_state* state, const bool first_frame)
{
	if(first_frame || state->first_frame_pending) {
		switch(state->zoom_default) {
		case NQIV_ZOOM_DEFAULT_KEEP:
			break;
		case NQIV_ZOOM_DEFAULT_FIT:
			state->images.zoom.viewport_horizontal_shift = 0.0;
			state->images.zoom.viewport_vertical_shift = 0.0;
			state->images.zoom.image_to_viewport_ratio = state->images.zoom.fit_level;
			break;
		case NQIV_ZOOM_DEFAULT_ACTUAL:
			state->images.zoom.viewport_horizontal_shift = 0.0;
			state->images.zoom.viewport_vertical_shift = 0.0;
			state->images.zoom.image_to_viewport_ratio = state->images.zoom.actual_size_level;
			break;
		default:
			assert(false);
			break;
		}
	}
}

void nqiv_apply_zoom_modifications(nqiv_state* state, const bool first_frame)
{
	nqiv_apply_zoom_default(state, first_frame);
	if(state->images.zoom.image_to_viewport_ratio == state->images.zoom.fit_level) {
		nqiv_image_manager_pan_center(&state->images);
	}
}

/* TODO STEP FRAME? */
/* TODO Reset frame */
bool render_from_form(nqiv_state* state, nqiv_image* image, const bool is_montage, const SDL_Rect* dstrect, const bool is_thumbnail, const bool first_frame, const bool next_frame, const bool selected, const bool hard, const bool lock, const int base_priority)
{
	/* TODO Srcrect easily can make this work for both views DONE */
	/* TODO Merge load/save thumbnail, or have an short load to check for the thumbnail before saving  DONE*/
	/* TODO Use load thumbnail for is_thumbnail? */
	bool cleared = is_montage;
	nqiv_image_form* form = is_thumbnail ? &image->thumbnail : &image->image;
	if(is_montage) {
		state->is_loading = false;
	}
	if(lock) {
		nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Locking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
		if(!omp_test_lock(&image->lock)) {
			nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Failed to lock image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
			if(dstrect != NULL) {
				SDL_Rect tmp_srcrect;
				SDL_Rect tmp_dstrect;
				const SDL_Rect* tmp_dstrect_ptr = dstrect;
				if(form->master_srcrect.w > 0 && form->master_srcrect.h > 0 && form->master_dstrect.w > 0 && form->master_dstrect.h > 0) {
					memcpy( &tmp_srcrect, &form->master_srcrect, sizeof(SDL_Rect) );
					memcpy( &tmp_dstrect, &form->master_dstrect, sizeof(SDL_Rect) );
					tmp_dstrect_ptr = &tmp_dstrect;
				}
				state->is_loading = !is_montage;
				bool clearedtmp = true;
				if(form->master_dimensions_set && form->fallback_texture != NULL && form->master_srcrect.w > 0 && form->master_srcrect.h > 0 && form->master_dstrect.w > 0 && form->master_dstrect.h > 0) {
					nqiv_image_manager_calculate_zoom_parameters(&state->images, !is_montage, &tmp_srcrect, &tmp_dstrect);
					nqiv_apply_zoom_modifications(state, first_frame);
					nqiv_image_manager_calculate_zoomrect(&state->images, !is_montage, state->stretch_images, &tmp_srcrect, &tmp_dstrect); /* TODO aspect ratio */
					if( !nqiv_state_update_alpha_background_dimensions(state, tmp_dstrect.w, tmp_dstrect.h) ) {
						return false;
					}
					if( !render_texture(&cleared, NULL, state, state->texture_alpha_background, NULL, &tmp_dstrect) ) {
						return false;
					}
					if( !render_texture(&cleared, NULL, state, form->fallback_texture, &tmp_srcrect, &tmp_dstrect) ) {
						return false;
					}
				} else if( !render_texture(&clearedtmp, dstrect, state, state->texture_montage_unloaded_background, NULL, tmp_dstrect_ptr) ) {
					return false;
				}
				if(selected && is_montage) {
					if( !render_texture(&cleared, NULL, state, state->texture_montage_selection, NULL, dstrect) ) {
						return false;
					}
				}
				if(image->marked && is_montage) {
					if( !render_texture(&cleared, NULL, state, state->texture_montage_mark, NULL, dstrect) ) {
						return false;
					}
				}
			}
			return true;
		}
		nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Locked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
	}
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
		memcpy( &form->master_srcrect, &srcrect, sizeof(SDL_Rect) );
		memcpy( &form->master_dstrect, &dstrect_zoom, sizeof(SDL_Rect) );
		form->master_dimensions_set = true;
		nqiv_image_manager_calculate_zoom_parameters(&state->images, !is_montage, &srcrect, dstrect_zoom_ptr);
		nqiv_apply_zoom_modifications(state, first_frame);
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
				memcpy( &form->master_srcrect, &srcrect, sizeof(SDL_Rect) );
				memcpy( &form->master_dstrect, &dstrect_zoom, sizeof(SDL_Rect) );
				form->master_dimensions_set = true;
				nqiv_image_manager_calculate_zoom_parameters(&state->images, !is_montage, &srcrect, dstrect_zoom_ptr);
				nqiv_apply_zoom_modifications(state, first_frame);
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
			event.options.image_load.thumbnail_options.clear_error = true;
			event.options.image_load.create_thumbnail = true;
			if( !nqiv_send_thread_event(state, base_priority + NQIV_EVENT_PRIORITY_THUMBNAIL_SAVE_LOAD_NO, &event) ) {
				nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
				omp_unset_lock(&image->lock);
				nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
				return false;
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
			if(first_frame || hard) {
				if( !render_texture(&cleared, dstrect, state, state->texture_montage_unloaded_background, NULL, dstrect_zoom_ptr == NULL ? dstrect : dstrect_zoom_ptr) ) {
					nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
					omp_unset_lock(&image->lock);
					nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
					return false;
				}
			}
			if(!image->thumbnail_attempted && state->images.thumbnail.save) {
				nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Creating thumbnail after failing to load it.\n");
				nqiv_event event = {0};
				event.type = NQIV_EVENT_IMAGE_LOAD;
				event.options.image_load.image = image;
				event.options.image_load.set_thumbnail_path = true;
				event.options.image_load.thumbnail_options.clear_error = state->images.thumbnail.save;
				event.options.image_load.create_thumbnail = true;
				if( !nqiv_send_thread_event(state, base_priority + NQIV_EVENT_PRIORITY_THUMBNAIL_SAVE_LOAD_FAIL, &event) ) {
					nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
					omp_unset_lock(&image->lock);
					nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
					return false;
				}
				/* TODO Signal to create thumbnail from main image, set thumbnail attempted after */
				/* TODO UNSET ERROR */
			} else {
				nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Generating ephemeral thumbnail for image.\n");
				nqiv_event event = {0};
				event.type = NQIV_EVENT_IMAGE_LOAD;
				event.options.image_load.image = image;
				event.options.image_load.thumbnail_options.clear_error = true;
				if(hard) {
					event.options.image_load.thumbnail_options.file = true;
					event.options.image_load.thumbnail_options.vips = true;
				} else {
					event.options.image_load.thumbnail_options.file_soft = true;
					event.options.image_load.thumbnail_options.vips_soft = true;
				}
				if( !nqiv_send_thread_event(state, base_priority + NQIV_EVENT_PRIORITY_THUMBNAIL_LOAD_EPHEMERAL, &event) ) {
					nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
					omp_unset_lock(&image->lock);
					nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
					return false;
				}
			}
		} else {
			state->is_loading = false;
			if( !render_texture(&cleared, dstrect, state, state->texture_montage_error_background, NULL, dstrect_zoom_ptr == NULL ? dstrect : dstrect_zoom_ptr) ) {
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
			nqiv_unload_image_form_fallback_texture(form);
			form->fallback_texture = form->texture;
			if(form->texture == NULL) {
				nqiv_log_write( &state->logger, NQIV_LOG_ERROR, "Failed to load texture for image form %s (%s).\n", form->path, SDL_GetError() );
				nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
				omp_unset_lock(&image->lock);
				nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
				return false;
			}
		} else {
			if(first_frame || hard) {
				state->is_loading = true;
				if( state->show_loading_indicator && !render_texture(&cleared, dstrect, state, state->texture_montage_unloaded_background, NULL, dstrect_zoom_ptr == NULL ? dstrect : dstrect_zoom_ptr) ) {
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
				event.options.image_load.thumbnail_options.clear_error = state->images.thumbnail.save;
				event.options.image_load.create_thumbnail = state->images.thumbnail.save;
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
				event.options.image_load.thumbnail_options.next_frame = next_frame && !first_frame && form->animation.frame_rendered;
				if( !nqiv_send_thread_event(state, base_priority + NQIV_EVENT_PRIORITY_THUMBNAIL_LOAD, &event) ) {
					nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
					omp_unset_lock(&image->lock);
					nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
					return false;
				}
				/* TODO Signal to load thumbnail image if it's available, otherwise use main */
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
				if( image->image.vips != NULL && ( hard || next_frame || resample_zoom || (first_frame && image->image.animation.frame != 0) ) ) {
					event.options.image_load.image_options.raw = true;
					event.options.image_load.image_options.surface = true;
				} else {
					event.options.image_load.image_options.raw_soft = true;
					event.options.image_load.image_options.surface_soft = true;
				}
				event.options.image_load.image_options.first_frame = first_frame;
				event.options.image_load.image_options.next_frame = next_frame && !first_frame && form->animation.frame_rendered;
				if( !nqiv_send_thread_event(state, base_priority + NQIV_EVENT_PRIORITY_IMAGE_LOAD, &event) ) {
					nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
					omp_unset_lock(&image->lock);
					nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
					return false;
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
			state->is_loading = false;
			if( dstrect_zoom_ptr != NULL && !nqiv_state_update_alpha_background_dimensions(state, dstrect_zoom_ptr->w, dstrect_zoom_ptr->h) ) {
				nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
				omp_unset_lock(&image->lock);
				nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
				return false;
			}
			if( !render_texture(&cleared, dstrect, state, state->texture_alpha_background, NULL, dstrect_zoom_ptr) ) {
				nqiv_log_write(&state->logger, NQIV_LOG_ERROR, "Failed to draw image alpha background.\n");
				nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
				omp_unset_lock(&image->lock);
				nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
				return false;
			}
			if( !render_texture(&cleared, NULL, state, form->texture, srcrect_ptr, dstrect_zoom_ptr) ) {
				nqiv_log_write(&state->logger, NQIV_LOG_ERROR, "Failed to draw image texture.\n");
				nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
				omp_unset_lock(&image->lock);
				nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
				return false;
			}
			state->first_frame_pending = false;
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
				event.options.image_load.image_options.raw = true;
				event.options.image_load.image_options.surface = true;
				event.options.image_load.image_options.first_frame = first_frame;
				event.options.image_load.image_options.next_frame = next_frame && !first_frame && form->animation.frame_rendered;
				if( !nqiv_send_thread_event(state, base_priority + NQIV_EVENT_PRIORITY_IMAGE_LOAD_ANIMATION, &event) ) {
					nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
					omp_unset_lock(&image->lock);
					nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
					return false;
				}
			}
		}
	}
	if(selected) {
		if( !render_texture(&cleared, NULL, state, state->texture_montage_selection, NULL, dstrect) ) {
			nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
			omp_unset_lock(&image->lock);
			nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
			return false;
		}
	}
	if(image->marked && is_montage) {
		if( !render_texture(&cleared, NULL, state, state->texture_montage_mark, NULL, dstrect) ) {
			nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
			omp_unset_lock(&image->lock);
			nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
			return false;
		}
	}
	nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
	omp_unset_lock(&image->lock);
	nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
	if(is_montage) {
		state->is_loading = false;
	}
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
	snprintf( count_string, INT_MAX_STRLEN, "%d", nqiv_array_get_units_count(state->images.images) );
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
		!state->in_montage ? "% (" : "",
		!state->in_montage ? nqiv_zoom_default_names[state->zoom_default] : "",
		!state->in_montage ? ") - " : "",
		image->marked ? "MARKED - " : "",
		state->is_loading ? "LOADING - " : "",
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

bool render_montage(nqiv_state* state, const bool hard, const bool preload_only)
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
	if(!preload_only && SDL_RenderClear(state->renderer) != 0) {
		nqiv_log_write(&state->logger, NQIV_LOG_ERROR, "Failed to clear renderer for montage.\n");
		return false;
	}
	if(!preload_only && SDL_RenderCopy(state->renderer, state->texture_background, NULL, NULL) != 0) {
		nqiv_log_write(&state->logger, NQIV_LOG_ERROR, "Failed to copy texture background for montage.\n");
		return false;
	}
	state->render_cleared = !preload_only;
	const int images_len = nqiv_array_get_units_count(state->images.images);
	const int raw_start_idx = state->montage.positions.start - state->montage.preload.behind;
	const int raw_end = state->montage.positions.end + state->montage.preload.ahead;
	const int start_idx = raw_start_idx >= 0 ? raw_start_idx : 0;
	const int end = raw_end <= images_len ? raw_end : images_len;
	nqiv_image** images = state->logger.streams->data;
	int idx;
	nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Preload Start: %d Preload End: %d Montage Start: %d Montage Selection: %d Montage End %d\n", start_idx, end, state->montage.positions.start, state->montage.positions.selection, state->montage.positions.end);
	for(idx = start_idx; idx < end; ++idx) {
		nqiv_image* image = images[idx];
		if(idx >= state->montage.positions.start && idx < state->montage.positions.end && !preload_only) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Rendering montage image %s at %d.\n", image->image.path, idx);
			SDL_Rect dstrect;
			nqiv_montage_get_image_rect(&state->montage, idx, &dstrect);
			if( !render_from_form(state, image, true, &dstrect, true, true, false, state->montage.positions.selection == idx, hard, true, 0) ) {
				return false;
			}
			if( idx == state->montage.positions.selection && !set_title(state, image) ) {
				return false;
			}
		} else {
			if( !render_from_form(state, image, true, NULL, true, true, false, state->montage.positions.selection == idx, hard, true, 1) ) {
				return false;
			}
		}
	}
	return true;
}

bool render_image(nqiv_state* state, const bool start, const bool hard)
{
	nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Rendering selected image.\n");
	nqiv_image* image = ( (nqiv_image**)state->images.images->data )[state->montage.positions.selection];
	assert( state->montage.positions.selection < nqiv_array_get_units_count(state->images.images) );
	SDL_Rect dstrect = {0};
	/* TODO RECT DONE BUT ASPECT RATIO */
	SDL_GetWindowSizeInPixels(state->window, &dstrect.w, &dstrect.h);
	if( !render_from_form(state, image, false, &dstrect, false, start, true, false, hard, true, 0) ) {
		return false;
	}
	const bool render_cleared = state->render_cleared;
	if( !render_montage(state, false, true) ) {
		return false;
	}
	if( !set_title(state, image) ) {
		return false;
	}
	state->render_cleared = render_cleared;
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
		if( !render_montage(state, hard, false) ) {
			*running = false;
			*result = false;
		}
	} else {
		if( !render_image(state, first_render, hard) ) {
			*running = false;
			*result = false;
		}
	}
	if(*result != false && state->render_cleared) {
		SDL_RenderPresent(state->renderer);
		state->render_cleared = false;
	}
}

/* TODO Won't running simulated just also run the events on non-simulated events? */
void nqiv_handle_keyactions(nqiv_state* state, bool* running, bool* result, const bool simulated, const nqiv_keyrate_release_option released)
{
	nqiv_key_action action;
	while( nqiv_queue_pop_front(&state->key_actions, &action) ) {
		const int images_count = nqiv_array_get_units_count(state->images.images);
		assert(state->montage.positions.selection < images_count);
		nqiv_image** images = state->images.images->data;
		nqiv_image* image = images[state->montage.positions.selection];
		if( !simulated && !nqiv_keyrate_filter_action(&state->keystates, action, released) ) {
			/* NOOP */
		} else if(action == NQIV_KEY_ACTION_QUIT) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action quit.\n");
			*running = false;
			return;
		} else if(action == NQIV_KEY_ACTION_IMAGE_PREVIOUS) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action image previous.\n");
			if(!state->in_montage) {
				nqiv_montage_previous_selection(&state->montage);
				render_and_update(state, running, result, true, false);
			}
		} else if(action == NQIV_KEY_ACTION_IMAGE_NEXT) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action image next.\n");
			if(!state->in_montage) {
				nqiv_montage_next_selection(&state->montage);
				render_and_update(state, running, result, true, false);
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
			} else if(state->montage.dimensions.count > 2) {
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
				nqiv_image_manager_pan_down(&state->images);
				render_and_update(state, running, result, false, false);
			}
		} else if(action == NQIV_KEY_ACTION_ZOOM_IN_MORE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action zoom in more.\n");
			if(!state->in_montage) {
				nqiv_image_manager_zoom_in_more(&state->images);
				render_and_update(state, running, result, false, false);
			} else if(state->montage.dimensions.count > 2) {
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
		} else if(action == NQIV_KEY_ACTION_PAN_CENTER) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action pan center.\n");
			if(!state->in_montage) {
				nqiv_image_manager_pan_center(&state->images);
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
		} else if(action == NQIV_KEY_ACTION_FIT) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action fit.\n");
			state->images.zoom.image_to_viewport_ratio = state->images.zoom.fit_level;
			render_and_update(state, running, result, false, false);
		} else if(action == NQIV_KEY_ACTION_ACTUAL_SIZE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action actual_size.\n");
			state->images.zoom.image_to_viewport_ratio = state->images.zoom.actual_size_level;
			render_and_update(state, running, result, false, false);
		} else if(action == NQIV_KEY_ACTION_KEEP_FIT) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action keep_fit.\n");
			state->zoom_default = NQIV_ZOOM_DEFAULT_FIT;
			render_and_update(state, running, result, false, false);
		} else if(action == NQIV_KEY_ACTION_KEEP_ACTUAL_SIZE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action keep_actual_size.\n");
			state->zoom_default = NQIV_ZOOM_DEFAULT_ACTUAL;
			render_and_update(state, running, result, false, false);
		} else if(action == NQIV_KEY_ACTION_KEEP_CURRENT_ZOOM) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action keep_current_zoom.\n");
			state->zoom_default = NQIV_ZOOM_DEFAULT_KEEP;
			render_and_update(state, running, result, false, false);
		} else if(action == NQIV_KEY_ACTION_TOGGLE_KEPT_ZOOM) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action toggle_kept_zoom.\n");
			if(state->zoom_default == NQIV_ZOOM_DEFAULT_ACTUAL) {
				state->zoom_default = NQIV_ZOOM_DEFAULT_KEEP;
			} else {
				state->zoom_default += 1;
			}
			render_and_update(state, running, result, false, false);
		} else if (action == NQIV_KEY_ACTION_SCALE_MODE_NEAREST) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action scale_mode_nearest.\n");
			state->texture_scale_mode = SDL_ScaleModeNearest;
			render_and_update(state, running, result, false, false);
		} else if (action == NQIV_KEY_ACTION_SCALE_MODE_LINEAR) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action scale_mode_linear.\n");
			state->texture_scale_mode = SDL_ScaleModeLinear;
			render_and_update(state, running, result, false, false);
		} else if (action == NQIV_KEY_ACTION_SCALE_MODE_ANISOTROPIC) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action scale_mode_anisotropic.\n");
			state->texture_scale_mode = SDL_ScaleModeBest;
			render_and_update(state, running, result, false, false);
		} else if (action == NQIV_KEY_ACTION_TOGGLE_SCALE_MODE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action toggle_scale_mode.\n");
			if(state->texture_scale_mode == SDL_ScaleModeNearest) {
				state->texture_scale_mode = SDL_ScaleModeLinear;
			} else if(state->texture_scale_mode == SDL_ScaleModeLinear) {
				state->texture_scale_mode = SDL_ScaleModeBest;
			} else {
				assert(state->texture_scale_mode == SDL_ScaleModeBest);
				state->texture_scale_mode = SDL_ScaleModeNearest;
			}
			render_and_update(state, running, result, false, false);
		} else if(action == NQIV_KEY_ACTION_IMAGE_MARK_TOGGLE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action image mark.\n");
			image->marked = !image->marked;
			nqiv_log_write(&state->logger, NQIV_LOG_INFO, "%sarked %s\n", image->marked ? "Unm" : "M", image->image.path);
			render_and_update(state, running, result, false, false);
		} else if(action == NQIV_KEY_ACTION_IMAGE_MARK) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action image mark.\n");
			image->marked = true;
			nqiv_log_write(&state->logger, NQIV_LOG_INFO, "%sarked %s\n", image->marked ? "Unm" : "M", image->image.path);
			render_and_update(state, running, result, false, false);
		} else if(action == NQIV_KEY_ACTION_IMAGE_UNMARK) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action image unmark.\n");
			image->marked = false;
			nqiv_log_write(&state->logger, NQIV_LOG_INFO, "%sarked %s\n", image->marked ? "Unm" : "M", image->image.path);
			render_and_update(state, running, result, false, false);
		} else if(action == NQIV_KEY_ACTION_PRINT_MARKED) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action image print marked.\n");
			int iidx;
			for(iidx = 0; iidx < images_count; ++iidx) {
				if(images[iidx]->marked) {
					fprintf(stdout, "%s\n", images[iidx]->image.path);
				}
			}
			render_and_update(state, running, result, false, false);
		} else if(action == NQIV_KEY_ACTION_MONTAGE_SELECT_AT_MOUSE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action montage select at mouse.\n");
			if(state->in_montage) {
				int x, y;
				SDL_GetMouseState(&x, &y);
				nqiv_montage_set_selection( &state->montage, nqiv_montage_find_index_at_point(&state->montage, x, y) );
				render_and_update(state, running, result, false, false);
			}
		} else if(action == NQIV_KEY_ACTION_IMAGE_MARK_AT_MOUSE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action image mark at mouse.\n");
			if(state->in_montage) {
				int x, y;
				SDL_GetMouseState(&x, &y);
				nqiv_image* tmp_image = images[nqiv_montage_find_index_at_point(&state->montage, x, y)];
				tmp_image->marked = true;
				nqiv_log_write(&state->logger, NQIV_LOG_INFO, "%sarked %s\n", tmp_image->marked ? "Unm" : "M", tmp_image->image.path);
				render_and_update(state, running, result, false, false);
			}
		} else if(action == NQIV_KEY_ACTION_IMAGE_UNMARK_AT_MOUSE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action image unmark at mouse.\n");
			if(state->in_montage) {
				int x, y;
				SDL_GetMouseState(&x, &y);
				nqiv_image* tmp_image = images[nqiv_montage_find_index_at_point(&state->montage, x, y)];
				tmp_image->marked = false;
				nqiv_log_write(&state->logger, NQIV_LOG_INFO, "%sarked %s\n", tmp_image->marked ? "Unm" : "M", tmp_image->image.path);
				render_and_update(state, running, result, false, false);
			}
		} else if(action == NQIV_KEY_ACTION_IMAGE_MARK_TOGGLE_AT_MOUSE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action image unmark at mouse.\n");
			if(state->in_montage) {
				int x, y;
				SDL_GetMouseState(&x, &y);
				nqiv_image* tmp_image = images[nqiv_montage_find_index_at_point(&state->montage, x, y)];
				tmp_image->marked = !tmp_image->marked;
				nqiv_log_write(&state->logger, NQIV_LOG_INFO, "%sarked %s\n", tmp_image->marked ? "Unm" : "M", tmp_image->image.path);
				render_and_update(state, running, result, false, false);
			}
		} else if(action == NQIV_KEY_ACTION_START_MOUSE_PAN) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action start mouse pan.\n");
			if(!state->in_montage) {
				state->is_mouse_panning = true;
			}
		} else if(action == NQIV_KEY_ACTION_END_MOUSE_PAN) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action end mouse pan.\n");
			if(!state->in_montage) {
				state->is_mouse_panning = false;
			}
		} else if (action == NQIV_KEY_ACTION_IMAGE_ZOOM_IN) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action image zoom in.\n");
			if(!state->in_montage) {
				nqiv_image_manager_zoom_in(&state->images);
				render_and_update(state, running, result, false, false);
			}
		} else if (action == NQIV_KEY_ACTION_IMAGE_ZOOM_OUT) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action image zoom out.\n");
			if(!state->in_montage) {
				nqiv_image_manager_zoom_out(&state->images);
				render_and_update(state, running, result, false, false);
			}
		} else if (action == NQIV_KEY_ACTION_IMAGE_ZOOM_IN_MORE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action image zoom in more.\n");
			if(!state->in_montage) {
				nqiv_image_manager_zoom_in_more(&state->images);
				render_and_update(state, running, result, false, false);
			}
		} else if (action == NQIV_KEY_ACTION_IMAGE_ZOOM_OUT_MORE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action image zoom in out.\n");
			if(!state->in_montage) {
				nqiv_image_manager_zoom_out_more(&state->images);
				render_and_update(state, running, result, false, false);
			}
		} else if(action == NQIV_KEY_ACTION_RELOAD) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action reload.\n");
			render_and_update(state, running, result, true, true);
		}
	}
}

void nqiv_set_match_keymods(nqiv_key_match* match)
{
	const SDL_Keymod mods = SDL_GetModState();
	if( (mods & ~KMOD_GUI & ~KMOD_SCROLL & ~KMOD_NUM) != 0 ) {
		assert( (match->mode & NQIV_KEY_MATCH_MODE_KEY_MOD) == 0 );
		assert(match->data.key.mod == 0);
		match->mode |= NQIV_KEY_MATCH_MODE_KEY_MOD;
		match->data.key.mod = mods;
	}
}

bool nqiv_master_thread(nqiv_state* state)
{
	bool result = true;
	bool running = true;
	while(running) {
		nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Waiting on SDL event.\n");
		SDL_PumpEvents();
		SDL_Event input_event = {0};
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
						render_and_update(state, &running, &result, false, false);
					} else if( ( Uint32)input_event.user.code == state->cfg_event_number ) {
						nqiv_handle_keyactions(state, &running, &result, false, NQIV_KEYRATE_ON_DOWN); /* TODO No simulated actions for now. */
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
				nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received key event.\n");
				{
					nqiv_key_match match = {0};
					if(input_event.key.keysym.scancode != SDL_SCANCODE_UNKNOWN) {
						match.mode |= NQIV_KEY_MATCH_MODE_KEY;
					}
					if( (input_event.key.keysym.mod & ~KMOD_GUI & ~KMOD_SCROLL & ~KMOD_NUM) != 0 ) {
						match.mode |= NQIV_KEY_MATCH_MODE_KEY_MOD;
					}
					memcpy( &match.data.key, &input_event.key.keysym, sizeof(SDL_Keysym) );
					const nqiv_key_lookup_summary lookup_summary = nqiv_keybind_lookup(&state->keybinds, &match, &state->key_actions);
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
						nqiv_handle_keyactions(state, &running, &result, false, input_event.type == SDL_KEYUP ? NQIV_KEYRATE_ON_UP : NQIV_KEYRATE_ON_DOWN);
						render_and_update(state, &running, &result, false, false);
					}
				}
				break;
			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
				assert(input_event.type == SDL_MOUSEBUTTONDOWN || input_event.type == SDL_MOUSEBUTTONUP);
				nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received mouse button event.\n");
				{
					nqiv_key_match match = {0};
					match.mode |= NQIV_KEY_MATCH_MODE_MOUSE_BUTTON;
					memcpy( &match.data.mouse_button, &input_event.button, sizeof(SDL_MouseButtonEvent) );
					nqiv_set_match_keymods(&match);
					const nqiv_key_lookup_summary lookup_summary = nqiv_keybind_lookup(&state->keybinds, &match, &state->key_actions);
					if(lookup_summary == NQIV_KEY_LOOKUP_FAILURE) {
						running = false;
						result = false;
					} else if(lookup_summary == NQIV_KEY_LOOKUP_FOUND) {
						nqiv_handle_keyactions(state, &running, &result, false, input_event.type == SDL_MOUSEBUTTONUP ? NQIV_KEYRATE_ON_UP : NQIV_KEYRATE_ON_DOWN);
						render_and_update(state, &running, &result, false, false);
					}
				}
				break;
			case SDL_MOUSEWHEEL:
				assert(input_event.type == SDL_MOUSEWHEEL);
				nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received mouse wheel event.\n");
				{
					nqiv_key_match match = {0};
					if(input_event.wheel.x < 0) {
						match.mode |= NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_LEFT;
					}
					if(input_event.wheel.x > 0) {
						match.mode |= NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_RIGHT;
					}
					if(input_event.wheel.y < 0) {
						match.mode |= NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_BACKWARD;
					}
					if(input_event.wheel.y > 0) {
						match.mode |= NQIV_KEY_MATCH_MODE_MOUSE_WHEEL_FORWARD;
					}
					nqiv_set_match_keymods(&match);
					const nqiv_key_lookup_summary lookup_summary = nqiv_keybind_lookup(&state->keybinds, &match, &state->key_actions);
					if(lookup_summary == NQIV_KEY_LOOKUP_FAILURE) {
						running = false;
						result = false;
					} else if(lookup_summary == NQIV_KEY_LOOKUP_FOUND) {
						nqiv_handle_keyactions(state, &running, &result, false, NQIV_KEYRATE_ON_DOWN | NQIV_KEYRATE_ON_UP);
						render_and_update(state, &running, &result, false, false);
					}
				}
				break;
			case SDL_MOUSEMOTION:
				nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received mouse motion event.\n");
				if(state->is_mouse_panning && !state->in_montage) {
					SDL_Rect coordinates = {0};
					SDL_GetWindowSizeInPixels(state->window, &coordinates.w, &coordinates.h);
					coordinates.x = input_event.motion.xrel;
					coordinates.y = input_event.motion.yrel;
					nqiv_image_manager_pan_coordinates(&state->images, &coordinates);
					render_and_update(state, &running, &result, false, false);
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
	}
	return result;
}

bool nqiv_run(nqiv_state* state)
{
	bool result;
	bool* result_ptr = &result;
	const int thread_count = state->thread_count;
	const int thread_event_interval = state->thread_event_interval;
	const int extra_wakeup_delay = state->extra_wakeup_delay;
	nqiv_log_ctx* logger = &state->logger;
	nqiv_priority_queue* thread_queue = &state->thread_queue;
	const int64_t* thread_event_transaction_group = &state->thread_event_transaction_group;
	omp_lock_t* thread_event_transaction_group_lock = &state->thread_event_transaction_group_lock;
	const Uint32 event_code = state->thread_event_number;
	#pragma omp parallel default(none) firstprivate(state, logger, thread_count, extra_wakeup_delay, thread_event_interval, thread_queue, event_code, result_ptr, thread_event_transaction_group, thread_event_transaction_group_lock) num_threads(thread_count + 1)
	{
		#pragma omp master
		{
			int thread;
			for(thread = 0; thread < thread_count; ++thread) {
				#pragma omp task default(none) firstprivate(logger, thread_queue, extra_wakeup_delay, thread_event_interval, event_code, thread_event_transaction_group, thread_event_transaction_group_lock)
				nqiv_worker_main(logger, thread_queue, extra_wakeup_delay, thread_event_interval, event_code, thread_event_transaction_group, thread_event_transaction_group_lock);
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
