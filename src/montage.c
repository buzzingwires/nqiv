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

void nqiv_montage_calculate_axis(int* counter, const double margin, const double space, const double ratio)
{
	*counter = 0;
	double fill = 0;
	fill += margin;
	fill += margin;
	fill += ratio;
	while(fill < 1.0) {
		*counter += 1;
		fill += space;
		fill += ratio;
	}
}

void nqiv_montage_calculate_dimensions(nqiv_montage_state* state)
{
	nqiv_log_write(state->logger, NQIV_LOG_DEBUG, "Caculating montage dimensions.\n");
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
	const double width_ratio = (double)(state->images->thumbnail.width) / (double)(state->dimensions.window_width);
	const double height_ratio = (double)(state->images->thumbnail.height) / (double)(state->dimensions.window_height);
	const double width_times = (double)(state->dimensions.window_width) / (double)(state->images->thumbnail.width);
	const double height_times = (double)(state->dimensions.window_height) / (double)(state->images->thumbnail.height);
	state->dimensions.horizontal_margin = ( width_times - floor(width_times) ) * 0.1;
	state->dimensions.vertical_margin = ( height_times - floor(height_times) ) * 0.1;
	state->dimensions.row_space = state->dimensions.vertical_margin * 0.5;
	state->dimensions.column_space = state->dimensions.horizontal_margin * 0.5;
	int count_per_column;
	nqiv_montage_calculate_axis(&count_per_column, state->dimensions.vertical_margin, state->dimensions.row_space, height_ratio);
	nqiv_montage_calculate_axis(&state->dimensions.count_per_row, state->dimensions.horizontal_margin, state->dimensions.column_space, width_ratio);
	state->dimensions.count = count_per_column * state->dimensions.count_per_row;
	state->positions.end = state->positions.start + state->dimensions.count;
	const int images_len = state->images->images->position / sizeof(nqiv_image*);
	if(state->positions.end > images_len) {
		state->positions.end = images_len;
	}
	if(state->positions.selection >= state->positions.end) {
		state->positions.selection = state->positions.end - 1;
	}
	state->dimensions.horizontal_margin *= 0.5;
	state->dimensions.vertical_margin *= 0.5;
}

void nqiv_montage_set_selection(nqiv_montage_state* state, const int idx)
{
	int new_idx = idx;
	const int images_len = state->images->images->position / sizeof(nqiv_image*);
	if(new_idx >= images_len) {
		new_idx = images_len - 1;
	}
	if(new_idx < 0) {
		new_idx = 0;
	}
	if(new_idx == state->positions.selection) {
		return;
	}
	state->positions.selection = new_idx;
	if(new_idx < state->positions.start) {
		state->positions.end -= (state->positions.start - new_idx);
		state->positions.start = new_idx;
	} else if(new_idx >= state->positions.end) {
		state->positions.start += (new_idx - state->positions.end);
		state->positions.end = new_idx;
	}
	nqiv_log_write(state->logger, NQIV_LOG_DEBUG, "Setting montage selection to %d.\n", state->positions.selection);
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
	const int images_len = state->images->images->position / sizeof(nqiv_image*);
	assert(idx >= 0);
	assert(idx < images_len);
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
	rect->x = vertical_margin_pixels + (column_space_pixels + state->images->thumbnail.width) * column;
	rect->y = horizontal_margin_pixels + (row_space_pixels + state->images->thumbnail.height) * row;
	rect->w = state->images->thumbnail.width;
	rect->h = state->images->thumbnail.height;
}
