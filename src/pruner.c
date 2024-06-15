#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "logging.h"
#include "array.h"
#include "event.h"
#include "montage.h"
#include "image.h"
#include "cmd.h"
#include "pruner.h"

void nqiv_pruner_destroy(nqiv_pruner* pruner)
{
	if(pruner->pruners != NULL) {
		nqiv_array_destroy(pruner->pruners);
	}
	memset( pruner, 0, sizeof(nqiv_pruner) );
}

bool nqiv_pruner_init(nqiv_pruner* pruner, nqiv_log_ctx* logger, const int queue_length)
{
	nqiv_array* new_pruners = nqiv_array_create(queue_length);
	if(new_pruners == NULL) {
		nqiv_log_write(logger, NQIV_LOG_ERROR, "Failed to create queue of length %d for pruners array.\n", queue_length);
		return false;
	}
	nqiv_pruner_destroy(pruner);
	pruner->logger = logger;
	pruner->pruners = new_pruners;
	return true;
}

/*
typedef union nqiv_pruner_desc_datapoint_content
{
	int as_int;
	int as_int_pair[2];
	bool as_bool;
} nqiv_pruner_desc_datapoint_content;

typedef struct nqiv_pruner_desc_datapoint
{
	bool active;
	nqiv_pruner_desc_datapoint_content condition;
	nqiv_pruner_desc_datapoint_content value;
} nqiv_pruner_desc_datapoint;

	nqiv_pruner_desc_datapoint loaded_self;
	nqiv_pruner_desc_datapoint loaded_ahead;
	nqiv_pruner_desc_datapoint loaded_behind;
	nqiv_pruner_desc_datapoint bytes_ahead;
	nqiv_pruner_desc_datapoint bytes_behind;
*/

void nqiv_pruner_update_state_boolean(nqiv_pruner* pruner, const bool value)
{
	if(!pruner->state.and_is_set) {
		pruner->state.and_result = true;
	}
	pruner->state.and_is_set = true;
	pruner->state.or_result = pruner->state.or_result || value;
	pruner->state.and_result = pruner->state.and_result && value;
	nqiv_log_write(pruner->logger, NQIV_LOG_DEBUG, "New prune state boolean is or_result: %s and_result: %s\n", pruner->state.or_result ? "true" : "false", pruner->state.and_result ? "true" : "false");
}

void nqiv_pruner_update_state_integer(nqiv_pruner* pruner, const int value)
{
	pruner->state.total_sum += value;
	nqiv_log_write(pruner->logger, NQIV_LOG_DEBUG, "New prune state total_sum is %d\n", pruner->state.total_sum);
}

void nqiv_pruner_run_not_animated(nqiv_pruner* pruner, nqiv_pruner_desc_datapoint* datapoint, const void* object)
{
	if(datapoint->active) {
		const bool result = !( ( (nqiv_image_form*)object )->animation.exists );
		datapoint->value.as_bool = result;
		nqiv_log_write(pruner->logger, NQIV_LOG_DEBUG, "not_animated result is: %s\n", result ? "true" : "false");
		nqiv_pruner_update_state_boolean(pruner, result);
	}
}

void nqiv_pruner_run_loaded_self(nqiv_pruner* pruner, nqiv_pruner_desc_datapoint* datapoint, const void* object)
{
	if(datapoint->active) {
		const bool result = object != NULL;
		datapoint->value.as_bool = result;
		nqiv_log_write(pruner->logger, NQIV_LOG_DEBUG, "loaded_self result is: %s\n", result ? "true" : "false");
		nqiv_pruner_update_state_boolean(pruner, result);
	}
}

void nqiv_pruner_loaded_count_body(nqiv_pruner* pruner, nqiv_pruner_desc_datapoint* datapoint, const int increment)
{
	datapoint->value.as_int += increment;
	const bool result = datapoint->value.as_int > datapoint->condition.as_int_pair[1];
	nqiv_log_write(pruner->logger, NQIV_LOG_DEBUG, "count_body result is %s new datapoint value %d compared to %d\n", result ? "true" : "false", datapoint->value.as_int, datapoint->condition.as_int_pair[1]);
	nqiv_pruner_update_state_boolean(pruner, result);
	nqiv_pruner_update_state_integer(pruner, increment);
}

void nqiv_pruner_run_loaded_ahead(nqiv_pruner* pruner, nqiv_pruner_desc_datapoint* datapoint, const void* object)
{
	nqiv_log_write(pruner->logger, NQIV_LOG_DEBUG, "Prune checking loaded_ahead\n");
	if(datapoint->active && object != NULL && pruner->state.idx - pruner->state.montage_end > datapoint->condition.as_int_pair[0]) {
		nqiv_pruner_loaded_count_body(pruner, datapoint, 1);
	}
}

void nqiv_pruner_run_loaded_behind(nqiv_pruner* pruner, nqiv_pruner_desc_datapoint* datapoint, const void* object)
{
	nqiv_log_write(pruner->logger, NQIV_LOG_DEBUG, "Prune checking loaded_behind\n");
	if(datapoint->active && object != NULL && pruner->state.montage_start - pruner->state.idx > datapoint->condition.as_int_pair[0]) {
		nqiv_pruner_loaded_count_body(pruner, datapoint, 1);
	}
}

void nqiv_pruner_run_bytes_ahead(nqiv_pruner* pruner, nqiv_pruner_desc_datapoint* datapoint, const void* object, const int size)
{
	nqiv_log_write(pruner->logger, NQIV_LOG_DEBUG, "Prune checking bytes_ahead\n");
	if(datapoint->active && object != NULL && pruner->state.idx - pruner->state.montage_end > datapoint->condition.as_int_pair[0]) {
		nqiv_pruner_loaded_count_body(pruner, datapoint, size);
	}
}

void nqiv_pruner_run_bytes_behind(nqiv_pruner* pruner, nqiv_pruner_desc_datapoint* datapoint, const void* object, const int size)
{
	nqiv_log_write(pruner->logger, NQIV_LOG_DEBUG, "Prune checking bytes_behind\n");
	if(datapoint->active && object != NULL && pruner->state.montage_start - pruner->state.idx > datapoint->condition.as_int_pair[0]) {
		nqiv_pruner_loaded_count_body(pruner, datapoint, size);
	}
}

void nqiv_pruner_run_set(nqiv_pruner* pruner, nqiv_pruner_desc_dataset* set, const nqiv_image_form* form, const void* object, const int size)
{
	nqiv_pruner_run_not_animated(pruner, &(set->not_animated), form);
	nqiv_pruner_run_loaded_self(pruner, &(set->loaded_self), object);
	nqiv_pruner_run_loaded_ahead(pruner, &(set->loaded_ahead), object);
	nqiv_pruner_run_loaded_behind(pruner, &(set->loaded_behind), object);
	nqiv_pruner_run_bytes_ahead(pruner, &(set->bytes_ahead), object, size);
	nqiv_pruner_run_bytes_behind(pruner, &(set->bytes_behind), object, size);
}

void nqiv_pruner_run_desc(nqiv_pruner* pruner, nqiv_pruner_desc* desc, const nqiv_image* image)
{
	/* CHeck loaded self, pruner, datapoint, void ptr */
	/* CHeck loaded ahead, pruner, datapoint ( param 1 (point to start counting), param 2 (max count) ), void ptr */
	/* CHeck loaded ahead, pruner, datapoint ( param 1 (point to start counting), param 2 (max count) ), void ptr */
	/* CHeck bytes ahead, pruner, form (param), datapoint ( param 1 (point to start counting), param 2 (max count) ), void ptr */
	/* CHeck bytes behind, pruner, form (param), datapoint ( param 1 (point to start counting), param 2 (max count) ), void ptr */
	/*
	if(dataset->loaded_self.active) {
		if(dataset->loaded_self)
	}*/
	nqiv_log_write(pruner->logger, NQIV_LOG_DEBUG, "Prune checking vips_set\n");
	nqiv_pruner_run_set(pruner, &(desc->vips_set), &image->image, image->image.vips, image->image.effective_width * image->image.effective_height * 4);
	nqiv_log_write(pruner->logger, NQIV_LOG_DEBUG, "Prune checking raw_set\n");
	nqiv_pruner_run_set(pruner, &(desc->raw_set), &image->image,  image->image.data, image->image.effective_width * image->image.effective_height * 4);
	nqiv_log_write(pruner->logger, NQIV_LOG_DEBUG, "Prune checking surface_set\n");
	nqiv_pruner_run_set(pruner, &(desc->surface_set), &image->image,  image->image.surface, image->image.effective_width * image->image.effective_height * 4);
	nqiv_log_write(pruner->logger, NQIV_LOG_DEBUG, "Prune checking texture_set\n");
	nqiv_pruner_run_set(pruner, &(desc->texture_set), &image->image,  image->image.texture, image->image.effective_width * image->image.effective_height * 4);
	nqiv_log_write(pruner->logger, NQIV_LOG_DEBUG, "Prune checking thumbnail_vips_set\n");
	nqiv_pruner_run_set(pruner, &(desc->thumbnail_vips_set), &image->thumbnail, image->thumbnail.vips, image->thumbnail.effective_width * image->thumbnail.effective_height * 4);
	nqiv_log_write(pruner->logger, NQIV_LOG_DEBUG, "Prune checking thumbnail_raw_set\n");
	nqiv_pruner_run_set(pruner, &(desc->thumbnail_raw_set), &image->thumbnail, image->thumbnail.data, image->thumbnail.effective_width * image->thumbnail.effective_height * 4);
	nqiv_log_write(pruner->logger, NQIV_LOG_DEBUG, "Prune checking thumbnail_surface_set\n");
	nqiv_pruner_run_set(pruner, &(desc->thumbnail_surface_set), &image->thumbnail, image->thumbnail.surface, image->thumbnail.effective_width * image->thumbnail.effective_height * 4);
	nqiv_log_write(pruner->logger, NQIV_LOG_DEBUG, "Prune checking thumbnail_texture_set\n");
	nqiv_pruner_run_set(pruner, &(desc->thumbnail_texture_set), &image->thumbnail, image->thumbnail.texture, image->thumbnail.effective_width * image->thumbnail.effective_height * 4);
}

int nqiv_pruner_run_image(nqiv_pruner* pruner, nqiv_montage_state* montage, nqiv_priority_queue* thread_queue, const int iidx, nqiv_image* image)
{
	nqiv_log_write( pruner->logger, NQIV_LOG_DEBUG, "Locking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
	if( !omp_test_lock(&image->lock) ) {
		nqiv_log_write( pruner->logger, NQIV_LOG_DEBUG, "Failed to lock image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
		return 0;
	}
	nqiv_log_write( pruner->logger, NQIV_LOG_DEBUG, "Locked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
	if( !nqiv_image_has_loaded_form(image) ) {
		nqiv_log_write( pruner->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
		omp_unset_lock(&image->lock);
		nqiv_log_write( pruner->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
		return 0;
	}
	int prune_count = 0;
	int idx;
	const int num_descs = pruner->pruners->position / sizeof(nqiv_pruner_desc);
	for(idx = 0; idx < num_descs; ++idx) {
		nqiv_pruner_desc desc;
		if( nqiv_array_get_bytes(pruner->pruners, idx, sizeof(nqiv_pruner_desc), &desc) ) {
			pruner->state.idx = iidx;
			pruner->state.selection = montage->positions.selection;
			const int raw_start_idx = montage->positions.start - montage->preload.behind;
			pruner->state.montage_start = raw_start_idx >= 0 ? raw_start_idx : 0;
			pruner->state.montage_end = montage->positions.end + montage->preload.ahead;
			pruner->state.or_result = false;
			pruner->state.and_result = false;
			pruner->state.and_is_set = false;
			nqiv_log_write(pruner->logger, NQIV_LOG_DEBUG, "Checking prune directive %d.\n", idx);
			nqiv_pruner_run_desc(pruner, &desc, image);
			if( (desc.counter & NQIV_PRUNER_COUNT_OP_SUM && pruner->state.total_sum > desc.state_check.total_sum) ||
				(desc.counter & NQIV_PRUNER_COUNT_OP_OR && pruner->state.or_result == desc.state_check.or_result) ||
				(pruner->state.and_is_set && desc.counter & NQIV_PRUNER_COUNT_OP_AND && pruner->state.and_result == desc.state_check.and_result) ) {
				nqiv_log_write(pruner->logger, NQIV_LOG_DEBUG, "Pruning image %s.\n", image->image.path);
				bool send_event = false;
				/* TODO: This cannot be called from a thread. If we plan to have the pruner run in a thread, we need to do this in a thread safe way, probably by sending an SDL event for master. */
				if( desc.unload_texture && (image->image.texture != NULL || image->image.fallback_texture != NULL) ) {
					nqiv_unload_image_form_texture(&image->image);
					nqiv_unload_image_form_fallback_texture(&image->image);
					nqiv_unload_image_form_texture(&image->image);
					assert(image->image.texture == NULL);
					assert(image->image.fallback_texture == NULL);
					send_event = true;
				}
				if( desc.unload_thumbnail_texture && (image->thumbnail.texture != NULL || image->thumbnail.fallback_texture != NULL) ) {
					nqiv_unload_image_form_texture(&image->thumbnail);
					nqiv_unload_image_form_fallback_texture(&image->thumbnail);
					nqiv_unload_image_form_texture(&image->thumbnail);
					assert(image->thumbnail.texture == NULL);
					assert(image->thumbnail.fallback_texture == NULL);
					send_event = true;
				}
				nqiv_event event = {0};
				event.type = NQIV_EVENT_IMAGE_LOAD;
				event.options.image_load.image = image;
				event.options.image_load.image_options.unload = true;
				event.options.image_load.image_options.vips = desc.unload_vips && image->image.vips != NULL;
				event.options.image_load.image_options.raw = desc.unload_raw && image->image.data != NULL;
				event.options.image_load.image_options.surface = desc.unload_surface && image->image.surface != NULL;
				event.options.image_load.thumbnail_options.unload = true;
				event.options.image_load.thumbnail_options.vips = desc.unload_thumbnail_vips && image->thumbnail.vips != NULL;
				event.options.image_load.thumbnail_options.raw = desc.unload_thumbnail_raw && image->thumbnail.data != NULL;
				event.options.image_load.thumbnail_options.surface = desc.unload_thumbnail_surface && image->thumbnail.surface != NULL;
				send_event = send_event ||
							 event.options.image_load.image_options.vips ||
							 event.options.image_load.image_options.raw ||
							 event.options.image_load.image_options.surface ||
							 event.options.image_load.thumbnail_options.vips ||
							 event.options.image_load.thumbnail_options.raw ||
							 event.options.image_load.thumbnail_options.surface;
				if(send_event) {
					prune_count += 1;
				}
				nqiv_log_write(pruner->logger, NQIV_LOG_INFO, "%sending prune event for image %d desc %d/%d.\n", send_event ? "S" : "Not s", iidx, idx, num_descs);
				if(send_event) {
					event.transaction_group = -1;
					if( !nqiv_priority_queue_push(thread_queue, 1, sizeof(nqiv_event), &event) ) {
						nqiv_log_write( pruner->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
						omp_unset_lock(&image->lock);
						nqiv_log_write( pruner->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
						return false;
					}
				}
			}
		}
	}
	nqiv_log_write( pruner->logger, NQIV_LOG_DEBUG, "Unlocking image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
	omp_unset_lock(&image->lock);
	nqiv_log_write( pruner->logger, NQIV_LOG_DEBUG, "Unlocked image %s, from thread %d.\n", image->image.path, omp_get_thread_num() );
	return prune_count;
}

int nqiv_pruner_run(nqiv_pruner* pruner, nqiv_montage_state* montage, nqiv_image_manager* images, nqiv_priority_queue* thread_queue)
{
	int output = 0;
	const int num_images = images->images->position / sizeof(nqiv_image*);
	nqiv_image** images_array = images->images->data;
	int iidx;
	for(iidx = 0; iidx < num_images; ++iidx) {
		const int result = nqiv_pruner_run_image(pruner, montage, thread_queue, iidx, images_array[iidx]);
		if(result == -1) {
			return result;
		}
		output += result;
	}
	memset( &pruner->state, 0, sizeof(nqiv_pruner_state) );
	return output;
}

/*
int nqiv_cmd_scan_not_whitespace(const char* data, const int start, const int end, int* length)
{
	const char* whitespace[] = {" ", "\t", NULL};
	return nqiv_cmd_scan_subs(data, start, end, true, whitespace, length);
}
SUM OR AND
thumbnail_vips thumbnail_data thumbnail_surface thumbnail_texture vips data surface texture
loaded_ahead INTEGER loaded_behind INTEGER bytes_ahead INTEGER bytes_behind INTEGER self
UNLOAD
vips data surface texture
*/
int nqiv_pruner_parse_int(nqiv_log_ctx* logger, const char* text, const int idx, const int end_idx, int* output)
{
	int nidx = idx;
	nidx = nqiv_cmd_scan_not_whitespace(text, nidx, end_idx, NULL);
	if(nidx == -1) {
		nqiv_log_write(logger, NQIV_LOG_ERROR, "Failed to get non-whitespace for integer value\n");
		return nidx;
	}
	nqiv_log_write(logger, NQIV_LOG_DEBUG, "Trying to get integer at %s\n", &text[nidx]);
	char* end = NULL;
	const int tmp = strtol(&text[nidx], &end, 10);
	if(errno != ERANGE && end != NULL) {
		nqiv_log_write(logger, NQIV_LOG_DEBUG, "Int arg is %d for input %s\n", tmp, &text[nidx]);
		*output = tmp;
		nidx = end - text;
	} else {
		nqiv_log_write(logger, NQIV_LOG_ERROR, "Error parsing int arg with input %s\n", &text[nidx]);
	}
	return nidx;
}

int nqiv_pruner_parse_int_pair(nqiv_log_ctx* logger, const char* text, const int idx, const int end_idx, nqiv_pruner_desc_datapoint_content* output)
{
	int nidx = idx;
	nidx = nqiv_cmd_scan_not_whitespace(text, nidx, end_idx, NULL);
	if(nidx == -1) {
		nqiv_log_write(logger, NQIV_LOG_ERROR, "Failed to get non-whitespace for integer value\n");
		return nidx;
	}
	nqiv_log_write(logger, NQIV_LOG_DEBUG, "Trying to get integer pair at %s\n", &text[nidx]);
	const int first_value_orig = output->as_int_pair[0];
	nidx = nqiv_pruner_parse_int(logger, text, nidx, end_idx, &output->as_int_pair[0]);
	if(nidx == -1) {
		output->as_int_pair[0] = first_value_orig;
		return nidx;
	}
	nidx = nqiv_pruner_parse_int(logger, text, nidx, end_idx, &output->as_int_pair[1]);
	if(nidx == -1) {
		output->as_int_pair[0] = first_value_orig;
	}
	return nidx;
}

int nqiv_pruner_set_true(nqiv_log_ctx* logger, const char* text, const int idx, const int end_idx, nqiv_pruner_desc_datapoint_content* output)
{
	(void) logger;
	(void) text;
	(void) end_idx;
	output->as_bool = true;
	return idx;
}

int nqiv_pruner_parse_set_check( nqiv_log_ctx* logger,
								 const char* text,
								 const int idx,
								 const int end,
								 const bool inside_no,
								 const bool inside_unload,
								 nqiv_pruner_desc_datapoint* point,
								 bool* unload_setting,
								 int (*parse_func)(nqiv_log_ctx*, const char*, const int, const int, nqiv_pruner_desc_datapoint_content*) )
{
	int nidx = idx;
	if(inside_unload) {
		if(inside_no) {
			nqiv_log_write(logger, NQIV_LOG_DEBUG, "Disabling unload\n");
			*unload_setting = false;
		} else {
			nqiv_log_write(logger, NQIV_LOG_DEBUG, "Enabling unload\n");
			*unload_setting = true;
		}
	} else {
		if(inside_no) {
			nqiv_log_write(logger, NQIV_LOG_DEBUG, "Clearing datapoint condition\n");
			memset( &point->condition, 0, sizeof(nqiv_pruner_desc_datapoint_content) );
			point->active = false;
		} else {
			nidx = parse_func(logger, text, nidx, end,  &point->condition);
			point->active = true;
		}
	}
	return nidx;
}

int nqiv_pruner_parse_check( nqiv_log_ctx* logger,
							 const char* text,
							 const int idx,
							 const int end,
							 const bool inside_no,
							 const bool inside_unload,
							 const bool inside_image,
							 const bool inside_thumbnail,
							 nqiv_pruner_desc_datapoint* point,
							 nqiv_pruner_desc_datapoint* thumbnail_point,
							 bool* unload_setting,
							 bool* thumbnail_unload_setting,
							 int (*parse_func)(nqiv_log_ctx*, const char*, const int, const int, nqiv_pruner_desc_datapoint_content*) )
{
	int nidx = idx;
	int tidx = -1;
	if(inside_image) {
		tidx = nqiv_pruner_parse_set_check(logger, text, nidx, end, inside_no, inside_unload, point, unload_setting, parse_func);
	}
	if(inside_thumbnail && tidx != -1) {
		nidx = nqiv_pruner_parse_set_check(logger, text, nidx, end, inside_no, inside_unload, thumbnail_point, thumbnail_unload_setting, parse_func);
	}
	return nidx;
}

bool nqiv_pruner_append(nqiv_pruner* pruner, nqiv_pruner_desc* desc)
{
	nqiv_log_write(pruner->logger, NQIV_LOG_DEBUG, "Adding desc to pruner list.\n");
	if( !nqiv_array_push_bytes( pruner->pruners, desc, sizeof(nqiv_pruner_desc) ) ) {
		nqiv_log_write(pruner->logger, NQIV_LOG_ERROR, "Failed to add pruning desc to list\n");
		return false;
	}
	return true;
}

bool nqiv_pruner_check_token(const char* text, const int idx, const int end, const char* subs)
{
	int token_end = nqiv_cmd_scan_whitespace(text, idx, end, NULL);
	if(token_end == -1) {
		token_end = end;
	}
	return strncmp( &text[idx], subs, strlen(subs) ) == 0 && idx + (int)strlen(subs) == token_end;
}

bool nqiv_pruner_create_desc(nqiv_log_ctx* logger, const char* text, nqiv_pruner_desc* desc)
{
	memset( desc, 0, sizeof(nqiv_pruner_desc) );
	/* TODO Make it so we can set multiple sets at once? */
	nqiv_pruner_desc_dataset* set = NULL;
	nqiv_pruner_desc_dataset* thumbnail_set = NULL;
	bool* unload_setting = NULL;
	bool* thumbnail_unload_setting = NULL;
	bool inside_no = false;
	bool inside_unload = false;
	bool inside_image = true;
	bool inside_thumbnail = false;
	const int end = strlen(text);
	int idx = 0;
	nqiv_log_write(logger, NQIV_LOG_DEBUG, "Generating pruner desc from %s\n", text);
	while(idx < end) {
		idx = nqiv_cmd_scan_not_whitespace(text, idx, end, NULL);
		if(idx == -1) {
			break;
		}
		if( nqiv_pruner_check_token(text, idx, end, "no") ) {
			nqiv_log_write(logger, NQIV_LOG_DEBUG, "Parsing 'no' at %s\n", &text[idx]);
			inside_no = true;
			idx += strlen("no");
		} else if( nqiv_pruner_check_token(text, idx, end, "sum") ) {
			nqiv_log_write(logger, NQIV_LOG_DEBUG, "Parsing 'sum' at %s\n", &text[idx]);
			idx += strlen("sum");
			if(inside_no) {
				nqiv_log_write(logger, NQIV_LOG_DEBUG, "Disabling sum\n");
				desc->counter &= ~NQIV_PRUNER_COUNT_OP_SUM;
				desc->state_check.total_sum = 0;
				inside_no = false;
			} else {
				nqiv_log_write(logger, NQIV_LOG_DEBUG, "Enabling sum\n");
				desc->counter |= NQIV_PRUNER_COUNT_OP_SUM;
				idx = nqiv_pruner_parse_int(logger, text, idx, end, &desc->state_check.total_sum);
			}
		} else if( nqiv_pruner_check_token(text, idx, end, "or") ) {
			nqiv_log_write(logger, NQIV_LOG_DEBUG, "Parsing 'or' at %s\n", &text[idx]);
			idx += strlen("or");
			if(inside_no) {
				nqiv_log_write(logger, NQIV_LOG_DEBUG, "Disabling or\n");
				desc->counter &= ~NQIV_PRUNER_COUNT_OP_OR;
				desc->state_check.or_result = false;
				inside_no = false;
			} else {
				nqiv_log_write(logger, NQIV_LOG_DEBUG, "Enabling or\n");
				desc->counter |= NQIV_PRUNER_COUNT_OP_OR;
				desc->state_check.or_result = true;
			}
		} else if( nqiv_pruner_check_token(text, idx, end, "and") ) {
			nqiv_log_write(logger, NQIV_LOG_DEBUG, "Parsing 'and' at %s\n", &text[idx]);
			idx += strlen("and");
			if(inside_no) {
				nqiv_log_write(logger, NQIV_LOG_DEBUG, "Disabling and\n");
				desc->counter &= ~NQIV_PRUNER_COUNT_OP_AND;
				desc->state_check.and_result = false;
				inside_no = false;
			} else {
				nqiv_log_write(logger, NQIV_LOG_DEBUG, "Enabling and\n");
				desc->counter |= NQIV_PRUNER_COUNT_OP_AND;
				desc->state_check.and_result = true;
			}
		} else if( nqiv_pruner_check_token(text, idx, end, "unload") ) {
			nqiv_log_write(logger, NQIV_LOG_DEBUG, "Parsing 'unload' at %s\n", &text[idx]);
			idx += strlen("unload");
			if(inside_no) {
				nqiv_log_write(logger, NQIV_LOG_DEBUG, "Disabling unload\n");
				inside_unload = false;
				inside_no = false;
			} else {
				nqiv_log_write(logger, NQIV_LOG_DEBUG, "Enabling unload\n");
				inside_unload = true;
			}
		} else if( nqiv_pruner_check_token(text, idx, end, "thumbnail") ) {
			nqiv_log_write(logger, NQIV_LOG_DEBUG, "Parsing 'thumbnail' at %s\n", &text[idx]);
			idx += strlen("thumbnail");
			if(inside_no) {
				nqiv_log_write(logger, NQIV_LOG_DEBUG, "Disabling thumbnail\n");
				inside_thumbnail = false;
				inside_no = false;
			} else {
				nqiv_log_write(logger, NQIV_LOG_DEBUG, "Enabling thumbnail\n");
				inside_thumbnail = true;
			}
		} else if( nqiv_pruner_check_token(text, idx, end, "image") ) {
			nqiv_log_write(logger, NQIV_LOG_DEBUG, "Parsing 'image' at %s\n", &text[idx]);
			idx += strlen("image");
			if(inside_no) {
				nqiv_log_write(logger, NQIV_LOG_DEBUG, "Disabling image\n");
				inside_image = false;
				inside_no = false;
			} else {
				nqiv_log_write(logger, NQIV_LOG_DEBUG, "Enabling image\n");
				inside_image = true;
			}
		} else if( nqiv_pruner_check_token(text, idx, end, "vips") ) {
			nqiv_log_write(logger, NQIV_LOG_DEBUG, "Parsing 'vips' at %s\n", &text[idx]);
			idx += strlen("vips");
			if(inside_no) {
				if(inside_unload) {
					if(inside_thumbnail) {
						desc->unload_thumbnail_vips = false;
					}
					if(inside_image) {
						desc->unload_vips = false;
					}
				}
				nqiv_log_write(logger, NQIV_LOG_DEBUG, "Disabling vips\n");
				set = NULL;
				unload_setting = NULL;
				thumbnail_set = NULL;
				thumbnail_unload_setting = NULL;
				inside_no = false;
			} else if(inside_unload) {
				if(inside_thumbnail) {
					desc->unload_thumbnail_vips = true;
				}
				if(inside_image) {
					desc->unload_vips = true;
				}
			} else {
				nqiv_log_write(logger, NQIV_LOG_DEBUG, "Enabling vips\n");
				set = &desc->vips_set;
				unload_setting = &desc->unload_vips;
				thumbnail_set = &desc->thumbnail_vips_set;
				thumbnail_unload_setting = &desc->unload_thumbnail_vips;
			}
		} else if( nqiv_pruner_check_token(text, idx, end, "raw") ) {
			nqiv_log_write(logger, NQIV_LOG_DEBUG, "Parsing 'raw' at %s\n", &text[idx]);
			idx += strlen("raw");
			if(inside_no) {
				if(inside_unload) {
					if(inside_thumbnail) {
						desc->unload_thumbnail_raw = false;
					}
					if(inside_image) {
						desc->unload_raw = false;
					}
				}
				nqiv_log_write(logger, NQIV_LOG_DEBUG, "Disabling raw\n");
				set = NULL;
				unload_setting = NULL;
				thumbnail_set = NULL;
				thumbnail_unload_setting = NULL;
				inside_no = false;
			} else if(inside_unload) {
				if(inside_thumbnail) {
					desc->unload_thumbnail_raw = true;
				}
				if(inside_image) {
					desc->unload_raw = true;
				}
			} else {
				nqiv_log_write(logger, NQIV_LOG_DEBUG, "Enabling raw\n");
				set = &desc->raw_set;
				unload_setting = &desc->unload_raw;
				thumbnail_set = &desc->thumbnail_raw_set;
				thumbnail_unload_setting = &desc->unload_thumbnail_raw;
			}
		} else if( nqiv_pruner_check_token(text, idx, end, "surface") ) {
			nqiv_log_write(logger, NQIV_LOG_DEBUG, "Parsing 'surface' at %s\n", &text[idx]);
			idx += strlen("surface");
			if(inside_no) {
				if(inside_unload) {
					if(inside_thumbnail) {
						desc->unload_thumbnail_surface = false;
					}
					if(inside_image) {
						desc->unload_surface = false;
					}
				}
				nqiv_log_write(logger, NQIV_LOG_DEBUG, "Disabling surface\n");
				set = NULL;
				unload_setting = NULL;
				thumbnail_set = NULL;
				thumbnail_unload_setting = NULL;
				inside_no = false;
			} else if(inside_unload) {
				if(inside_thumbnail) {
					desc->unload_thumbnail_surface = true;
				}
				if(inside_image) {
					desc->unload_surface = true;
				}
			} else {
				nqiv_log_write(logger, NQIV_LOG_DEBUG, "Enabling surface\n");
				set = &desc->surface_set;
				unload_setting = &desc->unload_surface;
				thumbnail_set = &desc->thumbnail_surface_set;
				thumbnail_unload_setting = &desc->unload_thumbnail_surface;
			}
		} else if( nqiv_pruner_check_token(text, idx, end, "texture") ) {
			nqiv_log_write(logger, NQIV_LOG_DEBUG, "Parsing 'texture' at %s\n", &text[idx]);
			idx += strlen("texture");
			if(inside_no) {
				if(inside_unload) {
					if(inside_thumbnail) {
						desc->unload_thumbnail_texture = false;
					}
					if(inside_image) {
						desc->unload_texture = false;
					}
				}
				nqiv_log_write(logger, NQIV_LOG_DEBUG, "Disabling texture\n");
				set = NULL;
				unload_setting = NULL;
				thumbnail_set = NULL;
				thumbnail_unload_setting = NULL;
				inside_no = false;
			} else if(inside_unload) {
				if(inside_thumbnail) {
					desc->unload_thumbnail_texture = true;
				}
				if(inside_image) {
					desc->unload_texture = true;
				}
			} else {
				nqiv_log_write(logger, NQIV_LOG_DEBUG, "Enabling texture\n");
				set = &desc->texture_set;
				unload_setting = &desc->unload_texture;
				thumbnail_set = &desc->thumbnail_texture_set;
				thumbnail_unload_setting = &desc->unload_thumbnail_texture;
			}
		} else if(set == NULL) {
			nqiv_log_write(logger, NQIV_LOG_ERROR, "Failed to continue with unknown set target from %s\n", &text[idx]);
			return false;
		} else if( nqiv_pruner_check_token(text, idx, end, "loaded_ahead") ) {
			nqiv_log_write(logger, NQIV_LOG_DEBUG, "Parsing 'loaded_ahead' %s\n", &text[idx]);
			idx += strlen("loaded_ahead");
			idx = nqiv_pruner_parse_check(logger, text, idx, end, inside_no, inside_unload, inside_image, inside_thumbnail, &set->loaded_ahead, &thumbnail_set->loaded_ahead, unload_setting, thumbnail_unload_setting, nqiv_pruner_parse_int_pair);
			inside_no = false;
		} else if( nqiv_pruner_check_token(text, idx, end, "loaded_behind") ) {
			nqiv_log_write(logger, NQIV_LOG_DEBUG, "Parsing 'loaded_behind' %s\n", &text[idx]);
			idx += strlen("loaded_behind");
			idx = nqiv_pruner_parse_check(logger, text, idx, end, inside_no, inside_unload, inside_image, inside_thumbnail, &set->loaded_behind, &thumbnail_set->loaded_behind, unload_setting, thumbnail_unload_setting, nqiv_pruner_parse_int_pair);
			inside_no = false;
		} else if( nqiv_pruner_check_token(text, idx, end, "bytes_ahead") ) {
			nqiv_log_write(logger, NQIV_LOG_DEBUG, "Parsing 'bytes_ahead' %s\n", &text[idx]);
			idx += strlen("bytes_ahead");
			idx = nqiv_pruner_parse_check(logger, text, idx, end, inside_no, inside_unload, inside_image, inside_thumbnail, &set->bytes_ahead, &thumbnail_set->bytes_ahead, unload_setting, thumbnail_unload_setting, nqiv_pruner_parse_int_pair);
			inside_no = false;
		} else if( nqiv_pruner_check_token(text, idx, end, "bytes_behind") ) {
			nqiv_log_write(logger, NQIV_LOG_DEBUG, "Parsing 'bytes_behind' %s\n", &text[idx]);
			idx += strlen("bytes_behind");
			idx = nqiv_pruner_parse_check(logger, text, idx, end, inside_no, inside_unload, inside_image, inside_thumbnail, &set->bytes_behind, &thumbnail_set->bytes_behind, unload_setting, thumbnail_unload_setting, nqiv_pruner_parse_int_pair);
			inside_no = false;
		} else if( nqiv_pruner_check_token(text, idx, end, "self_opened") ) {
			nqiv_log_write(logger, NQIV_LOG_DEBUG, "Parsing 'self_opened' %s\n", &text[idx]);
			idx += strlen("self_opened");
			idx = nqiv_pruner_parse_check(logger, text, idx, end, inside_no, inside_unload, inside_image, inside_thumbnail, &set->loaded_self, &thumbnail_set->loaded_self, unload_setting, thumbnail_unload_setting, nqiv_pruner_set_true);
			inside_no = false;
		} else if( nqiv_pruner_check_token(text, idx, end, "not_animated") ) {
			nqiv_log_write(logger, NQIV_LOG_DEBUG, "Parsing 'not_animated' %s\n", &text[idx]);
			idx += strlen("not_animated");
			idx = nqiv_pruner_parse_check(logger, text, idx, end, inside_no, inside_unload, inside_image, inside_thumbnail, &set->not_animated, &thumbnail_set->not_animated, unload_setting, thumbnail_unload_setting, nqiv_pruner_set_true);
			inside_no = false;
		} else {
			nqiv_log_write(logger, NQIV_LOG_ERROR, "Failed to identify %s\n", &text[idx]);
			return false;
		}
		if(idx == -1) {
			return false;
		}
	}
	return true;
}
