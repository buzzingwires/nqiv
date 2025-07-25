#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "logging.h"
#include "array.h"
#include "helpers.h"
#include "event.h"
#include "montage.h"
#include "image.h"
#include "cmd.h"
#include "state.h"
#include "pruner.h"

void nqiv_pruner_destroy(nqiv_pruner* pruner)
{
	if(pruner->pruners != NULL) {
		nqiv_array_destroy(pruner->pruners);
	}
	memset(pruner, 0, sizeof(nqiv_pruner));
}

bool nqiv_pruner_init(nqiv_pruner* pruner, nqiv_log_ctx* logger, const int queue_length)
{
	nqiv_array* new_pruners = nqiv_array_create(sizeof(nqiv_pruner_desc), queue_length);
	if(new_pruners == NULL) {
		nqiv_log_write(logger, NQIV_LOG_ERROR,
		               "Failed to create queue of length %d for pruners array.\n", queue_length);
		return false;
	}
	nqiv_pruner_destroy(pruner);
	pruner->logger = logger;
	pruner->pruners = new_pruners;
	return true;
}

static void nqiv_pruner_update_state_boolean(nqiv_pruner* pruner, const bool value)
{
	if(!pruner->state.and_is_set) {
		pruner->state.and_result = true;
	}
	pruner->state.and_is_set = true;
	pruner->state.or_result = pruner->state.or_result || value;
	pruner->state.and_result = pruner->state.and_result && value;
	nqiv_log_write(pruner->logger, NQIV_LOG_DEBUG,
	               "New prune state boolean is or_result: %s and_result: %s\n",
	               NQIV_BOOLSTR(pruner->state.or_result), NQIV_BOOLSTR(pruner->state.and_result));
}

static void nqiv_pruner_update_state_integer(nqiv_pruner* pruner, const int value)
{
	pruner->state.total_sum += value;
	nqiv_log_write(pruner->logger, NQIV_LOG_DEBUG, "New prune state total_sum is %d\n",
	               pruner->state.total_sum);
}

static void nqiv_pruner_run_not_animated(nqiv_pruner*                pruner,
                                         nqiv_pruner_desc_datapoint* datapoint,
                                         const void*                 object)
{
	if(datapoint->active) {
		const bool result = !(((nqiv_image_form*)object)->animation.exists);
		datapoint->value.as_bool = result;
		nqiv_log_write(pruner->logger, NQIV_LOG_DEBUG, "not_animated result is: %s\n",
		               NQIV_BOOLSTR(result));
		nqiv_pruner_update_state_boolean(pruner, result);
	}
}

static void nqiv_pruner_run_loaded_self(nqiv_pruner*                pruner,
                                        nqiv_pruner_desc_datapoint* datapoint,
                                        const void*                 object)
{
	if(datapoint->active) {
		const bool result = object != NULL;
		datapoint->value.as_bool = result;
		nqiv_log_write(pruner->logger, NQIV_LOG_DEBUG, "loaded_self result is: %s\n",
		               NQIV_BOOLSTR(result));
		nqiv_pruner_update_state_boolean(pruner, result);
	}
}

static void nqiv_pruner_loaded_count_body(nqiv_pruner*                pruner,
                                          nqiv_pruner_desc_datapoint* datapoint,
                                          const int                   increment)
{
	/* Mutate value by increment amount and check against condition, updating
	 * result against the pruner-wide nqiv_pruner_count_op */
	datapoint->value.as_int += increment;
	const bool result = datapoint->value.as_int > datapoint->condition.as_int_pair[1];
	nqiv_log_write(pruner->logger, NQIV_LOG_DEBUG,
	               "count_body result is %s new datapoint value %d compared to %d\n",
	               NQIV_BOOLSTR(result), datapoint->value.as_int,
	               datapoint->condition.as_int_pair[1]);
	nqiv_pruner_update_state_boolean(pruner, result);
	nqiv_pruner_update_state_integer(pruner, increment);
}

static void nqiv_pruner_run_loaded_ahead(nqiv_pruner*                pruner,
                                         nqiv_pruner_desc_datapoint* datapoint,
                                         const void*                 object)
{
	if(datapoint->active && object != NULL
	   && (pruner->state.idx - pruner->state.montage_end) + 1
	          > datapoint->condition.as_int_pair[0]) {
		nqiv_pruner_loaded_count_body(pruner, datapoint, 1);
	}
}

static void nqiv_pruner_run_loaded_behind(nqiv_pruner*                pruner,
                                          nqiv_pruner_desc_datapoint* datapoint,
                                          const void*                 object)
{
	if(datapoint->active && object != NULL
	   && (pruner->state.montage_start - pruner->state.idx) > datapoint->condition.as_int_pair[0]) {
		nqiv_pruner_loaded_count_body(pruner, datapoint, 1);
	}
}

static void nqiv_pruner_run_bytes_ahead(nqiv_pruner*                pruner,
                                        nqiv_pruner_desc_datapoint* datapoint,
                                        const void*                 object,
                                        const int                   size)
{
	if(datapoint->active && object != NULL
	   && (pruner->state.idx - pruner->state.montage_end) + 1
	          > datapoint->condition.as_int_pair[0]) {
		nqiv_pruner_loaded_count_body(pruner, datapoint, size);
	}
}

static void nqiv_pruner_run_bytes_behind(nqiv_pruner*                pruner,
                                         nqiv_pruner_desc_datapoint* datapoint,
                                         const void*                 object,
                                         const int                   size)
{
	if(datapoint->active && object != NULL
	   && (pruner->state.montage_start - pruner->state.idx) > datapoint->condition.as_int_pair[0]) {
		nqiv_pruner_loaded_count_body(pruner, datapoint, size);
	}
}

static void nqiv_pruner_run_set(nqiv_pruner*              pruner,
                                nqiv_pruner_desc_dataset* set,
                                const nqiv_image_form*    form,
                                const void*               object,
                                const int                 size)
{
	nqiv_pruner_run_not_animated(pruner, &(set->not_animated), form);
	nqiv_pruner_run_loaded_self(pruner, &(set->loaded_self), object);
	nqiv_pruner_run_loaded_ahead(pruner, &(set->loaded_ahead), object);
	nqiv_pruner_run_loaded_behind(pruner, &(set->loaded_behind), object);
	nqiv_pruner_run_bytes_ahead(pruner, &(set->bytes_ahead), object, size);
	nqiv_pruner_run_bytes_behind(pruner, &(set->bytes_behind), object, size);
}

static void
nqiv_pruner_run_desc(nqiv_pruner* pruner, nqiv_pruner_desc* desc, const nqiv_image* image)
{
	nqiv_pruner_run_set(pruner, &(desc->vips_set), &image->image, image->image.vips,
	                    image->image.effective_width * image->image.effective_height * 4);
	nqiv_pruner_run_set(pruner, &(desc->surface_set), &image->image, image->image.surface,
	                    image->image.effective_width * image->image.effective_height * 4);
	nqiv_pruner_run_set(pruner, &(desc->texture_set), &image->image, image->image.texture,
	                    image->image.effective_width * image->image.effective_height * 4);
	nqiv_pruner_run_set(pruner, &(desc->thumbnail_vips_set), &image->thumbnail,
	                    image->thumbnail.vips,
	                    image->thumbnail.effective_width * image->thumbnail.effective_height * 4);
	nqiv_pruner_run_set(pruner, &(desc->thumbnail_surface_set), &image->thumbnail,
	                    image->thumbnail.surface,
	                    image->thumbnail.effective_width * image->thumbnail.effective_height * 4);
	nqiv_pruner_run_set(pruner, &(desc->thumbnail_texture_set), &image->thumbnail,
	                    image->thumbnail.texture,
	                    image->thumbnail.effective_width * image->thumbnail.effective_height * 4);
}

static int nqiv_pruner_run_image(nqiv_pruner*         pruner,
                                 nqiv_montage_state*  montage,
                                 nqiv_priority_queue* thread_queue,
                                 const int            iidx,
                                 nqiv_image*          image)
{
	nqiv_event                     event = {0};
	nqiv_event_image_load_options* event_options = &(event.options.image_load);
	bool                           send_event = false;
	if(!nqiv_image_test_lock(image)) {
		return 0;
	}
	if(!nqiv_image_has_loaded_form(image)) {
		nqiv_image_unlock(image);
		return 0;
	}
	event.type = NQIV_EVENT_IMAGE_LOAD;
	event_options->image = image;
	event_options->image_options.unload = true;
	event_options->thumbnail_options.unload = true;
	int               prune_count = 0;
	int               idx;
	nqiv_pruner_desc* descs_array = pruner->pruners->data;
	const int         num_descs = nqiv_array_get_units_count(pruner->pruners);
	for(idx = 0; idx < num_descs; ++idx) {
		nqiv_pruner_desc* desc = &descs_array[idx];
		assert(desc->counter != NQIV_PRUNER_COUNT_OP_UNKNOWN);
		pruner->state.idx = iidx;
		const int raw_start_idx = montage->positions.start - montage->preload.behind;
		pruner->state.montage_start = NQIV_MAX(raw_start_idx, 0);
		pruner->state.montage_end = montage->positions.end + montage->preload.ahead;
		pruner->state.or_result = false;
		pruner->state.and_result = false;
		pruner->state.and_is_set = false;
		nqiv_pruner_run_desc(pruner, desc, image);
		/* If any of the operations passed. */
		if((desc->counter & NQIV_PRUNER_COUNT_OP_SUM
		    && pruner->state.total_sum > desc->state_check.total_sum)
		   || (desc->counter & NQIV_PRUNER_COUNT_OP_OR
		       && pruner->state.or_result == desc->state_check.or_result)
		   || (pruner->state.and_is_set && desc->counter & NQIV_PRUNER_COUNT_OP_AND
		       && pruner->state.and_result == desc->state_check.and_result)) {
			nqiv_log_write(pruner->logger, NQIV_LOG_DEBUG, "Desc %d pruning image %s\n", idx,
			               image->image.path);
			/* TODO: This cannot be called from a thread. If we plan to have the pruner run in a
			 * thread, we need to do this in a thread safe way, probably by sending an SDL event for
			 * master. */
			if(desc->unload_texture
			   && (image->image.texture != NULL || image->image.fallback_texture != NULL)) {
				nqiv_unload_image_form_all_textures(&image->image);
				send_event = true;
			}
			if(desc->unload_thumbnail_texture
			   && (image->thumbnail.texture != NULL || image->thumbnail.fallback_texture != NULL)) {
				nqiv_unload_image_form_all_textures(&image->thumbnail);
				send_event = true;
			}
			/* Long determination of whether there is anything that actually
			 * needs unloading. It is important that we avoid doing unnecessary
			 * work. Send a single event for each image, and only set each
			 * unload option if the object to be unloaded actually exists. */
			event_options->image_options.vips = event_options->image_options.vips
			                                    || (desc->unload_vips && image->image.vips != NULL);
			event_options->image_options.surface =
				event_options->image_options.surface
				|| (desc->unload_surface && image->image.surface != NULL);
			event_options->image_options.vips_soft =
				event_options->image_options.vips_soft
				|| (desc->unload_vips_soft && image->image.vips != NULL);
			event_options->image_options.surface_soft =
				event_options->image_options.surface_soft
				|| (desc->unload_surface_soft && image->image.surface != NULL);
			event_options->thumbnail_options.vips =
				event_options->thumbnail_options.vips
				|| (desc->unload_thumbnail_vips && image->thumbnail.vips != NULL);
			event_options->thumbnail_options.surface =
				event_options->thumbnail_options.surface
				|| (desc->unload_thumbnail_surface && image->thumbnail.surface != NULL);
			event_options->thumbnail_options.vips_soft =
				event_options->thumbnail_options.vips_soft
				|| (desc->unload_thumbnail_vips_soft && image->thumbnail.vips != NULL);
			event_options->thumbnail_options.surface_soft =
				event_options->thumbnail_options.surface_soft
				|| (desc->unload_thumbnail_surface_soft && image->thumbnail.surface != NULL);
			/* Finally, track if there's anything to do. */
			send_event =
				send_event || event.options.image_load.image_options.vips
				|| event_options->image_options.surface || event_options->image_options.vips_soft
				|| event_options->image_options.surface_soft
				|| event_options->thumbnail_options.vips || event_options->thumbnail_options.surface
				|| event_options->thumbnail_options.vips_soft
				|| event_options->thumbnail_options.surface_soft;
		}
	}
	/* We send the event if there's something to do. */
	nqiv_log_write(pruner->logger, NQIV_LOG_DEBUG, "%sending prune event for image %d.\n",
	               send_event ? "S" : "Not s", iidx);
	if(send_event) {
		event.transaction_group = pruner->thread_event_transaction_group;
		if(!nqiv_priority_queue_push(thread_queue, NQIV_EVENT_PRIORITY_PRUNE, &event)) {
			nqiv_image_unlock(image);
			return false;
		}
		prune_count = 1;
	}
	nqiv_image_unlock(image);
	return prune_count;
}

static void nqiv_pruner_clean_desc_set(nqiv_pruner_desc_dataset* set)
{
	memset(&set->not_animated.value, 0, sizeof(nqiv_pruner_desc_datapoint_content));
	memset(&set->loaded_self.value, 0, sizeof(nqiv_pruner_desc_datapoint_content));
	memset(&set->loaded_ahead.value, 0, sizeof(nqiv_pruner_desc_datapoint_content));
	memset(&set->loaded_behind.value, 0, sizeof(nqiv_pruner_desc_datapoint_content));
	memset(&set->bytes_ahead.value, 0, sizeof(nqiv_pruner_desc_datapoint_content));
	memset(&set->bytes_behind.value, 0, sizeof(nqiv_pruner_desc_datapoint_content));
}

static void nqiv_pruner_clean_desc(nqiv_pruner_desc* desc)
{
	/* Clean ephemeral state information. */
	nqiv_pruner_clean_desc_set(&desc->vips_set);
	nqiv_pruner_clean_desc_set(&desc->surface_set);
	nqiv_pruner_clean_desc_set(&desc->texture_set);
	nqiv_pruner_clean_desc_set(&desc->thumbnail_vips_set);
	nqiv_pruner_clean_desc_set(&desc->thumbnail_surface_set);
	nqiv_pruner_clean_desc_set(&desc->thumbnail_texture_set);
}

int nqiv_pruner_run(nqiv_pruner*         pruner,
                    nqiv_montage_state*  montage,
                    nqiv_image_manager*  images,
                    nqiv_priority_queue* thread_queue)
{
	int          output = 0;
	const int    num_images = nqiv_array_get_units_count(images->images);
	nqiv_image** images_array = images->images->data;
	int          iidx;
	for(iidx = 0; iidx < num_images; ++iidx) {
		const int result =
			nqiv_pruner_run_image(pruner, montage, thread_queue, iidx, images_array[iidx]);
		if(result == -1) {
			return result;
		}
		output += result;
	}
	int               idx;
	const int         num_descs = nqiv_array_get_units_count(pruner->pruners);
	nqiv_pruner_desc* descs_array = pruner->pruners->data;
	for(idx = 0; idx < num_descs; ++idx) {
		nqiv_pruner_clean_desc(&descs_array[idx]);
	}
	memset(&pruner->state, 0, sizeof(nqiv_pruner_state));
	if(output > 0) {
		nqiv_log_write(pruner->logger, NQIV_LOG_INFO,
		               "Sending %d prune events (texture prunes not counted).\n", output);
	}
	return output;
}

static int nqiv_pruner_parse_int(
	nqiv_log_ctx* logger, const char* text, const int idx, const int end_idx, int* output)
{
	int nidx = idx;
	nidx = nqiv_cmd_scan_not_whitespace(text, nidx, end_idx);
	if(nidx == -1) {
		nqiv_log_write(logger, NQIV_LOG_ERROR, "Failed to get non-whitespace for integer value\n");
		return nidx;
	}
	nqiv_log_write(logger, NQIV_LOG_DEBUG, "Trying to get integer at %s\n", &text[nidx]);
	char*          end = NULL;
	const long int tmp = strtol(&text[nidx], &end, 10);
	if(errno != ERANGE && end != NULL && &text[nidx] != end && tmp >= INT_MIN && tmp <= INT_MAX) {
		nqiv_log_write(logger, NQIV_LOG_DEBUG, "Int arg is %d for input %s\n", tmp, &text[nidx]);
		*output = (int)tmp;
		nidx = nqiv_ptrdiff(end, text);
	} else {
		nqiv_log_write(logger, NQIV_LOG_ERROR, "Error parsing int arg with input %s\n",
		               &text[nidx]);
	}
	return nidx;
}

static int nqiv_pruner_parse_int_pair(nqiv_log_ctx*                       logger,
                                      const char*                         text,
                                      const int                           idx,
                                      const int                           end_idx,
                                      nqiv_pruner_desc_datapoint_content* output)
{
	int nidx = idx;
	nidx = nqiv_cmd_scan_not_whitespace(text, nidx, end_idx);
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

static int nqiv_pruner_set_true(nqiv_log_ctx*                       logger,
                                const char*                         text,
                                const int                           idx,
                                const int                           end_idx,
                                nqiv_pruner_desc_datapoint_content* output)
{
	(void)logger;
	(void)text;
	(void)end_idx;
	output->as_bool = true;
	return idx;
}

static int nqiv_pruner_parse_set_check(nqiv_log_ctx*               logger,
                                       const char*                 text,
                                       const int                   idx,
                                       const int                   end,
                                       const bool                  inside_no,
                                       nqiv_pruner_desc_datapoint* point,
                                       int (*parse_func)(nqiv_log_ctx*,
                                                         const char*,
                                                         const int,
                                                         const int,
                                                         nqiv_pruner_desc_datapoint_content*))
{
	/* Based on the state of the parser, we parse the given condition of the
	 * check, disable the check, enable or disable its unload setting, setting
	 * the index as appropriate, or returning -1 on failure. */
	int nidx = idx;
	if(inside_no) {
		nqiv_log_write(logger, NQIV_LOG_DEBUG, "Clearing datapoint condition\n");
		memset(&point->condition, 0, sizeof(nqiv_pruner_desc_datapoint_content));
		point->active = false;
	} else {
		nidx = parse_func(logger, text, nidx, end, &point->condition);
		point->active = true;
	}
	return nidx;
}

static int nqiv_pruner_parse_check(nqiv_log_ctx*               logger,
                                   const char*                 text,
                                   const int                   idx,
                                   const int                   end,
                                   const bool                  inside_no,
                                   const bool                  inside_image,
                                   const bool                  inside_thumbnail,
                                   nqiv_pruner_desc_datapoint* point,
                                   nqiv_pruner_desc_datapoint* thumbnail_point,
                                   int (*parse_func)(nqiv_log_ctx*,
                                                     const char*,
                                                     const int,
                                                     const int,
                                                     nqiv_pruner_desc_datapoint_content*))
{
	int nidx = idx;
	int tidx = -1;
	if(inside_image) {
		tidx = nqiv_pruner_parse_set_check(logger, text, nidx, end, inside_no, point, parse_func);
	}
	if(!inside_thumbnail) {
		nidx = tidx;
	} else if(tidx != -1 || !inside_image) {
		nidx = nqiv_pruner_parse_set_check(logger, text, nidx, end, inside_no, thumbnail_point,
		                                   parse_func);
	}
	return nidx;
}

bool nqiv_pruner_append(nqiv_pruner* pruner, const nqiv_pruner_desc* desc)
{
	nqiv_log_write(pruner->logger, NQIV_LOG_DEBUG, "Adding desc to pruner list.\n");
	if(!nqiv_array_push(pruner->pruners, desc)) {
		nqiv_log_write(pruner->logger, NQIV_LOG_ERROR, "Failed to add pruning desc to list\n");
		return false;
	}
	return true;
}

static bool nqiv_pruner_check_token(
	nqiv_log_ctx* logger, const char* text, int* idx, const int end, const char* subs)
{
	nqiv_log_write(logger, NQIV_LOG_DEBUG, "Parsing '%s' at %s\n", subs, &text[*idx]);
	int token_end = nqiv_cmd_scan_whitespace(text, *idx, end);
	if(token_end == -1) {
		token_end = end;
	}
	if(strncmp(&text[*idx], subs, strlen(subs)) == 0 && *idx + (int)strlen(subs) == token_end) {
		*idx += (int)strlen(subs);
		return true;
	}
	return false;
}

typedef struct nqiv_pruner_create_common_args
{
	nqiv_log_ctx*              logger;
	const char*                text;
	int*                       idx;
	const int                  end;
	bool*                      inside_no;
	bool*                      inside_unload;
	bool*                      inside_image;
	bool*                      inside_thumbnail;
	nqiv_pruner_desc_dataset** set;
	nqiv_pruner_desc_dataset** thumbnail_set;
} nqiv_pruner_create_common_args;

static bool nqiv_pruner_create_desc_toggle(const nqiv_pruner_create_common_args* c,
                                           const char*                           name,
                                           bool*                                 inside_x)
{
	if(nqiv_pruner_check_token(c->logger, c->text, c->idx, c->end, name)) {
		if(*c->inside_no) {
			nqiv_log_write(c->logger, NQIV_LOG_DEBUG, "Disabling '%s'\n", name);
			*inside_x = false;
			*c->inside_no = false;
		} else {
			nqiv_log_write(c->logger, NQIV_LOG_DEBUG, "Enabling '%s'\n", name);
			*inside_x = true;
		}
		return true;
	}
	return false;
}

static void nqiv_pruner_desc_create_unload_flag(const bool* inside_hard,
                                                bool*       unload,
                                                bool*       unload_soft,
                                                const bool  value)
{
	if(*inside_hard) {
		if(unload != NULL) {
			*unload = value;
		}
	} else if(unload_soft != NULL) {
		*unload_soft = value;
	}
}

static void nqiv_pruner_create_desc_unload_pair(const nqiv_pruner_create_common_args* c,
                                                const bool*                           inside_hard,
                                                bool*                                 unload,
                                                bool*                                 unload_soft,
                                                bool*      unload_thumbnail,
                                                bool*      unload_thumbnail_soft,
                                                const bool value)
{
	if(*c->inside_thumbnail) {
		nqiv_pruner_desc_create_unload_flag(inside_hard, unload_thumbnail, unload_thumbnail_soft,
		                                    value);
	}
	if(*c->inside_image) {
		nqiv_pruner_desc_create_unload_flag(inside_hard, unload, unload_soft, value);
	}
}

static bool nqiv_pruner_create_desc_datatype(const nqiv_pruner_create_common_args* c,
                                             const bool*                           inside_hard,
                                             const char*                           name,
                                             bool*                                 unload,
                                             bool*                                 unload_soft,
                                             bool*                                 unload_thumbnail,
                                             bool*                     unload_thumbnail_soft,
                                             nqiv_pruner_desc_dataset* new_set,
                                             nqiv_pruner_desc_dataset* new_thumbnail_set)
{
	if(nqiv_pruner_check_token(c->logger, c->text, c->idx, c->end, name)) {
		assert(!*c->inside_unload || *c->set == NULL);
		if(*c->inside_no) {
			if(*c->inside_unload) {
				nqiv_pruner_create_desc_unload_pair(c, inside_hard, unload, unload_soft,
				                                    unload_thumbnail, unload_thumbnail_soft, false);
			}
			if(*c->set == new_set) {
				nqiv_log_write(c->logger, NQIV_LOG_DEBUG, "Disabling %s\n", name);
				*c->set = NULL;
				*c->thumbnail_set = NULL;
			}
			*c->inside_no = false;
		} else if(*c->inside_unload) {
			nqiv_pruner_create_desc_unload_pair(c, inside_hard, unload, unload_soft,
			                                    unload_thumbnail, unload_thumbnail_soft, true);
		} else {
			nqiv_log_write(c->logger, NQIV_LOG_DEBUG, "Enabling %s\n", name);
			*c->set = new_set;
			*c->thumbnail_set = new_thumbnail_set;
		}
		return true;
	}
	return false;
}

bool nqiv_pruner_create_desc(nqiv_log_ctx* logger, const char* text, nqiv_pruner_desc* desc)
{
	memset(desc, 0, sizeof(nqiv_pruner_desc));
	/* TODO Make it so we can set multiple sets at once? */
	/* If we have to deprecate something, write it to the dummy objects. */
	nqiv_pruner_desc_dataset             dummy_set = {0};
	nqiv_pruner_desc_dataset*            set = NULL;
	nqiv_pruner_desc_dataset*            thumbnail_set = NULL;
	bool                                 inside_no = false;
	bool                                 inside_unload = false;
	const bool                           always_hard = true;
	bool                                 inside_hard = false;
	bool                                 inside_image = true;
	bool                                 inside_thumbnail = false;
	const int                            end = nqiv_strlen(text);
	int                                  idx = 0;
	const nqiv_pruner_create_common_args common = {.logger = logger,
	                                               .text = text,
	                                               .idx = &idx,
	                                               .end = end,
	                                               .inside_no = &inside_no,
	                                               .inside_unload = &inside_unload,
	                                               .inside_image = &inside_image,
	                                               .inside_thumbnail = &inside_thumbnail,
	                                               .set = &set,
	                                               .thumbnail_set = &thumbnail_set};
	nqiv_log_write(logger, NQIV_LOG_DEBUG, "Generating pruner desc from %s\n", text);
	if(end > NQIV_PRUNER_DESC_STRLEN) {
		nqiv_log_write(logger, NQIV_LOG_ERROR, "Pruner desc must be %d characters or less.\n",
		               NQIV_PRUNER_DESC_STRLEN);
		return false;
	}
	while(idx < end) {
		idx = nqiv_cmd_scan_not_whitespace(text, idx, end);
		if(idx == -1) {
			break;
		}
		if(nqiv_pruner_create_desc_toggle(&common, "hard", &inside_hard)
		   || nqiv_pruner_create_desc_toggle(&common, "no", &inside_no)
		   || nqiv_pruner_create_desc_toggle(&common, "thumbnail", &inside_thumbnail)
		   || nqiv_pruner_create_desc_toggle(&common, "image", &inside_image)
		   || nqiv_pruner_create_desc_datatype(
			   &common, &inside_hard, "vips", &desc->unload_vips, &desc->unload_vips_soft,
			   &desc->unload_thumbnail_vips, &desc->unload_thumbnail_vips_soft, &desc->vips_set,
			   &desc->thumbnail_vips_set)
		   || nqiv_pruner_create_desc_datatype(
			   &common, &inside_hard, "surface", &desc->unload_surface, &desc->unload_surface_soft,
			   &desc->unload_thumbnail_surface, &desc->unload_thumbnail_surface_soft,
			   &desc->surface_set, &desc->thumbnail_surface_set)
		   || nqiv_pruner_create_desc_datatype(&common, &always_hard, "texture",
		                                       &desc->unload_texture, NULL,
		                                       &desc->unload_thumbnail_texture, NULL,
		                                       &desc->texture_set, &desc->thumbnail_texture_set)) {
			/* NOOP */
		} else if(nqiv_pruner_create_desc_toggle(&common, "unload", &inside_unload)) {
			if(inside_unload && set != NULL) {
				nqiv_log_write(logger, NQIV_LOG_DEBUG, "Disabling checks for unload\n");
				set = NULL;
				thumbnail_set = NULL;
			}
		} else if(nqiv_pruner_create_desc_datatype(&common, &inside_hard, "raw", NULL, NULL, NULL,
		                                           NULL, &dummy_set, &dummy_set)) {
			nqiv_log_write(logger, NQIV_LOG_WARNING, "'raw' is deprecated and has been skipped.\n");
		} else if(inside_unload) {
			nqiv_log_write(logger, NQIV_LOG_ERROR,
			               "Unload still enabled while parsing checks and comparisons at %s\n",
			               &text[idx]);
			return false;
		} else if(nqiv_pruner_check_token(logger, text, &idx, end, "sum")) {
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
		} else if(nqiv_pruner_check_token(logger, text, &idx, end, "or")) {
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
		} else if(nqiv_pruner_check_token(logger, text, &idx, end, "and")) {
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
		} else if(nqiv_pruner_check_token(logger, text, &idx, end, "not_animated")) {
			idx = nqiv_pruner_parse_check(
				logger, text, idx, end, inside_no, inside_image, inside_thumbnail,
				set != NULL ? &set->not_animated : &desc->vips_set.not_animated,
				thumbnail_set != NULL ? &thumbnail_set->not_animated
									  : &desc->thumbnail_vips_set.not_animated,
				nqiv_pruner_set_true);
			inside_no = false;
		} else if(set == NULL) {
			/* After this point, we need a data to run checks. */
			nqiv_log_write(logger, NQIV_LOG_ERROR,
			               "Failed to continue with unknown set target from %s\n", &text[idx]);
			return false;
		} else if(nqiv_pruner_check_token(logger, text, &idx, end, "loaded_ahead")) {
			idx = nqiv_pruner_parse_check(logger, text, idx, end, inside_no, inside_image,
			                              inside_thumbnail, &set->loaded_ahead,
			                              &thumbnail_set->loaded_ahead, nqiv_pruner_parse_int_pair);
			inside_no = false;
		} else if(nqiv_pruner_check_token(logger, text, &idx, end, "loaded_behind")) {
			idx = nqiv_pruner_parse_check(
				logger, text, idx, end, inside_no, inside_image, inside_thumbnail,
				&set->loaded_behind, &thumbnail_set->loaded_behind, nqiv_pruner_parse_int_pair);
			inside_no = false;
		} else if(nqiv_pruner_check_token(logger, text, &idx, end, "bytes_ahead")) {
			idx = nqiv_pruner_parse_check(logger, text, idx, end, inside_no, inside_image,
			                              inside_thumbnail, &set->bytes_ahead,
			                              &thumbnail_set->bytes_ahead, nqiv_pruner_parse_int_pair);
			inside_no = false;
		} else if(nqiv_pruner_check_token(logger, text, &idx, end, "bytes_behind")) {
			idx = nqiv_pruner_parse_check(logger, text, idx, end, inside_no, inside_image,
			                              inside_thumbnail, &set->bytes_behind,
			                              &thumbnail_set->bytes_behind, nqiv_pruner_parse_int_pair);
			inside_no = false;
		} else if(nqiv_pruner_check_token(logger, text, &idx, end, "self_opened")) {
			idx = nqiv_pruner_parse_check(logger, text, idx, end, inside_no, inside_image,
			                              inside_thumbnail, &set->loaded_self,
			                              &thumbnail_set->loaded_self, nqiv_pruner_set_true);
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

typedef struct nqiv_pruner_render_state
{
	bool  in_image;
	bool  in_thumbnail;
	bool  hard_unload;
	char* data_name;
} nqiv_pruner_render_state;

static int nqiv_pruner_update_render_state_form(nqiv_pruner_render_state* state,
                                                nqiv_array*               builder,
                                                const nqiv_pruner_render_state* new)
{
	bool success = true;
	if(new->in_image != state->in_image) {
		success = success
		          && nqiv_array_push_sprintf(builder, "%s ", new->in_image ? "image" : "no image");
	}
	if(new->in_thumbnail != state->in_thumbnail) {
		success = success
		          && nqiv_array_push_sprintf(builder, "%s ",
		                                     new->in_thumbnail ? "thumbnail" : "no thumbnail");
	}
	if(new->hard_unload != state->hard_unload) {
		success = success
		          && nqiv_array_push_sprintf(builder, "%s ", new->hard_unload ? "hard" : "no hard");
	}
	if(new->data_name != NULL
	   && (state->data_name == NULL || strcmp(new->data_name, state->data_name) != 0)) {
		success = success && nqiv_array_push_sprintf(builder, "%s ", new->data_name);
	}
	memcpy(state, new, sizeof(nqiv_pruner_render_state));
	return success;
}

static int nqiv_pruner_desc_dataset_to_string(nqiv_pruner_render_state*       state,
                                              const nqiv_pruner_desc_dataset* set,
                                              nqiv_array*                     builder,
                                              const nqiv_pruner_render_state* new_state)
{
	bool success = true;
	/* Do we have any kinds of checks? */
	if(set->not_animated.active || set->loaded_self.active || set->loaded_ahead.active
	   || set->loaded_behind.active || set->bytes_ahead.active || set->bytes_behind.active) {
		success = success && nqiv_pruner_update_render_state_form(state, builder, new_state);
	}
	if(set->not_animated.active) {
		success = success && nqiv_array_push_sprintf(builder, "not_animated ");
	}
	if(set->loaded_self.active) {
		success = success && nqiv_array_push_sprintf(builder, "self_opened ");
	}
	if(set->loaded_ahead.active) {
		success = success
		          && nqiv_array_push_sprintf(builder, "loaded_ahead %d %d ",
		                                     set->loaded_ahead.condition.as_int_pair[0],
		                                     set->loaded_ahead.condition.as_int_pair[1]);
	}
	if(set->loaded_behind.active) {
		success = success
		          && nqiv_array_push_sprintf(builder, "loaded_behind %d %d ",
		                                     set->loaded_behind.condition.as_int_pair[0],
		                                     set->loaded_behind.condition.as_int_pair[1]);
	}
	if(set->bytes_ahead.active) {
		success = success
		          && nqiv_array_push_sprintf(builder, "bytes_ahead %d %d ",
		                                     set->bytes_ahead.condition.as_int_pair[0],
		                                     set->bytes_ahead.condition.as_int_pair[1]);
	}
	if(set->bytes_behind.active) {
		success = success
		          && nqiv_array_push_sprintf(builder, "bytes_behind %d %d ",
		                                     set->bytes_behind.condition.as_int_pair[0],
		                                     set->bytes_behind.condition.as_int_pair[1]);
	}
	return success;
}

static bool nqiv_pruner_desc_datapoint_int(const nqiv_pruner_desc_datapoint* first,
                                           const nqiv_pruner_desc_datapoint* second)
{
	return first->active == second->active
	       && first->condition.as_int_pair[0] == second->condition.as_int_pair[0]
	       && first->condition.as_int_pair[1] == second->condition.as_int_pair[1]
	       && first->value.as_int == second->value.as_int;
}

static bool nqiv_pruner_desc_datapoint_bool(const nqiv_pruner_desc_datapoint* first,
                                            const nqiv_pruner_desc_datapoint* second)
{
	return first->active == second->active && first->value.as_bool == second->value.as_bool;
}

bool nqiv_pruner_desc_dataset_compare(const nqiv_pruner_desc_dataset* first,
                                      const nqiv_pruner_desc_dataset* second)
{
	return nqiv_pruner_desc_datapoint_bool(&first->not_animated, &second->not_animated)
	       && nqiv_pruner_desc_datapoint_bool(&first->loaded_self, &second->loaded_self)
	       && nqiv_pruner_desc_datapoint_int(&first->loaded_ahead, &second->loaded_ahead)
	       && nqiv_pruner_desc_datapoint_int(&first->loaded_behind, &second->loaded_behind)
	       && nqiv_pruner_desc_datapoint_int(&first->bytes_ahead, &second->bytes_ahead)
	       && nqiv_pruner_desc_datapoint_int(&first->bytes_behind, &second->bytes_behind);
}

bool nqiv_pruner_desc_compare(const nqiv_pruner_desc* first, const nqiv_pruner_desc* second)
{
	/* We're primarily comparing conditions and operations, not ephemeral
	 * information about the state of the pruner. */
	return first->counter == second->counter && first->state_check.idx == second->state_check.idx
	       && first->state_check.montage_start == second->state_check.montage_start
	       && first->state_check.montage_end == second->state_check.montage_end
	       && first->state_check.total_sum == second->state_check.total_sum
	       && first->state_check.or_result == second->state_check.or_result
	       && first->state_check.and_result == second->state_check.and_result
	       && first->state_check.and_is_set == second->state_check.and_is_set
	       && nqiv_pruner_desc_dataset_compare(&first->vips_set, &second->vips_set)
	       && nqiv_pruner_desc_dataset_compare(&first->surface_set, &second->surface_set)
	       && nqiv_pruner_desc_dataset_compare(&first->texture_set, &second->texture_set)
	       && nqiv_pruner_desc_dataset_compare(&first->thumbnail_vips_set,
	                                           &second->thumbnail_vips_set)
	       && nqiv_pruner_desc_dataset_compare(&first->thumbnail_surface_set,
	                                           &second->thumbnail_surface_set)
	       && nqiv_pruner_desc_dataset_compare(&first->thumbnail_texture_set,
	                                           &second->thumbnail_texture_set)
	       && first->unload_vips == second->unload_vips
	       && first->unload_surface == second->unload_surface
	       && first->unload_texture == second->unload_texture
	       && first->unload_thumbnail_vips == second->unload_thumbnail_vips
	       && first->unload_thumbnail_surface == second->unload_thumbnail_surface
	       && first->unload_thumbnail_texture == second->unload_thumbnail_texture
	       && first->unload_vips_soft == second->unload_vips_soft
	       && first->unload_surface_soft == second->unload_surface_soft
	       && first->unload_thumbnail_vips_soft == second->unload_thumbnail_vips_soft
	       && first->unload_thumbnail_surface_soft == second->unload_thumbnail_surface_soft;
}

static bool nqiv_pruner_desc_dataset_pair_to_string(nqiv_pruner_render_state*       state,
                                                    char*                           name,
                                                    const nqiv_pruner_desc_dataset* image_set,
                                                    const nqiv_pruner_desc_dataset* thumbnail_set,
                                                    nqiv_array*                     builder)
{
	int success = true;
	/* If the checks exist and are the same for images and thumbnail, only
	 * render them once. */
	if(nqiv_pruner_desc_dataset_compare(image_set, thumbnail_set)
	   && (image_set->not_animated.active || image_set->loaded_self.active
	       || image_set->loaded_ahead.active || image_set->loaded_behind.active
	       || image_set->bytes_ahead.active || image_set->bytes_behind.active)) {
		success = success
		          && nqiv_pruner_desc_dataset_to_string(
					  state, image_set, builder,
					  &(nqiv_pruner_render_state){.in_image = true,
		                                          .in_thumbnail = true,
		                                          .hard_unload = state->hard_unload,
		                                          .data_name = name});
	} else {
		success = success
		          && nqiv_pruner_desc_dataset_to_string(
					  state, image_set, builder,
					  &(nqiv_pruner_render_state){.in_image = true,
		                                          .in_thumbnail = false,
		                                          .hard_unload = state->hard_unload,
		                                          .data_name = name});
		success = success
		          && nqiv_pruner_desc_dataset_to_string(
					  state, thumbnail_set, builder,
					  &(nqiv_pruner_render_state){.in_image = false,
		                                          .in_thumbnail = true,
		                                          .hard_unload = state->hard_unload,
		                                          .data_name = name});
	}
	return success;
}

static bool nqiv_pruner_unload_pair_to_string(nqiv_pruner_render_state* state,
                                              char*                     name,
                                              const bool                image_unload,
                                              const bool                thumbnail_unload,
                                              const bool                hard_unload,
                                              nqiv_array*               builder)
{
	int success = true;
	if(image_unload || thumbnail_unload) {
		success = success
		          && nqiv_pruner_update_render_state_form(
					  state, builder,
					  &(nqiv_pruner_render_state){.in_image = image_unload,
		                                          .in_thumbnail = thumbnail_unload,
		                                          .hard_unload = hard_unload,
		                                          .data_name = name});
	}
	return success;
}

bool nqiv_pruner_desc_to_string(const nqiv_pruner_desc* desc, char* buf)
{
	nqiv_array builder;
	nqiv_array_inherit(&builder, buf, sizeof(char), NQIV_PRUNER_DESC_STRLEN);
	bool result = true;
	if((desc->counter & NQIV_PRUNER_COUNT_OP_SUM) != 0) {
		result =
			result && nqiv_array_push_sprintf(&builder, "sum %d ", desc->state_check.total_sum);
	}
	if((desc->counter & NQIV_PRUNER_COUNT_OP_OR) != 0) {
		result = result && nqiv_array_push_sprintf(&builder, "or ");
	}
	if((desc->counter & NQIV_PRUNER_COUNT_OP_AND) != 0) {
		result = result && nqiv_array_push_sprintf(&builder, "and ");
	}
	/* Track the state of the description to reduce redundant instructions. */
	nqiv_pruner_render_state render_state = {.in_image = true,
	                                         .in_thumbnail = false,
	                                         .hard_unload = false,
	                                         .data_name = NULL};
	result = result
	         && nqiv_pruner_desc_dataset_pair_to_string(&render_state, "vips", &desc->vips_set,
	                                                    &desc->thumbnail_vips_set, &builder);
	result =
		result
		&& nqiv_pruner_desc_dataset_pair_to_string(&render_state, "surface", &desc->surface_set,
	                                               &desc->thumbnail_surface_set, &builder);
	result =
		result
		&& nqiv_pruner_desc_dataset_pair_to_string(&render_state, "texture", &desc->texture_set,
	                                               &desc->thumbnail_texture_set, &builder);
	/* Do we have anything to unload? */
	if(desc->unload_vips || desc->unload_surface || desc->unload_texture
	   || desc->unload_thumbnail_vips || desc->unload_thumbnail_surface
	   || desc->unload_thumbnail_texture || desc->unload_vips_soft || desc->unload_surface_soft
	   || desc->unload_thumbnail_vips_soft || desc->unload_thumbnail_surface_soft) {
		result = result && nqiv_array_push_sprintf(&builder, "unload ");
	}
	/* Add hard unloads. */
	if(desc->unload_vips || desc->unload_surface || desc->unload_texture
	   || desc->unload_thumbnail_vips || desc->unload_thumbnail_surface
	   || desc->unload_thumbnail_texture) {
		result = result
		         && nqiv_pruner_unload_pair_to_string(&render_state, "vips", desc->unload_vips,
		                                              desc->unload_thumbnail_vips, true, &builder);
		result =
			result
			&& nqiv_pruner_unload_pair_to_string(&render_state, "surface", desc->unload_surface,
		                                         desc->unload_thumbnail_surface, true, &builder);
		result =
			result
			&& nqiv_pruner_unload_pair_to_string(&render_state, "texture", desc->unload_texture,
		                                         desc->unload_thumbnail_texture, true, &builder);
	}
	/* Add soft unloads. */
	if(desc->unload_vips_soft || desc->unload_surface_soft || desc->unload_thumbnail_vips_soft
	   || desc->unload_thumbnail_surface_soft) {
		result =
			result
			&& nqiv_pruner_unload_pair_to_string(&render_state, "vips", desc->unload_vips_soft,
		                                         desc->unload_thumbnail_vips_soft, false, &builder);
		result = result
		         && nqiv_pruner_unload_pair_to_string(
					 &render_state, "surface", desc->unload_surface_soft,
					 desc->unload_thumbnail_surface_soft, false, &builder);
	}
	/* There's a trailing space at the end of the string in anticipation of
	 * more things to append. When we're done, make it a NULL terminator. */
	if(result && buf[nqiv_array_get_last_idx(&builder)] == ' ') {
		buf[nqiv_array_get_last_idx(&builder)] = '\0';
	}
	return result;
}
