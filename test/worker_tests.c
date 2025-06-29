#include <string.h>
#include <assert.h>

#include "../src/worker.h"

static bool worker_test_compare_specs(const nqiv_worker_spec* a, const nqiv_worker_spec* b)
{
	if(a->delay_base != b->delay_base) {
		return false;
	}
	if(a->event_interval != b->event_interval) {
		return false;
	}
	int idx;
	for(idx = 0; idx < THREAD_QUEUE_BIN_COUNT + 1; ++idx) {
		if(a->queue_bins[idx] != b->queue_bins[idx]) {
			return false;
		}
	}
	return true;
}

static void worker_test_spec_parse_print_instance_stringdiff(const nqiv_worker_spec* spec,
                                                             const char*             in_string,
                                                             const char*             out_string)
{
	nqiv_worker_spec new_spec;
	assert(nqiv_worker_string_to_spec(in_string, &new_spec));
	assert(worker_test_compare_specs(spec, &new_spec));
	char new_string[NQIV_WORKER_SPEC_STRLEN + 1] = {0};
	assert(nqiv_worker_spec_to_string(spec, new_string));
	assert(strcmp(out_string, new_string) == 0);
}

static void worker_test_spec_parse_print_instance(const nqiv_worker_spec* spec, const char* string)
{
	worker_test_spec_parse_print_instance_stringdiff(spec, string, string);
}

static void worker_test_spec_clear_bins(nqiv_worker_spec* spec)
{
	int idx;
	for(idx = 0; idx < THREAD_QUEUE_BIN_COUNT + 1; ++idx) {
		spec->queue_bins[idx] = -1;
	}
}

void worker_test_spec_parse_print(void)
{
	nqiv_worker_spec spec;
	spec.delay_base = -1;
	spec.event_interval = -1;
	worker_test_spec_clear_bins(&spec);
	spec.queue_bins[0] = 0;
	worker_test_spec_parse_print_instance(&spec, "");
	spec.delay_base = 1;
	worker_test_spec_parse_print_instance(&spec, "extra_wakeup_delay 1");
	spec.delay_base = -1;
	spec.event_interval = 1;
	worker_test_spec_parse_print_instance(&spec, "event_interval 1");
	spec.event_interval = -1;
	spec.queue_bins[1] = NQIV_EVENT_PRIORITY_IMAGE_LOAD_ANIMATION;
	worker_test_spec_parse_print_instance(&spec, "priorities image_load_animation");
	spec.queue_bins[2] = NQIV_EVENT_PRIORITY_IMAGE_LOAD;
	worker_test_spec_parse_print_instance(&spec, "priorities image_load_animation,image_load");
	spec.queue_bins[2] = -1;
	spec.delay_base = 1;
	spec.event_interval = 2;
	worker_test_spec_parse_print_instance(
		&spec, "extra_wakeup_delay 1 event_interval 2 priorities image_load_animation");
	spec.queue_bins[1] = NQIV_EVENT_PRIORITY_PRUNE;
	spec.queue_bins[2] = NQIV_EVENT_PRIORITY_IMAGE_LOAD;
	worker_test_spec_parse_print_instance(
		&spec, "extra_wakeup_delay 1 event_interval 2 priorities prune,image_load");
	spec.queue_bins[1] = NQIV_EVENT_PRIORITY_IMAGE_LOAD_ANIMATION;
	spec.queue_bins[2] = NQIV_EVENT_PRIORITY_REATTEMPT_THUMBNAIL;
	spec.queue_bins[3] = NQIV_EVENT_PRIORITY_PRUNE;
	spec.queue_bins[4] = NQIV_EVENT_PRIORITY_IMAGE_LOAD;
	spec.queue_bins[5] = NQIV_EVENT_PRIORITY_THUMBNAIL_LOAD_EPHEMERAL;
	spec.queue_bins[6] = NQIV_EVENT_PRIORITY_THUMBNAIL_LOAD;
	spec.queue_bins[7] = NQIV_EVENT_PRIORITY_THUMBNAIL_SAVE_LOAD_FAIL;
	spec.queue_bins[8] = NQIV_EVENT_PRIORITY_THUMBNAIL_SAVE_LOAD_NO;
	worker_test_spec_parse_print_instance(
		&spec, "extra_wakeup_delay 1 event_interval 2 priorities "
			   "image_load_animation,reattempt_thumbnail,prune,image_load,thumbnail_load_ephemeral,"
			   "thumbnail_load,thumbnail_save_load_fail,thumbnail_save_load_no");
	worker_test_spec_parse_print_instance_stringdiff(
		&spec,
		"extra_wakeup_delay 1 event_interval 2 priorities image_load_animation , "
		"reattempt_thumbnail,prune ,image_load, "
		"thumbnail_load_ephemeral,thumbnail_load,thumbnail_save_load_fail,thumbnail_save_load_no",
		"extra_wakeup_delay 1 event_interval 2 priorities "
		"image_load_animation,reattempt_thumbnail,prune,image_load,thumbnail_load_ephemeral,"
		"thumbnail_load,thumbnail_save_load_fail,thumbnail_save_load_no");
	worker_test_spec_parse_print_instance_stringdiff(
		&spec,
		"   extra_wakeup_delay 1 priorities   "
		"image_load_animation,reattempt_thumbnail,prune,image_load,thumbnail_load_ephemeral,"
		"thumbnail_load,thumbnail_save_load_fail,thumbnail_save_load_no  event_interval  2 ",
		"extra_wakeup_delay 1 event_interval 2 priorities "
		"image_load_animation,reattempt_thumbnail,prune,image_load,thumbnail_load_ephemeral,"
		"thumbnail_load,thumbnail_save_load_fail,thumbnail_save_load_no");
	worker_test_spec_parse_print_instance_stringdiff(
		&spec,
		"   extra_wakeup_delay 2 priorities "
		"image_load_animation,reattempt_thumbnail,prune,image_load,thumbnail_load_ephemeral,"
		"thumbnail_load,thumbnail_save_load_fail,thumbnail_save_load_no event_interval  2  "
		"extra_wakeup_delay 1",
		"extra_wakeup_delay 1 event_interval 2 priorities "
		"image_load_animation,reattempt_thumbnail,prune,image_load,thumbnail_load_ephemeral,"
		"thumbnail_load,thumbnail_save_load_fail,thumbnail_save_load_no");
	spec.queue_bins[5] = NQIV_EVENT_PRIORITY_THUMBNAIL_LOAD;
	spec.queue_bins[6] = NQIV_EVENT_PRIORITY_THUMBNAIL_LOAD_EPHEMERAL;
	worker_test_spec_parse_print_instance(
		&spec, "extra_wakeup_delay 1 event_interval 2 priorities "
			   "image_load_animation,reattempt_thumbnail,prune,image_load,thumbnail_load,thumbnail_"
			   "load_ephemeral,thumbnail_save_load_fail,thumbnail_save_load_no");
	spec.delay_base = 100;
	spec.event_interval = 20;
	worker_test_spec_parse_print_instance(
		&spec, "extra_wakeup_delay 100 event_interval 20 priorities "
			   "image_load_animation,reattempt_thumbnail,prune,image_load,thumbnail_load,thumbnail_"
			   "load_ephemeral,thumbnail_save_load_fail,thumbnail_save_load_no");
	spec.delay_base = -1;
	spec.event_interval = -1;
	worker_test_spec_clear_bins(&spec);
	spec.queue_bins[0] = 0;
	spec.queue_bins[1] = NQIV_EVENT_PRIORITY_PRUNE;
	spec.queue_bins[2] = NQIV_EVENT_PRIORITY_IMAGE_LOAD;
	worker_test_spec_parse_print_instance_stringdiff(
		&spec, "priorities image_load,prune priorities prune,image_load",
		"priorities prune,image_load");
	spec.queue_bins[1] = NQIV_EVENT_PRIORITY_IMAGE_LOAD_ANIMATION;
	spec.queue_bins[2] = NQIV_EVENT_PRIORITY_REATTEMPT_THUMBNAIL;
	spec.queue_bins[3] = NQIV_EVENT_PRIORITY_PRUNE;
	spec.queue_bins[4] = NQIV_EVENT_PRIORITY_IMAGE_LOAD;
	spec.queue_bins[5] = NQIV_EVENT_PRIORITY_THUMBNAIL_LOAD_EPHEMERAL;
	spec.queue_bins[6] = NQIV_EVENT_PRIORITY_THUMBNAIL_LOAD;
	spec.queue_bins[7] = NQIV_EVENT_PRIORITY_THUMBNAIL_SAVE_LOAD_FAIL;
	spec.queue_bins[8] = NQIV_EVENT_PRIORITY_THUMBNAIL_SAVE_LOAD_NO;
	spec.queue_bins[9] = NQIV_EVENT_PRIORITY_PRELOAD_IMAGE_LOAD;
	spec.queue_bins[10] = NQIV_EVENT_PRIORITY_PRELOAD_THUMBNAIL_LOAD_EPHEMERAL;
	spec.queue_bins[11] = NQIV_EVENT_PRIORITY_PRELOAD_THUMBNAIL_LOAD;
	spec.queue_bins[12] = NQIV_EVENT_PRIORITY_PRELOAD_THUMBNAIL_SAVE_LOAD_FAIL;
	spec.queue_bins[13] = NQIV_EVENT_PRIORITY_PRELOAD_THUMBNAIL_SAVE_LOAD_NO;
	worker_test_spec_parse_print_instance(
		&spec, "priorities "
			   "image_load_animation,reattempt_thumbnail,prune,image_load,thumbnail_load_ephemeral,"
			   "thumbnail_load,thumbnail_save_load_fail,thumbnail_save_load_no,preload_image_load,"
			   "preload_thumbnail_load_ephemeral,preload_thumbnail_load,preload_thumbnail_save_"
			   "load_fail,preload_thumbnail_save_load_no");
	nqiv_worker_spec ref_spec = {.delay_base = -1, .event_interval = -1, .queue_bins = {0}};
	nqiv_worker_spec fail_spec = {.delay_base = -1, .event_interval = -1, .queue_bins = {0}};
	worker_test_spec_clear_bins(&ref_spec);
	worker_test_spec_clear_bins(&fail_spec);
	assert(!nqiv_worker_string_to_spec(
		"priorities "
		"image_load_animation,image_load_animation,image_load_animation,image_load_animation,image_"
		"load_animation,image_load_animation,image_load_animation,image_load_animation,reattempt_"
		"thumbnail,prune,image_load,thumbnail_load_ephemeral,thumbnail_load,thumbnail_save_load_"
		"fail,thumbnail_save_load_no",
		&fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
	assert(!nqiv_worker_string_to_spec(
		"priorities "
		"image_load_animation,image_load_animation,image_load_animation,image_load_animation,image_"
		"load_animation,image_load_animation,image_load_animation,reattempt_thumbnail,prune,image_"
		"load,thumbnail_load_ephemeral,thumbnail_load,thumbnail_save_load_fail,thumbnail_save_load_"
		"no",
		&fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
	assert(!nqiv_worker_string_to_spec(
		"priorities "
		"quit,reattempt_thumbnail,prune,image_load,thumbnail_load_ephemeral,thumbnail_load,"
		"thumbnail_save_load_fail,thumbnail_save_load_no",
		&fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
	assert(!nqiv_worker_string_to_spec("extra_wakeup_delay -1", &fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
	assert(!nqiv_worker_string_to_spec("event_interval -1", &fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
	assert(!nqiv_worker_string_to_spec("priorities asdf", &fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
	assert(!nqiv_worker_string_to_spec("priorities thumbnail_save_load_noextra_wakeup_delay 0",
	                                   &fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
	assert(!nqiv_worker_string_to_spec("priorities thumbnail_save_load_no,extra_wakeup_delay 0",
	                                   &fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
	assert(!nqiv_worker_string_to_spec("priorities ,thumbnail_save_load_no extra_wakeup_delay 0",
	                                   &fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
	assert(!nqiv_worker_string_to_spec("priorities thumbnail_save_load_no,", &fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
	assert(!nqiv_worker_string_to_spec("extra_wakeup_delay ", &fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
	assert(!nqiv_worker_string_to_spec("extra_wakeup_delay 0event_interval 0", &fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
	assert(!nqiv_worker_string_to_spec("event_interval ", &fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
	assert(!nqiv_worker_string_to_spec("priorities ", &fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
	assert(!nqiv_worker_string_to_spec("none ", &fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
	assert(!nqiv_worker_string_to_spec(
		"extra_wakeup_delay 0 extra_wakeup_delay 0 extra_wakeup_delay 0 extra_wakeup_delay 0 "
		"extra_wakeup_delay 0 extra_wakeup_delay 0 extra_wakeup_delay 0 extra_wakeup_delay 0 "
		"extra_wakeup_delay 0 extra_wakeup_delay 0 extra_wakeup_delay 0 extra_wakeup_delay 0 "
		"extra_wakeup_delay 0 extra_wakeup_delay 0 extra_wakeup_delay 0 extra_wakeup_delay 0 "
		"extra_wakeup_delay 0 extra_wakeup_delay 0 extra_wakeup_delay 0 extra_wakeup_delay 0 "
		"extra_wakeup_delay 0 extra_wakeup_delay 0 extra_wakeup_delay 0 extra_wakeup_delay 0 "
		"extra_wakeup_delay 0 extra_wakeup_delay 0 extra_wakeup_delay 0 extra_wakeup_delay 0",
		&fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
}
