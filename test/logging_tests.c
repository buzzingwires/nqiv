#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "../src/logging.h"

#include "logging_tests.h"

size_t get_file_contents(FILE* f, char* buf, const int n)
{
	const long orig_pos = ftell(f);
	fseek(f, 0, SEEK_SET);
	const size_t bytes_read = fread(buf, 1, n, f);
	fseek(f, orig_pos, SEEK_SET);
	assert(ftell(f) == orig_pos);
	return bytes_read;
}

void logging_test_general(void)
{
	nqiv_log_ctx ctx = {0};

	/* Update set prefix and level use info level */
	nqiv_log_init(&ctx);
	int streams_len = ctx.streams->position / (int)sizeof(FILE*);
	ctx.level = NQIV_LOG_INFO;
	assert(ctx.streams != NULL);
	assert(streams_len == 0);
	assert(ctx.error_message[0] == '\0');
	assert(ctx.prefix_format[0] == '\0');
	assert(ctx.level == NQIV_LOG_INFO);
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
	nqiv_log_add_stream(&ctx, stderr);
	assert(strlen(ctx.error_message) == 0);
	streams_len = ctx.streams->position / (int)sizeof(FILE*);
	assert(streams_len == 3);

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
	assert(strncmp(buf1, "LOG#INFO ", strlen("LOG#INFO ")) == 0);
	assert(buf1[strlen("LOG#INFO ")] >= '0');
	assert(buf1[strlen("LOG#INFO ")] <= '9');
	assert(buf1[strlen("LOG#INFO 0000-00-00 00:00:00+0000")] == ' ');

	fclose(testfile_1);
	fclose(testfile_2);

	nqiv_log_write(&ctx, NQIV_LOG_INFO, NULL);
	assert(strcmp(ctx.error_message, "No format message to write.\n") == 0);

	nqiv_log_destroy(&ctx);
	assert(ctx.prefix_format[0] == '\0');
	assert(ctx.error_message[0] == '\0');
	assert(ctx.level == NQIV_LOG_ANY);
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
