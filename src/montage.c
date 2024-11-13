#include <assert.h>
#include <math.h>

#include <SDL2/SDL.h>

#include "image.h"
#include "montage.h"

/* TODO LOGGING AND CHECKS */

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

bool nqiv_montage_compare_range(const nqiv_montage_state* first, const nqiv_montage_state* second)
{
	return first->positions.start == second->positions.start
	       && first->positions.end == second->positions.end;
}

void nqiv_montage_set_selection(nqiv_montage_state* state, const int idx)
{
	int       new_idx = idx;
	const int images_len = nqiv_array_get_units_count(state->images->images);
	if(new_idx >= images_len) {
		new_idx = images_len - 1;
	}
	if(new_idx < 0) {
		new_idx = 0;
	}

	if(state->positions.start != state->positions.end && new_idx >= state->positions.start
	   && new_idx < state->positions.end
	   && (state->positions.end == images_len
	       || state->positions.end - state->positions.start == state->dimensions.count)) {
		state->positions.selection = new_idx;
		return;
	}

	nqiv_montage_state original = {0};
	memcpy(&original, state, sizeof(nqiv_montage_state));

	if(state->positions.selection > new_idx) {
		const int row = (new_idx / state->dimensions.count_per_row);
		const int row_index = state->dimensions.count_per_row * row;
		state->positions.start = row_index;
		state->positions.end = row_index + state->dimensions.count;
	} else {
		const int row = (new_idx / state->dimensions.count_per_row) + 1;
		const int row_index = state->dimensions.count_per_row * row;
		state->positions.start = row_index - state->dimensions.count;
		state->positions.end = row_index;
	}
	state->positions.selection = new_idx;

	if(state->positions.end > images_len) {
		state->positions.end = images_len;
	}
	if(state->positions.start < 0) {
		state->positions.start = 0;
	}

	const int range_length = state->positions.end - state->positions.start;
	if(range_length < state->dimensions.count) {
		const int end_diff = state->dimensions.count - range_length;
		const int final_diff = images_len - state->positions.end;
		state->positions.end += end_diff < final_diff ? end_diff : final_diff;
	}

	assert(state->positions.selection >= state->positions.start);
	assert(state->positions.selection < state->positions.end
	       || (state->positions.selection == 0 && state->positions.end == 0));

	nqiv_log_write(state->logger, NQIV_LOG_DEBUG, "Setting montage selection to %d.\n",
	               state->positions.selection);
	state->range_changed = state->range_changed || !nqiv_montage_compare_range(&original, state);
}

void nqiv_montage_calculate_dimensions(nqiv_montage_state* state, const int width, const int height)
{
	assert(state != NULL);
	assert(state->images != NULL);
	assert(state->logger != NULL);
	assert(width > 0);
	assert(height > 0);
	nqiv_log_write(state->logger, NQIV_LOG_DEBUG, "Calculating montage dimensions.\n");
	nqiv_montage_state original = {0};
	memcpy(&original, state, sizeof(nqiv_montage_state));
	state->dimensions.window_width = width;
	state->dimensions.window_height = height;
	const double width_ratio =
		(double)(state->images->thumbnail.size) / (double)(state->dimensions.window_width);
	const double height_ratio =
		(double)(state->images->thumbnail.size) / (double)(state->dimensions.window_height);
	int          count_per_column;
	const double row_leftover = nqiv_montage_calculate_axis(&count_per_column, height_ratio);
	const double column_leftover =
		nqiv_montage_calculate_axis(&state->dimensions.count_per_row, width_ratio);
	state->dimensions.count = count_per_column * state->dimensions.count_per_row;
	state->dimensions.row_space = row_leftover / ((double)count_per_column + 3.0);
	state->dimensions.column_space =
		column_leftover / ((double)state->dimensions.count_per_row + 3.0);
	state->dimensions.vertical_margin = state->dimensions.row_space * 2.0;
	state->dimensions.horizontal_margin = state->dimensions.column_space * 2.0;
	nqiv_montage_set_selection(state, state->positions.selection);
	state->range_changed = state->range_changed || !nqiv_montage_compare_range(&original, state);
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
	nqiv_montage_set_selection(state, state->positions.selection % state->dimensions.count_per_row
	                                      + idx * state->dimensions.count_per_row);
}

void nqiv_montage_jump_selection_row(nqiv_montage_state* state, const int offset)
{
	nqiv_montage_set_selection_row(
		state, state->positions.selection / state->dimensions.count_per_row + offset);
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
	nqiv_montage_set_selection(state, nqiv_array_get_last_idx(state->images->images));
}

void nqiv_montage_get_image_rect(nqiv_montage_state* state, const int idx, SDL_Rect* rect)
{
	assert(idx >= 0);
	assert(idx < nqiv_array_get_units_count(state->images->images));
	const int horizontal_margin_pixels =
		(int)(state->dimensions.horizontal_margin * (double)state->dimensions.window_width);
	const int vertical_margin_pixels =
		(int)(state->dimensions.vertical_margin * (double)state->dimensions.window_height);
	const int column_space_pixels =
		(int)(state->dimensions.column_space * (double)state->dimensions.window_width);
	const int row_space_pixels =
		(int)(state->dimensions.row_space * (double)state->dimensions.window_height);
	const int native_position = idx - state->positions.start;
	const int row = native_position / state->dimensions.count_per_row;
	const int column = native_position % state->dimensions.count_per_row;
	rect->x =
		horizontal_margin_pixels + (column_space_pixels + state->images->thumbnail.size) * column;
	rect->y = vertical_margin_pixels + (row_space_pixels + state->images->thumbnail.size) * row;
	rect->w = state->images->thumbnail.size;
	rect->h = state->images->thumbnail.size;
}

int nqiv_montage_find_index_at_point(nqiv_montage_state* state, const int x, const int y)
{
	SDL_Rect rect = {0};
	int      idx;
	for(idx = state->positions.start; idx < state->positions.end; ++idx) {
		nqiv_montage_get_image_rect(state, idx, &rect);
		if(x >= rect.x && x < rect.x + rect.w && y >= rect.y && y < rect.y + rect.h) {
			return idx;
		}
	}
	return -1;
}
