#include "platform.h"

#include <stdio.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <assert.h>

#include <vips/vips.h>
#include <SDL2/SDL.h>
#include <omp.h>

#include "typedefs.h"
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

/*
 * nqiv follows this general method of operation:
 * - Initialize data structures and dependencies.
 * - Parse the first argument pass to determine which default config to use.
 * - Parse the second argument pass with default config to actually take further
 *   options. In general, command line args are primarily a way to funnel
 *   commands into the parser specified in cmd.h
 * - Load a list of images as positional arguments.
 * - Start worker threads. See worker.h
 * - Start the master thread. The master thread follows this cycle:
 *   - Wait on an SDL event
 *   - If it's a key/mouse-related event, match and filter the keybind,
 *     then perform the related action. Otherwise, it can be a response from
 *     a worker or a command. Finally,update the state and do any needed rendering.
 *     - Do pruning if necessary.
 *     - Update transaction group depending on state of montage.
 *     - Dispatch relevant loading events and load textures.
 *     - Set the title.
 *     - Render the image or montage.
 *     - Repaint the screen as necessary.
 */

void nqiv_close_log_streams(nqiv_state* state)
{
	const int streams_len = nqiv_array_get_units_count(state->logger.streams);
	assert(streams_len == nqiv_array_get_units_count(state->logger_stream_names));
	FILE** fileptrs = state->logger.streams->data;
	char** nameptrs = state->logger_stream_names->data;
	int    idx;
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
	memset(state, 0, sizeof(nqiv_state));
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
}

bool nqiv_setup_sdl(nqiv_state* state)
{
	if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
		nqiv_log_write(&state->logger, NQIV_LOG_ERROR, "Failed to init SDL. SDL Error: %s\n",
		               SDL_GetError());
		return false;
	}
	state->SDL_inited = true;
	state->window = SDL_CreateWindow("nqiv", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800,
	                                 600, SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
	if(state->window == NULL) {
		nqiv_log_write(&state->logger, NQIV_LOG_ERROR,
		               "Failed to create SDL Window. SDL Error: %s\n", SDL_GetError());
		return false;
	}
	state->renderer = SDL_CreateRenderer(state->window, -1,
	                                     SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
	if(state->renderer == NULL) {
		nqiv_log_write(&state->logger, NQIV_LOG_ERROR,
		               "Failed to create SDL Renderer. SDL Error: %s\n", SDL_GetError());
		return false;
	}
	SDL_RendererInfo renderer_info = {0};
	if(SDL_GetRendererInfo(state->renderer, &renderer_info) != 0) {
		nqiv_log_write(&state->logger, NQIV_LOG_ERROR,
		               "Failed to retrieve SDL renderer info. SDL Error: %s\n", SDL_GetError());
		return false;
	}
	state->images.max_texture_width = renderer_info.max_texture_width;
	state->images.max_texture_height = renderer_info.max_texture_height;
	if(SDL_SetRenderDrawColor(state->renderer, 0, 0, 0, 255) != 0) {
		nqiv_log_write(&state->logger, NQIV_LOG_ERROR,
		               "Failed to set SDL Renderer draw color. SDL Error: %s\n", SDL_GetError());
		return false;
	}

	if(!nqiv_state_create_single_color_texture(state, &state->background_color,
	                                           &state->texture_background)
	   || !nqiv_state_create_single_color_texture(state, &state->loading_color,
	                                              &state->texture_montage_unloaded_background)
	   || !nqiv_state_create_single_color_texture(state, &state->error_color,
	                                              &state->texture_montage_error_background)
	   || !nqiv_state_create_thumbnail_selection_texture(state)
	   || !nqiv_state_create_mark_texture(state)) {
		return false;
	}

	if(state->thread_event_number == 0) {
		state->thread_event_number = SDL_RegisterEvents(1);
		if(state->thread_event_number == 0xFFFFFFFF) {
			nqiv_log_write(&state->logger, NQIV_LOG_ERROR,
			               "Failed to create SDL event for messages from threads. SDL Error: %s\n",
			               SDL_GetError());
			return false;
		}
	}

	if(state->cfg_event_number == 0) {
		state->cfg_event_number = SDL_RegisterEvents(1);
		if(state->cfg_event_number == 0xFFFFFFFF) {
			nqiv_log_write(&state->logger, NQIV_LOG_ERROR,
			               "Failed to create SDL event for messages from threads. SDL Error: %s\n",
			               SDL_GetError());
			return false;
		}
	}

	SDL_Delay(
		1); /* So that we are completely guaranteed to have at least one ticks passed for SDL. */
	return true;
}

void nqiv_update_montage_dimensions(nqiv_state* state)
{
	int width, height;
	SDL_GetWindowSizeInPixels(state->window, &width, &height);
	nqiv_montage_calculate_dimensions(&state->montage, width, height);
}

void nqiv_setup_montage(nqiv_state* state)
{
	state->montage.logger = &state->logger;
	state->montage.images = &state->images;
	nqiv_update_montage_dimensions(state);
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

bool nqiv_load_builtin_config(nqiv_state* state, const char* exe, const char* default_config_path)
{
	char thumbnail_cmd[PATH_MAX + 19 + 1] = "set thumbnail path ";
	if(!nqiv_get_default_cfg_thumbnail_dir(thumbnail_cmd + 19, PATH_MAX)) {
		nqiv_log_write(&state->logger, NQIV_LOG_ERROR,
		               "Failed to retrieve reasonable path to save thumbnails in the local "
		               "environment. Proceeding without.\n");
		thumbnail_cmd[19] = '\0';
	}
	const char* cmds[] = {
		"set log prefix #level# #time:%Y-%m-%d %T%z# ",
		thumbnail_cmd,
		"append log stream stderr",
		"append keybind Q=quit",
		"append keybind Home=allow_on_down+deny_on_up+montage_start",
		"append keybind End=allow_on_down+deny_on_up+montage_end",
		"append keybind PageUp=allow_on_down+deny_on_up+page_up",
		"append keybind PageDown=allow_on_down+deny_on_up+page_down",
		"append keybind Backspace=allow_on_down+deny_on_up+image_previous",
		"append keybind Backspace=allow_on_down+deny_on_up+montage_left",
		"append keybind Left=allow_on_down+deny_on_up+montage_left",
		"append keybind Right=allow_on_down+deny_on_up+montage_right",
		"append keybind Up=allow_on_down+deny_on_up+montage_up",
		"append keybind Down=allow_on_down+deny_on_up+montage_down",
		"append keybind Space=allow_on_down+deny_on_up+image_next",
		"append keybind Space=allow_on_down+deny_on_up+montage_right",
		"append keybind Return=set_viewing",
		"append keybind M=toggle_montage",
		"append keybind Left=allow_on_down+deny_on_up+pan_left",
		"append keybind Up=allow_on_down+deny_on_up+pan_up",
		"append keybind Right=allow_on_down+deny_on_up+pan_right",
		"append keybind Down=allow_on_down+deny_on_up+pan_down",
		"append keybind Z=allow_on_down+deny_on_up+zoom_in",
		"append keybind Z+shift=allow_on_down+deny_on_up+zoom_out",
		"append keybind ctrl+Z=allow_on_down+deny_on_up+zoom_in_more",
		"append keybind ctrl+Z+shift=allow_on_down+deny_on_up+zoom_out_more",
		"append keybind Keypad +=allow_on_down+deny_on_up+zoom_in",
		"append keybind Keypad -=allow_on_down+deny_on_up+zoom_out",
		"append keybind shift+Keypad +=allow_on_down+deny_on_up+zoom_in_more",
		"append keybind shift+Keypad -=allow_on_down+deny_on_up+zoom_out_more",
		"append keybind ctrl+Left=allow_on_down+deny_on_up+pan_left_more",
		"append keybind ctrl+Up=allow_on_down+deny_on_up+pan_up_more",
		"append keybind ctrl+Right=allow_on_down+deny_on_up+pan_right_more",
		"append keybind ctrl+Down=allow_on_down+deny_on_up+pan_down_more",
		"append keybind C=pan_center",
		"append keybind S=toggle_stretch",
		"append keybind '=image_mark_toggle",
		"append keybind shift+'=print_marked",
		"append keybind ;=allow_on_down+deny_on_up+image_mark",
		"append keybind ;=allow_on_down+deny_on_up+montage_right",
		"append keybind ;=allow_on_down+deny_on_up+image_next",
		"append keybind R=reload",
		"append keybind F=fit",
		"append keybind A=actual_size",
		"append keybind D=toggle_kept_zoom",
		"append keybind T=toggle_scale_mode",
		"append keybind shift+F=keep_fit",
		"append keybind shift+A=keep_actual_size",
		"append keybind shift+D=keep_current_zoom",
		"append keybind mouse1=montage_select_at_mouse",
		"append keybind mouse1_double=set_viewing",
		"append keybind shift+mouse1=montage_select_at_mouse",
		"append keybind shift+mouse1=image_mark_toggle",
		"append keybind mouse1=allow_on_down+deny_on_up+start_mouse_pan",
		"append keybind mouse1=end_mouse_pan",
		"append keybind scroll_forward=image_zoom_in",
		"append keybind scroll_backward=image_zoom_out",
		"append keybind scroll_forward=montage_up",
		"append keybind scroll_backward=montage_down",
		"append keybind shift+scroll_forward=zoom_in",
		"append keybind shift+scroll_backward=zoom_out",
		"append pruner or thumbnail no image texture self_opened unload surface vips",
		"append pruner and no thumbnail image texture self_opened not_animated unload surface "
		"vips",
		"append pruner or no thumbnail image texture self_opened unload surface",
		"append pruner and thumbnail no image texture self_opened image no thumbnail not_animated "
		"hard unload image thumbnail surface vips",
		"append pruner or thumbnail image texture loaded_behind 0 0 loaded_ahead 0 0 surface "
		"loaded_behind 0 0 loaded_ahead 0 0 vips "
		"loaded_behind 0 0 loaded_ahead 0 0 hard unload texture surface vips",
		"append pruner or sum 0 thumbnail image texture bytes_ahead 0 0 bytes_behind 0 0 surface "
		"bytes_ahead 0 0 bytes_behind 0 0 vips bytes_ahead 0 "
		"0 bytes_behind 0 0 hard unload texture surface vips",
		NULL,
	};
	int idx;
	for(idx = 0; cmds[idx] != NULL; ++idx) {
		if(!nqiv_cmd_add_line_and_parse(&state->cmds, cmds[idx])) {
			return false;
		}
	}
	if(default_config_path != NULL) {
		fprintf(stderr, "Unable to find default config path. ");
		nqiv_suggest_cfg_setup(exe);
	}
	return true;
}

void nqiv_print_version(void)
{
	fprintf(stderr, "nqiv version: " VERSION "\n");
}

void nqiv_print_args(const char* exe)
{
	nqiv_print_version();
	fprintf(stderr, "\n");
	fprintf(stderr, "-s/--cmd-from-stdin Read commands from stdin.\n");
	fprintf(stderr, "-B/--built-in-config Force the built in config to load. It will do so in the "
	                "order it is specified.\n");
	fprintf(stderr, "-N/--no-default-cfg Do not try to load the default config file (or settings "
	                "if it does not exist).\n");
	fprintf(stderr, "-c/--cmd <cmd> Issue a single command to the image viewer's command "
	                "processor. Also pass help to get information about commands.\n");
	fprintf(stderr, "-C/--cfg <path> Specify a config file to be read by the image viewer's "
	                "command processor.\n");
	fprintf(stderr, "-v/--version Print version info.\n");
	fprintf(stderr, "-h/--help Print this help message.\n");
	fprintf(stderr, "\n");
	nqiv_suggest_cfg_setup(exe);
}

nqiv_op_result nqiv_parse_args(char* argv[], nqiv_state* state)
{
	state->zoom_default = NQIV_ZOOM_DEFAULT_FIT;
	state->texture_scale_mode = SDL_ScaleModeBest;
	state->no_resample_oversized = true;
	state->show_loading_indicator = true;
	state->thread_count = omp_get_num_procs() / 3;
	state->thread_count = state->thread_count > 0 ? state->thread_count : 1;
	state->vips_threads = omp_get_num_procs() / 2;
	state->vips_threads = state->vips_threads > 0 ? state->vips_threads : 1;
	state->thread_event_interval = 100 / state->thread_count;
	state->thread_event_interval =
		state->thread_event_interval > 0 ? state->thread_event_interval : 1;
	state->extra_wakeup_delay = state->thread_count * 20;
	state->prune_delay = 5000 / state->extra_wakeup_delay;
	state->event_timeout = 250000 / state->extra_wakeup_delay;
	vips_concurrency_set(state->vips_threads);
	state->logger_stream_names = nqiv_array_create(sizeof(char*), STARTING_QUEUE_LENGTH);
	if(state->logger_stream_names == NULL) {
		fputs("Failed to initialize logger stream names list.\n", stderr);
		return NQIV_FAIL;
	}
	nqiv_log_init(&state->logger);
	if(!nqiv_cmd_manager_init(&state->cmds, state)) {
		return NQIV_FAIL;
	}
	state->logger.level = NQIV_LOG_WARNING;
	if(!nqiv_check_and_print_logger_error(&state->logger)) {
		return NQIV_FAIL;
	}
	if(!nqiv_image_manager_init(&state->images, &state->logger, STARTING_QUEUE_LENGTH)) {
		fputs("Failed to initialize image manager.\n", stderr);
		return NQIV_FAIL;
	}
	if(!nqiv_pruner_init(&state->pruner, &state->logger, STARTING_QUEUE_LENGTH)) {
		fputs("Failed to initialize pruner.\n", stderr);
		return NQIV_FAIL;
	}
	if(!nqiv_keybind_create_manager(&state->keybinds, &state->logger, STARTING_QUEUE_LENGTH)) {
		fputs("Failed to initialize keybind manager.\n", stderr);
		return NQIV_FAIL;
	}
	nqiv_set_keyrate_defaults(&state->keystates);
	if(!nqiv_priority_queue_init(&state->thread_queue, &state->logger, sizeof(nqiv_event),
	                             STARTING_QUEUE_LENGTH, THREAD_QUEUE_BIN_COUNT)
	   || !nqiv_priority_queue_set_max_data_length(&state->thread_queue, THREAD_QUEUE_MAX_LENGTH)
	   || !nqiv_priority_queue_set_min_add_count(&state->thread_queue, THREAD_QUEUE_ADD_COUNT)) {
		fputs("Failed to initialize thread queue.\n", stderr);
		return NQIV_FAIL;
	}
	state->images.thread_queue = &state->thread_queue;
	if(!nqiv_queue_init(&state->key_actions, &state->logger, sizeof(nqiv_keybind_pair*),
	                    STARTING_QUEUE_LENGTH)) {
		fputs("Failed to initialize key action queue.\n", stderr);
		return NQIV_FAIL;
	}
	nqiv_state_set_default_colors(state);
	if(!nqiv_setup_sdl(state)) {
		nqiv_state_clear(state);
		return NQIV_FAIL;
	}
	nqiv_setup_montage(state);
	const struct optparse_long longopts[] = {
		{"cmd-from-stdin", 's', OPTPARSE_NONE},
        {"built-in-config", 'B', OPTPARSE_NONE},
		{"no-default-cfg", 'N', OPTPARSE_NONE},
        {"cmd", 'c', OPTPARSE_REQUIRED},
		{"cfg", 'C', OPTPARSE_REQUIRED},
        {"version", 'v', OPTPARSE_NONE},
		{"help", 'h', OPTPARSE_NONE},
        {0},
	};
	bool            load_default = true;
	struct optparse options;
	int             option;
	optparse_init(&options, argv);
	while((option = optparse_long(&options, longopts, NULL)) != -1) {
		switch(option) {
		case 'N':
			load_default = false;
			break;
		case 'h':
			nqiv_print_args(argv[0]);
			return NQIV_PASS;
		case 'v':
			nqiv_print_version();
			return NQIV_PASS;
		case '?':
			fprintf(stderr, "%s: %s\n", argv[0], options.errmsg);
			return NQIV_FAIL;
		case 's':
		case 'c':
		case 'B':
		case 'C':
			assert(true);
			break;
		default:
			assert(false);
		}
	}
	if(load_default) {
		char default_config_path[PATH_MAX + 1] = {0};
		if(nqiv_get_default_cfg(default_config_path, PATH_MAX)) {
			FILE* stream = nqiv_fopen(default_config_path, "r");
			if(stream != NULL) {
				const int c = fgetc(stream);
				if(c == EOF) {
					if(!nqiv_load_builtin_config(state, argv[0], default_config_path)) {
						return NQIV_FAIL;
					}
				} else {
					ungetc(c, stream);
					if(!nqiv_cmd_consume_stream(&state->cmds, stream)) {
						fprintf(stderr, "Failed to read commands from default config path %s\n",
						        default_config_path);
						fclose(stream);
						return NQIV_FAIL;
					}
				}
				fclose(stream);
			} else {
				if(!nqiv_load_builtin_config(state, argv[0], default_config_path)) {
					return NQIV_FAIL;
				}
			}
		}
	}
	bool success = true;
	optparse_init(&options, argv);
	while(success && (option = optparse_long(&options, longopts, NULL)) != -1) {
		switch(option) {
		case 's':
			success = success && nqiv_cmd_consume_stream(&state->cmds, stdin);
			break;
		case 'c':
			success = success && nqiv_cmd_add_line_and_parse(&state->cmds, options.optarg);
			break;
		case 'B':
			success = success && nqiv_load_builtin_config(state, argv[0], NULL);
			break;
		case 'C':
			success = success && nqiv_cmd_consume_stream_from_path(&state->cmds, options.optarg);
			break;
		case 'h':
			nqiv_print_args(argv[0]);
			return NQIV_PASS;
		case '?':
			fprintf(stderr, "%s: %s\n", argv[0], options.errmsg);
			return NQIV_FAIL;
		case 'N':
			assert(true);
			break;
		default:
			assert(false);
		}
	}
	if(!success) {
		return NQIV_FAIL;
	}
	if(!nqiv_setup_thread_info(state)) {
		nqiv_state_clear(state);
		return NQIV_FAIL;
	}
	const char* arg;
	while((arg = optparse_arg(&options))) {
		if(!nqiv_image_manager_append(&state->images, arg)) {
			return NQIV_FAIL;
		}
	}
	state->first_frame_pending = true;
	if(nqiv_array_get_units_count(state->images.images) > 1) {
		state->in_montage = true;
	} else if(nqiv_array_get_units_count(state->images.images) == 0) {
		fprintf(stderr, "No images specified. Quitting.\n");
		return NQIV_PASS;
	}
	nqiv_cmd_alert_main(&state->cmds);
	return NQIV_SUCCESS;
} /* parse_args */

bool nqiv_send_thread_event_base(nqiv_state*       state,
                                 const int         level,
                                 const nqiv_event* event,
                                 const bool        force)
{
	bool event_sent;
	if(force) {
		nqiv_priority_queue_push_force(&state->thread_queue, level, event);
		event_sent = true;
	} else {
		event_sent = nqiv_priority_queue_push(&state->thread_queue, level, event);
	}
	if(!event_sent) {
		nqiv_log_write(&state->logger, NQIV_LOG_ERROR, "Failed to send event.\n");
		return false;
	}
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

bool render_texture(bool*           cleared,
                    const SDL_Rect* cleardst,
                    nqiv_state*     state,
                    SDL_Texture*    texture,
                    SDL_Rect*       srcrect,
                    const SDL_Rect* dstrect)
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
	if(cleardst != NULL
	   && SDL_RenderCopy(state->renderer, state->texture_background, NULL, cleardst) != 0) {
		nqiv_log_write(&state->logger, NQIV_LOG_ERROR,
		               "Failed to clear rendering space using texture background.\n");
		return false;
	}
	if(SDL_SetTextureScaleMode(texture, state->texture_scale_mode) != 0) {
		nqiv_log_write(&state->logger, NQIV_LOG_ERROR, "Failed to set texture scale mode.\n");
		return false;
	}
	if(SDL_RenderCopy(state->renderer, texture, srcrect, dstrect) != 0) {
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
bool render_from_form(nqiv_state*     state,
                      nqiv_image*     image,
                      const bool      is_montage,
                      /* Where to draw to. */
                      const SDL_Rect* dstrect,
                      const bool      first_frame,
                      const bool      next_frame,
                      /* Selected in montage mode? */
                      const bool      selected,
                      /* Force reload */
                      const bool      hard,
                      /* Preload events will have lower priority (higher number), so actual priority
                         is added to this. */
                      const int       base_priority)
{
	bool             cleared = is_montage;
	nqiv_image_form* form = is_montage ? &image->thumbnail : &image->image;
	/* We try to lock the image. Don't wait on it and block the whole program, if not. Just use
	 * its fallback texture and return early. */
	if(!nqiv_image_test_lock(image)) {
		/* If this is a preload, there won't be a location to write to, so don't bother. Nowhere to
		 * render. */
		if(dstrect != NULL) {
			/* Track our own dimensions for the fallback texture. */
			SDL_Rect        tmp_srcrect;
			SDL_Rect        tmp_dstrect;
			const SDL_Rect* tmp_dstrect_ptr = dstrect;
			if(form->master_srcrect.w > 0 && form->master_srcrect.h > 0
			   && form->master_dstrect.w > 0 && form->master_dstrect.h > 0) {
				memcpy(&tmp_srcrect, &form->master_srcrect, sizeof(SDL_Rect));
				memcpy(&tmp_dstrect, &form->master_dstrect, sizeof(SDL_Rect));
				tmp_dstrect_ptr = &tmp_dstrect;
			}
			state->is_loading = !is_montage;
			bool clearedtmp = true;
			if(form->master_dimensions_set && form->fallback_texture != NULL
			   && form->master_srcrect.w > 0 && form->master_srcrect.h > 0
			   && form->master_dstrect.w > 0 && form->master_dstrect.h > 0) {
				nqiv_image_manager_calculate_zoom_parameters(&state->images, !is_montage,
				                                             &tmp_srcrect, &tmp_dstrect);
				nqiv_apply_zoom_modifications(state, first_frame);
				nqiv_image_manager_retrieve_zoomrect(
					&state->images, !is_montage, state->stretch_images, &tmp_srcrect, &tmp_dstrect);
				if(!nqiv_state_update_alpha_background_dimensions(state, tmp_dstrect.w,
				                                                  tmp_dstrect.h)) {
					return false;
				}
				if(!render_texture(&cleared, NULL, state, state->texture_alpha_background, NULL,
				                   &tmp_dstrect)) {
					return false;
				}
				if(!render_texture(&cleared, NULL, state, form->fallback_texture, &tmp_srcrect,
				                   &tmp_dstrect)) {
					return false;
				}
				/* Don't clear display if we're just drawing the loading indicator. */
			} else if(state->show_loading_indicator
			          && !render_texture(&clearedtmp, dstrect, state,
			                             state->texture_montage_unloaded_background, NULL,
			                             tmp_dstrect_ptr)) {
				return false;
			}
			if(selected && is_montage) {
				if(!render_texture(&cleared, NULL, state, state->texture_montage_selection, NULL,
				                   dstrect)) {
					return false;
				}
			}
			if(image->marked && is_montage) {
				if(!render_texture(&cleared, NULL, state, state->texture_montage_mark, NULL,
				                   dstrect)) {
					return false;
				}
			}
		}
		return true;
	}
	/* We must have locked the image by this point. */
	SDL_Rect  srcrect = {0};
	SDL_Rect* srcrect_ptr = &srcrect;
	SDL_Rect  dstrect_zoom = {0};
	SDL_Rect* dstrect_zoom_ptr = NULL;
	bool      resample_zoom = false; /* Do we have to reload a massive image? */
	/* Sane dimensions- sometimes the image or display may not be ready. */
	if(form->width > 0 && form->height > 0 && image->image.width > 0 && image->image.height > 0
	   && dstrect != NULL) {
		dstrect_zoom_ptr = &dstrect_zoom;
		if(is_montage) {
			/* Scale srcrect to thumbnail with appropriate aspect ratio. */
			srcrect.w = image->image.width;
			srcrect.h = image->image.height;
			if(form->width > srcrect.w) {
				const double multiplier = (double)form->width / (double)srcrect.w;
				srcrect.w = (int)((double)srcrect.w * multiplier);
				srcrect.h = (int)((double)srcrect.h * multiplier);
			}
			if(form->height > srcrect.h) {
				const double multiplier = (double)form->height / (double)srcrect.h;
				srcrect.w = (int)((double)srcrect.w * multiplier);
				srcrect.h = (int)((double)srcrect.h * multiplier);
			}
		} else {
			srcrect.w = form->width;
			srcrect.h = form->height;
		}
		dstrect_zoom.w = dstrect->w;
		dstrect_zoom.h = dstrect->h;
		dstrect_zoom.x = dstrect->x;
		dstrect_zoom.y = dstrect->y;
		/* Update master dimensions for fallback. */
		memcpy(&form->master_srcrect, &srcrect, sizeof(SDL_Rect));
		memcpy(&form->master_dstrect, &dstrect_zoom, sizeof(SDL_Rect));
		form->master_dimensions_set = true;
		nqiv_image_manager_calculate_zoom_parameters(&state->images, !is_montage, &srcrect,
		                                             dstrect_zoom_ptr);
		nqiv_apply_zoom_modifications(state, first_frame);
		nqiv_image_manager_retrieve_zoomrect(&state->images, !is_montage, state->stretch_images,
		                                     &srcrect, dstrect_zoom_ptr);
		if(!state->no_resample_oversized
		   && (form->height > state->images.max_texture_height
		       || form->width > state->images.max_texture_width)) {
			/* Unload texture and adjust sample area if image needs to be reloaded
			 * (no_resample_oversized, > texture limits, changed dimensions). */
			if(form->srcrect.x != srcrect.x || form->srcrect.y != srcrect.y
			   || form->srcrect.w != srcrect.w || form->srcrect.h != srcrect.h) {
				resample_zoom = true;
				nqiv_unload_image_form_texture(form);
				form->srcrect.x = srcrect.x;
				form->srcrect.y = srcrect.y;
				form->srcrect.w = srcrect.w;
				form->srcrect.h = srcrect.h;
			}
			/* Make sure to use entirety of this. */
			srcrect_ptr = NULL;
			/* No need to reload? Just prepare to resize entire image to manipulate. */
		} else {
			/* If dimension changed or unavailable, unload texture and prepare to resize whole
			 * thing. */
			if((form->srcrect.x != 0 || form->srcrect.y != 0 || form->srcrect.w != form->width
			    || form->srcrect.h != form->height)
			   || form->effective_height == 0 || form->effective_width == 0) {
				resample_zoom = true;
				nqiv_unload_image_form_texture(form);
				form->srcrect.x = 0;
				form->srcrect.y = 0;
				form->srcrect.w = form->width;
				form->srcrect.h = form->height;
				/* Otherwise, use what we have. */
			} else {
				srcrect.x = 0;
				srcrect.y = 0;
				srcrect.w = form->effective_width;
				srcrect.h = form->effective_height;
				dstrect_zoom.w = dstrect->w;
				dstrect_zoom.h = dstrect->h;
				dstrect_zoom.x = dstrect->x;
				dstrect_zoom.y = dstrect->y;
				memcpy(&form->master_srcrect, &srcrect, sizeof(SDL_Rect));
				memcpy(&form->master_dstrect, &dstrect_zoom, sizeof(SDL_Rect));
				form->master_dimensions_set = true;
				nqiv_image_manager_calculate_zoom_parameters(&state->images, !is_montage, &srcrect,
				                                             dstrect_zoom_ptr);
				nqiv_apply_zoom_modifications(state, first_frame);
				nqiv_image_manager_retrieve_zoomrect(
					&state->images, !is_montage, state->stretch_images, &srcrect, dstrect_zoom_ptr);
			}
		}
	}
	if(!is_montage && state->images.thumbnail.save) {
		if(!image->thumbnail_attempted) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG,
			               "Creating thumbnail for instance that won't load it.\n");
			nqiv_event event = {0};
			event.type = NQIV_EVENT_IMAGE_LOAD;
			event.options.image_load.image = image;
			event.options.image_load.set_thumbnail_path = true;
			event.options.image_load.thumbnail_options.clear_error = true;
			event.options.image_load.create_thumbnail = true;
			if(!nqiv_send_thread_event(
				   state, base_priority + NQIV_EVENT_PRIORITY_THUMBNAIL_SAVE_LOAD_NO, &event)) {
				nqiv_image_unlock(image);
				return false;
			}
		}
	}
	if(form->error) {
		/* If we're working with a thumbnail and a successful image, try to recover. */
		if(is_montage && !image->image.error) {
			/* If reloading, indicate so. */
			if(first_frame || hard) {
				if(!render_texture(&cleared, dstrect, state,
				                   state->texture_montage_unloaded_background, NULL,
				                   dstrect_zoom_ptr == NULL ? dstrect : dstrect_zoom_ptr)) {
					nqiv_image_unlock(image);
					return false;
				}
			}
			if(!image->thumbnail_attempted && state->images.thumbnail.save) {
				nqiv_log_write(&state->logger, NQIV_LOG_DEBUG,
				               "Creating thumbnail after failing to load it.\n");
				nqiv_event event = {0};
				event.type = NQIV_EVENT_IMAGE_LOAD;
				event.options.image_load.image = image;
				event.options.image_load.set_thumbnail_path = true;
				event.options.image_load.thumbnail_options.clear_error =
					state->images.thumbnail.save;
				event.options.image_load.create_thumbnail = true;
				if(!nqiv_send_thread_event(
					   state, base_priority + NQIV_EVENT_PRIORITY_THUMBNAIL_SAVE_LOAD_FAIL,
					   &event)) {
					nqiv_image_unlock(image);
					return false;
				}
				/* An 'ephemeral' thumbnail is generated from the image file, but not saved. */
			} else {
				nqiv_log_write(&state->logger, NQIV_LOG_DEBUG,
				               "Generating ephemeral thumbnail for image.\n");
				nqiv_event event = {0};
				event.type = NQIV_EVENT_IMAGE_LOAD;
				event.options.image_load.image = image;
				event.options.image_load.thumbnail_options.clear_error = true;
				if(hard) {
					event.options.image_load.thumbnail_options.vips = true;
				} else {
					event.options.image_load.thumbnail_options.vips_soft = true;
				}
				if(!nqiv_send_thread_event(
					   state, base_priority + NQIV_EVENT_PRIORITY_THUMBNAIL_LOAD_EPHEMERAL,
					   &event)) {
					nqiv_image_unlock(image);
					return false;
				}
			}
			/* If we can't recover, just show the error background. */
		} else {
			if(dstrect != NULL) {
				state->is_loading = false;
			}
			if(!render_texture(&cleared, dstrect, state, state->texture_montage_error_background,
			                   NULL, dstrect_zoom_ptr == NULL ? dstrect : dstrect_zoom_ptr)) {
				nqiv_image_unlock(image);
				return false;
			}
		}
		/* No error */
	} else {
		/* Unload the texture so we can return to the first frame. */
		if(form->texture != NULL && !is_montage && first_frame && form->animation.exists) {
			nqiv_unload_image_form_texture(form);
		}
		/* If we have a texture and don't need to render the next frame, do nothing. */
		if(form->texture != NULL && (!next_frame || !form->animation.frame_rendered)) {
			/* NOOP */
			/* Use the surface we have to make a texture, no need to resample or grab the next
			 * frame. */
		} else if(form->surface != NULL && !resample_zoom
		          && (is_montage || !first_frame || !form->animation.exists)) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Loading texture for image %s\n",
			               image->image.path);
			form->texture = SDL_CreateTextureFromSurface(state->renderer, form->surface);
			nqiv_unload_image_form_fallback_texture(form);
			form->fallback_texture = form->texture;
			if(form->texture == NULL) {
				nqiv_log_write(&state->logger, NQIV_LOG_ERROR,
				               "Failed to load texture for image form %s (%s).\n", form->path,
				               SDL_GetError());
				nqiv_image_unlock(image);
				return false;
			}
			/* Otherwise, we set the loading indicator where relevant and start sending events. */
		} else {
			if(first_frame || hard) {
				state->is_loading = true;
				if(state->show_loading_indicator
				   && !render_texture(&cleared, dstrect, state,
				                      state->texture_montage_unloaded_background, NULL,
				                      dstrect_zoom_ptr == NULL ? dstrect : dstrect_zoom_ptr)) {
					nqiv_image_unlock(image);
					return false;
				}
			}
			if(is_montage) {
				nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Loading thumbnail.\n");
				nqiv_event event = {0};
				event.type = NQIV_EVENT_IMAGE_LOAD;
				event.options.image_load.image = image;
				event.options.image_load.set_thumbnail_path = true;
				event.options.image_load.thumbnail_options.clear_error =
					state->images.thumbnail.save;
				event.options.image_load.create_thumbnail = state->images.thumbnail.save;
				if(hard) {
					event.options.image_load.thumbnail_options.vips = true;
					event.options.image_load.borrow_thumbnail_dimension_metadata = true;
				} else {
					event.options.image_load.thumbnail_options.vips_soft = true;
					event.options.image_load.borrow_thumbnail_dimension_metadata = true;
				}
				/* Force reload if we may not have the currently-needed data, for whatever reason.
				 * Most of this should never happen because thumbnails aren't animated at this time.
				 */
				if(hard || next_frame
				   || (first_frame && image->thumbnail.vips != NULL
				       && image->thumbnail.animation.frame != 0)) {
					event.options.image_load.thumbnail_options.surface = true;
				} else {
					event.options.image_load.thumbnail_options.surface_soft = true;
				}
				event.options.image_load.thumbnail_options.first_frame = first_frame;
				event.options.image_load.thumbnail_options.next_frame =
					next_frame && !first_frame && form->animation.frame_rendered;
				if(!nqiv_send_thread_event(
					   state, base_priority + NQIV_EVENT_PRIORITY_THUMBNAIL_LOAD, &event)) {
					nqiv_image_unlock(image);
					return false;
				}
			} else {
				nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Loading image %s\n",
				               image->image.path);
				nqiv_event event = {0};
				event.type = NQIV_EVENT_IMAGE_LOAD;
				event.options.image_load.image = image;
				if(hard) {
					event.options.image_load.image_options.vips = true;
				} else {
					event.options.image_load.image_options.vips_soft = true;
				}
				/* If due to animation, hard loading, or need for resampling, hard load data and
				 * surface. */
				if(image->image.vips != NULL
				   && (hard || next_frame || resample_zoom
				       || (first_frame && image->image.animation.frame != 0))) {
					event.options.image_load.image_options.surface = true;
				} else {
					event.options.image_load.image_options.surface_soft = true;
				}
				event.options.image_load.image_options.first_frame = first_frame;
				event.options.image_load.image_options.next_frame =
					next_frame && !first_frame && form->animation.frame_rendered;
				if(!nqiv_send_thread_event(state, base_priority + NQIV_EVENT_PRIORITY_IMAGE_LOAD,
				                           &event)) {
					nqiv_image_unlock(image);
					return false;
				}
			}
		}
		/* Draw thumbnail if it exists. */
		if(form->texture != NULL) {
			if(dstrect_zoom_ptr != NULL
			   && !nqiv_state_update_alpha_background_dimensions(state, dstrect_zoom_ptr->w,
			                                                     dstrect_zoom_ptr->h)) {
				nqiv_image_unlock(image);
				return false;
			}
			if(!render_texture(&cleared, dstrect, state, state->texture_alpha_background, NULL,
			                   dstrect_zoom_ptr)) {
				nqiv_log_write(&state->logger, NQIV_LOG_ERROR,
				               "Failed to draw image alpha background.\n");
				nqiv_image_unlock(image);
				return false;
			}
			if(!render_texture(&cleared, NULL, state, form->texture, srcrect_ptr,
			                   dstrect_zoom_ptr)) {
				nqiv_log_write(&state->logger, NQIV_LOG_ERROR, "Failed to draw image texture.\n");
				nqiv_image_unlock(image);
				return false;
			}
			if(dstrect != NULL) {
				state->is_loading = false;
			}
			state->first_frame_pending = false;
			/* Special operation to load next thumbnail frame right away. */
			if(form->animation.exists && next_frame && !is_montage) {
				nqiv_unload_image_form_texture(form);
				form->animation.frame_rendered = true;
				nqiv_event event = {0};
				event.type = NQIV_EVENT_IMAGE_LOAD;
				event.options.image_load.image = image;
				if(hard) {
					event.options.image_load.image_options.vips = true;
				} else {
					event.options.image_load.image_options.vips_soft = true;
				}
				event.options.image_load.image_options.surface = true;
				event.options.image_load.image_options.first_frame = first_frame;
				event.options.image_load.image_options.next_frame =
					next_frame && !first_frame && form->animation.frame_rendered;
				if(!nqiv_send_thread_event(
					   state, base_priority + NQIV_EVENT_PRIORITY_IMAGE_LOAD_ANIMATION, &event)) {
					nqiv_image_unlock(image);
					return false;
				}
			}
		}
	}
	/* Simple quick overdrawn stuff, selection and mark boxes. */
	if(selected && is_montage) {
		if(!render_texture(&cleared, NULL, state, state->texture_montage_selection, NULL,
		                   dstrect)) {
			nqiv_image_unlock(image);
			return false;
		}
	}
	if(image->marked && is_montage) {
		if(!render_texture(&cleared, NULL, state, state->texture_montage_mark, NULL, dstrect)) {
			nqiv_image_unlock(image);
			return false;
		}
	}
	nqiv_image_unlock(image);
	if(is_montage && dstrect != NULL) {
		state->is_loading = false;
	}
	return true;
}

/*2305843009213693951*/
#define INT_MAX_STRLEN 19
bool set_title(nqiv_state* state, nqiv_image* image)
{
	char idx_string[INT_MAX_STRLEN + 1] = {0};
	char count_string[INT_MAX_STRLEN + 1] = {0};
	char width_string[INT_MAX_STRLEN + 1] = {0};
	char height_string[INT_MAX_STRLEN + 1] = {0};
	char zoom_string[INT_MAX_STRLEN + 1] = {0};
	snprintf(idx_string, INT_MAX_STRLEN, "%d", state->montage.positions.selection + 1);
	snprintf(count_string, INT_MAX_STRLEN, "%d", nqiv_array_get_units_count(state->images.images));
	const bool do_dimensions = image->image.width > 0 && image->image.height > 0;
	if(do_dimensions) {
		snprintf(width_string, INT_MAX_STRLEN, "%d", image->image.width);
		snprintf(height_string, INT_MAX_STRLEN, "%d", image->image.height);
	}
	if(!state->in_montage) {
		snprintf(zoom_string, INT_MAX_STRLEN, "%d",
		         nqiv_image_manager_get_zoom_percent(&state->images));
	}
	const char* path_components[] = {
		"nqiv - ",
		idx_string,
		"/",
		count_string,
		do_dimensions ? " - " : "",
		width_string,
		do_dimensions ? "x" : "",
		height_string,
		!state->in_montage ? " - " : " ",
		zoom_string,
		!state->in_montage ? "% " : "",
		"(",
		nqiv_zoom_default_names[state->zoom_default],
		") - ",
		image->marked ? "MARKED - " : "",
		state->is_loading ? "LOADING - " : "",
		image->image.path,
		NULL,
	};
	char       window_title[WINDOW_TITLE_LEN + 1] = {0};
	nqiv_array builder;
	nqiv_array_inherit(&builder, window_title, sizeof(char), WINDOW_TITLE_LEN);
	int idx;
	for(idx = 0; path_components[idx] != NULL; ++idx) {
		if(!nqiv_array_push_str(&builder, path_components[idx])) {
			nqiv_log_write(
				&state->logger, NQIV_LOG_ERROR,
				"Window title of %d is longer than limit of %d This is probably a bug.\n",
				nqiv_array_get_units_count(&builder) + strlen(path_components[idx]),
				WINDOW_TITLE_LEN);
			return false;
		}
	}
	SDL_SetWindowTitle(state->window, window_title);
	return true;
}
#undef INT_MAX_STRLEN

bool render_montage(nqiv_state* state, const bool hard, const bool preload_only)
{
	nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Rendering montage.\n");
	if(!preload_only && SDL_RenderClear(state->renderer) != 0) {
		nqiv_log_write(&state->logger, NQIV_LOG_ERROR, "Failed to clear renderer for montage.\n");
		return false;
	}
	if(!preload_only
	   && SDL_RenderCopy(state->renderer, state->texture_background, NULL, NULL) != 0) {
		nqiv_log_write(&state->logger, NQIV_LOG_ERROR,
		               "Failed to copy texture background for montage.\n");
		return false;
	}
	state->render_cleared = !preload_only;
	const int    images_len = nqiv_array_get_units_count(state->images.images);
	const int    raw_start_idx = state->montage.positions.start - state->montage.preload.behind;
	const int    raw_end = state->montage.positions.end + state->montage.preload.ahead;
	const int    start_idx = raw_start_idx >= 0 ? raw_start_idx : 0;
	const int    end = raw_end <= images_len ? raw_end : images_len;
	nqiv_image** images = state->images.images->data;
	int          idx;
	nqiv_log_write(&state->logger, NQIV_LOG_DEBUG,
	               "Preload Start: %d Preload End: %d Montage Start: %d Montage Selection: %d "
	               "Montage End %d\n",
	               start_idx, end, state->montage.positions.start,
	               state->montage.positions.selection, state->montage.positions.end);
	for(idx = start_idx; idx < end; ++idx) {
		nqiv_image* image = images[idx];
		if(idx >= state->montage.positions.start && idx < state->montage.positions.end
		   && !preload_only) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Rendering montage image %s at %d.\n",
			               image->image.path, idx);
			SDL_Rect dstrect;
			nqiv_montage_get_image_rect(&state->montage, idx, &dstrect);
			if(!render_from_form(state, image, true, &dstrect, true, false,
			                     state->montage.positions.selection == idx, hard, 0)
			   || (idx == state->montage.positions.selection && !set_title(state, image))) {
				return false;
			}
		} else if(!render_from_form(state, image, true, NULL, true, false,
		                            state->montage.positions.selection == idx, hard, 1)) {
			return false;
		}
	}
	return true;
}

bool render_image(nqiv_state* state, const bool start, const bool hard)
{
	assert(state->montage.positions.selection <= nqiv_array_get_units_count(state->images.images));
	if(state->montage.positions.selection == nqiv_array_get_units_count(state->images.images)) {
		nqiv_log_write(&state->logger, NQIV_LOG_ERROR, "Failed to get image index %d.\n",
		               state->montage.positions.selection);
		return false;
	}
	nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Rendering selected image.\n");
	nqiv_image* image =
		((nqiv_image**)state->images.images->data)[state->montage.positions.selection];
	SDL_Rect dstrect = {0};
	SDL_GetWindowSizeInPixels(state->window, &dstrect.w, &dstrect.h);
	if(!render_from_form(state, image, false, &dstrect, start, true, false, hard, 0)) {
		return false;
	}
	const bool render_cleared = state->render_cleared;
	/* Preloading and title. */
	if(!render_montage(state, false, true) || !set_title(state, image)) {
		return false;
	}
	state->render_cleared = render_cleared;
	return true;
}

void nqiv_check_pruning(nqiv_state* state, bool* running, bool* result)
{
	/* Do pruning if needed. */
	const Uint64 new_time = SDL_GetTicks64();
	if(new_time - state->time_of_last_prune > state->prune_delay) {
		state->time_of_last_prune = new_time;
		const int prune_count =
			nqiv_pruner_run(&state->pruner, &state->montage, &state->images, &state->thread_queue);
		if(prune_count == -1) {
			*running = false;
			*result = false;
		}
	}
}

void render_and_update(
	nqiv_state* state, bool* running, bool* result, const bool first_render, const bool hard)
{
	/* Hard forces reload. first_render basically means to show the first frame. */
	/* Adapt screen dimensions to montage. */
	nqiv_update_montage_dimensions(state);

	nqiv_check_pruning(state, running, result);

	/* Update transaction group. */
	if(state->montage.range_changed) {
		omp_set_lock(&state->thread_event_transaction_group_lock);
		state->thread_event_transaction_group += 1;
		state->pruner.thread_event_transaction_group = state->thread_event_transaction_group;
		nqiv_log_write(&state->logger, NQIV_LOG_DEBUG,
		               "Increased transaction group value to %" PRIi64 " at position %d.\n",
		               state->thread_event_transaction_group, state->montage.positions.selection);
		omp_unset_lock(&state->thread_event_transaction_group_lock);
		state->montage.range_changed = false;
	}
	if(state->in_montage) {
		if(!render_montage(state, hard, false)) {
			*running = false;
			*result = false;
		}
	} else {
		if(!render_image(state, first_render, hard)) {
			*running = false;
			*result = false;
		}
	}
	if(*result != false && state->render_cleared) {
		SDL_RenderPresent(state->renderer);
		state->render_cleared = false;
	}
}

void nqiv_handle_thumbnail_resize_action(nqiv_state* state,
                                         bool*       running,
                                         bool*       result,
                                         void (*op)(nqiv_image_manager*))
{
	const int old_size = state->images.thumbnail.size;
	op(&state->images);
	if(!nqiv_image_manager_reattempt_thumbnails(&state->images, old_size)) {
		*running = false;
		*result = false;
	}
	render_and_update(state, running, result, false, false);
}

void nqiv_mark_op(
	nqiv_state* state, bool* running, bool* result, nqiv_image* image, const bool value)
{
	image->marked = value;
	nqiv_log_write(&state->logger, NQIV_LOG_INFO, "%sarked %s\n", image->marked ? "Unm" : "M",
	               image->image.path);
	render_and_update(state, running, result, false, false);
}

void nqiv_mark_op_toggle(nqiv_state* state, bool* running, bool* result, nqiv_image* image)
{
	nqiv_mark_op(state, running, result, image, !image->marked);
}

int nqiv_get_index_at_mouse(nqiv_state* state)
{
	int x, y;
	SDL_GetMouseState(&x, &y);
	return nqiv_montage_find_index_at_point(&state->montage, x, y);
}

void nqiv_handle_keyactions(nqiv_state*                       state,
                            bool*                             running,
                            bool*                             result,
                            const bool                        simulated,
                            const nqiv_keyrate_release_option released)
{
	nqiv_keybind_pair* pair;
	while(*running && nqiv_queue_pop_front(&state->key_actions, &pair)) {
		const int images_count = nqiv_array_get_units_count(state->images.images);
		assert(state->montage.positions.selection < images_count);
		nqiv_image** images = state->images.images->data;
		nqiv_image*  image = images[state->montage.positions.selection];
		if(!simulated
		   && !nqiv_keyrate_filter_action(&state->keystates, &pair->keyrate, released,
		                                  SDL_GetTicks64())) {
			/* NOOP */
		} else if(pair->action == NQIV_KEY_ACTION_QUIT) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action quit.\n");
			*running = false;
		} else if(pair->action == NQIV_KEY_ACTION_IMAGE_PREVIOUS) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG,
			               "Received nqiv action image previous.\n");
			if(!state->in_montage) {
				nqiv_montage_previous_selection(&state->montage);
				render_and_update(state, running, result, true, false);
			}
		} else if(pair->action == NQIV_KEY_ACTION_IMAGE_NEXT) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action image next.\n");
			if(!state->in_montage) {
				nqiv_montage_next_selection(&state->montage);
				render_and_update(state, running, result, true, false);
			}
		} else if(pair->action == NQIV_KEY_ACTION_MONTAGE_RIGHT) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action montage right.\n");
			if(state->in_montage) {
				nqiv_montage_next_selection(&state->montage);
				render_and_update(state, running, result, false, false);
			}
		} else if(pair->action == NQIV_KEY_ACTION_MONTAGE_LEFT) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action montage left.\n");
			if(state->in_montage) {
				nqiv_montage_previous_selection(&state->montage);
				render_and_update(state, running, result, false, false);
			}
		} else if(pair->action == NQIV_KEY_ACTION_MONTAGE_UP) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action montage up.\n");
			if(state->in_montage) {
				nqiv_montage_previous_selection_row(&state->montage);
				render_and_update(state, running, result, false, false);
			}
		} else if(pair->action == NQIV_KEY_ACTION_MONTAGE_DOWN) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action montage down.\n");
			if(state->in_montage) {
				nqiv_montage_next_selection_row(&state->montage);
				render_and_update(state, running, result, false, false);
			}
		} else if(pair->action == NQIV_KEY_ACTION_PAGE_UP) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action page up.\n");
			if(state->in_montage) {
				nqiv_montage_previous_selection_page(&state->montage);
				render_and_update(state, running, result, false, false);
			}
		} else if(pair->action == NQIV_KEY_ACTION_PAGE_DOWN) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action page down.\n");
			if(state->in_montage) {
				nqiv_montage_next_selection_page(&state->montage);
				render_and_update(state, running, result, false, false);
			}
		} else if(pair->action == NQIV_KEY_ACTION_MONTAGE_START) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action montage start.\n");
			if(state->in_montage) {
				nqiv_montage_jump_selection_start(&state->montage);
				render_and_update(state, running, result, false, false);
			}
		} else if(pair->action == NQIV_KEY_ACTION_MONTAGE_END) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action montage end.\n");
			if(state->in_montage) {
				nqiv_montage_jump_selection_end(&state->montage);
				render_and_update(state, running, result, false, false);
			}
		} else if(pair->action == NQIV_KEY_ACTION_TOGGLE_MONTAGE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG,
			               "Received nqiv action montage toggle.\n");
			state->in_montage = !state->in_montage;
			render_and_update(state, running, result, true, false);
		} else if(pair->action == NQIV_KEY_ACTION_SET_MONTAGE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv montage set.\n");
			if(!state->in_montage) {
				state->in_montage = true;
				render_and_update(state, running, result, true, false);
			}
		} else if(pair->action == NQIV_KEY_ACTION_SET_VIEWING) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action viewing set.\n");
			if(state->in_montage) {
				state->in_montage = false;
				render_and_update(state, running, result, true, false);
			}
		} else if(pair->action == NQIV_KEY_ACTION_ZOOM_IN) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action zoom in.\n");
			if(!state->in_montage) {
				nqiv_image_manager_zoom_in(&state->images);
				render_and_update(state, running, result, false, false);
			} else if(state->montage.dimensions.count > 2) {
				nqiv_handle_thumbnail_resize_action(state, running, result,
				                                    nqiv_image_manager_increment_thumbnail_size);
			}
		} else if(pair->action == NQIV_KEY_ACTION_ZOOM_OUT) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action zoom out.\n");
			if(!state->in_montage) {
				nqiv_image_manager_zoom_out(&state->images);
				render_and_update(state, running, result, false, false);
			} else {
				nqiv_handle_thumbnail_resize_action(state, running, result,
				                                    nqiv_image_manager_decrement_thumbnail_size);
			}
		} else if(pair->action == NQIV_KEY_ACTION_PAN_LEFT) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action pan left.\n");
			if(!state->in_montage) {
				nqiv_image_manager_pan_left(&state->images);
				render_and_update(state, running, result, false, false);
			}
		} else if(pair->action == NQIV_KEY_ACTION_PAN_RIGHT) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action pan right.\n");
			if(!state->in_montage) {
				nqiv_image_manager_pan_right(&state->images);
				render_and_update(state, running, result, false, false);
			}
		} else if(pair->action == NQIV_KEY_ACTION_PAN_UP) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action pan up.\n");
			if(!state->in_montage) {
				nqiv_image_manager_pan_up(&state->images);
				render_and_update(state, running, result, false, false);
			}
		} else if(pair->action == NQIV_KEY_ACTION_PAN_DOWN) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action pan down more.\n");
			if(!state->in_montage) {
				nqiv_image_manager_pan_down(&state->images);
				render_and_update(state, running, result, false, false);
			}
		} else if(pair->action == NQIV_KEY_ACTION_ZOOM_IN_MORE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action zoom in more.\n");
			if(!state->in_montage) {
				nqiv_image_manager_zoom_in_more(&state->images);
				render_and_update(state, running, result, false, false);
			} else if(state->montage.dimensions.count > 2) {
				nqiv_handle_thumbnail_resize_action(
					state, running, result, nqiv_image_manager_increment_thumbnail_size_more);
			}
		} else if(pair->action == NQIV_KEY_ACTION_ZOOM_OUT_MORE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action zoom out more.\n");
			if(!state->in_montage) {
				nqiv_image_manager_zoom_out_more(&state->images);
				render_and_update(state, running, result, false, false);
			} else {
				nqiv_handle_thumbnail_resize_action(
					state, running, result, nqiv_image_manager_decrement_thumbnail_size_more);
			}
		} else if(pair->action == NQIV_KEY_ACTION_PAN_LEFT_MORE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action pan left more.\n");
			if(!state->in_montage) {
				nqiv_image_manager_pan_left_more(&state->images);
				render_and_update(state, running, result, false, false);
			}
		} else if(pair->action == NQIV_KEY_ACTION_PAN_RIGHT_MORE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG,
			               "Received nqiv action pan right more.\n");
			if(!state->in_montage) {
				nqiv_image_manager_pan_right_more(&state->images);
				render_and_update(state, running, result, false, false);
			}
		} else if(pair->action == NQIV_KEY_ACTION_PAN_UP_MORE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action pan up more.\n");
			if(!state->in_montage) {
				nqiv_image_manager_pan_up_more(&state->images);
				render_and_update(state, running, result, false, false);
			}
		} else if(pair->action == NQIV_KEY_ACTION_PAN_DOWN_MORE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action pan down more.\n");
			if(!state->in_montage) {
				nqiv_image_manager_pan_down_more(&state->images);
				render_and_update(state, running, result, false, false);
			}
		} else if(pair->action == NQIV_KEY_ACTION_PAN_CENTER) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action pan center.\n");
			if(!state->in_montage) {
				nqiv_image_manager_pan_center(&state->images);
				render_and_update(state, running, result, false, false);
			}
		} else if(pair->action == NQIV_KEY_ACTION_TOGGLE_STRETCH) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG,
			               "Received nqiv action toggle stretch.\n");
			state->stretch_images = !state->stretch_images;
			render_and_update(state, running, result, false, false);
		} else if(pair->action == NQIV_KEY_ACTION_STRETCH) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action stretch.\n");
			state->stretch_images = true;
			render_and_update(state, running, result, false, false);
		} else if(pair->action == NQIV_KEY_ACTION_KEEP_ASPECT_RATIO) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG,
			               "Received nqiv action keep aspect ratio.\n");
			state->stretch_images = false;
			render_and_update(state, running, result, false, false);
		} else if(pair->action == NQIV_KEY_ACTION_FIT) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action fit.\n");
			state->images.zoom.image_to_viewport_ratio = state->images.zoom.fit_level;
			render_and_update(state, running, result, false, false);
		} else if(pair->action == NQIV_KEY_ACTION_ACTUAL_SIZE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action actual_size.\n");
			state->images.zoom.image_to_viewport_ratio = state->images.zoom.actual_size_level;
			render_and_update(state, running, result, false, false);
		} else if(pair->action == NQIV_KEY_ACTION_KEEP_FIT) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action keep_fit.\n");
			state->zoom_default = NQIV_ZOOM_DEFAULT_FIT;
			render_and_update(state, running, result, false, false);
		} else if(pair->action == NQIV_KEY_ACTION_KEEP_ACTUAL_SIZE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG,
			               "Received nqiv action keep_actual_size.\n");
			state->zoom_default = NQIV_ZOOM_DEFAULT_ACTUAL;
			render_and_update(state, running, result, false, false);
		} else if(pair->action == NQIV_KEY_ACTION_KEEP_CURRENT_ZOOM) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG,
			               "Received nqiv action keep_current_zoom.\n");
			state->zoom_default = NQIV_ZOOM_DEFAULT_KEEP;
			render_and_update(state, running, result, false, false);
		} else if(pair->action == NQIV_KEY_ACTION_TOGGLE_KEPT_ZOOM) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG,
			               "Received nqiv action toggle_kept_zoom.\n");
			if(state->zoom_default == NQIV_ZOOM_DEFAULT_ACTUAL) {
				state->zoom_default = NQIV_ZOOM_DEFAULT_KEEP;
			} else {
				state->zoom_default += 1;
			}
			render_and_update(state, running, result, false, false);
		} else if(pair->action == NQIV_KEY_ACTION_SCALE_MODE_NEAREST) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG,
			               "Received nqiv action scale_mode_nearest.\n");
			state->texture_scale_mode = SDL_ScaleModeNearest;
			render_and_update(state, running, result, false, false);
		} else if(pair->action == NQIV_KEY_ACTION_SCALE_MODE_LINEAR) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG,
			               "Received nqiv action scale_mode_linear.\n");
			state->texture_scale_mode = SDL_ScaleModeLinear;
			render_and_update(state, running, result, false, false);
		} else if(pair->action == NQIV_KEY_ACTION_SCALE_MODE_ANISOTROPIC) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG,
			               "Received nqiv action scale_mode_anisotropic.\n");
			state->texture_scale_mode = SDL_ScaleModeBest;
			render_and_update(state, running, result, false, false);
		} else if(pair->action == NQIV_KEY_ACTION_TOGGLE_SCALE_MODE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG,
			               "Received nqiv action toggle_scale_mode.\n");
			if(state->texture_scale_mode == SDL_ScaleModeNearest) {
				state->texture_scale_mode = SDL_ScaleModeLinear;
			} else if(state->texture_scale_mode == SDL_ScaleModeLinear) {
				state->texture_scale_mode = SDL_ScaleModeBest;
			} else {
				assert(state->texture_scale_mode == SDL_ScaleModeBest);
				state->texture_scale_mode = SDL_ScaleModeNearest;
			}
			render_and_update(state, running, result, false, false);
		} else if(pair->action == NQIV_KEY_ACTION_IMAGE_MARK_TOGGLE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG,
			               "Received nqiv action image mark toggle.\n");
			nqiv_mark_op_toggle(state, running, result, image);
		} else if(pair->action == NQIV_KEY_ACTION_IMAGE_MARK) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action image mark.\n");
			nqiv_mark_op(state, running, result, image, true);
		} else if(pair->action == NQIV_KEY_ACTION_IMAGE_UNMARK) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action image unmark.\n");
			nqiv_mark_op(state, running, result, image, false);
		} else if(pair->action == NQIV_KEY_ACTION_PRINT_MARKED) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG,
			               "Received nqiv action image print marked.\n");
			int iidx;
			for(iidx = 0; iidx < images_count; ++iidx) {
				if(images[iidx]->marked) {
					fprintf(stdout, "%s\n", images[iidx]->image.path);
				}
			}
			render_and_update(state, running, result, false, false);
		} else if(pair->action == NQIV_KEY_ACTION_MONTAGE_SELECT_AT_MOUSE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG,
			               "Received nqiv action montage select at mouse.\n");
			if(state->in_montage) {
				const int i = nqiv_get_index_at_mouse(state);
				if(i >= 0) {
					nqiv_montage_set_selection(&state->montage, i);
					render_and_update(state, running, result, false, false);
				}
			}
		} else if(pair->action == NQIV_KEY_ACTION_IMAGE_MARK_AT_MOUSE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG,
			               "Received nqiv action image mark at mouse.\n");
			if(state->in_montage) {
				const int i = nqiv_get_index_at_mouse(state);
				if(i >= 0) {
					nqiv_mark_op(state, running, result, images[i], true);
				}
			}
		} else if(pair->action == NQIV_KEY_ACTION_IMAGE_UNMARK_AT_MOUSE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG,
			               "Received nqiv action image unmark at mouse.\n");
			if(state->in_montage) {
				const int i = nqiv_get_index_at_mouse(state);
				if(i >= 0) {
					nqiv_mark_op(state, running, result, images[i], false);
				}
			}
		} else if(pair->action == NQIV_KEY_ACTION_IMAGE_MARK_TOGGLE_AT_MOUSE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG,
			               "Received nqiv action image toggle at mouse.\n");
			if(state->in_montage) {
				const int i = nqiv_get_index_at_mouse(state);
				if(i >= 0) {
					nqiv_mark_op_toggle(state, running, result, images[i]);
				}
			}
		} else if(pair->action == NQIV_KEY_ACTION_START_MOUSE_PAN) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG,
			               "Received nqiv action start mouse pan.\n");
			if(!state->in_montage) {
				state->is_mouse_panning = true;
			}
		} else if(pair->action == NQIV_KEY_ACTION_END_MOUSE_PAN) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action end mouse pan.\n");
			if(!state->in_montage) {
				state->is_mouse_panning = false;
			}
		} else if(pair->action == NQIV_KEY_ACTION_IMAGE_ZOOM_IN) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action image zoom in.\n");
			if(!state->in_montage) {
				nqiv_image_manager_zoom_in(&state->images);
				render_and_update(state, running, result, false, false);
			}
		} else if(pair->action == NQIV_KEY_ACTION_IMAGE_ZOOM_OUT) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG,
			               "Received nqiv action image zoom out.\n");
			if(!state->in_montage) {
				nqiv_image_manager_zoom_out(&state->images);
				render_and_update(state, running, result, false, false);
			}
		} else if(pair->action == NQIV_KEY_ACTION_IMAGE_ZOOM_IN_MORE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG,
			               "Received nqiv action image zoom in more.\n");
			if(!state->in_montage) {
				nqiv_image_manager_zoom_in_more(&state->images);
				render_and_update(state, running, result, false, false);
			}
		} else if(pair->action == NQIV_KEY_ACTION_IMAGE_ZOOM_OUT_MORE) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG,
			               "Received nqiv action image zoom in out.\n");
			if(!state->in_montage) {
				nqiv_image_manager_zoom_out_more(&state->images);
				render_and_update(state, running, result, false, false);
			}
		} else if(pair->action == NQIV_KEY_ACTION_RELOAD) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action reload.\n");
			render_and_update(state, running, result, true, true);
		} else {
			assert(false);
		}
	}
}

void nqiv_set_match_keymods(nqiv_key_match* match)
{
	const SDL_Keymod mods = SDL_GetModState();
	if((mods & ~KMOD_GUI & ~KMOD_SCROLL & ~KMOD_NUM) != 0) {
		assert((match->mode & NQIV_KEY_MATCH_MODE_KEY_MOD) == 0);
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
		SDL_PumpEvents();
		SDL_Event    input_event = {0};
		const Uint64 wait_start = SDL_GetTicks64();
		const int    event_result = state->event_timeout > 0
		                                ? SDL_WaitEventTimeout(&input_event, state->event_timeout)
		                                : SDL_WaitEvent(&input_event);
		if(event_result == 0) {
			if(state->event_timeout == 0) {
				nqiv_log_write(&state->logger, NQIV_LOG_ERROR,
				               "Failed to wait on an SDL event (with limitless waiting period). "
				               "SDL Error: %s\n",
				               SDL_GetError());
				running = false;
				result = false;
			} else {
				const Uint64 wait_diff = SDL_GetTicks64() - wait_start;
				/* TODO: It seems SDL_WaitEventTimeout does not guarantee that it will
				 * wait for at least the time specified, like most sleeping
				 * functions. During testing, the ticks/ms slept for were sometimes
				 * off by one. For now, deciding this error check is not worthwhile.*/
				nqiv_log_write(&state->logger, NQIV_LOG_DEBUG,
				               "Waited for %" PRIu64 "ms with expected delay of %" PRIu64
				               "ms Checking if a prune is needed.\n",
				               wait_diff, state->event_timeout);
				render_and_update(state, &running, &result, false, false);
			}
			continue;
		}
		switch(input_event.type) {
		case SDL_USEREVENT:
			if(input_event.user.code >= 0) {
				if((Uint32)input_event.user.code == state->thread_event_number) {
					render_and_update(state, &running, &result, false, false);
				} else if((Uint32)input_event.user.code == state->cfg_event_number) {
					nqiv_handle_keyactions(state, &running, &result, true, NQIV_KEYRATE_ON_DOWN);
					render_and_update(state, &running, &result, false, false);
				}
			}
			break;
		case SDL_WINDOWEVENT:
			if(input_event.window.event == SDL_WINDOWEVENT_RESIZED
			   || input_event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED
			   || input_event.window.event == SDL_WINDOWEVENT_EXPOSED
			   || input_event.window.event == SDL_WINDOWEVENT_MAXIMIZED
			   || input_event.window.event == SDL_WINDOWEVENT_RESTORED
			   || input_event.window.event == SDL_WINDOWEVENT_SHOWN
			   || input_event.window.event == SDL_WINDOWEVENT_ICCPROF_CHANGED
			   || input_event.window.event == SDL_WINDOWEVENT_DISPLAY_CHANGED) {
				render_and_update(state, &running, &result, false, false);
			}
			break;
		case SDL_QUIT:
			running = false;
			break;
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			assert(input_event.type == SDL_KEYDOWN || input_event.type == SDL_KEYUP);
			{
				nqiv_key_match match = {0};
				if(input_event.key.keysym.scancode != SDL_SCANCODE_UNKNOWN) {
					match.mode |= NQIV_KEY_MATCH_MODE_KEY;
				}
				if((input_event.key.keysym.mod & ~KMOD_GUI & ~KMOD_SCROLL & ~KMOD_NUM) != 0) {
					match.mode |= NQIV_KEY_MATCH_MODE_KEY_MOD;
				}
				memcpy(&match.data.key, &input_event.key.keysym, sizeof(SDL_Keysym));
				const nqiv_op_result lookup_summary =
					nqiv_keybind_lookup(&state->keybinds, &match, &state->key_actions);
				if(lookup_summary == NQIV_FAIL) {
					running = false;
					result = false;
				} else if(lookup_summary == NQIV_SUCCESS) {
					nqiv_handle_keyactions(state, &running, &result, false,
					                       input_event.type == SDL_KEYUP ? NQIV_KEYRATE_ON_UP
					                                                     : NQIV_KEYRATE_ON_DOWN);
					render_and_update(state, &running, &result, false, false);
				}
			}
			break;
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			assert(input_event.type == SDL_MOUSEBUTTONDOWN
			       || input_event.type == SDL_MOUSEBUTTONUP);
			{
				nqiv_key_match match = {0};
				match.mode |= NQIV_KEY_MATCH_MODE_MOUSE_BUTTON;
				memcpy(&match.data.mouse_button, &input_event.button, sizeof(SDL_MouseButtonEvent));
				nqiv_set_match_keymods(&match);
				const nqiv_op_result lookup_summary =
					nqiv_keybind_lookup(&state->keybinds, &match, &state->key_actions);
				if(lookup_summary == NQIV_FAIL) {
					running = false;
					result = false;
				} else if(lookup_summary == NQIV_SUCCESS) {
					nqiv_handle_keyactions(state, &running, &result, false,
					                       input_event.type == SDL_MOUSEBUTTONUP
					                           ? NQIV_KEYRATE_ON_UP
					                           : NQIV_KEYRATE_ON_DOWN);
					render_and_update(state, &running, &result, false, false);
				}
			}
			break;
		case SDL_MOUSEWHEEL:
			assert(input_event.type == SDL_MOUSEWHEEL);
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
				const nqiv_op_result lookup_summary =
					nqiv_keybind_lookup(&state->keybinds, &match, &state->key_actions);
				if(lookup_summary == NQIV_FAIL) {
					running = false;
					result = false;
				} else if(lookup_summary == NQIV_SUCCESS) {
					nqiv_handle_keyactions(state, &running, &result, false,
					                       NQIV_KEYRATE_ON_DOWN | NQIV_KEYRATE_ON_UP);
					render_and_update(state, &running, &result, false, false);
				}
			}
			break;
		case SDL_MOUSEMOTION:
			if(state->is_mouse_panning && !state->in_montage) {
				SDL_Rect coordinates = {0};
				SDL_GetWindowSizeInPixels(state->window, &coordinates.w, &coordinates.h);
				coordinates.x = input_event.motion.xrel;
				coordinates.y = input_event.motion.yrel;
				nqiv_image_manager_pan_coordinates(&state->images, &coordinates);
				render_and_update(state, &running, &result, false, false);
			}
			break;
		default:
			assert(true);
		}
	}
	nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Finished waiting on events.\n");
	int idx;
	for(idx = 0; idx < state->thread_count; ++idx) {
		nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Killing worker %d.\n", idx);
		nqiv_event output_event = {0};
		output_event.type = NQIV_EVENT_WORKER_STOP;
		nqiv_send_thread_event_force(state, 0, &output_event);
	}
	return result;
}

bool nqiv_run(nqiv_state* state)
{
	bool                 result;
	bool*                result_ptr = &result;
	const int            thread_count = state->thread_count;
	const int            thread_event_interval = state->thread_event_interval;
	const int            extra_wakeup_delay = state->extra_wakeup_delay;
	nqiv_log_ctx*        logger = &state->logger;
	nqiv_priority_queue* thread_queue = &state->thread_queue;
	const int64_t*       thread_event_transaction_group = &state->thread_event_transaction_group;
	omp_lock_t*  thread_event_transaction_group_lock = &state->thread_event_transaction_group_lock;
	const Uint32 event_code = state->thread_event_number;
	/* clang-format insists on unindenting pragmas. */
	/* clang-format off */
	#pragma omp parallel                                  \
		default(none)                                     \
		firstprivate(state,                               \
					 logger,                              \
					 thread_count,                        \
					 extra_wakeup_delay,                  \
					 thread_event_interval,               \
					 thread_queue,                        \
					 event_code,                          \
					 result_ptr,                          \
					 thread_event_transaction_group,      \
					 thread_event_transaction_group_lock) \
		num_threads(thread_count + 1)
	/* clang-format on */
	{
		/* clang-format off */
		#pragma omp master
		/* clang-format on */
		{
			int thread;
			for(thread = 0; thread < thread_count; ++thread) {
				/* clang-format off */
				#pragma omp task                                      \
					default(none)                                     \
					firstprivate(logger,                              \
								 thread_queue,                        \
								 extra_wakeup_delay,                  \
								 thread_event_interval,               \
								 event_code,                          \
								 thread_event_transaction_group,      \
								 thread_event_transaction_group_lock)
				/* clang-format on */
				nqiv_worker_main(logger, thread_queue, extra_wakeup_delay, thread_event_interval,
				                 event_code, thread_event_transaction_group,
				                 thread_event_transaction_group_lock);
			}
			*result_ptr = nqiv_master_thread(state);
		}
		/* clang-format off */
		#pragma omp taskwait
		/* clang-format on */
	}
	return result;
}

int main(int argc, char* argv[])
{
	if(argc == 0) {
		return 0;
	}

	nqiv_state state;
	memset(&state, 0, sizeof(nqiv_state));

	if(VIPS_INIT(argv[0]) != 0) {
		fputs("Failed to initialize vips.\n", stderr);
	}

	const nqiv_op_result args_result = nqiv_parse_args(argv, &state);
	if(args_result != NQIV_SUCCESS) {
		nqiv_state_clear(&state);
		return args_result == NQIV_FAIL ? 1 : 0;
	}

	const bool result = nqiv_run(&state);
	nqiv_state_clear(&state);
	return result ? 0 : 1;
}
