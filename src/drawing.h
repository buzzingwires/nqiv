#ifndef NQIV_DRAWING_H
#define NQIV_DRAWING_H

#include <SDL2/SDL.h>

/* Fill a rect with checkers of color_one and color_two (these can be the same), alternating when
 * the count of pixels on an axis meets its respective check size (use -1 to never switch checks).
 */
void nqiv_fill_checked_rect(SDL_Surface*     surface,
                            const SDL_Rect*  rect,
                            const int        x_check_size,
                            const int        y_check_size,
                            const SDL_Color* color_one,
                            const SDL_Color* color_two);
/* Draw an unfilled rect, potentially with dashed lines. -1 dash_size or matching colors for no
 * checkers. */
void nqiv_draw_rect(SDL_Surface*     surface,
                    const SDL_Rect*  from_rect,
                    const int        dash_size,
                    const SDL_Color* color,
                    const SDL_Color* dash_color,
                    const int        thickness);

#endif /* NQIV_DRAWING_H */
