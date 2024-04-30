#include <assert.h>
#include <math.h>

#include <SDL2/SDL.h>

#include "image.h"
#include "montage.h"

/* TODO LOGGING AND CHECKS */

/*
typedef struct nqiv_montage_dimensions
{
	double horizontal_margin;
	double vertical_margin;
	double column_space;
	double row_space;
	int row_count;
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
*/

double nqiv_montage_calculate_axis(int* counter, const double ratio)
{
	*counter = 0;
	double fill = 0;
	fill += ratio;
	while(fill < 1.0) {
		*counter += 1;
		fill += ratio;
	}
	if(*counter == 0) {
		*counter = 1;
	}
	if(fill > 1.0) {
		fill -= ratio;
	}
	return 1.0 - fill;
}

void nqiv_montage_calculate_dimensions(nqiv_montage_state* state)
{
	nqiv_montage_state original = {0};
	memcpy( &original, state, sizeof(nqiv_montage_state) );
	nqiv_log_write(state->logger, NQIV_LOG_DEBUG, "Calculating montage dimensions.\n");
	assert(state != NULL);
	assert(state->images != NULL);
	assert(state->window != NULL);
	assert(state->logger != NULL);
	SDL_GetWindowSizeInPixels(state->window, &state->dimensions.window_width, &state->dimensions.window_height);
	/*
	double horizontal_margin;
	double vertical_margin;
	double column_space;
	double row_space;
	*/
	const double width_ratio = (double)(state->images->thumbnail.size) / (double)(state->dimensions.window_width);
	const double height_ratio = (double)(state->images->thumbnail.size) / (double)(state->dimensions.window_height);
	int count_per_column;
	const double row_leftover = nqiv_montage_calculate_axis(&count_per_column, height_ratio);
	const double column_leftover = nqiv_montage_calculate_axis(&state->dimensions.count_per_row, width_ratio);
	state->dimensions.count = count_per_column * state->dimensions.count_per_row;
	state->positions.end = state->positions.start + state->dimensions.count;
	const int images_len = state->images->images->position / sizeof(nqiv_image*);
	if(state->positions.end > images_len) {
		state->positions.end = images_len;
	}
	if(state->positions.selection < 0) {
		state->positions.selection = 0;
	}
	if(state->positions.selection >= state->positions.end) {
		state->positions.selection = state->positions.end - 1;
	}
	state->dimensions.row_space = row_leftover / ( (double)count_per_column + 3.0 );
	state->dimensions.column_space = column_leftover / ( (double)state->dimensions.count_per_row + 3.0 );
	state->dimensions.vertical_margin = state->dimensions.row_space * 2.0;
	state->dimensions.horizontal_margin = state->dimensions.column_space * 2.0;
	original.positions.selection = state->positions.selection;
	state->range_changed = state->range_changed || memcmp( &original, state, sizeof(nqiv_montage_state) ) != 0;
}

void nqiv_montage_set_selection(nqiv_montage_state* state, const int idx)
{
	nqiv_montage_state original = {0};
	memcpy( &original, state, sizeof(nqiv_montage_state) );
	int new_idx = idx;
	const int images_len = state->images->images->position / sizeof(nqiv_image*);
	if(new_idx >= images_len) {
		new_idx = images_len - 1;
	}
	if(new_idx < 0) {
		new_idx = 0;
	}
	if(new_idx == state->positions.selection) {
		original.positions.selection = state->positions.selection;
		state->range_changed = state->range_changed || memcmp( &original, state, sizeof(nqiv_montage_state) ) != 0;
		return;
	}
	if(new_idx >= state->positions.start && new_idx < state->positions.end) {
		state->positions.selection = new_idx;
		original.positions.selection = state->positions.selection;
		state->range_changed = state->range_changed || memcmp( &original, state, sizeof(nqiv_montage_state) ) != 0;
		return;
	}
	const int page = new_idx / state->dimensions.count;
	const int page_start = state->dimensions.count * page;

	state->positions.start = page_start;
	state->positions.selection = new_idx;
	state->positions.end = state->positions.start + state->dimensions.count;
	if(state->positions.start < 0) {
		state->positions.start = 0;
	}
	if(state->positions.end >= images_len) {
		state->positions.end = images_len - 1;
	}
	nqiv_log_write(state->logger, NQIV_LOG_DEBUG, "Setting montage selection to %d.\n", state->positions.selection);
	original.positions.selection = state->positions.selection;
	state->range_changed = state->range_changed || memcmp( &original, state, sizeof(nqiv_montage_state) ) != 0;
}

void nqiv_montage_jump_selection(nqiv_montage_state* state, const int offset)
{
	nqiv_montage_set_selection(state, state->positions.selection + offset);
}

void nqiv_montage_next_selection(nqiv_montage_state* state)
{
	nqiv_montage_jump_selection(state, 1);
}

void nqiv_montage_previous_selection(nqiv_montage_state* state)
{
	nqiv_montage_jump_selection(state, -1);
}

void nqiv_montage_set_selection_row(nqiv_montage_state* state, const int idx)
{
	nqiv_montage_set_selection(state, state->positions.selection % state->dimensions.count_per_row + idx * state->dimensions.count_per_row);
}

void nqiv_montage_jump_selection_row(nqiv_montage_state* state, const int offset)
{
	nqiv_montage_set_selection_row(state, state->positions.selection / state->dimensions.count_per_row + offset);
}

void nqiv_montage_next_selection_row(nqiv_montage_state* state)
{
	nqiv_montage_jump_selection_row(state, 1);
}

void nqiv_montage_previous_selection_row(nqiv_montage_state* state)
{
	nqiv_montage_jump_selection_row(state, -1);
}

void nqiv_montage_next_selection_page(nqiv_montage_state* state)
{
	nqiv_montage_jump_selection(state, state->dimensions.count);
}

void nqiv_montage_previous_selection_page(nqiv_montage_state* state)
{
	nqiv_montage_jump_selection(state, -state->dimensions.count);
}

void nqiv_montage_jump_selection_start(nqiv_montage_state* state)
{
	nqiv_montage_set_selection(state, 0);
}

void nqiv_montage_jump_selection_end(nqiv_montage_state* state)
{
	nqiv_montage_set_selection(state, state->images->images->position / sizeof(nqiv_image*) - 1);
}

void nqiv_montage_get_image_rect(nqiv_montage_state* state, const int idx, SDL_Rect* rect)
{
	assert(idx >= 0);
	assert( idx < state->images->images->position / (int)sizeof(nqiv_image*) );
	/*
	if(idx < 0) {
		return;
	}
	if(idx >= images_len) {
		return;
	}
	*/
	const int horizontal_margin_pixels = (int)(state->dimensions.horizontal_margin * (double)state->dimensions.window_width);
	const int vertical_margin_pixels = (int)(state->dimensions.vertical_margin * (double)state->dimensions.window_height);
	const int column_space_pixels = (int)(state->dimensions.column_space * (double)state->dimensions.window_width);
	const int row_space_pixels = (int)(state->dimensions.row_space * (double)state->dimensions.window_height);
	const int native_position = idx - state->positions.start;
	const int row = native_position / state->dimensions.count_per_row;
	const int column = native_position % state->dimensions.count_per_row;
	rect->x = horizontal_margin_pixels + (column_space_pixels + state->images->thumbnail.size) * column;
	rect->y = vertical_margin_pixels + (row_space_pixels + state->images->thumbnail.size) * row;
	rect->w = state->images->thumbnail.size;
	rect->h = state->images->thumbnail.size;
}
