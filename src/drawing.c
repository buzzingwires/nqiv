#include <assert.h>

#include <SDL2/SDL.h>

#include "drawing.h"

void nqiv_fill_rect(SDL_Surface* surface, const SDL_Rect* rect, const SDL_Color* color)
{
	assert(surface != NULL);
	assert(surface->format->format == SDL_PIXELFORMAT_ABGR8888);
	assert(surface->format->BytesPerPixel == 4);
	assert(rect != NULL);
	assert(color != NULL);
	SDL_LockSurface(surface);
	int y;
	int x;
	for(y = rect->y; y < rect->y + rect->h; ++y) {
		for(x = rect->x; x < rect->x + rect->w; ++x) {
			assert( sizeof(Uint8*) == sizeof(void*) );
			Uint8* pixel = (Uint8*)(surface->pixels) + y * surface->pitch + x * surface->format->BytesPerPixel;
			pixel[0] = color->r;
			pixel[1] = color->g;
			pixel[2] = color->b;
			pixel[3] = color->a;
		}
	}
	SDL_UnlockSurface(surface);
}

void nqiv_draw_rect(SDL_Surface* surface, const SDL_Rect* from_rect, const SDL_Color* color, const int thickness)
{
	assert(from_rect != NULL);
	assert(thickness > 0);

	SDL_Rect to_rect;
	to_rect.x = from_rect->x;
	to_rect.y = from_rect->y;
	to_rect.w = from_rect->w;
	to_rect.h = thickness;
	nqiv_fill_rect(surface, &to_rect, color);

	to_rect.w = thickness;
	to_rect.h = from_rect->h;
	nqiv_fill_rect(surface, &to_rect, color);

	to_rect.y = from_rect->y + from_rect->h - thickness;
	to_rect.w = from_rect->w;
	to_rect.h = thickness;
	nqiv_fill_rect(surface, &to_rect, color);

	to_rect.x = from_rect->x + from_rect->w - thickness;
	to_rect.y = from_rect->y;
	to_rect.w = thickness;
	to_rect.h = from_rect->h;
	nqiv_fill_rect(surface, &to_rect, color);
}

void nqiv_draw_alpha_background(SDL_Surface* surface, const SDL_Rect* rect, const int size, const SDL_Color* color_one, const SDL_Color* color_two)
{
	assert(surface != NULL);
	assert(surface->format->format == SDL_PIXELFORMAT_ABGR8888);
	assert(surface->format->BytesPerPixel == 4);
	assert(rect != NULL);
	assert(color_one != NULL);
	assert(color_two != NULL);
	SDL_LockSurface(surface);
	int y;
	int x;
	const SDL_Color* color = color_one;
	int x_square_count = 0;
	int y_square_count = 0;
	for(y = rect->y; y < rect->h; ++y) {
		const SDL_Color* row_start_color = color;
		if(y_square_count == size) {
			row_start_color = color == color_one ? color_two : color_one;
			y_square_count = 0;
		}
		color = row_start_color;
		x_square_count = 0;
		for(x = rect->x; x < rect->w; ++x) {
			assert( sizeof(Uint8*) == sizeof(void*) );
			Uint8* pixel = (Uint8*)(surface->pixels) + y * surface->pitch + x * surface->format->BytesPerPixel;
			pixel[0] = color->r;
			pixel[1] = color->g;
			pixel[2] = color->b;
			pixel[3] = color->a;
			++x_square_count;
			if(x_square_count == size) {
				color = color == color_one ? color_two : color_one;
				x_square_count = 0;
			}
		}
		++y_square_count;
	}
	SDL_UnlockSurface(surface);
}
