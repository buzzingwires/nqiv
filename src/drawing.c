#include <assert.h>

#include <SDL2/SDL.h>

#include "drawing.h"

void nqiv_fill_checked_rect(SDL_Surface* surface, const SDL_Rect* rect, const int x_check_size, const int y_check_size, const SDL_Color* color_one, const SDL_Color* color_two)
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
	const SDL_Color* row_start_color = color;
	int y_square_count = 0;
	for(y = rect->y; y < rect->h; ++y) {
		if(y_square_count == y_check_size) {
			row_start_color = row_start_color == color_one ? color_two : color_one;
			y_square_count = 0;
		}
		color = row_start_color;
		int x_square_count = 0;
		for(x = rect->x; x < rect->w; ++x) {
			assert( sizeof(Uint8*) == sizeof(void*) );
			Uint8* pixel = (Uint8*)(surface->pixels) + y * surface->pitch + x * surface->format->BytesPerPixel;
			pixel[0] = color->r;
			pixel[1] = color->g;
			pixel[2] = color->b;
			pixel[3] = color->a;
			++x_square_count;
			if(x_square_count == x_check_size) {
				color = color == color_one ? color_two : color_one;
				x_square_count = 0;
			}
		}
		++y_square_count;
	}
	SDL_UnlockSurface(surface);
}

void nqiv_draw_rect(SDL_Surface* surface, const SDL_Rect* from_rect, const int dash_size, const SDL_Color* color, const SDL_Color* dash_color, const int thickness)
{
	assert(from_rect != NULL);
	assert(thickness > 0);

	SDL_Rect to_rect;
	to_rect.x = from_rect->x;
	to_rect.y = from_rect->y;
	to_rect.w = to_rect.x + from_rect->w;
	to_rect.h = to_rect.y + thickness;
	nqiv_fill_checked_rect(surface, &to_rect, dash_size, -1, color, dash_color);

	to_rect.w = to_rect.x + thickness;
	to_rect.h = to_rect.y + from_rect->h;
	nqiv_fill_checked_rect(surface, &to_rect, -1, dash_size, color, dash_color);

	to_rect.y = from_rect->y + from_rect->h - thickness;
	to_rect.w = to_rect.x + from_rect->w;
	to_rect.h = to_rect.y + thickness;
	nqiv_fill_checked_rect(surface, &to_rect, dash_size, -1, dash_color, color);

	to_rect.x = from_rect->x + from_rect->w - thickness;
	to_rect.y = from_rect->y;
	to_rect.w = to_rect.x + thickness;
	to_rect.h = to_rect.y + from_rect->h;
	nqiv_fill_checked_rect(surface, &to_rect, -1, dash_size, dash_color, color);
}
