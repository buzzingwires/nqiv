#include "state.h"
#include "drawing.h"

#include <SDL2/SDL.h>

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

void nqiv_state_set_default_colors(nqiv_state* state)
{
	state->background_color.r = 0;
	state->background_color.g = 0;
	state->background_color.b = 0;
	state->background_color.a = 255;
	state->error_color.r = 255;
	state->error_color.g = 0;
	state->error_color.b = 0;
	state->error_color.a = 255;
	state->loading_color.r = 0;
	state->loading_color.g = 255;
	state->loading_color.b = 255;
	state->loading_color.a = 255;
	state->selection_color.r = 255;
	state->selection_color.g = 255;
	state->selection_color.b = 0;
	state->selection_color.a = 255;
	state->mark_color.r = 255;
	state->mark_color.g = 255;
	state->mark_color.b = 255;
	state->mark_color.a = 255;
	state->alpha_checker_color_one.r = 60;
	state->alpha_checker_color_one.g = 60;
	state->alpha_checker_color_one.b = 60;
	state->alpha_checker_color_one.a = 255;
	state->alpha_checker_color_two.r = 40;
	state->alpha_checker_color_two.g = 40;
	state->alpha_checker_color_two.b = 40;
	state->alpha_checker_color_two.a = 255;
}

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
	nqiv_fill_checked_rect(surface, rect, -1, -1, color, color);
	if( !nqiv_sdl_surface_to_texture(logger, renderer, surface, texture) ) {
		return false;
	}
	return true;
}

bool nqiv_create_border_rect_texture(nqiv_log_ctx* logger, SDL_Renderer* renderer, const SDL_Rect* rect, const int dash_size, const SDL_Color* color, const SDL_Color* dash_color, SDL_Texture** texture)
{
	SDL_Surface* surface;
	if( !nqiv_create_sdl_drawing_surface(logger, rect->w, rect->h, &surface) ) {
		return false;
	}
	int pixel_size = ( (rect->w + rect->h) / 2 ) / 64;
	pixel_size = pixel_size > 0 ? pixel_size : 1;
	nqiv_draw_rect(surface, rect, dash_size, color, dash_color, pixel_size);
	if( !nqiv_sdl_surface_to_texture(logger, renderer, surface, texture) ) {
		return false;
	}
	return true;
}


bool nqiv_create_alpha_background_texture(nqiv_state* state, const SDL_Rect* rect, const int thickness, SDL_Texture** texture)
{
	/*nqiv_log_ctx* logger, SDL_Renderer* renderer*/
	SDL_Surface* surface;
	if( !nqiv_create_sdl_drawing_surface(&state->logger, rect->w, rect->h, &surface) ) {
		return false;
	}
	nqiv_fill_checked_rect(surface, rect, thickness, thickness, &state->alpha_checker_color_one, &state->alpha_checker_color_two);
	if( !nqiv_sdl_surface_to_texture(&state->logger, state->renderer, surface, texture) ) {
		return false;
	}
	return true;
}

bool nqiv_state_create_thumbnail_selection_texture(nqiv_state* state)
{
	SDL_Rect thumbnail_rect;
	thumbnail_rect.x = 0;
	thumbnail_rect.y = 0;
	thumbnail_rect.w = state->images.thumbnail.size;
	thumbnail_rect.h = state->images.thumbnail.size;
	if( !nqiv_create_border_rect_texture(&state->logger, state->renderer, &thumbnail_rect, -1, &state->selection_color, &state->selection_color, &state->texture_montage_selection) ) {
		return false;
	}
	return true;
}

bool nqiv_state_recreate_thumbnail_selection_texture(nqiv_state* state)
{
	SDL_Texture* old_texture = state->texture_montage_selection;
	if( !nqiv_state_create_thumbnail_selection_texture(state) ) {
		state->texture_montage_selection = old_texture;
		return false;
	}
	SDL_DestroyTexture(old_texture);
	return true;
}

bool nqiv_state_create_mark_texture(nqiv_state* state)
{
	SDL_Rect thumbnail_rect;
	thumbnail_rect.x = 0;
	thumbnail_rect.y = 0;
	thumbnail_rect.w = state->images.thumbnail.size;
	thumbnail_rect.h = state->images.thumbnail.size;
	SDL_Color dash_color;
	memcpy( &dash_color, &state->mark_color, sizeof(SDL_Color) );
	dash_color.a = 0;
	if( !nqiv_create_border_rect_texture(&state->logger, state->renderer, &thumbnail_rect, state->images.thumbnail.size / 16, &state->mark_color, &dash_color, &state->texture_montage_mark) ) {
		return false;
	}
	return true;
}

bool nqiv_state_recreate_mark_texture(nqiv_state* state)
{
	SDL_Texture* old_texture = state->texture_montage_mark;
	if( !nqiv_state_create_mark_texture(state) ) {
		state->texture_montage_mark = old_texture;
		return false;
	}
	SDL_DestroyTexture(old_texture);
	return true;
}

bool nqiv_state_create_montage_alpha_background_texture(nqiv_state* state)
{
	SDL_Rect thumbnail_rect;
	thumbnail_rect.x = 0;
	thumbnail_rect.h = state->images.thumbnail.size;
	SDL_Rect window_rect;
	window_rect.x = 0;
	window_rect.y = 0;
	SDL_GetWindowSizeInPixels(state->window, &window_rect.w, &window_rect.h);
	const int thumbnail_thickness = ( (thumbnail_rect.x + thumbnail_rect.h) / 2 ) / 32;
	if( !nqiv_create_alpha_background_texture(state, &window_rect, thumbnail_thickness, &state->texture_montage_alpha_background) ) {
		return false;
	}
	return true;
}

bool nqiv_state_create_alpha_background_texture(nqiv_state* state)
{
	SDL_Rect window_rect;
	window_rect.x = 0;
	window_rect.y = 0;
	SDL_GetWindowSizeInPixels(state->window, &window_rect.w, &window_rect.h);
	const int window_thickness = ( (window_rect.x + window_rect.h) / 2 ) / 32;
	if( !nqiv_create_alpha_background_texture(state, &window_rect, window_thickness, &state->texture_alpha_background) ) {
		return false;
	}
	return true;
}

bool nqiv_state_create_single_color_texture(nqiv_state* state, const SDL_Color* color, SDL_Texture** texture)
{
	SDL_Rect pixel_rect;
	pixel_rect.x = 0;
	pixel_rect.y = 0;
	pixel_rect.w = 1;
	pixel_rect.h = 1;
	if( !nqiv_create_solid_rect_texture(&state->logger, state->renderer, &pixel_rect, color, texture) ) {
		return false;
	}
	return true;
}

bool nqiv_state_recreate_single_color_texture(nqiv_state* state, const SDL_Color* color, SDL_Texture** texture)
{
	SDL_Texture* old_texture = *texture;
	if( !nqiv_state_create_single_color_texture(state, color, texture) ) {
		*texture = old_texture;
		return false;
	}
	SDL_DestroyTexture(old_texture);
	return true;
}

bool nqiv_state_recreate_background_texture(nqiv_state* state)
{
	return nqiv_state_recreate_single_color_texture(state, &state->background_color, &state->texture_background);
}

bool nqiv_state_recreate_error_texture(nqiv_state* state)
{
	return nqiv_state_recreate_single_color_texture(state, &state->error_color, &state->texture_montage_error_background);
}

bool nqiv_state_recreate_loading_texture(nqiv_state* state)
{
	return nqiv_state_recreate_single_color_texture(state, &state->loading_color, &state->texture_montage_unloaded_background);
}

bool nqiv_state_expand_queues(nqiv_state* state)
{
	if(!nqiv_array_grow(state->keybinds.lookup, state->queue_length) ||
	   !nqiv_array_grow(state->images.images, state->queue_length) ||
	   !nqiv_array_grow(state->images.extensions, state->queue_length) ||
	   !nqiv_array_grow(state->thread_queue.array, state->queue_length) ||
	   !nqiv_array_grow(state->key_actions.array, state->queue_length) ||
	   !nqiv_array_grow(state->cmds.buffer, state->queue_length) ) {
		return false;
	}
	return true;
}


bool nqiv_state_recreate_all_alpha_background_textures(nqiv_state* state)
{
	SDL_Texture* old_alpha_background = state->texture_alpha_background;
	SDL_Texture* old_montage_alpha_background = state->texture_montage_alpha_background;
	state->texture_alpha_background = NULL;
	state->texture_montage_alpha_background = NULL;
	nqiv_state_create_alpha_background_texture(state);
	if( state->texture_alpha_background == NULL ) {
		state->texture_alpha_background = old_alpha_background;
		state->texture_montage_alpha_background = old_montage_alpha_background;
		return false;
	}
	nqiv_state_create_montage_alpha_background_texture(state);
	if( state->texture_montage_alpha_background == NULL ) {
		SDL_DestroyTexture(state->texture_alpha_background);
		state->texture_alpha_background = old_alpha_background;
		state->texture_montage_alpha_background = old_montage_alpha_background;
		return false;
	}
	SDL_DestroyTexture(old_alpha_background);
	SDL_DestroyTexture(old_montage_alpha_background);
	return true;
}
