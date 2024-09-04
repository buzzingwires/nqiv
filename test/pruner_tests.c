#include <string.h>
#include <assert.h>

#include "../src/logging.h"
#include "../src/pruner.h"

#include "pruner_tests.h"

void pruner_test_default(void)
{
	nqiv_log_ctx logger = {0};

	nqiv_log_init(&logger);
	assert(!nqiv_log_has_error(&logger));

	nqiv_pruner_desc               desc = {0};
	nqiv_pruner_desc               cmp_desc = {0};
	char                           desc_str[NQIV_PRUNER_DESC_STRLEN] = {0};
	const nqiv_pruner_desc_dataset empty_dataset = {0};

	memset(&desc, 0, sizeof(nqiv_pruner_desc));
	memset(&cmp_desc, 0, sizeof(nqiv_pruner_desc));
	memset(desc_str, 0, NQIV_PRUNER_DESC_STRLEN);
	assert(nqiv_pruner_create_desc(
		&logger, "or thumbnail no image texture self_opened unload surface raw vips", &desc));
	nqiv_pruner_desc_to_string(&desc, desc_str);
	assert(nqiv_pruner_create_desc(&logger, desc_str, &cmp_desc));
	assert(memcmp(&desc, &cmp_desc, sizeof(nqiv_pruner_desc)) == 0);
	assert(desc.counter == NQIV_PRUNER_COUNT_OP_OR);
	assert(desc.state_check.or_result == true);
	assert(desc.state_check.and_result == false);
	assert(desc.state_check.total_sum == 0);
	assert(memcmp(&desc.vips_set, &empty_dataset, sizeof(nqiv_pruner_desc_dataset)) == 0);
	assert(memcmp(&desc.raw_set, &empty_dataset, sizeof(nqiv_pruner_desc_dataset)) == 0);
	assert(memcmp(&desc.surface_set, &empty_dataset, sizeof(nqiv_pruner_desc_dataset)) == 0);
	assert(memcmp(&desc.texture_set, &empty_dataset, sizeof(nqiv_pruner_desc_dataset)) == 0);
	assert(memcmp(&desc.thumbnail_vips_set, &empty_dataset, sizeof(nqiv_pruner_desc_dataset)) == 0);
	assert(memcmp(&desc.thumbnail_raw_set, &empty_dataset, sizeof(nqiv_pruner_desc_dataset)) == 0);
	assert(memcmp(&desc.thumbnail_surface_set, &empty_dataset, sizeof(nqiv_pruner_desc_dataset))
	       == 0);
	assert(desc.thumbnail_texture_set.loaded_self.active);
	assert(!desc.unload_vips);
	assert(!desc.unload_raw);
	assert(!desc.unload_surface);
	assert(!desc.unload_texture);
	assert(!desc.unload_thumbnail_vips);
	assert(!desc.unload_thumbnail_raw);
	assert(!desc.unload_thumbnail_surface);
	assert(!desc.unload_thumbnail_texture);
	assert(!desc.unload_vips_soft);
	assert(!desc.unload_raw_soft);
	assert(!desc.unload_surface_soft);
	assert(desc.unload_thumbnail_vips_soft);
	assert(desc.unload_thumbnail_raw_soft);
	assert(desc.unload_thumbnail_surface_soft);

	memset(&desc, 0, sizeof(nqiv_pruner_desc));
	memset(&cmp_desc, 0, sizeof(nqiv_pruner_desc));
	memset(desc_str, 0, NQIV_PRUNER_DESC_STRLEN);
	assert(nqiv_pruner_create_desc(
		&logger, "and no thumbnail image texture self_opened not_animated unload surface raw vips",
		&desc));
	nqiv_pruner_desc_to_string(&desc, desc_str);
	assert(nqiv_pruner_create_desc(&logger, desc_str, &cmp_desc));
	assert(memcmp(&desc, &cmp_desc, sizeof(nqiv_pruner_desc)) == 0);
	assert(desc.counter == NQIV_PRUNER_COUNT_OP_AND);
	assert(desc.state_check.or_result == false);
	assert(desc.state_check.and_result == true);
	assert(desc.state_check.total_sum == 0);
	assert(memcmp(&desc.vips_set, &empty_dataset, sizeof(nqiv_pruner_desc_dataset)) == 0);
	assert(memcmp(&desc.raw_set, &empty_dataset, sizeof(nqiv_pruner_desc_dataset)) == 0);
	assert(memcmp(&desc.surface_set, &empty_dataset, sizeof(nqiv_pruner_desc_dataset)) == 0);
	assert(desc.texture_set.loaded_self.active);
	assert(desc.texture_set.not_animated.active);
	assert(memcmp(&desc.thumbnail_vips_set, &empty_dataset, sizeof(nqiv_pruner_desc_dataset)) == 0);
	assert(memcmp(&desc.thumbnail_raw_set, &empty_dataset, sizeof(nqiv_pruner_desc_dataset)) == 0);
	assert(memcmp(&desc.thumbnail_surface_set, &empty_dataset, sizeof(nqiv_pruner_desc_dataset))
	       == 0);
	assert(memcmp(&desc.thumbnail_texture_set, &empty_dataset, sizeof(nqiv_pruner_desc_dataset))
	       == 0);
	assert(!desc.unload_vips);
	assert(!desc.unload_raw);
	assert(!desc.unload_surface);
	assert(!desc.unload_texture);
	assert(!desc.unload_thumbnail_vips);
	assert(!desc.unload_thumbnail_raw);
	assert(!desc.unload_thumbnail_surface);
	assert(!desc.unload_thumbnail_texture);
	assert(desc.unload_vips_soft);
	assert(desc.unload_raw_soft);
	assert(desc.unload_surface_soft);
	assert(!desc.unload_thumbnail_vips_soft);
	assert(!desc.unload_thumbnail_raw_soft);
	assert(!desc.unload_thumbnail_surface_soft);

	memset(&desc, 0, sizeof(nqiv_pruner_desc));
	memset(&cmp_desc, 0, sizeof(nqiv_pruner_desc));
	memset(desc_str, 0, NQIV_PRUNER_DESC_STRLEN);
	assert(nqiv_pruner_create_desc(
		&logger, "or no thumbnail image texture self_opened unload surface raw", &desc));
	nqiv_pruner_desc_to_string(&desc, desc_str);
	assert(nqiv_pruner_create_desc(&logger, desc_str, &cmp_desc));
	assert(memcmp(&desc, &cmp_desc, sizeof(nqiv_pruner_desc)) == 0);
	assert(desc.counter == NQIV_PRUNER_COUNT_OP_OR);
	assert(desc.state_check.or_result == true);
	assert(desc.state_check.and_result == false);
	assert(desc.state_check.total_sum == 0);
	assert(memcmp(&desc.vips_set, &empty_dataset, sizeof(nqiv_pruner_desc_dataset)) == 0);
	assert(memcmp(&desc.raw_set, &empty_dataset, sizeof(nqiv_pruner_desc_dataset)) == 0);
	assert(memcmp(&desc.surface_set, &empty_dataset, sizeof(nqiv_pruner_desc_dataset)) == 0);
	assert(desc.texture_set.loaded_self.active);
	assert(memcmp(&desc.thumbnail_vips_set, &empty_dataset, sizeof(nqiv_pruner_desc_dataset)) == 0);
	assert(memcmp(&desc.thumbnail_raw_set, &empty_dataset, sizeof(nqiv_pruner_desc_dataset)) == 0);
	assert(memcmp(&desc.thumbnail_surface_set, &empty_dataset, sizeof(nqiv_pruner_desc_dataset))
	       == 0);
	assert(memcmp(&desc.thumbnail_texture_set, &empty_dataset, sizeof(nqiv_pruner_desc_dataset))
	       == 0);
	assert(!desc.unload_vips);
	assert(!desc.unload_raw);
	assert(!desc.unload_surface);
	assert(!desc.unload_texture);
	assert(!desc.unload_thumbnail_vips);
	assert(!desc.unload_thumbnail_raw);
	assert(!desc.unload_thumbnail_surface);
	assert(!desc.unload_thumbnail_texture);
	assert(!desc.unload_vips_soft);
	assert(desc.unload_raw_soft);
	assert(desc.unload_surface_soft);
	assert(!desc.unload_thumbnail_vips_soft);
	assert(!desc.unload_thumbnail_raw_soft);
	assert(!desc.unload_thumbnail_surface_soft);

	memset(&desc, 0, sizeof(nqiv_pruner_desc));
	memset(&cmp_desc, 0, sizeof(nqiv_pruner_desc));
	memset(desc_str, 0, NQIV_PRUNER_DESC_STRLEN);
	assert(nqiv_pruner_create_desc(&logger,
	                               "and thumbnail no image texture self_opened image no thumbnail "
	                               "not_animated hard unload image thumbnail surface raw vips",
	                               &desc));
	nqiv_pruner_desc_to_string(&desc, desc_str);
	assert(nqiv_pruner_create_desc(&logger, desc_str, &cmp_desc));
	assert(memcmp(&desc, &cmp_desc, sizeof(nqiv_pruner_desc)) == 0);
	assert(desc.counter == NQIV_PRUNER_COUNT_OP_AND);
	assert(desc.state_check.or_result == false);
	assert(desc.state_check.and_result == true);
	assert(desc.state_check.total_sum == 0);
	assert(memcmp(&desc.vips_set, &empty_dataset, sizeof(nqiv_pruner_desc_dataset)) == 0);
	assert(memcmp(&desc.raw_set, &empty_dataset, sizeof(nqiv_pruner_desc_dataset)) == 0);
	assert(memcmp(&desc.surface_set, &empty_dataset, sizeof(nqiv_pruner_desc_dataset)) == 0);
	assert(desc.texture_set.not_animated.active);
	assert(memcmp(&desc.thumbnail_vips_set, &empty_dataset, sizeof(nqiv_pruner_desc_dataset)) == 0);
	assert(memcmp(&desc.thumbnail_raw_set, &empty_dataset, sizeof(nqiv_pruner_desc_dataset)) == 0);
	assert(memcmp(&desc.thumbnail_surface_set, &empty_dataset, sizeof(nqiv_pruner_desc_dataset))
	       == 0);
	assert(desc.thumbnail_texture_set.loaded_self.active);
	assert(desc.unload_vips);
	assert(desc.unload_raw);
	assert(desc.unload_surface);
	assert(!desc.unload_texture);
	assert(desc.unload_thumbnail_vips);
	assert(desc.unload_thumbnail_raw);
	assert(desc.unload_thumbnail_surface);
	assert(!desc.unload_thumbnail_texture);
	assert(!desc.unload_vips_soft);
	assert(!desc.unload_raw_soft);
	assert(!desc.unload_surface_soft);
	assert(!desc.unload_thumbnail_vips_soft);
	assert(!desc.unload_thumbnail_raw_soft);
	assert(!desc.unload_thumbnail_surface_soft);

	memset(&desc, 0, sizeof(nqiv_pruner_desc));
	memset(&cmp_desc, 0, sizeof(nqiv_pruner_desc));
	memset(desc_str, 0, NQIV_PRUNER_DESC_STRLEN);
	assert(nqiv_pruner_create_desc(
		&logger,
		"or thumbnail image texture loaded_behind 30 0 loaded_ahead 30 0 surface loaded_behind 30 "
	    "0 loaded_ahead 30 0 raw loaded_behind 30 0 loaded_ahead 30 0 vips loaded_behind 30 0 "
	    "loaded_ahead 30 0 hard unload texture surface raw vips",
		&desc));
	nqiv_pruner_desc_to_string(&desc, desc_str);
	assert(nqiv_pruner_create_desc(&logger, desc_str, &cmp_desc));
	assert(memcmp(&desc, &cmp_desc, sizeof(nqiv_pruner_desc)) == 0);
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
	assert(desc.thumbnail_raw_set.loaded_ahead.active);
	assert(desc.thumbnail_raw_set.loaded_ahead.condition.as_int_pair[0] == 30);
	assert(desc.thumbnail_raw_set.loaded_ahead.condition.as_int_pair[1] == 0);
	assert(desc.thumbnail_raw_set.loaded_behind.active);
	assert(desc.thumbnail_raw_set.loaded_behind.condition.as_int_pair[0] == 30);
	assert(desc.thumbnail_raw_set.loaded_behind.condition.as_int_pair[1] == 0);
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
	assert(desc.raw_set.loaded_ahead.active);
	assert(desc.raw_set.loaded_ahead.condition.as_int_pair[0] == 30);
	assert(desc.raw_set.loaded_ahead.condition.as_int_pair[1] == 0);
	assert(desc.raw_set.loaded_behind.active);
	assert(desc.raw_set.loaded_behind.condition.as_int_pair[0] == 30);
	assert(desc.raw_set.loaded_behind.condition.as_int_pair[1] == 0);
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
	assert(desc.unload_raw);
	assert(desc.unload_surface);
	assert(desc.unload_texture);
	assert(desc.unload_thumbnail_vips);
	assert(desc.unload_thumbnail_raw);
	assert(desc.unload_thumbnail_surface);
	assert(desc.unload_thumbnail_texture);
	assert(!desc.unload_vips_soft);
	assert(!desc.unload_raw_soft);
	assert(!desc.unload_surface_soft);
	assert(!desc.unload_thumbnail_vips_soft);
	assert(!desc.unload_thumbnail_raw_soft);
	assert(!desc.unload_thumbnail_surface_soft);

	memset(&desc, 0, sizeof(nqiv_pruner_desc));
	memset(&cmp_desc, 0, sizeof(nqiv_pruner_desc));
	memset(desc_str, 0, NQIV_PRUNER_DESC_STRLEN);
	assert(nqiv_pruner_create_desc(
		&logger,
		"or sum 1000000000 thumbnail image texture bytes_ahead 0 1000000000 bytes_behind 0 "
	    "1000000000 surface bytes_ahead 0 1000000000 bytes_behind 0 1000000000 raw bytes_ahead 0 "
	    "1000000000 bytes_behind 0 1000000000 vips bytes_ahead 0 1000000000 bytes_behind 0 "
	    "1000000000 hard unload texture surface raw vips",
		&desc));
	nqiv_pruner_desc_to_string(&desc, desc_str);
	assert(nqiv_pruner_create_desc(&logger, desc_str, &cmp_desc));
	assert(memcmp(&desc, &cmp_desc, sizeof(nqiv_pruner_desc)) == 0);
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
	assert(desc.thumbnail_raw_set.bytes_ahead.active);
	assert(desc.thumbnail_raw_set.bytes_ahead.condition.as_int_pair[0] == 0);
	assert(desc.thumbnail_raw_set.bytes_ahead.condition.as_int_pair[1] == 1000000000);
	assert(desc.thumbnail_raw_set.bytes_behind.active);
	assert(desc.thumbnail_raw_set.bytes_behind.condition.as_int_pair[0] == 0);
	assert(desc.thumbnail_raw_set.bytes_behind.condition.as_int_pair[1] == 1000000000);
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
	assert(desc.raw_set.bytes_ahead.active);
	assert(desc.raw_set.bytes_ahead.condition.as_int_pair[0] == 0);
	assert(desc.raw_set.bytes_ahead.condition.as_int_pair[1] == 1000000000);
	assert(desc.raw_set.bytes_behind.active);
	assert(desc.raw_set.bytes_behind.condition.as_int_pair[0] == 0);
	assert(desc.raw_set.bytes_behind.condition.as_int_pair[1] == 1000000000);
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
	assert(desc.unload_raw);
	assert(desc.unload_surface);
	assert(desc.unload_texture);
	assert(desc.unload_thumbnail_vips);
	assert(desc.unload_thumbnail_raw);
	assert(desc.unload_thumbnail_surface);
	assert(desc.unload_thumbnail_texture);
	assert(!desc.unload_vips_soft);
	assert(!desc.unload_raw_soft);
	assert(!desc.unload_surface_soft);
	assert(!desc.unload_thumbnail_vips_soft);
	assert(!desc.unload_thumbnail_raw_soft);
	assert(!desc.unload_thumbnail_surface_soft);
	nqiv_log_destroy(&logger);
}
