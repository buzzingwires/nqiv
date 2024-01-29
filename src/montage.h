#ifndef NQIV_MONTAGE_H
#define NQIV_MONTAGE_H

#include <SDL.h>

#include "image.h"

typedef struct nqiv_montage_dimensions
{
	double horizontal_margin;
	double vertical_margin;
	double column_space;
	double row_space;
	int count_per_row;
	int count;
} nqiv_montage_dimensions;

typedef struct nqiv_montage_positions
{
	int start;
	int selection;
	int end;
} nqiv_montage_positions;

typedef struct nqiv_montage_state
{
	nqiv_log_ctx* logger;
	SDL_Window* window;
	nqiv_image_manager* images;
	nqiv_montage_dimensions dimensions;
	nqiv_montage_positions positions;
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

#endif /* NQIV_MONTAGE_H */
