#include <assert.h>

#include "../src/logging.h"
#include "../src/queue.h"

#include "../src/array.h"
#include "queue_tests.h"

#define QUEUE_TEST_STANDARD_SIZE 3
#define QUEUE_TEST_BIN_COUNT     2

void queue_test_default(void)
{
	nqiv_log_ctx logger = {0};
	nqiv_queue   queue = {0};
	int          c = 0;

	nqiv_log_init(&logger);
	nqiv_log_set_prefix_format(&logger, "#level# #time:%Y-%m-%d %T%z# ");
	nqiv_log_add_stream(&logger, stderr);
	logger.level = NQIV_LOG_ERROR;
	assert(!nqiv_log_has_error(&logger));

	assert(nqiv_queue_init(&queue, &logger, sizeof(int), QUEUE_TEST_STANDARD_SIZE));
	assert(nqiv_queue_push(&queue, &c));
	c = 1;
	assert(nqiv_queue_push(&queue, &c));
	c = 2;
	assert(nqiv_queue_push(&queue, &c));
	c = 3;
	assert(!nqiv_queue_push(&queue, &c));
	nqiv_queue_push_force(&queue, &c);
	nqiv_queue_push_force(&queue, &c);

	assert(nqiv_queue_pop_front(&queue, &c));
	assert(c == 0);

	assert(nqiv_queue_pop(&queue, &c));
	assert(c == 3);

	nqiv_queue_destroy(&queue);
	nqiv_log_destroy(&logger);
}

void queue_test_priority_default(void)
{
	nqiv_log_ctx        logger = {0};
	nqiv_priority_queue queue = {0};
	int                 c = 0;

	nqiv_log_init(&logger);
	nqiv_log_set_prefix_format(&logger, "#level# #time:%Y-%m-%d %T%z# ");
	nqiv_log_add_stream(&logger, stderr);
	logger.level = NQIV_LOG_ERROR;
	assert(!nqiv_log_has_error(&logger));

	assert(nqiv_priority_queue_init(&queue, &logger, sizeof(int), QUEUE_TEST_STANDARD_SIZE - 2,
	                                QUEUE_TEST_BIN_COUNT));
	assert(nqiv_priority_queue_set_max_data_length(&queue, QUEUE_TEST_STANDARD_SIZE));
	assert(nqiv_priority_queue_set_min_add_count(&queue, 2));
	assert(nqiv_array_get_units_count(queue.bins[0].array) == QUEUE_TEST_STANDARD_SIZE - 2);
	assert(nqiv_array_get_units_count(queue.bins[1].array) == QUEUE_TEST_STANDARD_SIZE - 2);
	c = 0;
	assert(nqiv_priority_queue_push(&queue, 0, &c));
	assert(nqiv_array_get_units_count(queue.bins[0].array) == QUEUE_TEST_STANDARD_SIZE - 2);
	assert(nqiv_priority_queue_push(&queue, 0, &c));
	assert(nqiv_array_get_units_count(queue.bins[0].array) == QUEUE_TEST_STANDARD_SIZE);
	assert(nqiv_priority_queue_push(&queue, 0, &c));
	c = 1;
	assert(nqiv_priority_queue_push(&queue, 1, &c));
	assert(nqiv_array_get_units_count(queue.bins[1].array) == QUEUE_TEST_STANDARD_SIZE - 2);
	assert(nqiv_priority_queue_push(&queue, 1, &c));
	assert(nqiv_array_get_units_count(queue.bins[1].array) == QUEUE_TEST_STANDARD_SIZE);
	assert(nqiv_priority_queue_push(&queue, 1, &c));
	c = 2;
	assert(!nqiv_priority_queue_push(&queue, 0, &c));
	c = 3;
	assert(!nqiv_priority_queue_push(&queue, 1, &c));
	c = 2;
	nqiv_priority_queue_push_force(&queue, 0, &c);
	c = 3;
	nqiv_priority_queue_push_force(&queue, 1, &c);

	assert(nqiv_priority_queue_pop(&queue, &c));
	assert(c == 2);

	assert(nqiv_priority_queue_pop(&queue, &c));
	assert(c == 0);

	assert(nqiv_priority_queue_pop(&queue, &c));
	assert(c == 0);

	assert(nqiv_priority_queue_pop(&queue, &c));
	assert(c == 3);

	assert(nqiv_priority_queue_pop(&queue, &c));
	assert(c == 1);

	assert(nqiv_priority_queue_pop(&queue, &c));
	assert(c == 1);

	nqiv_priority_queue_destroy(&queue);
	nqiv_log_destroy(&logger);
}
