#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "../src/logging.h"

#include "logging_tests.h"

static size_t get_file_contents(FILE* f, char* buf, const int n)
{
	const long orig_pos = ftell(f);
	fseek(f, 0, SEEK_SET);
	const size_t bytes_read = fread(buf, 1, n, f);
	fseek(f, orig_pos, SEEK_SET);
	const long told_pos = ftell(f);
	assert(told_pos == orig_pos);
	return bytes_read;
}

static size_t check_log_step(const char* buf, const char* compare)
{
	assert(strncmp(buf, compare, strlen(compare)) == 0);
	return strlen(compare);
}

static size_t check_log_numbers(const char* buf, const char* compare)
{
	size_t offset = 0;
	for(offset = 0; offset < strlen(compare); ++offset) {
		assert(buf[offset] != '\0');
		if(compare[offset] == '0') {
			assert(buf[offset] >= '0');
			assert(buf[offset] <= '9');
		} else {
			assert(buf[offset] == compare[offset]);
		}
	}
	assert(compare[offset] == '\0');
	return offset;
}

static size_t check_log_entry(const char* buf, const nqiv_log_level level, const char* message)
{
	const char* level_msg = NULL;
	if(level == NQIV_LOG_DEBUG) {
		level_msg = "LOG#DEBUG ";
	} else if(level == NQIV_LOG_INFO) {
		level_msg = "LOG#INFO ";
	} else if(level == NQIV_LOG_WARNING) {
		level_msg = "LOG#WARNING ";
	} else if(level == NQIV_LOG_ERROR) {
		level_msg = "LOG#ERROR ";
	}
	assert(level_msg != NULL);
	size_t offset = 0;
	offset += check_log_step(buf + offset, level_msg);
	offset += check_log_numbers(buf + offset, "0000-00-00 00:00:00-0000 ");
	offset += check_log_step(buf + offset, message);
	return offset;
}

void logging_test_general(void)
{
	nqiv_log_ctx ctx = {0};

	/* Update set prefix and level use info level */
	nqiv_log_init(&ctx);
	int streams_len = ctx.streams->position / (int)sizeof(FILE*);
	SDL_AtomicSet(&ctx.level, NQIV_LOG_INFO);
	assert(ctx.streams != NULL);
	assert(streams_len == 0);
	assert(ctx.error_message[0] == '\0');
	assert(ctx.prefix_format[0] == '\0');
	assert(SDL_AtomicGet(&ctx.level) == NQIV_LOG_INFO);
	nqiv_log_set_prefix_format(&ctx, "LOG###level# #time:%Y-%m-%d %T%z# ");
	assert(strcmp(ctx.prefix_format, "LOG###level# #time:%Y-%m-%d %T%z# ") == 0);

	FILE* testfile_1 = tmpfile();
	assert(testfile_1 != NULL);
	FILE* testfile_2 = tmpfile();
	assert(testfile_2 != NULL);

	nqiv_log_add_stream(&ctx, testfile_1);
	assert(strlen(ctx.error_message) == 0);
	streams_len = ctx.streams->position / (int)sizeof(FILE*);
	assert(streams_len == 1);
	nqiv_log_add_stream(&ctx, testfile_2);
	assert(strlen(ctx.error_message) == 0);
	streams_len = ctx.streams->position / (int)sizeof(FILE*);
	assert(streams_len == 2);

	nqiv_log_write(&ctx, NQIV_LOG_DEBUG, "Should not be listed because debug.\n");
	nqiv_log_write(&ctx, NQIV_LOG_INFO, "Should be listed because equal to info.\n");
	nqiv_log_write(&ctx, NQIV_LOG_WARNING, "Should be listed because greater than info.\n");
	nqiv_log_write(&ctx, NQIV_LOG_WARNING, "Entry with value %d.\n", 5);

	char buf1[500];
	char buf2[500];
	memset(buf1, 0, 500);
	memset(buf2, 0, 500);
	assert(get_file_contents(testfile_1, buf1, 500) == get_file_contents(testfile_2, buf2, 500));
	assert(strncmp(buf1, buf2, 500) == 0);

	size_t offset = 0;
	offset +=
		check_log_entry(buf1 + offset, NQIV_LOG_INFO, "Should be listed because equal to info.\n");
	offset += check_log_entry(buf1 + offset, NQIV_LOG_WARNING,
	                          "Should be listed because greater than info.\n");
	offset += check_log_entry(buf1 + offset, NQIV_LOG_WARNING, "Entry with value 5.\n");
	assert(offset == strlen(buf1));

	fclose(testfile_1);
	fclose(testfile_2);

	nqiv_log_write(&ctx, NQIV_LOG_INFO, NULL);
	assert(strcmp(ctx.error_message, "No format message to write.\n") == 0);

	nqiv_log_destroy(&ctx);
	assert(ctx.prefix_format[0] == '\0');
	assert(ctx.error_message[0] == '\0');
	assert(SDL_AtomicGet(&ctx.level) == NQIV_LOG_ANY);
	assert(ctx.streams == NULL);
}

void logging_test_nulls(void)
{
	nqiv_log_clear_error(NULL);

	nqiv_log_destroy(NULL);

	nqiv_log_init(NULL);

	nqiv_log_ctx ctx = {0};
	nqiv_log_add_stream(NULL, NULL);
	nqiv_log_add_stream(&ctx, NULL);
	assert(strcmp(ctx.error_message, "Cannot add NULL stream.\n") == 0);
	nqiv_log_set_prefix_format(NULL, "TEST");
	nqiv_log_set_prefix_format(&ctx, "TEST");
	assert(strcmp(ctx.prefix_format, "TEST") == 0);
	nqiv_log_set_prefix_format(&ctx, NULL);
	assert(ctx.prefix_format[0] == '\0');

	nqiv_log_write(NULL, NQIV_LOG_INFO, "TEST");
}
