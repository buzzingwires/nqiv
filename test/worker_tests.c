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
	worker_test_spec_parse_print_instance(&spec, "");
	spec.delay_base = 1;
	worker_test_spec_parse_print_instance(&spec, "extra_wakeup_delay 1");
	spec.delay_base = -1;
	spec.event_interval = 1;
	worker_test_spec_parse_print_instance(&spec, "event_interval 1");
	spec.event_interval = -1;
	spec.queue_bins[0] = NQIV_EVENT_PRIORITY_ANIMATION;
	worker_test_spec_parse_print_instance(&spec, "priorities animation");
	spec.queue_bins[1] = NQIV_EVENT_PRIORITY_IMAGE;
	worker_test_spec_parse_print_instance(&spec, "priorities animation,image");
	spec.queue_bins[1] = -1;
	spec.delay_base = 1;
	spec.event_interval = 2;
	worker_test_spec_parse_print_instance(
		&spec, "extra_wakeup_delay 1 event_interval 2 priorities animation");
	spec.queue_bins[0] = NQIV_EVENT_PRIORITY_PRUNE;
	spec.queue_bins[1] = NQIV_EVENT_PRIORITY_IMAGE;
	worker_test_spec_parse_print_instance(
		&spec, "extra_wakeup_delay 1 event_interval 2 priorities prune,image");
	spec.queue_bins[0] = NQIV_EVENT_PRIORITY_ANIMATION;
	spec.queue_bins[1] = NQIV_EVENT_PRIORITY_UNLOAD;
	spec.queue_bins[2] = NQIV_EVENT_PRIORITY_PRUNE;
	spec.queue_bins[3] = NQIV_EVENT_PRIORITY_IMAGE;
	spec.queue_bins[4] = NQIV_EVENT_PRIORITY_THUMBNAIL_LOAD;
	spec.queue_bins[5] = NQIV_EVENT_PRIORITY_THUMBNAIL_SAVE;
	worker_test_spec_parse_print_instance(&spec, "extra_wakeup_delay 1 event_interval 2 priorities "
	                                             "animation,unload,prune,image,thumbnail_load,"
	                                             "thumbnail_save");
	worker_test_spec_parse_print_instance_stringdiff(
		&spec,
		"extra_wakeup_delay 1 event_interval 2 priorities animation , "
		"unload,prune ,image, "
		"thumbnail_load,thumbnail_save",
		"extra_wakeup_delay 1 event_interval 2 priorities "
		"animation,unload,prune,image,thumbnail_load,"
		"thumbnail_save");
	worker_test_spec_parse_print_instance_stringdiff(
		&spec,
		"   extra_wakeup_delay 1 priorities   "
		"animation,unload,prune,image,thumbnail_load,"
		"thumbnail_save  event_interval  2 ",
		"extra_wakeup_delay 1 event_interval 2 priorities "
		"animation,unload,prune,image,thumbnail_load,"
		"thumbnail_save");
	worker_test_spec_parse_print_instance_stringdiff(
		&spec,
		"   extra_wakeup_delay 2 priorities "
		"animation,unload,prune,image,thumbnail_load,"
		"thumbnail_save event_interval  2  "
		"extra_wakeup_delay 1",
		"extra_wakeup_delay 1 event_interval 2 priorities "
		"animation,unload,prune,image,thumbnail_load,"
		"thumbnail_save");
	spec.queue_bins[4] = NQIV_EVENT_PRIORITY_THUMBNAIL_LOAD;
	spec.queue_bins[5] = NQIV_EVENT_PRIORITY_THUMBNAIL_SAVE;
	worker_test_spec_parse_print_instance(&spec,
	                                      "extra_wakeup_delay 1 event_interval 2 priorities "
	                                      "animation,unload,prune,image,thumbnail_load,thumbnail_"
	                                      "save");
	spec.delay_base = 100;
	spec.event_interval = 20;
	worker_test_spec_parse_print_instance(&spec,
	                                      "extra_wakeup_delay 100 event_interval 20 priorities "
	                                      "animation,unload,prune,image,thumbnail_load,thumbnail_"
	                                      "save");
	spec.delay_base = -1;
	spec.event_interval = -1;
	worker_test_spec_clear_bins(&spec);
	spec.queue_bins[0] = NQIV_EVENT_PRIORITY_PRUNE;
	spec.queue_bins[1] = NQIV_EVENT_PRIORITY_IMAGE;
	worker_test_spec_parse_print_instance_stringdiff(
		&spec, "priorities image,prune priorities prune,image", "priorities prune,image");
	spec.queue_bins[0] = NQIV_EVENT_PRIORITY_ANIMATION;
	spec.queue_bins[1] = NQIV_EVENT_PRIORITY_UNLOAD;
	spec.queue_bins[2] = NQIV_EVENT_PRIORITY_PRUNE;
	spec.queue_bins[3] = NQIV_EVENT_PRIORITY_IMAGE;
	spec.queue_bins[4] = NQIV_EVENT_PRIORITY_THUMBNAIL_LOAD;
	spec.queue_bins[5] = NQIV_EVENT_PRIORITY_THUMBNAIL_SAVE;
	spec.queue_bins[6] = NQIV_EVENT_PRIORITY_PRELOAD_IMAGE;
	spec.queue_bins[7] = NQIV_EVENT_PRIORITY_PRELOAD_THUMBNAIL_LOAD;
	spec.queue_bins[8] = NQIV_EVENT_PRIORITY_PRELOAD_THUMBNAIL_SAVE;
	worker_test_spec_parse_print_instance(
		&spec, "priorities "
			   "animation,unload,prune,image,thumbnail_load,"
			   "thumbnail_save,preload_image,preload_thumbnail_load,preload_thumbnail_save");
	nqiv_worker_spec ref_spec = {.delay_base = -1, .event_interval = -1, .queue_bins = {0}};
	nqiv_worker_spec fail_spec = {.delay_base = -1, .event_interval = -1, .queue_bins = {0}};
	worker_test_spec_clear_bins(&ref_spec);
	worker_test_spec_clear_bins(&fail_spec);
	assert(
		!nqiv_worker_string_to_spec("priorities "
	                                "animation,animation,animation,animation,animation,animation,"
	                                "unload,prune,image,thumbnail_load,thumbnail_save",
	                                &fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
	assert(!nqiv_worker_string_to_spec("priorities "
	                                   "animation,animation,animation,animation,animation,"
	                                   "unload,prune,image"
	                                   ",thumbnail_load,thumbnail_save",
	                                   &fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
	assert(!nqiv_worker_string_to_spec("priorities "
	                                   "quit,unload,prune,image,thumbnail_load,thumbnail_save",
	                                   &fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
	assert(!nqiv_worker_string_to_spec("extra_wakeup_delay -1", &fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
	assert(!nqiv_worker_string_to_spec("event_interval -1", &fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
	assert(!nqiv_worker_string_to_spec("priorities asdf", &fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
	assert(
		!nqiv_worker_string_to_spec("priorities thumbnail_saveextra_wakeup_delay 0", &fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
	assert(
		!nqiv_worker_string_to_spec("priorities thumbnail_save,extra_wakeup_delay 0", &fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
	assert(
		!nqiv_worker_string_to_spec("priorities ,thumbnail_save extra_wakeup_delay 0", &fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
	assert(!nqiv_worker_string_to_spec("priorities thumbnail_save,", &fail_spec));
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
