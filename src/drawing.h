#ifndef NQIV_DRAWING_H
#define NQIV_DRAWING_H

#include <SDL2/SDL.h>

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
