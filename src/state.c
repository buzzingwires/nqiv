#include "state.h"
#include "drawing.h"

#include <SDL2/SDL.h>

const char* const nqiv_zoom_default_names[] = {
	"keep",
	"fit",
	"actual",
};

const char* const nqiv_texture_scale_mode_names[] = {
	"nearest", "linear", "anisotropic", "best", NULL,
};

const SDL_ScaleMode nqiv_texture_scale_modes[] = {
	SDL_ScaleModeNearest,
	SDL_ScaleModeLinear,
	SDL_ScaleModeBest,
	SDL_ScaleModeBest,
};

nqiv_zoom_default nqiv_text_to_zoom_default(const char* text)
{
	nqiv_zoom_default zd = NQIV_ZOOM_DEFAULT_UNKNOWN;
	for(zd = NQIV_ZOOM_DEFAULT_KEEP; zd <= NQIV_ZOOM_DEFAULT_ACTUAL; ++zd) {
		if(strncmp(text, nqiv_zoom_default_names[zd], strlen(nqiv_zoom_default_names[zd])) == 0
		   && strlen(text) == strlen(nqiv_zoom_default_names[zd])) {
			return zd;
		}
	}
	return NQIV_ZOOM_DEFAULT_UNKNOWN;
}

bool nqiv_text_to_scale_mode(const char* text, SDL_ScaleMode* sm)
{
	int idx;
	for(idx = 0; nqiv_texture_scale_mode_names[idx] != NULL; ++idx) {
		if(strncmp(text, nqiv_texture_scale_mode_names[idx],
		           strlen(nqiv_texture_scale_mode_names[idx]))
		       == 0
		   && strlen(text) == strlen(nqiv_texture_scale_mode_names[idx])) {
			*sm = nqiv_texture_scale_modes[idx];
			return true;
		}
	}
	return false;
}

const char* nqiv_scale_mode_to_text(const SDL_ScaleMode sm)
{
	int idx;
	for(idx = 0; nqiv_texture_scale_mode_names[idx] != NULL; ++idx) {
		if(nqiv_texture_scale_modes[idx] == sm) {
			return nqiv_texture_scale_mode_names[idx];
		}
	}
	return NULL;
}

bool nqiv_check_and_print_logger_error(const nqiv_log_ctx* logger)
{
	if(nqiv_log_has_error(logger)) {
		fputs(logger->error_message, stderr);
		return false;
	}
	return true;
}

bool nqiv_add_logger_path(nqiv_state* state, const char* path)
{
	FILE* stream = NULL;
	if(strcmp(path, "stdout") == 0) {
		stream = stdout;
	} else if(strcmp(path, "stderr") == 0) {
		stream = stderr;
	} else {
		stream = nqiv_fopen(path, "a");
	}
	if(stream == NULL) {
		return false;
	}
	char* persistent_path = (char*)calloc(1, strlen(path) + 1);
	if(persistent_path == NULL) {
		fclose(stream);
		return false;
	}
	strcpy(persistent_path, path);
	if(!nqiv_array_push(state->logger_stream_names, &persistent_path)) {
		free(persistent_path);
		fclose(stream);
		return false;
	}
	nqiv_log_add_stream(&state->logger, stream);
	if(!nqiv_check_and_print_logger_error(&state->logger)) {
		nqiv_array_pop(state->logger_stream_names, NULL);
		fclose(stream);
		free(persistent_path);
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
	state->loading_color.g = 0;
	state->loading_color.b = 0;
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

bool nqiv_create_sdl_drawing_surface(nqiv_log_ctx* logger,
                                     const int     width,
                                     const int     height,
                                     SDL_Surface** surface)
{
	*surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 4 * 8, SDL_PIXELFORMAT_ABGR8888);
	if(*surface == NULL) {
		nqiv_log_write(logger, NQIV_LOG_ERROR, "Failed to create SDL Surface. SDL Error: %s\n",
		               SDL_GetError());
		return false;
	}
	return true;
}

bool nqiv_sdl_surface_to_texture(nqiv_log_ctx* logger,
                                 SDL_Renderer* renderer,
                                 SDL_Surface*  surface,
                                 SDL_Texture** texture)
{
	*texture = SDL_CreateTextureFromSurface(renderer, surface);
	if(*texture == NULL) {
		nqiv_log_write(logger, NQIV_LOG_ERROR, "Failed to create SDL Texture. SDL Error: %s\n",
		               SDL_GetError());
		return false;
	}
	SDL_FreeSurface(surface);
	return true;
}

bool nqiv_create_solid_rect_texture(nqiv_log_ctx*    logger,
                                    SDL_Renderer*    renderer,
                                    const SDL_Rect*  rect,
                                    const SDL_Color* color,
                                    SDL_Texture**    texture)
{
	SDL_Surface* surface;
	if(!nqiv_create_sdl_drawing_surface(logger, rect->w, rect->h, &surface)) {
		return false;
	}
	nqiv_fill_checked_rect(surface, rect, -1, -1, color, color);
	if(!nqiv_sdl_surface_to_texture(logger, renderer, surface, texture)) {
		return false;
	}
	return true;
}

bool nqiv_create_border_rect_texture(nqiv_log_ctx*    logger,
                                     SDL_Renderer*    renderer,
                                     const SDL_Rect*  rect,
                                     const int        dash_size,
                                     const SDL_Color* color,
                                     const SDL_Color* dash_color,
                                     SDL_Texture**    texture)
{
	SDL_Surface* surface;
	if(!nqiv_create_sdl_drawing_surface(logger, rect->w, rect->h, &surface)) {
		return false;
	}
	int pixel_size = ((rect->w + rect->h) / 2) / 64;
	pixel_size = pixel_size > 0 ? pixel_size : 1;
	nqiv_draw_rect(surface, rect, dash_size, color, dash_color, pixel_size);
	if(!nqiv_sdl_surface_to_texture(logger, renderer, surface, texture)) {
		return false;
	}
	return true;
}

bool nqiv_create_alpha_background_texture(nqiv_state*     state,
                                          const SDL_Rect* rect,
                                          const int       thickness,
                                          SDL_Texture**   texture)
{
	SDL_Surface* surface;
	if(!nqiv_create_sdl_drawing_surface(&state->logger, rect->w, rect->h, &surface)) {
		return false;
	}
	nqiv_fill_checked_rect(surface, rect, thickness, thickness, &state->alpha_checker_color_one,
	                       &state->alpha_checker_color_two);
	if(!nqiv_sdl_surface_to_texture(&state->logger, state->renderer, surface, texture)) {
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
	if(!nqiv_create_border_rect_texture(&state->logger, state->renderer, &thumbnail_rect, -1,
	                                    &state->selection_color, &state->selection_color,
	                                    &state->texture_montage_selection)) {
		return false;
	}
	return true;
}

bool nqiv_state_recreate_thumbnail_selection_texture(nqiv_state* state)
{
	SDL_Texture* old_texture = state->texture_montage_selection;
	if(!nqiv_state_create_thumbnail_selection_texture(state)) {
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
	memcpy(&dash_color, &state->mark_color, sizeof(SDL_Color));
	dash_color.a = 0;
	if(!nqiv_create_border_rect_texture(&state->logger, state->renderer, &thumbnail_rect,
	                                    state->images.thumbnail.size / 16, &state->mark_color,
	                                    &dash_color, &state->texture_montage_mark)) {
		return false;
	}
	return true;
}

bool nqiv_state_recreate_mark_texture(nqiv_state* state)
{
	SDL_Texture* old_texture = state->texture_montage_mark;
	if(!nqiv_state_create_mark_texture(state)) {
		state->texture_montage_mark = old_texture;
		return false;
	}
	SDL_DestroyTexture(old_texture);
	return true;
}

bool nqiv_state_create_alpha_background_texture(nqiv_state* state)
{
	SDL_Rect window_rect;
	window_rect.x = 0;
	window_rect.y = 0;
	window_rect.w = state->alpha_background_width;
	window_rect.h = state->alpha_background_height;
	if(!nqiv_create_alpha_background_texture(state, &window_rect,
	                                         ((window_rect.x + window_rect.h) / 2)
	                                             / ALPHA_BACKGROUND_CHECKER_PROPORTION,
	                                         &state->texture_alpha_background)) {
		return false;
	}
	return true;
}

bool nqiv_state_create_single_color_texture(nqiv_state*      state,
                                            const SDL_Color* color,
                                            SDL_Texture**    texture)
{
	SDL_Rect pixel_rect;
	pixel_rect.x = 0;
	pixel_rect.y = 0;
	pixel_rect.w = 1;
	pixel_rect.h = 1;
	if(!nqiv_create_solid_rect_texture(&state->logger, state->renderer, &pixel_rect, color,
	                                   texture)) {
		return false;
	}
	return true;
}

bool nqiv_state_recreate_single_color_texture(nqiv_state*      state,
                                              const SDL_Color* color,
                                              SDL_Texture**    texture)
{
	SDL_Texture* old_texture = *texture;
	if(!nqiv_state_create_single_color_texture(state, color, texture)) {
		*texture = old_texture;
		return false;
	}
	SDL_DestroyTexture(old_texture);
	return true;
}

bool nqiv_state_recreate_background_texture(nqiv_state* state)
{
	return nqiv_state_recreate_single_color_texture(state, &state->background_color,
	                                                &state->texture_background);
}

bool nqiv_state_recreate_error_texture(nqiv_state* state)
{
	return nqiv_state_recreate_single_color_texture(state, &state->error_color,
	                                                &state->texture_montage_error_background);
}

bool nqiv_state_recreate_loading_texture(nqiv_state* state)
{
	return nqiv_state_recreate_single_color_texture(state, &state->loading_color,
	                                                &state->texture_montage_unloaded_background);
}

bool nqiv_state_recreate_all_alpha_background_textures(nqiv_state* state)
{
	if(state->alpha_background_width == 0 || state->alpha_background_height == 0) {
		return true;
	}
	SDL_Texture* old_alpha_background = state->texture_alpha_background;
	state->texture_alpha_background = NULL;
	nqiv_state_create_alpha_background_texture(state);
	if(state->texture_alpha_background == NULL) {
		state->texture_alpha_background = old_alpha_background;
		return false;
	}
	if(old_alpha_background != NULL) {
		SDL_DestroyTexture(old_alpha_background);
	}
	return true;
}

bool nqiv_state_update_alpha_background_dimensions(nqiv_state* state,
                                                   const int   alpha_background_width,
                                                   const int   alpha_background_height)
{
	if(alpha_background_width == state->alpha_background_width
	   && alpha_background_height == state->alpha_background_height) {
		return true;
	}
	const int old_alpha_background_width = state->alpha_background_width;
	const int old_alpha_background_height = state->alpha_background_height;
	state->alpha_background_width = alpha_background_width;
	state->alpha_background_height = alpha_background_height;
	if(!nqiv_state_recreate_all_alpha_background_textures(state)) {
		state->alpha_background_width = old_alpha_background_width;
		state->alpha_background_height = old_alpha_background_height;
		return false;
	}
	return true;
}
