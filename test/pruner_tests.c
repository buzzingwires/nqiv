#include <string.h>
#include <assert.h>

#include "../src/event.h"
#include "../src/queue.h"
#include "../src/state.h"
#include "../src/logging.h"
#include "../src/image.h"
#include "../src/montage.h"
#include "../src/pruner.h"

#include "pruner_tests.h"

void pruner_test_default(void)
{
	nqiv_log_ctx logger = {0};

	nqiv_log_init(&logger);
	assert(!nqiv_log_has_error(&logger));

	nqiv_pruner_desc               desc = {0};
	nqiv_pruner_desc               cmp_desc = {0};
	char                           desc_str[NQIV_PRUNER_DESC_STRLEN + 1] = {0};
	const nqiv_pruner_desc_dataset empty_dataset = {0};

	memset(&desc, 0, sizeof(nqiv_pruner_desc));
	memset(&cmp_desc, 0, sizeof(nqiv_pruner_desc));
	memset(desc_str, 0, NQIV_PRUNER_DESC_STRLEN);
	assert(nqiv_pruner_create_desc(
		&logger, "or thumbnail no image texture self_opened unload surface vips", &desc));
	nqiv_pruner_desc_to_string(&desc, desc_str);
	assert(nqiv_pruner_create_desc(&logger, desc_str, &cmp_desc));
	assert(nqiv_pruner_desc_compare(&desc, &cmp_desc));
	assert(desc.counter == NQIV_PRUNER_COUNT_OP_OR);
	assert(desc.state_check.or_result == true);
	assert(desc.state_check.and_result == false);
	assert(desc.state_check.total_sum == 0);
	assert(nqiv_pruner_desc_dataset_compare(&desc.vips_set, &empty_dataset));
	assert(nqiv_pruner_desc_dataset_compare(&desc.surface_set, &empty_dataset));
	assert(nqiv_pruner_desc_dataset_compare(&desc.texture_set, &empty_dataset));
	assert(nqiv_pruner_desc_dataset_compare(&desc.thumbnail_vips_set, &empty_dataset));
	assert(nqiv_pruner_desc_dataset_compare(&desc.thumbnail_surface_set, &empty_dataset));
	assert(desc.thumbnail_texture_set.loaded_self.active);
	assert(!desc.unload_vips);
	assert(!desc.unload_surface);
	assert(!desc.unload_texture);
	assert(!desc.unload_thumbnail_vips);
	assert(!desc.unload_thumbnail_surface);
	assert(!desc.unload_thumbnail_texture);
	assert(!desc.unload_vips_soft);
	assert(!desc.unload_surface_soft);
	assert(desc.unload_thumbnail_vips_soft);
	assert(desc.unload_thumbnail_surface_soft);

	memset(&desc, 0, sizeof(nqiv_pruner_desc));
	memset(&cmp_desc, 0, sizeof(nqiv_pruner_desc));
	memset(desc_str, 0, NQIV_PRUNER_DESC_STRLEN);
	assert(nqiv_pruner_create_desc(
		&logger, "and no thumbnail image texture self_opened not_animated unload surface vips",
		&desc));
	nqiv_pruner_desc_to_string(&desc, desc_str);
	assert(nqiv_pruner_create_desc(&logger, desc_str, &cmp_desc));
	assert(nqiv_pruner_desc_compare(&desc, &cmp_desc));
	assert(desc.counter == NQIV_PRUNER_COUNT_OP_AND);
	assert(desc.state_check.or_result == false);
	assert(desc.state_check.and_result == true);
	assert(desc.state_check.total_sum == 0);

	assert(nqiv_pruner_desc_dataset_compare(&desc.vips_set, &empty_dataset));
	assert(nqiv_pruner_desc_dataset_compare(&desc.surface_set, &empty_dataset));
	assert(desc.texture_set.loaded_self.active);
	assert(desc.texture_set.not_animated.active);
	assert(nqiv_pruner_desc_dataset_compare(&desc.thumbnail_vips_set, &empty_dataset));
	assert(nqiv_pruner_desc_dataset_compare(&desc.thumbnail_surface_set, &empty_dataset));
	assert(nqiv_pruner_desc_dataset_compare(&desc.thumbnail_texture_set, &empty_dataset));
	assert(!desc.unload_vips);
	assert(!desc.unload_surface);
	assert(!desc.unload_texture);
	assert(!desc.unload_thumbnail_vips);
	assert(!desc.unload_thumbnail_surface);
	assert(!desc.unload_thumbnail_texture);
	assert(desc.unload_vips_soft);
	assert(desc.unload_surface_soft);
	assert(!desc.unload_thumbnail_vips_soft);
	assert(!desc.unload_thumbnail_surface_soft);

	memset(&desc, 0, sizeof(nqiv_pruner_desc));
	memset(&cmp_desc, 0, sizeof(nqiv_pruner_desc));
	memset(desc_str, 0, NQIV_PRUNER_DESC_STRLEN);
	assert(nqiv_pruner_create_desc(
		&logger, "or no thumbnail image texture self_opened unload surface", &desc));
	nqiv_pruner_desc_to_string(&desc, desc_str);
	assert(nqiv_pruner_create_desc(&logger, desc_str, &cmp_desc));
	assert(nqiv_pruner_desc_compare(&desc, &cmp_desc));
	assert(desc.counter == NQIV_PRUNER_COUNT_OP_OR);
	assert(desc.state_check.or_result == true);
	assert(desc.state_check.and_result == false);
	assert(desc.state_check.total_sum == 0);
	assert(nqiv_pruner_desc_dataset_compare(&desc.vips_set, &empty_dataset));
	assert(nqiv_pruner_desc_dataset_compare(&desc.surface_set, &empty_dataset));
	assert(desc.texture_set.loaded_self.active);
	assert(nqiv_pruner_desc_dataset_compare(&desc.thumbnail_vips_set, &empty_dataset));
	assert(nqiv_pruner_desc_dataset_compare(&desc.thumbnail_surface_set, &empty_dataset));
	assert(nqiv_pruner_desc_dataset_compare(&desc.thumbnail_texture_set, &empty_dataset));
	assert(!desc.unload_vips);
	assert(!desc.unload_surface);
	assert(!desc.unload_texture);
	assert(!desc.unload_thumbnail_vips);
	assert(!desc.unload_thumbnail_surface);
	assert(!desc.unload_thumbnail_texture);
	assert(!desc.unload_vips_soft);
	assert(desc.unload_surface_soft);
	assert(!desc.unload_thumbnail_vips_soft);
	assert(!desc.unload_thumbnail_surface_soft);

	memset(&desc, 0, sizeof(nqiv_pruner_desc));
	memset(&cmp_desc, 0, sizeof(nqiv_pruner_desc));
	memset(desc_str, 0, NQIV_PRUNER_DESC_STRLEN);
	assert(nqiv_pruner_create_desc(&logger,
	                               "and thumbnail no image texture self_opened image no thumbnail "
	                               "not_animated hard unload image thumbnail surface vips",
	                               &desc));
	nqiv_pruner_desc_to_string(&desc, desc_str);
	assert(nqiv_pruner_create_desc(&logger, desc_str, &cmp_desc));
	assert(nqiv_pruner_desc_compare(&desc, &cmp_desc));
	assert(desc.counter == NQIV_PRUNER_COUNT_OP_AND);
	assert(desc.state_check.or_result == false);
	assert(desc.state_check.and_result == true);
	assert(desc.state_check.total_sum == 0);
	assert(nqiv_pruner_desc_dataset_compare(&desc.vips_set, &empty_dataset));
	assert(nqiv_pruner_desc_dataset_compare(&desc.surface_set, &empty_dataset));
	assert(desc.texture_set.not_animated.active);
	assert(nqiv_pruner_desc_dataset_compare(&desc.thumbnail_vips_set, &empty_dataset));
	assert(nqiv_pruner_desc_dataset_compare(&desc.thumbnail_surface_set, &empty_dataset));
	assert(desc.thumbnail_texture_set.loaded_self.active);
	assert(desc.unload_vips);
	assert(desc.unload_surface);
	assert(!desc.unload_texture);
	assert(desc.unload_thumbnail_vips);
	assert(desc.unload_thumbnail_surface);
	assert(!desc.unload_thumbnail_texture);
	assert(!desc.unload_vips_soft);
	assert(!desc.unload_surface_soft);
	assert(!desc.unload_thumbnail_vips_soft);
	assert(!desc.unload_thumbnail_surface_soft);

	memset(&desc, 0, sizeof(nqiv_pruner_desc));
	memset(&cmp_desc, 0, sizeof(nqiv_pruner_desc));
	memset(desc_str, 0, NQIV_PRUNER_DESC_STRLEN);
	assert(nqiv_pruner_create_desc(
		&logger,
		"or thumbnail image texture loaded_behind 30 0 loaded_ahead 30 0 surface loaded_behind 30 "
		"0 loaded_ahead 30 0 vips loaded_behind 30 0 "
		"loaded_ahead 30 0 hard unload texture surface vips",
		&desc));
	nqiv_pruner_desc_to_string(&desc, desc_str);
	assert(nqiv_pruner_create_desc(&logger, desc_str, &cmp_desc));
	assert(nqiv_pruner_desc_compare(&desc, &cmp_desc));
	assert(desc.counter == NQIV_PRUNER_COUNT_OP_OR);
	assert(desc.state_check.or_result == true);
	assert(desc.state_check.and_result == false);
	assert(desc.state_check.total_sum == 0);
	assert(desc.thumbnail_vips_set.loaded_ahead.active);
	assert(desc.thumbnail_vips_set.loaded_ahead.condition.as_int_pair[0] == 30);
	assert(desc.thumbnail_vips_set.loaded_ahead.condition.as_int_pair[1] == 0);
	assert(desc.thumbnail_vips_set.loaded_behind.active);
	assert(desc.thumbnail_vips_set.loaded_behind.condition.as_int_pair[0] == 30);
	assert(desc.thumbnail_vips_set.loaded_behind.condition.as_int_pair[1] == 0);
	assert(desc.thumbnail_surface_set.loaded_ahead.active);
	assert(desc.thumbnail_surface_set.loaded_ahead.condition.as_int_pair[0] == 30);
	assert(desc.thumbnail_surface_set.loaded_ahead.condition.as_int_pair[1] == 0);
	assert(desc.thumbnail_surface_set.loaded_behind.active);
	assert(desc.thumbnail_surface_set.loaded_behind.condition.as_int_pair[0] == 30);
	assert(desc.thumbnail_surface_set.loaded_behind.condition.as_int_pair[1] == 0);
	assert(desc.thumbnail_texture_set.loaded_ahead.active);
	assert(desc.thumbnail_texture_set.loaded_ahead.condition.as_int_pair[0] == 30);
	assert(desc.thumbnail_texture_set.loaded_ahead.condition.as_int_pair[1] == 0);
	assert(desc.thumbnail_texture_set.loaded_behind.active);
	assert(desc.thumbnail_texture_set.loaded_behind.condition.as_int_pair[0] == 30);
	assert(desc.thumbnail_texture_set.loaded_behind.condition.as_int_pair[1] == 0);
	assert(desc.vips_set.loaded_ahead.active);
	assert(desc.vips_set.loaded_ahead.condition.as_int_pair[0] == 30);
	assert(desc.vips_set.loaded_ahead.condition.as_int_pair[1] == 0);
	assert(desc.vips_set.loaded_behind.active);
	assert(desc.vips_set.loaded_behind.condition.as_int_pair[0] == 30);
	assert(desc.vips_set.loaded_behind.condition.as_int_pair[1] == 0);
	assert(desc.surface_set.loaded_ahead.active);
	assert(desc.surface_set.loaded_ahead.condition.as_int_pair[0] == 30);
	assert(desc.surface_set.loaded_ahead.condition.as_int_pair[1] == 0);
	assert(desc.surface_set.loaded_behind.active);
	assert(desc.surface_set.loaded_behind.condition.as_int_pair[0] == 30);
	assert(desc.surface_set.loaded_behind.condition.as_int_pair[1] == 0);
	assert(desc.texture_set.loaded_ahead.active);
	assert(desc.texture_set.loaded_ahead.condition.as_int_pair[0] == 30);
	assert(desc.texture_set.loaded_ahead.condition.as_int_pair[1] == 0);
	assert(desc.texture_set.loaded_behind.active);
	assert(desc.texture_set.loaded_behind.condition.as_int_pair[0] == 30);
	assert(desc.texture_set.loaded_behind.condition.as_int_pair[1] == 0);
	assert(desc.unload_vips);
	assert(desc.unload_surface);
	assert(desc.unload_texture);
	assert(desc.unload_thumbnail_vips);
	assert(desc.unload_thumbnail_surface);
	assert(desc.unload_thumbnail_texture);
	assert(!desc.unload_vips_soft);
	assert(!desc.unload_surface_soft);
	assert(!desc.unload_thumbnail_vips_soft);
	assert(!desc.unload_thumbnail_surface_soft);

	memset(&desc, 0, sizeof(nqiv_pruner_desc));
	memset(&cmp_desc, 0, sizeof(nqiv_pruner_desc));
	memset(desc_str, 0, NQIV_PRUNER_DESC_STRLEN);
	assert(nqiv_pruner_create_desc(
		&logger,
		"or sum 1000000000 thumbnail image texture bytes_ahead 0 1000000000 bytes_behind 0 "
		"1000000000 surface bytes_ahead 0 1000000000 bytes_behind 0 1000000000 "
		"vips bytes_ahead 0 1000000000 bytes_behind 0 "
		"1000000000 hard unload texture surface vips",
		&desc));
	nqiv_pruner_desc_to_string(&desc, desc_str);
	assert(nqiv_pruner_create_desc(&logger, desc_str, &cmp_desc));
	assert(nqiv_pruner_desc_compare(&desc, &cmp_desc));
	assert((desc.counter & NQIV_PRUNER_COUNT_OP_OR) == NQIV_PRUNER_COUNT_OP_OR);
	assert((desc.counter & NQIV_PRUNER_COUNT_OP_SUM) == NQIV_PRUNER_COUNT_OP_SUM);
	assert(desc.state_check.or_result == true);
	assert(desc.state_check.and_result == false);
	assert(desc.state_check.total_sum == 1000000000);
	assert(desc.thumbnail_vips_set.bytes_ahead.active);
	assert(desc.thumbnail_vips_set.bytes_ahead.condition.as_int_pair[0] == 0);
	assert(desc.thumbnail_vips_set.bytes_ahead.condition.as_int_pair[1] == 1000000000);
	assert(desc.thumbnail_vips_set.bytes_behind.active);
	assert(desc.thumbnail_vips_set.bytes_behind.condition.as_int_pair[0] == 0);
	assert(desc.thumbnail_vips_set.bytes_behind.condition.as_int_pair[1] == 1000000000);
	assert(desc.thumbnail_surface_set.bytes_ahead.active);
	assert(desc.thumbnail_surface_set.bytes_ahead.condition.as_int_pair[0] == 0);
	assert(desc.thumbnail_surface_set.bytes_ahead.condition.as_int_pair[1] == 1000000000);
	assert(desc.thumbnail_surface_set.bytes_behind.active);
	assert(desc.thumbnail_surface_set.bytes_behind.condition.as_int_pair[0] == 0);
	assert(desc.thumbnail_surface_set.bytes_behind.condition.as_int_pair[1] == 1000000000);
	assert(desc.thumbnail_texture_set.bytes_ahead.active);
	assert(desc.thumbnail_texture_set.bytes_ahead.condition.as_int_pair[0] == 0);
	assert(desc.thumbnail_texture_set.bytes_ahead.condition.as_int_pair[1] == 1000000000);
	assert(desc.thumbnail_texture_set.bytes_behind.active);
	assert(desc.thumbnail_texture_set.bytes_behind.condition.as_int_pair[0] == 0);
	assert(desc.thumbnail_texture_set.bytes_behind.condition.as_int_pair[1] == 1000000000);
	assert(desc.vips_set.bytes_ahead.active);
	assert(desc.vips_set.bytes_ahead.condition.as_int_pair[0] == 0);
	assert(desc.vips_set.bytes_ahead.condition.as_int_pair[1] == 1000000000);
	assert(desc.vips_set.bytes_behind.active);
	assert(desc.vips_set.bytes_behind.condition.as_int_pair[0] == 0);
	assert(desc.vips_set.bytes_behind.condition.as_int_pair[1] == 1000000000);
	assert(desc.surface_set.bytes_ahead.active);
	assert(desc.surface_set.bytes_ahead.condition.as_int_pair[0] == 0);
	assert(desc.surface_set.bytes_ahead.condition.as_int_pair[1] == 1000000000);
	assert(desc.surface_set.bytes_behind.active);
	assert(desc.surface_set.bytes_behind.condition.as_int_pair[0] == 0);
	assert(desc.surface_set.bytes_behind.condition.as_int_pair[1] == 1000000000);
	assert(desc.texture_set.bytes_ahead.active);
	assert(desc.texture_set.bytes_ahead.condition.as_int_pair[0] == 0);
	assert(desc.texture_set.bytes_ahead.condition.as_int_pair[1] == 1000000000);
	assert(desc.texture_set.bytes_behind.active);
	assert(desc.texture_set.bytes_behind.condition.as_int_pair[0] == 0);
	assert(desc.texture_set.bytes_behind.condition.as_int_pair[1] == 1000000000);
	assert(desc.unload_vips);
	assert(desc.unload_surface);
	assert(desc.unload_texture);
	assert(desc.unload_thumbnail_vips);
	assert(desc.unload_thumbnail_surface);
	assert(desc.unload_thumbnail_texture);
	assert(!desc.unload_vips_soft);
	assert(!desc.unload_surface_soft);
	assert(!desc.unload_thumbnail_vips_soft);
	assert(!desc.unload_thumbnail_surface_soft);
	nqiv_log_destroy(&logger);
}

typedef struct prune_effects
{
	bool load_thumbnail_vips;
	bool load_thumbnail_surface;
	bool load_image_vips;
	bool load_image_surface;
	bool unload_thumbnail_vips;
	bool unload_thumbnail_surface;
	bool unload_image_vips;
	bool unload_image_surface;
	bool hard_unload_thumbnail_vips;
	bool hard_unload_thumbnail_surface;
	bool hard_unload_image_vips;
	bool hard_unload_image_surface;
	bool animated_image;
	bool animated_thumbnail;
	int  prune_count;
} prune_effects;

void pruner_test_check_instance(const char*          pruner_string,
                                const int            image_count,
                                const int            montage_position,
                                const prune_effects* effects)
{
	nqiv_log_ctx        logger = {0};
	nqiv_priority_queue queue = {0};
	nqiv_image_manager  images = {0};
	nqiv_montage_state  montage = {0};
	nqiv_pruner         pruner = {0};
	int                 c;

	nqiv_log_init(&logger);
	nqiv_log_set_prefix_format(&logger, "#level# #time:%Y-%m-%d %T%z# ");
	nqiv_log_add_stream(&logger, stderr);
	logger.level = NQIV_LOG_ERROR;
	assert(!nqiv_log_has_error(&logger));

	assert(nqiv_priority_queue_init(&queue, &logger, sizeof(nqiv_event), STARTING_QUEUE_LENGTH,
	                                THREAD_QUEUE_BIN_COUNT));

	assert(nqiv_image_manager_init(&images, &logger, STARTING_QUEUE_LENGTH));
	assert(images.thumbnail.size == 256);
	for(c = image_count; c > 0; --c) {
		nqiv_image* img;
		assert(nqiv_image_manager_append(&images, "DEADBEEF"));
		assert(nqiv_array_get(images.images, nqiv_array_get_last_idx(images.images), &img));
		img->thumbnail.effective_height = 1;
		img->thumbnail.effective_width = 1;
		img->thumbnail.vips = effects->load_thumbnail_vips ? (void*)0xDEADBEEF : NULL;
		img->thumbnail.data = effects->load_thumbnail_surface ? (void*)0xDEADBEEF : NULL;
		img->thumbnail.surface = effects->load_thumbnail_surface ? (void*)0xDEADBEEF : NULL;
		img->thumbnail.animation.exists = effects->animated_thumbnail;
		img->image.effective_height = 1;
		img->image.effective_width = 1;
		img->image.vips = effects->load_image_vips ? (void*)0xDEADBEEF : NULL;
		img->image.data = effects->load_image_surface ? (void*)0xDEADBEEF : NULL;
		img->image.surface = effects->load_image_surface ? (void*)0xDEADBEEF : NULL;
		img->image.animation.exists = effects->animated_image;
	}
	assert(montage_position >= 0);
	assert(montage_position <= nqiv_array_get_units_count(images.images));

	montage.logger = &logger;
	montage.images = &images;
	nqiv_montage_calculate_dimensions(&montage, 800, 600);
	assert(montage.dimensions.count_per_row == 3);
	assert(montage.dimensions.count == 6);
	nqiv_montage_set_selection(&montage, montage_position);

	assert(nqiv_pruner_init(&pruner, &logger, STARTING_QUEUE_LENGTH));
	nqiv_pruner_desc desc = {0};
	assert(nqiv_pruner_create_desc(&logger, pruner_string, &desc));
	assert(nqiv_pruner_append(&pruner, &desc));

	assert(effects->prune_count == nqiv_pruner_run(&pruner, &montage, &images, &queue));
	nqiv_event e = {0};
	while(nqiv_priority_queue_pop(&queue, &e)) {
		assert(e.type == NQIV_EVENT_IMAGE_LOAD);
		assert(e.transaction_group == 0);
		assert(e.options.image_load.image != NULL);
		assert(!e.options.image_load.set_thumbnail_path);
		assert(!e.options.image_load.create_thumbnail);
		assert(!e.options.image_load.borrow_thumbnail_dimension_metadata);
		assert(!e.options.image_load.image_options.clear_error);
		assert(e.options.image_load.image_options.unload);
		assert(!e.options.image_load.image_options.first_frame);
		assert(!e.options.image_load.image_options.next_frame);
		assert(e.options.image_load.image_options.vips == effects->hard_unload_image_vips);
		assert(e.options.image_load.image_options.vips_soft == effects->unload_image_vips);
		assert(e.options.image_load.image_options.surface == effects->hard_unload_image_surface);
		assert(e.options.image_load.image_options.surface_soft == effects->unload_image_surface);
		assert(!e.options.image_load.thumbnail_options.clear_error);
		assert(e.options.image_load.thumbnail_options.unload);
		assert(!e.options.image_load.thumbnail_options.first_frame);
		assert(!e.options.image_load.thumbnail_options.next_frame);
		assert(e.options.image_load.thumbnail_options.vips == effects->hard_unload_thumbnail_vips);
		assert(e.options.image_load.thumbnail_options.vips_soft == effects->unload_thumbnail_vips);
		assert(e.options.image_load.thumbnail_options.surface
		       == effects->hard_unload_thumbnail_surface);
		assert(e.options.image_load.thumbnail_options.surface_soft
		       == effects->unload_thumbnail_surface);
		memset(&e, 0, sizeof(nqiv_event));
	}
	for(c = 0; c < image_count; ++c) {
		nqiv_image* img;
		assert(nqiv_array_get(images.images, c, &img));
		img->thumbnail.vips = NULL;
		img->thumbnail.data = NULL;
		img->thumbnail.surface = NULL;
		img->image.vips = NULL;
		img->image.data = NULL;
		img->image.surface = NULL;
	}

	nqiv_pruner_destroy(&pruner);
	nqiv_image_manager_destroy(&images);
	nqiv_priority_queue_destroy(&queue);
	nqiv_log_destroy(&logger);
}

void reset_prune_effects(prune_effects* effects)
{
	memset(effects, 0, sizeof(prune_effects));
}

void pruner_test_send_event(prune_effects* effects, const char* pruner, bool* load, bool* unload)
{
	reset_prune_effects(effects);
	*unload = true;
	pruner_test_check_instance(pruner, 0, 0, effects);
	*load = true;
	effects->prune_count = 1;
	pruner_test_check_instance(pruner, 1, 0, effects);
}

void pruner_test_send_all_events(prune_effects* effects)
{
	pruner_test_send_event(effects, "or vips self_opened unload vips surface",
	                       &effects->load_image_vips, &effects->unload_image_vips);
	pruner_test_send_event(effects, "or surface self_opened unload vips surface",
	                       &effects->load_image_surface, &effects->unload_image_surface);
	pruner_test_send_event(effects,
	                       "or no image thumbnail vips self_opened unload vips surface",
	                       &effects->load_thumbnail_vips, &effects->unload_thumbnail_vips);
	pruner_test_send_event(effects,
	                       "or no image thumbnail surface self_opened unload vips surface",
	                       &effects->load_thumbnail_surface, &effects->unload_thumbnail_surface);
	pruner_test_send_event(effects, "or vips self_opened hard unload vips surface",
	                       &effects->load_image_vips, &effects->hard_unload_image_vips);
	pruner_test_send_event(effects, "or surface self_opened hard unload vips surface",
	                       &effects->load_image_surface, &effects->hard_unload_image_surface);
	pruner_test_send_event(effects,
	                       "or no image thumbnail vips self_opened hard unload vips surface",
	                       &effects->load_thumbnail_vips, &effects->hard_unload_thumbnail_vips);
	pruner_test_send_event(
		effects, "or no image thumbnail surface self_opened hard unload vips surface",
		&effects->load_thumbnail_surface, &effects->hard_unload_thumbnail_surface);
}

void pruner_test_thumbnail_and_image(const prune_effects* effects,
                                     const char*          image_pruner,
                                     const char*          thumbnail_pruner,
                                     bool*                image_load,
                                     bool*                image_unload,
                                     bool*                thumbnail_load,
                                     bool*                thumbnail_unload,
                                     const int            position,
                                     const int            count)
{
	assert(!*image_load);
	assert(!*image_unload);
	assert(!*thumbnail_load);
	assert(!*thumbnail_unload);

	*image_load = true;
	*image_unload = true;
	pruner_test_check_instance(image_pruner, count, position, effects);

	*image_load = false;
	*image_unload = false;
	*thumbnail_load = true;
	*thumbnail_unload = true;
	pruner_test_check_instance(thumbnail_pruner, count, position, effects);

	*thumbnail_load = false;
	*thumbnail_unload = false;
}

void pruner_test_check(void)
{
	prune_effects effects = {0};
	pruner_test_send_all_events(&effects);

	reset_prune_effects(&effects);
	effects.prune_count = 1;
	pruner_test_thumbnail_and_image(
		&effects, "or not_animated unload vips", "or no image thumbnail not_animated unload vips",
		&effects.load_image_vips, &effects.unload_image_vips, &effects.load_thumbnail_vips,
		&effects.unload_thumbnail_vips, 0, 1);
	pruner_test_thumbnail_and_image(&effects, "or vips self_opened unload vips",
	                                "or no image thumbnail vips self_opened unload vips",
	                                &effects.load_image_vips, &effects.unload_image_vips,
	                                &effects.load_thumbnail_vips, &effects.unload_thumbnail_vips, 0,
	                                1);
	effects.prune_count = 1;
	pruner_test_thumbnail_and_image(&effects, "or vips loaded_ahead 1 2 unload vips",
	                                "or no image thumbnail vips loaded_ahead 1 2 unload vips",
	                                &effects.load_image_vips, &effects.unload_image_vips,
	                                &effects.load_thumbnail_vips, &effects.unload_thumbnail_vips, 0,
	                                10);
	effects.prune_count = 3;
	pruner_test_thumbnail_and_image(&effects, "or vips loaded_behind 1 2 unload vips",
	                                "or no image thumbnail vips loaded_behind 1 2 unload vips",
	                                &effects.load_image_vips, &effects.unload_image_vips,
	                                &effects.load_thumbnail_vips, &effects.unload_thumbnail_vips,
	                                11, 12);
	effects.prune_count = 1;
	pruner_test_thumbnail_and_image(&effects, "or vips bytes_ahead 1 8 unload vips",
	                                "or no image thumbnail vips bytes_ahead 1 8 unload vips",
	                                &effects.load_image_vips, &effects.unload_image_vips,
	                                &effects.load_thumbnail_vips, &effects.unload_thumbnail_vips, 0,
	                                10);
	effects.prune_count = 3;
	pruner_test_thumbnail_and_image(&effects, "or vips bytes_behind 1 8 unload vips",
	                                "or no image thumbnail vips bytes_behind 1 8 unload vips",
	                                &effects.load_image_vips, &effects.unload_image_vips,
	                                &effects.load_thumbnail_vips, &effects.unload_thumbnail_vips,
	                                11, 12);

	effects.prune_count = 1;
	effects.load_image_vips = true;
	effects.unload_image_vips = true;
	pruner_test_check_instance("and not_animated vips self_opened unload vips", 1, 0, &effects);

	effects.load_image_surface = true;
	pruner_test_check_instance("sum 1 vips loaded_ahead 0 0 surface loaded_ahead 0 0 unload vips",
	                           7, 0, &effects);

	effects.prune_count = 0;
	effects.load_image_surface = false;
	pruner_test_check_instance("or vips loaded_ahead 0 2 unload vips", 8, 1, &effects);
	pruner_test_check_instance("or vips bytes_ahead 0 8 unload vips", 8, 1, &effects);
	pruner_test_check_instance("or vips loaded_behind 0 6 unload vips", 12, 11, &effects);
	pruner_test_check_instance("or vips bytes_behind 0 24 unload vips", 12, 11, &effects);
}
