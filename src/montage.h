#ifndef NQIV_MONTAGE_H
#define NQIV_MONTAGE_H

#include <SDL2/SDL.h>

#include "image.h"

/*
 * 'Montage' mode represents images in the form of thumbnails. This module paginates and determines
 * their placement based on their size and the size of the screen
 */

typedef struct nqiv_montage_dimensions
{
	int    window_width;
	int    window_height;
	double horizontal_margin;
	double vertical_margin;
	double column_space;
	double row_space;
	int    count_per_row;
	/* Number of thumbnails per 'page' */
	int count;
} nqiv_montage_dimensions;

typedef struct nqiv_montage_positions
{
	/* Image indexes for start, end, and current selection in the montage. */
	int start;
	int selection;
	int end;
} nqiv_montage_positions;

typedef struct nqiv_montage_preload
{
	/* Given nqiv_montage_positions, how many images ahead or behind of end or start to load? */
	int ahead;
	int behind;
} nqiv_montage_preload;

typedef struct nqiv_montage_state
{
	nqiv_log_ctx*           logger;
	nqiv_image_manager*     images;
	nqiv_montage_dimensions dimensions;
	nqiv_montage_positions  positions;
	nqiv_montage_preload    preload;
	/* Are we viewing a different set/page of thumbnails now?
	 * Used to set loading 'transaction group' to discard queued out of sight thumbnails.
	 */
	bool range_changed;
} nqiv_montage_state;

/* Update montage to new dimensions. */
void nqiv_montage_calculate_dimensions(nqiv_montage_state* state,
                                       const int           width,
                                       const int           height);
/* Set operations set to a specific value, jump operations set to an offset from the current value,
 * next and previous respectively jump at an offset of 1 and -1 */
void nqiv_montage_set_selection(nqiv_montage_state* state, const int idx);
void nqiv_montage_jump_selection(nqiv_montage_state* state, const int offset);
void nqiv_montage_next_selection(nqiv_montage_state* state);
void nqiv_montage_previous_selection(nqiv_montage_state* state);
void nqiv_montage_set_selection_row(nqiv_montage_state* state, const int idx);
void nqiv_montage_jump_selection_row(nqiv_montage_state* state, const int offset);
void nqiv_montage_next_selection_row(nqiv_montage_state* state);
void nqiv_montage_previous_selection_row(nqiv_montage_state* state);
/* Get rect for particular thumbnail to be displayed in. */
void nqiv_montage_get_image_rect(nqiv_montage_state* state, const int idx, SDL_Rect* rect);
void nqiv_montage_next_selection_page(nqiv_montage_state* state);
void nqiv_montage_previous_selection_page(nqiv_montage_state* state);
void nqiv_montage_jump_selection_start(nqiv_montage_state* state);
void nqiv_montage_jump_selection_end(nqiv_montage_state* state);

/* Get image index of thumbnail at particular X/Y coordinates. */
int nqiv_montage_find_index_at_point(nqiv_montage_state* state, const int x, const int y);

#endif /* NQIV_MONTAGE_H */
