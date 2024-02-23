#include <stdio.h>
#include <assert.h>

#include <MagickCore/MagickCore.h>
#include <MagickWand/MagickWand.h>
#include <SDL2/SDL.h>
#include <omp.h>

#include "logging.h"
#include "image.h"
#include "worker.h"
#include "array.h"
#include "queue.h"
#include "event.h"
#include "montage.h"
#include "drawing.h"
#include "keybinds.h"

#define OPTPARSE_IMPLEMENTATION
#define OPTPARSE_API static
#include "optparse.h"

#define STARTING_QUEUE_LENGTH 1024

typedef struct nqiv_state
{
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
} nqiv_state;

bool nqiv_check_and_print_logger_error(nqiv_log_ctx* logger)
{
	if( nqiv_log_has_error(logger) ) {
		fputs(logger->error_message, stderr);
		return false;
	}
	return true;
}

bool nqiv_add_logger_path(nqiv_log_ctx* logger, const char* path)
{
	FILE* stream = NULL;
	if( strcmp(path, "stdout") == 0 ) {
		stream = stdout;
	} else if( strcmp(path, "stderr") == 0 ) {
		stream = stderr;
	} else {
		stream = fopen(path, "r");
	}
	if(stream == NULL) {
		return false;
	}
	nqiv_log_add_stream(logger, stream);
	if( !nqiv_check_and_print_logger_error(logger) ) {
		return false;
	}
	return true;
}

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

void nqiv_state_clear(nqiv_state* state)
{
	if(state->thread_queue.array != NULL) {
		nqiv_queue_destroy(&state->thread_queue);
	}
	if(state->keybinds.lookup != NULL) {
		nqiv_keybind_destroy_manager(&state->keybinds);
	}
	if(state->images.images != NULL) {
		nqiv_image_manager_destroy(&state->images);
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
		int idx;
		for(idx = 0; idx < state->thread_count; ++idx) {
			omp_destroy_lock(&state->thread_locks[idx]);
		}
		free(state->thread_locks);
	}
	if(state->window_title != NULL) {
		free(state->window_title);
	}
	memset( state, 0, sizeof(nqiv_state) );
	MagickWandTerminus();
}

bool nqiv_parse_args(char *argv[], nqiv_state* state)
{
	/*
	nqiv_log_level arg_log_level = NQIV_LOG_WARNING;
	char* arg_log_prefix = "#time:%Y-%m-%d_%H:%M:%S%z# #level#: ";
	*/
	state->read_from_stdin = false;
	nqiv_log_init(&state->logger);
	if( !nqiv_check_and_print_logger_error(&state->logger) ) {
		return false;
	}
	if( !nqiv_image_manager_init(&state->images, &state->logger, STARTING_QUEUE_LENGTH) ) {
		fputs("Failed to initialize image manager.\n", stderr);
		return false;
	}
	if( !nqiv_keybind_create_manager(&state->keybinds, &state->logger, STARTING_QUEUE_LENGTH) ) {
		fputs("Failed to initialize image manager.\n", stderr);
		return false;
	}
	if( !nqiv_queue_init(&state->thread_queue, &state->logger, STARTING_QUEUE_LENGTH) ) {
		fputs("Failed to initialize thread queue.\n", stderr);
		return false;
	}
	struct optparse_long longopts[] = {
		{"log-level", 'v', OPTPARSE_REQUIRED},
		{"log-stream", 'l', OPTPARSE_REQUIRED},
		{"log-prefix", 'L', OPTPARSE_REQUIRED},
		{"read-from-stdin", 'r', OPTPARSE_NONE},
		{"thumbnail-root", 'P', OPTPARSE_REQUIRED},
		{"thumbnail-height", 'H', OPTPARSE_REQUIRED},
		{"thumbnail-width", 'W', OPTPARSE_REQUIRED},
		{"thumbnail-load", 't', OPTPARSE_NONE},
		{"thumbnail-save", 'T', OPTPARSE_NONE},
		{"accepted-extension", 'e', OPTPARSE_REQUIRED},
		{"keybind", 'k', OPTPARSE_REQUIRED},
		{"queue-length", 'q', OPTPARSE_REQUIRED},
		{0}
	};
	struct optparse options;
	optparse_init(&options, argv);
	int option;
	int tmpint = 0;
    while ((option = optparse_long(&options, longopts, NULL)) != -1) {
        switch (option) {
        case 'v':
			state->logger.level = nqiv_log_level_from_string(options.optarg);
			if(state->logger.level == NQIV_LOG_UNKNOWN) {
				fprintf(stderr, "Unknown log level %s", options.optarg);
				return false;
			}
            break;
        case 'l':
			if( !nqiv_add_logger_path(&state->logger, options.optarg) ) {
				return false;
			}
            break;
        case 'L':
			nqiv_log_set_prefix_format(&state->logger, options.optarg);
			if( !nqiv_check_and_print_logger_error(&state->logger) ) {
				return false;
			}
            break;
		case 'r':
			state->read_from_stdin = true;
			break;
        case 'P':
			if(strlen(options.optarg) < 1) {
				fprintf(stderr, "Thumbnail root must be at least one character long.\n");
				return false;
			}
			state->images.thumbnail.root = options.optarg;
			break;
        case 'H':
            tmpint = strtol(options.optarg, NULL, 10);
			if(tmpint <= 0 || errno == ERANGE) {
				fprintf(stderr, "Thumbnail height of %s must be a valid base-ten integer and greater than or equal to %d\n", options.optarg, 1);
				return false;
			}
			state->images.thumbnail.height = tmpint;
            break;
        case 'W':
            tmpint = strtol(options.optarg, NULL, 10);
			if(tmpint <= 0 || errno == ERANGE) {
				fprintf(stderr, "Thumbnail width of %s must be a valid base-ten integer and greater than or equal to %d\n", options.optarg, 1);
				return false;
			}
			state->images.thumbnail.width = tmpint;
            break;
		case 't':
			state->images.thumbnail.load = true;
			break;
		case 'T':
			state->images.thumbnail.save = true;
			break;
		case 'e':
			if( !nqiv_image_manager_add_extension(&state->images, options.optarg) ) {
				fprintf(stderr, "Failed to add allowed extension of %s\n", options.optarg);
				return false;
			}
			break;
        case 'k':
			if( !nqiv_keybind_add_from_text(&state->keybinds, options.optarg) ) {
				fprintf(stderr, "Failed to add keybind of %s\n", options.optarg);
				return false;
			}
			break;
        case 'q':
            tmpint = strtol(options.optarg, NULL, 10);
			if(tmpint < STARTING_QUEUE_LENGTH || errno == ERANGE) {
				fprintf(stderr, "Queue length of %s must be a valid base-ten integer and greater than or equal to %d\n", options.optarg, STARTING_QUEUE_LENGTH);
				return false;
			}
			if( !nqiv_array_grow(state->keybinds.lookup, tmpint) ||
				!nqiv_array_grow(state->images.images, tmpint) ||
				!nqiv_array_grow(state->images.extensions, tmpint) ||
				!nqiv_array_grow(state->thread_queue.array, tmpint) ) {
				fprintf(stderr, "Failed to grow to queue length of %s\n", options.optarg);
				return false;
			}
            break;
        case '?':
            fprintf(stderr, "%s: %s\n", argv[0], options.errmsg);
			return false;
        }
    }
	char* arg;
    while ( ( arg = optparse_arg(&options) ) ) {
		if( nqiv_image_manager_has_path_extension(&state->images, arg) ) {
			if( !nqiv_image_manager_append(&state->images, arg) ) {
				return false;
			}
		}
	}
	state->images.thumbnail.interpolation = LanczosFilter;
	/* TODO: STDIN */
	return true;
} /* parse_args */

bool nqiv_create_sdl_drawing_surface(nqiv_log_ctx* logger, const int width, const int height, SDL_Surface** surface)
{
	*surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 4 * 8, SDL_PIXELFORMAT_ABGR8888);
	if(*surface == NULL) {
		nqiv_log_write( logger, NQIV_LOG_ERROR, "Failed to create SDL Surface. SDL Error: %s\n", SDL_GetError() );
		return false;
	}
	return true;
}

bool nqiv_sdl_surface_to_texture(nqiv_log_ctx* logger, SDL_Renderer* renderer, SDL_Surface* surface, SDL_Texture** texture)
{
	*texture = SDL_CreateTextureFromSurface(renderer, surface);
	if(*texture == NULL) {
		nqiv_log_write( logger, NQIV_LOG_ERROR, "Failed to create SDL Texture. SDL Error: %s\n", SDL_GetError() );
		return false;
	}
	SDL_FreeSurface(surface);
	return true;
}

bool nqiv_create_solid_rect_texture(nqiv_log_ctx* logger, SDL_Renderer* renderer, const SDL_Rect* rect, const SDL_Color* color, SDL_Texture** texture)
{
	SDL_Surface* surface;
	if( !nqiv_create_sdl_drawing_surface(logger, rect->w, rect->h, &surface) ) {
		return false;
	}
	nqiv_fill_rect(surface, rect, color);
	if( !nqiv_sdl_surface_to_texture(logger, renderer, surface, texture) ) {
		return false;
	}
	return true;
}

bool nqiv_create_border_rect_texture(nqiv_log_ctx* logger, SDL_Renderer* renderer, const SDL_Rect* rect, const SDL_Color* color, SDL_Texture** texture)
{
	SDL_Surface* surface;
	if( !nqiv_create_sdl_drawing_surface(logger, rect->w, rect->h, &surface) ) {
		return false;
	}
	int pixel_size = ( (rect->w + rect->h) / 2 ) / 64;
	pixel_size = pixel_size > 0 ? pixel_size : 1;
	nqiv_draw_rect(surface, rect, color, pixel_size);
	if( !nqiv_sdl_surface_to_texture(logger, renderer, surface, texture) ) {
		return false;
	}
	return true;
}


bool nqiv_create_alpha_background_texture(nqiv_log_ctx* logger, SDL_Renderer* renderer, const SDL_Rect* rect, const int thickness, SDL_Texture** texture)
{
	SDL_Surface* surface;
	if( !nqiv_create_sdl_drawing_surface(logger, rect->w, rect->h, &surface) ) {
		return false;
	}
	SDL_Color color_one;
	color_one.r = 40;
	color_one.g = 40;
	color_one.b = 40;
	color_one.a = 255;
	SDL_Color color_two;
	color_two.r = 60;
	color_two.g = 60;
	color_two.b = 60;
	color_two.a = 255;
	nqiv_draw_alpha_background(surface, rect, thickness, &color_one, &color_two);
	if( !nqiv_sdl_surface_to_texture(logger, renderer, surface, texture) ) {
		return false;
	}
	return true;
}

bool nqiv_setup_sdl(nqiv_state* state)
{
	/* TODO Starting size? */
	if( SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0 ) {
		nqiv_log_write( &state->logger, NQIV_LOG_ERROR, "Failed to init SDL. SDL Error: %s\n", SDL_GetError() );
		return false;
	}
	state->SDL_inited = true;
	state->window = SDL_CreateWindow("nqiv", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
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
	SDL_Color color;
	color.r = 0;
	color.g = 0;
	color.b = 0;
	color.a = 255;
	SDL_Rect window_rect;
	window_rect.x = 0;
	window_rect.y = 0;
	SDL_GetWindowSizeInPixels(state->window, &window_rect.w, &window_rect.h);
	if( !nqiv_create_solid_rect_texture(&state->logger, state->renderer, &window_rect, &color, &state->texture_background) ) {
		return false;
	}
	SDL_Rect thumbnail_rect;
	thumbnail_rect.x = 0;
	thumbnail_rect.y = 0;
	thumbnail_rect.w = state->images.thumbnail.width;
	thumbnail_rect.h = state->images.thumbnail.height;
	SDL_Rect pixel_rect;
	pixel_rect.x = 0;
	pixel_rect.y = 0;
	pixel_rect.w = 1;
	pixel_rect.h = 1;
	color.g = 255;
	color.b = 255;
	if( !nqiv_create_border_rect_texture(&state->logger, state->renderer, &thumbnail_rect, &color, &state->texture_montage_selection) ) {
		return false;
	}
	color.g = 0;
	if( !nqiv_create_solid_rect_texture(&state->logger, state->renderer, &pixel_rect, &color, &state->texture_montage_unloaded_background) ) {
		return false;
	}
	color.r = 255;
	color.b = 0;
	if( !nqiv_create_solid_rect_texture(&state->logger, state->renderer, &pixel_rect, &color, &state->texture_montage_error_background) ) {
		return false;
	}
	const int thumbnail_thickness = ( (thumbnail_rect.x + thumbnail_rect.h) / 2 ) / 32;
	if( !nqiv_create_alpha_background_texture(&state->logger, state->renderer, &window_rect, thumbnail_thickness, &state->texture_montage_alpha_background) ) {
		return false;
	}
	const int window_thickness = ( (window_rect.x + window_rect.h) / 2 ) / 32;
	if( !nqiv_create_alpha_background_texture(&state->logger, state->renderer, &window_rect, window_thickness, &state->texture_alpha_background) ) {
		return false;
	}
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
		nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Acted on thread %d.\n", idx);
		action(&state->thread_locks[idx]);
	}
}

bool nqiv_setup_thread_info(nqiv_state* state)
{
	state->thread_event_number = SDL_RegisterEvents(1);
	if(state->thread_event_number == 0xFFFFFFFF) {
		nqiv_log_write( &state->logger, NQIV_LOG_ERROR, "Failed to create SDL event for messages from threads. SDL Error: %s\n", SDL_GetError() );
		return false;
	}
	state->thread_count = omp_get_num_threads();
	state->thread_locks = (omp_lock_t*)calloc( state->thread_count, sizeof(omp_lock_t) );
	if(state->thread_locks == NULL) {
		nqiv_log_write(&state->logger, NQIV_LOG_ERROR, "Failed to allocate memory for %d thread locks.\n", state->thread_count);
		return false;
	}
	nqiv_act_on_thread_locks(state, omp_init_lock);
	return true;
}

void nqiv_unlock_threads(nqiv_state* state)
{
	nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Unlocking threads.\n");
	nqiv_act_on_thread_locks(state, omp_unset_lock);
}

bool nqiv_send_thread_event(nqiv_state* state, nqiv_event* event)
{
		nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Sending event.\n");
		const bool event_sent = nqiv_queue_push(&state->thread_queue, sizeof(nqiv_event), event);
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
		nqiv_unlock_threads(state);
		nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Unlocked threads for event.\n");
		return true;
}

/* TODO STEP FRAME? */
/* TODO Reset frame */
bool render_from_form(nqiv_state* state, nqiv_image* image, SDL_Texture* alpha_background, const bool do_zoom, const SDL_Rect* dstrect, const bool is_thumbnail, const bool first_frame, const bool next_frame, const bool selected, const bool hard, const bool lock)
{
	/* TODO Srcrect easily can make this work for both views DONE */
	/* TODO Merge load/save thumbnail, or have an short load to check for the thumbnail before saving  DONE*/
	/* TODO Use load thumbnail for is_thumbnail? */
	if(lock) {
		nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Locking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
		omp_set_lock(&image->lock);
		nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Locked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
	}
	nqiv_image_form* form = is_thumbnail ? &image->thumbnail : &image->image;
	if(!is_thumbnail && state->images.thumbnail.save) {
		if(!image->thumbnail_attempted) {
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Creating thumbnail for instance that won't load it.\n");
			nqiv_event event = {0};
			event.type = NQIV_EVENT_IMAGE_LOAD;
			event.options.image_load.image = image;
			event.options.image_load.set_thumbnail_path = true;
			event.options.image_load.create_thumbnail = true;
			if( !nqiv_send_thread_event(state, &event) ) {
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
			if(!image->thumbnail_attempted && state->images.thumbnail.save) {
				nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Creating thumbnail after failing to load it.\n");
				form->error = false;
				nqiv_event event = {0};
				event.type = NQIV_EVENT_IMAGE_LOAD;
				event.options.image_load.image = image;
				event.options.image_load.set_thumbnail_path = true;
				event.options.image_load.create_thumbnail = true;
				if( !nqiv_send_thread_event(state, &event) ) {
					nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
					omp_unset_lock(&image->lock);
					nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
					return false;
				}
				/* TODO Signal to create thumbnail from main image, set thumbnail attempted after */
				/* TODO UNSET ERROR */
			} else {
				if( !render_from_form(state, image, alpha_background, do_zoom, dstrect, false, true, false, selected, hard, false) ) {
					nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
					omp_unset_lock(&image->lock);
					nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
					return false;
				}
				/* TODO Use main image for the thumbnail */
			}
		} else {
			if( SDL_RenderCopy(state->renderer, state->texture_montage_error_background, NULL, dstrect) != 0 ) {
				nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
				omp_unset_lock(&image->lock);
				nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
				return false;
			}
		}
	} else {
		if(form->texture != NULL) {
			/* NOOP */
		} else if(form->surface != NULL) {
			form->texture = SDL_CreateTextureFromSurface(state->renderer, form->surface);
			nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Loaded texture for image %s.\n", image->image.path);
			if(form->texture == NULL) {
				nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
				omp_unset_lock(&image->lock);
				nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
				return false;
			}
		} else {
			if( SDL_RenderCopy(state->renderer, state->texture_montage_unloaded_background, NULL, dstrect) != 0 ) {
				nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
				omp_unset_lock(&image->lock);
				nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
				return false;
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
					event.options.image_load.thumbnail_options.wand = true;
					event.options.image_load.borrow_thumbnail_dimension_metadata = true;
				} else {
					event.options.image_load.thumbnail_options.file_soft = true;
					event.options.image_load.thumbnail_options.wand_soft = true;
					event.options.image_load.borrow_thumbnail_dimension_metadata = true;
				}
				if( hard || next_frame || (first_frame && image->thumbnail.wand != NULL && MagickGetIteratorIndex(image->thumbnail.wand) != 0) ) {
					event.options.image_load.thumbnail_options.raw = true;
					event.options.image_load.thumbnail_options.surface = true;
				} else {
					event.options.image_load.thumbnail_options.raw_soft = true;
					event.options.image_load.thumbnail_options.surface_soft = true;
				}
				event.options.image_load.thumbnail_options.first_frame = first_frame;
				event.options.image_load.thumbnail_options.next_frame = next_frame;
				if( !nqiv_send_thread_event(state, &event) ) {
					nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
					omp_unset_lock(&image->lock);
					nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
					return false;
				}
				/* TODO Signal to load thumbnail image if it's available, otherwise use main */
			} else {
				nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Loading image.\n");
				nqiv_event event = {0};
				event.type = NQIV_EVENT_IMAGE_LOAD;
				event.options.image_load.image = image;
				if(hard) {
					event.options.image_load.image_options.file = true;
					event.options.image_load.image_options.wand = true;
				} else {
					event.options.image_load.image_options.file_soft = true;
					event.options.image_load.image_options.wand_soft = true;
				}
				if( hard || next_frame || (first_frame && image->thumbnail.wand != NULL && MagickGetIteratorIndex(image->thumbnail.wand) != 0) ) {
					event.options.image_load.image_options.raw = true;
					event.options.image_load.image_options.surface = true;
				} else {
					event.options.image_load.image_options.raw_soft = true;
					event.options.image_load.image_options.surface_soft = true;
				}
				event.options.image_load.image_options.first_frame = first_frame;
				event.options.image_load.image_options.next_frame = next_frame;
				if( !nqiv_send_thread_event(state, &event) ) {
					nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
					omp_unset_lock(&image->lock);
					nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
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
			SDL_Rect srcrect = {0};
			if(is_thumbnail) {
				srcrect.w = image->image.width;
				srcrect.h = image->image.height;
			} else {
				srcrect.w = form->width;
				srcrect.h = form->height;
			}
			SDL_Rect dstrect_zoom = {0};
			dstrect_zoom.w = dstrect->w;
			dstrect_zoom.h = dstrect->h;
			dstrect_zoom.x = dstrect->x;
			dstrect_zoom.y = dstrect->y;
			nqiv_image_manager_calculate_zoomrect(&state->images, do_zoom, state->stretch_images, &srcrect, &dstrect_zoom); /* TODO aspect ratio */
			if( SDL_RenderCopy(state->renderer, alpha_background, &dstrect_zoom, &dstrect_zoom) != 0 ) {
				nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
				omp_unset_lock(&image->lock);
				nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
				return false;
			}
			if( SDL_RenderCopy(state->renderer, form->texture, &srcrect, &dstrect_zoom) != 0 ) {
				nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
				omp_unset_lock(&image->lock);
				nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
				return false;
			}
		}
	}
	if(selected) {
		if( SDL_RenderCopy(state->renderer, state->texture_montage_selection, NULL, dstrect) != 0 ) {
			nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
			omp_unset_lock(&image->lock);
			nqiv_log_write( &state->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
			return false;
		}
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
	snprintf(idx_string, INT_MAX_STRLEN, "%d", state->montage.positions.selection + 1);
	snprintf( count_string, INT_MAX_STRLEN, "%lu", state->images.images->position / sizeof(nqiv_image*) );
	const char* path_components[] =
	{
		"nqiv - (",
		idx_string,
		"/",
		count_string,
		") - ",
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
	int idx;
	for(idx = state->montage.positions.start; idx < state->montage.positions.end; ++idx) {
		SDL_Rect dstrect;
		nqiv_montage_get_image_rect(&state->montage, idx, &dstrect);
		nqiv_image* image;
		if( !nqiv_array_get_bytes(state->images.images, idx, sizeof(nqiv_image*), &image) ) {
			return false;
		}
		nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Rendering montage image %s at %d.\n", image->image.path, idx);
		if( !render_from_form(state, image, state->texture_montage_alpha_background, false, &dstrect, state->images.thumbnail.load, true, false, state->montage.positions.selection == idx, hard, true) ) {
			return false;
		}
		if( idx == state->montage.positions.selection && !set_title(state, image) ) {
			return false;
		}
	}
	return true;
}

bool render_image(nqiv_state* state, const bool start, const bool hard)
{
	nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Rendering selected image.\n");
	if(SDL_RenderClear(state->renderer) != 0) {
		return false;
	}
	if(SDL_RenderCopy(state->renderer, state->texture_background, NULL, NULL) != 0) {
		return false;
	}
	nqiv_image* image;
	if( !nqiv_array_get_bytes(state->images.images, state->montage.positions.selection, sizeof(nqiv_image*), &image) ) {
		return false;
	}
	SDL_Rect dstrect = {0};
	/* TODO RECT DONE BUT ASPECT RATIO */
	SDL_GetWindowSizeInPixels(state->window, &dstrect.w, &dstrect.h);
	if( !render_from_form(state, image, state->texture_alpha_background, true, &dstrect, false, start, true, false, hard, true) ) {
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
	if(state->in_montage) {
		nqiv_montage_calculate_dimensions(&state->montage);
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

bool nqiv_master_thread(nqiv_state* state)
{
	nqiv_array* keybind_actions = nqiv_array_create(STARTING_QUEUE_LENGTH);
	if(keybind_actions == NULL) {
		return false;
	}
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
				if(input_event.user.code >= 0 && (Uint32)input_event.user.code == state->thread_event_number) {
					/*
					nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Locking thread from master.\n");
					omp_set_lock(input_event.user.data1);
					nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Locked thread from master.\n");
					*/
					omp_set_lock(&state->thread_queue.lock);
					const int queue_length = state->thread_queue.array->position / sizeof(nqiv_event*);
					if(queue_length == 0) {
						if( omp_test_lock(input_event.user.data1) ) {
							nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Locked thread from master.\n");
						} else {
							nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Thread already locked from master.\n");
						}
					} else {
						nqiv_unlock_threads(state);
					}
					omp_unset_lock(&state->thread_queue.lock);
				}
				render_and_update(state, &running, &result, false, false);
				break;
			case SDL_WINDOWEVENT:
				if(input_event.window.event == SDL_WINDOWEVENT_RESIZED || input_event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
					nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received window resize event.\n");
					render_and_update(state, &running, &result, false, false);
				}
				break;
			case SDL_QUIT:
				nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received window quit event.\n");
				running = false;
				break;
			case SDL_KEYUP:
				/*
				 * TODO
				SDL_Keysym
				nqiv_key_lookup_summary lookup_summary = nqiv_keybind_lookup(&state->keybinds, const SDL_Keysym* key, nqiv_array* output);
				*/
				nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received key up event.\n");
				{
					const nqiv_key_lookup_summary lookup_summary = nqiv_keybind_lookup(&state->keybinds, &input_event.key.keysym, keybind_actions);
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
						nqiv_key_action action;
						while( nqiv_array_get_bytes(keybind_actions, 0, sizeof(nqiv_key_action), &action) ) {
							/*
	NQIV_KEY_ACT	ION_TOGGLE_STRETCH,
	NQIV_KEY_ACT	ION_STRETCH,
	NQIV_KEY_ACT	ION_KEEP_ASPECT_RATIO,
	NQIV_KEY_ACT	ION_RELOAD,
	*/
							if(action == NQIV_KEY_ACTION_QUIT) {
								nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action quit.\n");
								running = false;
							} else if(action == NQIV_KEY_ACTION_MONTAGE_RIGHT) {
								nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action montage right.\n");
								if(state->in_montage) {
									nqiv_montage_next_selection(&state->montage);
									render_and_update(state, &running, &result, false, false);
								}
							} else if(action == NQIV_KEY_ACTION_MONTAGE_LEFT) {
								nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action montage left.\n");
								if(state->in_montage) {
									nqiv_montage_previous_selection(&state->montage);
									render_and_update(state, &running, &result, false, false);
								}
							} else if(action == NQIV_KEY_ACTION_MONTAGE_UP) {
								nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action montage up.\n");
								if(state->in_montage) {
									nqiv_montage_previous_selection_row(&state->montage);
									render_and_update(state, &running, &result, false, false);
								}
							} else if(action == NQIV_KEY_ACTION_MONTAGE_DOWN) {
								nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action montage down.\n");
								if(state->in_montage) {
									nqiv_montage_next_selection_row(&state->montage);
									render_and_update(state, &running, &result, false, false);
								}
							} else if(action == NQIV_KEY_ACTION_PAGE_UP) {
								nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action page up.\n");
								if(state->in_montage) {
									nqiv_montage_previous_selection_page(&state->montage);
									render_and_update(state, &running, &result, false, false);
								}
							} else if(action == NQIV_KEY_ACTION_PAGE_DOWN) {
								nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action page down.\n");
								if(state->in_montage) {
									nqiv_montage_next_selection_page(&state->montage);
									render_and_update(state, &running, &result, false, false);
								}
							} else if(action == NQIV_KEY_ACTION_MONTAGE_START) {
								nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action montage start.\n");
								if(state->in_montage) {
									nqiv_montage_jump_selection_start(&state->montage);
									render_and_update(state, &running, &result, false, false);
								}
							} else if(action == NQIV_KEY_ACTION_MONTAGE_END) {
								nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action montage end.\n");
								if(state->in_montage) {
									nqiv_montage_jump_selection_end(&state->montage);
									render_and_update(state, &running, &result, false, false);
								}
							} else if(action == NQIV_KEY_ACTION_TOGGLE_MONTAGE) {
								nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action montage toggle.\n");
								state->in_montage = !state->in_montage;
								render_and_update(state, &running, &result, false, false);
							} else if(action == NQIV_KEY_ACTION_SET_MONTAGE) {
								nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv montage set.\n");
								state->in_montage = true;
								render_and_update(state, &running, &result, false, false);
							} else if(action == NQIV_KEY_ACTION_SET_VIEWING) {
								nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action viewing set.\n");
								state->in_montage = false;
								render_and_update(state, &running, &result, false, false);
							} else if(action == NQIV_KEY_ACTION_ZOOM_IN) {
								nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action zoom in.\n");
								if(!state->in_montage) {
									nqiv_image_manager_zoom_in(&state->images);
									render_and_update(state, &running, &result, false, false);
								} else if(state->montage.dimensions.count > 1) {
									nqiv_image_manager_increment_thumbnail_size(&state->images);
									render_and_update(state, &running, &result, false, false);
								}
							} else if(action == NQIV_KEY_ACTION_ZOOM_OUT) {
								nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action zoom out.\n");
								if(!state->in_montage) {
									nqiv_image_manager_zoom_out(&state->images);
									render_and_update(state, &running, &result, false, false);
								} else {
									nqiv_image_manager_decrement_thumbnail_size(&state->images);
									render_and_update(state, &running, &result, false, false);
								}
							} else if(action == NQIV_KEY_ACTION_PAN_LEFT) {
								nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action pan left.\n");
								if(!state->in_montage) {
									nqiv_image_manager_pan_left(&state->images);
									render_and_update(state, &running, &result, false, false);
								}
							} else if(action == NQIV_KEY_ACTION_PAN_RIGHT) {
								nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action pan right.\n");
								if(!state->in_montage) {
									nqiv_image_manager_pan_right(&state->images);
									render_and_update(state, &running, &result, false, false);
								}
							} else if(action == NQIV_KEY_ACTION_PAN_UP) {
								nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action pan up.\n");
								if(!state->in_montage) {
									nqiv_image_manager_pan_up(&state->images);
									render_and_update(state, &running, &result, false, false);
								}
							} else if(action == NQIV_KEY_ACTION_PAN_DOWN) {
								nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action pan down.\n");
								if(!state->in_montage) {
									nqiv_image_manager_pan_down(&state->images);
									render_and_update(state, &running, &result, false, false);
								}
							} else if(action == NQIV_KEY_ACTION_TOGGLE_STRETCH) {
								nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action toggle stretch.\n");
								state->stretch_images = !state->stretch_images;
								render_and_update(state, &running, &result, false, false);
							} else if(action == NQIV_KEY_ACTION_STRETCH) {
								nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action stretch.\n");
								state->stretch_images = true;
								render_and_update(state, &running, &result, false, false);
							} else if(action == NQIV_KEY_ACTION_KEEP_ASPECT_RATIO) {
								nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action keep aspect ratio.\n");
								state->stretch_images = false;
								render_and_update(state, &running, &result, false, false);
							} else if(action == NQIV_KEY_ACTION_RELOAD) {
								nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Received nqiv action reload.\n");
								render_and_update(state, &running, &result, false, true);
							}
							nqiv_array_remove_bytes( keybind_actions, 0, sizeof(nqiv_key_action) );
						}
					}
				}
				break;
		}
	}
	nqiv_log_write(&state->logger, NQIV_LOG_DEBUG, "Finished waiting on events.\n");
	nqiv_array_destroy(keybind_actions);
	nqiv_event output_event = {0};
	output_event.type = NQIV_EVENT_WORKER_STOP;
	if( !nqiv_send_thread_event(state, &output_event) ) {
		nqiv_log_write(&state->logger, NQIV_LOG_ERROR, "Failed to send shutdown signal to threads. Aborting.\n");
		abort();
	}
	return result;
}

bool nqiv_run(nqiv_state* state)
{
	bool result;
	bool* result_ptr = &result;
	const int thread_count = state->thread_count;
	omp_lock_t* thread_locks = state->thread_locks;
	nqiv_queue* thread_queue = &state->thread_queue;
	const Uint32 event_code = state->thread_event_number;
	#pragma omp parallel default(none) firstprivate(state, thread_count, thread_locks, thread_queue, event_code, result_ptr)
	{
		#pragma omp master
		{
			int thread;
			for(thread = 0; thread < thread_count; ++thread) {
				omp_lock_t* lock = &thread_locks[thread];
				#pragma omp task default(none) firstprivate(thread_queue, lock, event_code)
				nqiv_worker_main(thread_queue, lock, event_code);
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

	state.in_montage = true;

	MagickWandGenesis();

	if( !nqiv_parse_args(argv, &state) ) {
		nqiv_state_clear(&state);
		return 1;
	}
	if( !nqiv_setup_sdl(&state) ) {
		nqiv_state_clear(&state);
		return 1;
	}
	nqiv_setup_montage(&state);
	if( !nqiv_setup_thread_info(&state) ) {
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
