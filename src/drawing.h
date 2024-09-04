#ifndef NQIV_DRAWING_H
#define NQIV_DRAWING_H

#include <SDL2/SDL.h>

/*
void set_pixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
  Uint32 * const target_pixel = (Uint32 *) ((Uint8 *) surface->pixels
                                             + y * surface->pitch
                                             + x * surface->format->BytesPerPixel);
  *target_pixel = pixel;
}
*/

void nqiv_fill_checked_rect(SDL_Surface*     surface,
                            const SDL_Rect*  rect,
                            const int        x_check_size,
                            const int        y_check_size,
                            const SDL_Color* color_one,
                            const SDL_Color* color_two);
void nqiv_draw_rect(SDL_Surface*     surface,
                    const SDL_Rect*  from_rect,
                    const int        dash_size,
                    const SDL_Color* color,
                    const SDL_Color* dash_color,
                    const int        thickness);

#endif /* NQIV_DRAWING_H */
