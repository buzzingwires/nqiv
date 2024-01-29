#ifndef NQIV_DRAWING_H
#define NQIV_DRAWING_H

/*
void set_pixel(SDL_Surface *surface, int x, int y, Uint32 pixel)
{
  Uint32 * const target_pixel = (Uint32 *) ((Uint8 *) surface->pixels
                                             + y * surface->pitch
                                             + x * surface->format->BytesPerPixel);
  *target_pixel = pixel;
}
*/

void nqiv_fill_rect(SDL_Surface* surface, const SDL_Rect* rect, const SDL_Color* color);
void nqiv_draw_rect(SDL_Surface* surface, const SDL_Rect* from_rect, const SDL_Color* color, const int thickness);
void nqiv_draw_alpha_background(SDL_Surface* surface, SDL_Rect* rect, const int size, const SDL_Color* color_one, const SDL_Color* color_two);

#endif /* NQIV_DRAWING_H */
