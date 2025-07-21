#include <assert.h>
#include <math.h>

#include <SDL2/SDL.h>

#include "image.h"
#include "montage.h"

/* TODO LOGGING AND CHECKS */

static double nqiv_montage_calculate_axis(int* counter, const double ratio)
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

static bool nqiv_montage_compare_range(const nqiv_montage_state* first,
                                       const nqiv_montage_state* second)
{
	return first->positions.start == second->positions.start
	       && first->positions.end == second->positions.end;
}

void nqiv_montage_set_selection(nqiv_montage_state* state, const int idx)
{
	/* Clamp the new index to a sane value. */
	int       new_idx = idx;
	const int images_len = nqiv_array_get_units_count(state->images->images);
	if(new_idx >= images_len) {
		new_idx = images_len - 1;
	}
	if(new_idx < 0) {
		new_idx = 0;
	}

	int       range_length = state->positions.end - state->positions.start;
	const int whole_rows = range_length / state->dimensions.count_per_row;
	/* If there is a partial row in the given range and its length length matches that of the
	 * partial row for the whole montage.
	 */
	const int partial_row_length = range_length % state->dimensions.count_per_row;
	const int partial_row = (partial_row_length != 0
	                         && partial_row_length == images_len % state->dimensions.count_per_row)
	                            ? 1
	                            : 0;
	const int row_count = state->dimensions.count / state->dimensions.count_per_row;

	/* If we have changed the selection or the row count matches the current rows (as calculated
	 * above), but not gone outside the current montage range, and that montage range has been
	 * sanely set to include a whole page (or less if we're at the end of the montage where
	 * incomplete rows may appear), set the new selection and skip range calculations. */
	if((state->positions.selection != new_idx || whole_rows + partial_row == row_count)
	   && state->positions.start != state->positions.end && new_idx >= state->positions.start
	   && new_idx < state->positions.end
	   && ((state->positions.end == images_len && range_length <= state->dimensions.count)
	       || range_length == state->dimensions.count)) {
		state->positions.selection = new_idx;
		return;
	}

	/* Track the current state. We will compare to this after updating and set a flag if it has
	 * changed. */
	nqiv_montage_state original = {0};
	memcpy(&original, state, sizeof(nqiv_montage_state));

	int row = (new_idx / state->dimensions.count_per_row);
	int row_index = state->dimensions.count_per_row * row;
	if(state->positions.selection > new_idx) {
		/* If new selection is behind the old one, move backwards so the page starts at the
		 * beginning of the new selection's row. */
		state->positions.start = row_index;
		state->positions.end = row_index + state->dimensions.count;
	} else if(state->positions.selection < new_idx) {
		/* If new selection is after the old one, move forward so the page ends after the new
		 * selection's row. */
		row += 1;
		row_index = state->dimensions.count_per_row * row;
		state->positions.start = row_index - state->dimensions.count;
		state->positions.end = row_index;
	} else if(range_length != state->dimensions.count) {
		/* On size changes, recalculate the range of the current selection's page. */
		const int page = new_idx / state->dimensions.count;
		const int page_start = (state->dimensions.count * page);
		state->positions.start = page_start;
		state->positions.end = page_start + state->dimensions.count;
	}
	state->positions.selection = new_idx;

	/* Cap the end of the page to the end of the montage. */
	if(state->positions.end > images_len) {
		state->positions.end = images_len;
	}

	/* At the end of the montage, it is possible for the given page to not fill the range (as
	 * implied above). Fill it in with as many preceding complete rows that will fit. We don't cram
	 * as many images as possible since that will throw off the composition of each row. */
	range_length = state->positions.end - state->positions.start;
	const int range_length_diff = state->dimensions.count - range_length;
	const int addable_rows = range_length_diff / state->dimensions.count_per_row;
	state->positions.start -= addable_rows * state->dimensions.count_per_row;

	/* Cap the start of the page to the start of the montage. */
	if(state->positions.start < 0) {
		state->positions.start = 0;
	}

	range_length = state->positions.end - state->positions.start;
	assert(state->positions.selection >= state->positions.start);
	assert(state->positions.selection < state->positions.end
	       || (state->positions.selection == 0 && state->positions.end == 0));
	assert(state->positions.end > state->positions.start
	       || (state->positions.selection == 0 && state->positions.end == 0));
	assert(range_length <= state->dimensions.count);

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
	const int    thumbnail_size = SDL_AtomicGet(&state->images->thumbnail.size);
	const double raw_width_ratio =
		(double)(thumbnail_size) / (double)(state->dimensions.window_width);
	const double raw_height_ratio =
		(double)(thumbnail_size) / (double)(state->dimensions.window_height);
	const double width_ratio = raw_width_ratio >= 1.0 ? 1.0 : raw_width_ratio;
	const double height_ratio = raw_height_ratio >= 1.0 ? 1.0 : raw_height_ratio;
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

static void nqiv_montage_jump_selection(nqiv_montage_state* state, const int offset)
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

static void nqiv_montage_set_selection_row(nqiv_montage_state* state, const int idx)
{
	nqiv_montage_set_selection(state, state->positions.selection % state->dimensions.count_per_row
	                                      + idx * state->dimensions.count_per_row);
}

static void nqiv_montage_jump_selection_row(nqiv_montage_state* state, const int offset)
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

static int nqiv_montage_scan_marked(nqiv_montage_state* state, const int start, const int increment)
{
	nqiv_image** images = state->images->images->data;
	int          idx;
	for(idx = start; idx >= 0 && idx < nqiv_array_get_units_count(state->images->images);
	    idx += increment) {
		if(idx != start && images[idx]->marked) {
			return idx;
		}
	}
	return -1;
}

static void nqiv_montage_set_existing_selection(nqiv_montage_state* state, const int idx)
{
	if(idx >= 0) {
		nqiv_montage_set_selection(state, idx);
	}
}

void nqiv_montage_previous_marked_selection(nqiv_montage_state* state)
{
	nqiv_montage_set_existing_selection(
		state, nqiv_montage_scan_marked(state, state->positions.selection, -1));
}

void nqiv_montage_next_marked_selection(nqiv_montage_state* state)
{
	nqiv_montage_set_existing_selection(
		state, nqiv_montage_scan_marked(state, state->positions.selection, 1));
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
	const int thumbnail_size = SDL_AtomicGet(&state->images->thumbnail.size);
	rect->x = horizontal_margin_pixels + (column_space_pixels + thumbnail_size) * column;
	rect->y = vertical_margin_pixels + (row_space_pixels + thumbnail_size) * row;
	rect->w = thumbnail_size;
	rect->h = thumbnail_size;
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
