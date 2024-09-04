#ifndef NQIV_MONTAGE_H
#define NQIV_MONTAGE_H

#include <SDL2/SDL.h>

#include "image.h"

typedef struct nqiv_montage_dimensions
{
	int    window_width;
	int    window_height;
	double horizontal_margin;
	double vertical_margin;
	double column_space;
	double row_space;
	int    count_per_row;
	int    count;
} nqiv_montage_dimensions;

typedef struct nqiv_montage_positions
{
	int start;
	int selection;
	int end;
} nqiv_montage_positions;

typedef struct nqiv_montage_preload
{
	int ahead;
	int behind;
} nqiv_montage_preload;

typedef struct nqiv_montage_state
{
	nqiv_log_ctx*           logger;
	SDL_Window*             window;
	nqiv_image_manager*     images;
	nqiv_montage_dimensions dimensions;
	nqiv_montage_positions  positions;
	nqiv_montage_preload    preload;
	bool                    range_changed;
} nqiv_montage_state;

void nqiv_montage_calculate_dimensions(nqiv_montage_state* state);
void nqiv_montage_set_selection(nqiv_montage_state* state, const int idx);
void nqiv_montage_jump_selection(nqiv_montage_state* state, const int offset);
void nqiv_montage_next_selection(nqiv_montage_state* state);
void nqiv_montage_previous_selection(nqiv_montage_state* state);
void nqiv_montage_set_selection_row(nqiv_montage_state* state, const int idx);
void nqiv_montage_jump_selection_row(nqiv_montage_state* state, const int offset);
void nqiv_montage_next_selection_row(nqiv_montage_state* state);
void nqiv_montage_previous_selection_row(nqiv_montage_state* state);
void nqiv_montage_get_image_rect(nqiv_montage_state* state, const int idx, SDL_Rect* rect);
void nqiv_montage_next_selection_page(nqiv_montage_state* state);
void nqiv_montage_previous_selection_page(nqiv_montage_state* state);
void nqiv_montage_jump_selection_start(nqiv_montage_state* state);
void nqiv_montage_jump_selection_end(nqiv_montage_state* state);
int  nqiv_montage_find_index_at_point(nqiv_montage_state* state, const int x, const int y);

#endif /* NQIV_MONTAGE_H */
