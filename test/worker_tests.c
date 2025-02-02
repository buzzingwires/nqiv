#include <string.h>
#include <assert.h>

#include "../src/worker.h"

bool worker_test_compare_specs(const nqiv_worker_spec* a, const nqiv_worker_spec* b)
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

void worker_test_spec_parse_print_instance_stringdiff(const nqiv_worker_spec* spec,
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

void worker_test_spec_parse_print_instance(const nqiv_worker_spec* spec, const char* string)
{
	worker_test_spec_parse_print_instance_stringdiff(spec, string, string);
}

void worker_test_spec_clear_bins(nqiv_worker_spec* spec)
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
	spec.queue_bins[0] = 0;
	worker_test_spec_parse_print_instance(&spec, "bins 0");
	spec.delay_base = 1;
	spec.event_interval = 2;
	spec.queue_bins[0] = 3;
	spec.queue_bins[1] = 4;
	worker_test_spec_parse_print_instance(&spec, "extra_wakeup_delay 1 event_interval 2 bins 3 4");
	spec.queue_bins[0] = 0;
	spec.queue_bins[1] = 1;
	spec.queue_bins[2] = 2;
	spec.queue_bins[3] = 3;
	spec.queue_bins[4] = 4;
	spec.queue_bins[5] = 5;
	spec.queue_bins[6] = 6;
	spec.queue_bins[7] = 7;
	spec.queue_bins[8] = 8;
	worker_test_spec_parse_print_instance(
		&spec, "extra_wakeup_delay 1 event_interval 2 bins 0 1 2 3 4 5 6 7 8");
	worker_test_spec_parse_print_instance_stringdiff(
		&spec, "   extra_wakeup_delay 1 bins 0   1 2 3 4 5 6 7 8 event_interval  2 ",
		"extra_wakeup_delay 1 event_interval 2 bins 0 1 2 3 4 5 6 7 8");
	worker_test_spec_parse_print_instance_stringdiff(
		&spec,
		"   extra_wakeup_delay 2 bins 0   1 2 3 4 5 6 7 8 event_interval  2  extra_wakeup_delay 1",
		"extra_wakeup_delay 1 event_interval 2 bins 0 1 2 3 4 5 6 7 8");
	worker_test_spec_parse_print_instance_stringdiff(
		&spec,
		"   extra_wakeup_delay 2 bins 00   1 2 3 4 5 6 7 8 event_interval  2  extra_wakeup_delay 1",
		"extra_wakeup_delay 1 event_interval 2 bins 0 1 2 3 4 5 6 7 8");
	spec.queue_bins[4] = 5;
	spec.queue_bins[5] = 4;
	worker_test_spec_parse_print_instance(
		&spec, "extra_wakeup_delay 1 event_interval 2 bins 0 1 2 3 5 4 6 7 8");
	spec.delay_base = 100;
	spec.event_interval = 20;
	worker_test_spec_parse_print_instance(
		&spec, "extra_wakeup_delay 100 event_interval 20 bins 0 1 2 3 5 4 6 7 8");
	nqiv_worker_spec ref_spec = {.delay_base = -1, .event_interval = -1, .queue_bins = {0}};
	nqiv_worker_spec fail_spec = {.delay_base = -1, .event_interval = -1, .queue_bins = {0}};
	worker_test_spec_clear_bins(&ref_spec);
	worker_test_spec_clear_bins(&fail_spec);
	assert(!nqiv_worker_string_to_spec("extra_wakeup_delay -1", &fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
	assert(!nqiv_worker_string_to_spec("event_interval -1", &fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
	assert(!nqiv_worker_string_to_spec("bins 1000", &fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
	assert(!nqiv_worker_string_to_spec("bins 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0", &fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
	assert(!nqiv_worker_string_to_spec("bins 0 1 2 3 4 5 6 7 8 9", &fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
	assert(!nqiv_worker_string_to_spec("bins 0 1 2 3 4 5 6 7 8extra_wakeup_delay 0", &fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
	assert(!nqiv_worker_string_to_spec("bins 0 10 1 2 3 4 5 6 7 8 9", &fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
	assert(!nqiv_worker_string_to_spec("extra_wakeup_delay ", &fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
	assert(!nqiv_worker_string_to_spec("extra_wakeup_delay 0event_interval 0", &fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
	assert(!nqiv_worker_string_to_spec("event_interval ", &fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
	assert(!nqiv_worker_string_to_spec("bins ", &fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
	assert(!nqiv_worker_string_to_spec("bins 0 one", &fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
	assert(!nqiv_worker_string_to_spec("none ", &fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
	assert(!nqiv_worker_string_to_spec(
		"extra_wakeup_delay 0 extra_wakeup_delay 0 extra_wakeup_delay 0 extra_wakeup_delay 0 "
	    "extra_wakeup_delay 0 extra_wakeup_delay 0 extra_wakeup_delay 0",
		&fail_spec));
	assert(worker_test_compare_specs(&ref_spec, &fail_spec));
}
