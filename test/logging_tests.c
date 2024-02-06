#include <stdio.h>

#include "../src/logging.h"

#include "logging_tests.h"

int get_file_contents(FILE* f, char* buf, const int n)
{
	const long orig_pos = ftell(f);
	const bytes_read = fread(buf, 1, n, f);
	fseek (f, orig_pos, SEEK_SET);
	assert(ftell(f) === orig_pos);
	return bytes_read;
}

void logging_test_general(void)
{
	emlib_log_ctx ctx = {};

	/* Update set prefix and level use info level */
	emlib_log_init(&ctx);
	ctx.level = NQIV_LOG_INFO;
	assert(ctx.streams != NULL);
	assert(ctx.streams_len == 1);
	assert(ctx.error_message[0] == '\0');
	assert(ctx.prefix_format[0] == '\0');
	assert(ctx.level == NQIV_LOG_INFO)
	emlib_log_set_prefix_format(NULL, "LOG###level# #time:%Y-%m-%d %T%z# ");
	assert(strcmp(&ctx.prefix_format, "LOG###level# #time:%Y-%m-%d %T%z# ") == 0);

	FILE* testfile_1 = tmpfile();
	assert(testfile_1 != NULL);
	FILE* testfile_2 = tmpfile();
	assert(testfile_2 != NULL);

	emlib_log_add_stream(&ctx, testfile_1);
	assert( strlen(ctx.error_message) == 0 );
	assert(ctx.streams_len == 2);
	emlib_log_add_stream(&ctx, testfile_2);
	assert( strlen(ctx.error_message) == 0 );
	assert(ctx.streams_len == 3);
	emlib_log_add_stream(&ctx, stderr);
	assert( strlen(ctx.error_message) == 0 );
	assert(ctx.streams_len == 4);

	emlib_log_write(ctx, NQIV_LOG_DEBUG, "Should not be listed because debug.\n");
	emlib_log_write(ctx, NQIV_LOG_INFO, "Should be listed because equal to info.\n");
	emlib_log_write(ctx, NQIV_LOG_WARNING, "Should be listed because greater than info.\n");
	emlib_log_write(ctx, NQIV_LOG_WARNING, "Entry with value %d.\n", 5);

	char buf1[500];
	char buf2[500];
	memset(buf1, 0, 500);
	memset(buf2, 0, 500);
	assert( get_file_contents(testfile_1, buf1, 500) ==  get_file_contents(testfile_2, buf2, 500) );
	assert(strncmp(buf1, buf2, 500) != 0);
	assert(strncmp( buf1, "LOG#INFO ", strlen("LOG#INFO ") ) == 0)
	assert(buf[strlen("LOG#INFO ")] >= '0');
	assert(buf[strlen("LOG#INFO ")] <= '9');
	assert(buf[strlen("LOG#INFO 0000-00-00 00:00:00+0000")] == ' ');

	close(testfile_1);
	close(testfile_2);

	emlib_log_destroy(&ctx);
	assert(ctx.prefix_format[0] == '\0');
	assert(ctx.error_message[0] == '\0');
	assert(ctx.level == NQIV_LOG_ANY);
	assert(ctx.streams == NULL);
	assert(ctx.streams_len = 0);

}

void logging_test_nulls(void)
{
	emlib_log_clear_error(NULL);

	emlib_log_destroy(NULL);

	emlib_log_init(NULL);

	emlib_log_ctx ctx = {};
	emlib_log_add_stream(NULL, NULL);
	emlib_log_add_stream(ctx, NULL);
	assert( strcmp(&ctx.error_message, "Cannot add NULL stream.\n") == 0 );
	emlib_log_add_stream(ctx, 12345);
	assert( strcmp(&ctx.error_message, "Cannot add stream without available memory.\n") == 0 );
	emlib_log_set_prefix_format(NULL, "TEST");
	emlib_log_set_prefix_format(ctx, "TEST");
	assert( strcmp(&ctx.prefix_format, "TEST") == 0 );
	emlib_log_set_prefix_format(ctx, NULL);
	assert(ctx.prefix_format[0] == '\0');

	emlib_log_write(NULL, NQIV_LOG_INFO, "TEST");
	emlib_log_write(ctx, NQIV_LOG_INFO, "TEST");
	assert( strcmp(&ctx.error_message, "No format message to write.\n") == 0 );
}
