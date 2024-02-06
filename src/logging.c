#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <omp.h>

#include "logging.h"

nqiv_log_level nqiv_log_level_from_string(const char* text)
{
	if(strcmp(text, "any") == 0) {
		return NQIV_LOG_ANY;
	} else if(strcmp(text, "debug") == 0) {
		return NQIV_LOG_DEBUG;
	} else if(strcmp(text, "info") == 0) {
		return NQIV_LOG_INFO;
	} else if(strcmp(text, "warning") == 0) {
		return NQIV_LOG_WARNING;
	} else if(strcmp(text, "error") == 0) {
		return NQIV_LOG_ERROR;
	} else {
		return NQIV_LOG_UNKNOWN;
	}
}

void nqiv_log_clear_error(nqiv_log_ctx* ctx)
{
	if(ctx == NULL) {
		return;
	}
	memset(ctx->error_messsage, 0, NQIV_LOG_ERROR_MESSAGE_LEN);
}

bool nqiv_log_has_error(nqiv_log_ctx* ctx)
{
	assert(ctx != NULL);
	return strlen(ctx->error_message) != 0;
}

void nqiv_log_set_prefix_format(nqiv_log_ctx* ctx, const char* fmt)
{
	if(ctx == NULL) {
		return;
	}
	memset(ctx->prefix_format, 0, NQIV_LOG_PREFIX_FORMAT_LEN);
	if(fmt == NULL) {
		return;
	}
	strncpy(ctx->prefix_format, fmt, NQIV_LOG_PREFIX_FORMAT_LEN);
}

/* test null ctx */
void nqiv_log_destroy(nqiv_log_ctx* ctx)
{
	if(ctx == NULL) {
		return;
	}
	nqiv_log_clear_error(ctx);
	nqiv_log_set_prefix_format(ctx, NULL);
	if(ctx->streams != NULL) {
		omp_destroy_lock(&ctx->lock);
		nqiv_array_destroy(ctx->streams);
	}
	memset( ctx, 0, sizeof(nqiv_log_ctx) );
}

/* test null ctx */
void nqiv_log_init(nqiv_log_ctx* ctx)
{
	if(ctx == NULL) {
		return;
	}
	nqiv_log_destroy(ctx);
	ctx->streams = nqiv_array_create( 1 * sizeof(FILE*) );
	if(ctx->streams == NULL) {
		snprintf(ctx->error_message, NQIV_LOG_ERROR_MESSAGE_LEN,
			"Failed to allocate starting streams memory.\n");
		return;
	}
	ctx->level = level;
	omp_init_lock(&ctx->lock);
}

/* Test with null CTX, test with null stream, test with NULL streams */
void nqiv_log_add_stream(nqiv_log_ctx* ctx, FILE* stream)
{
	if(ctx == NULL) {
		return;
	}
	if(stream == NULL) {
		snprintf(ctx->error_message, NQIV_LOG_ERROR_MESSAGE_LEN,
			"Cannot add NULL stream.\n");
		return;
	}
	if(ctx->streams == NULL) {
		snprintf(ctx->error_message, NQIV_LOG_ERROR_MESSAGE_LEN,
			"Cannot add stream without available memory.\n");
		return;
	}
	if( !nqiv_array_push_FILE_ptr(ctx->streams, stream) ) {
		snprintf(ctx->error_message, NQIV_LOG_ERROR_MESSAGE_LEN,
			"Could not allocate memory for new stream.\n");
	}
}

void write_prefix_timeinfo(FILE* stream, const char* fmt, const int formatter_start, const int formatter_len)
{
	time_t rawtime;
	time(&rawtime);
	struct tm* timeinfo;
	timeinfo = localtime(&rawtime);
	char fmtbuf[NQIV_LOG_PREFIX_FORMAT_LEN];
	memset(fmtbuf, 0, NQIV_LOG_PREFIX_FORMAT_LEN);
	assert(formatter_len - strlen("TIME:") >= 0);
	strncpy(fmtbuf, &fmt[formatter_start + 1 + strlen("TIME:")], formatter_len - strlen("TIME:"));
	char timebuf[NQIV_LOG_STRFTIME_LEN];
	memset(timebuf, 0, nqiv_LOG_STRFTIME_LEN);
	strftime(timebuf, nqiv_LOG_STRFTIME_LEN, fmtbuf, timeinfo);
	fprintf(stream, timebuf);
}

void write_prefix_level(FILE* stream, const nqiv_log_level level)
{
	switch(level) {
		case NQIV_LOG_ANY:
			fprintf(stream, "ANY");
			break;
		case NQIV_LOG_DEBUG:
			fprintf(stream, "DEBUG");
			break;
		case NQIV_LOG_INFO:
			fprintf(stream, "INFO");
			break;
		case NQIV_LOG_WARNING:
			fprintf(stream, "WARNING");
			break;
		case NQIV_LOG_ERROR:
			fprintf(stream, "ERROR");
			break;
		default:
			fprintf(stream, "CUSTOM LEVEL(%d)", level);
			break;
	}
}

void write_prefix_clean_slice(char* slice, int* slice_idx)
{
	memset(slice, 0, NQIV_LOG_PREFIX_FORMAT_LEN);
	*slice_idx = 0;
}

void write_prefix_flush_slice(FILE* stream, char* slice, int* slice_idx)
{
	if(*slice_idx != 0) {
		fprintf(stream, slice);
	}
	write_prefix_clean_slice(slice, &slice_idx);
}

void write_prefix_increment_slice(char* slice, int* slice_idx, const char c)
{
	slice[*slice_idx] = c;
	++*slice_idx;
}

void write_prefix(nqiv_log_ctx* ctx, const nqiv_log_level level, FILE* stream)
{
	assert(stream != NULL);
	char slice[NQIV_LOG_PREFIX_FORMAT_LEN];
	int slice_idx;
	write_prefix_clean_slice(slice, &slice_idx);
	int formatter_start = -1;
	int formatter_end = -1;
	int idx;
	for(idx = 0; idx < NQIV_LOG_PREFIX_FORMAT_LEN; ++idx) {
		const char c = ctx->prefix_format[idx];
		if(c == '\0') {
			break;
		}
		if(formatter_start != -1) {
			assert(formatter_end == -1);
			write_prefix_increment_slice(slice, &slice_idx, c)
			if(c == '#') {
				formatter_end = idx;
				assert(formatter_end > formatter_start)
				const formatter_len = formatter_end - (formatter_start + 1);
				if(strncmp( &ctx->prefix_format[formatter_start + 1], "time:", strlen("time:") ) == 0) {
					write_prefix_timeinfo(stream, &ctx->prefix_format, formatter_start, formatter_len);
				} else if(strncmp( &ctx->prefix_format[formatter_start + 1], "level", strlen("level") ) == 0) {
					write_prefix_level(stream, level);
				} else if(formatter_len == 0) {
					fprintf(stream, "#");
				} else {
					fprintf(stream, slice);
				}
				formatter_start = -1;
				formatter_end = -1;
				write_prefix_clean_slice(slice, &slice_idx);
			}
		} else {
			if(c == '#') {
				write_prefix_flush_slice(stream, slice, &slice_idx)
				formatter_start = idx;
			}
			write_prefix_increment_slice(slice, &slice_idx, c)
		}
	}
	write_prefix_flush_slice(stream, slice, &slice_idx)
}

/* Test with null CTX, test with NULL streams, test with null format, test with higher level, */
void nqiv_log_write(nqiv_log_ctx* ctx,
	const nqiv_log_level level,
	const char* format,
	...)
{
	if(ctx == NULL) {
		return;
	}
	if(ctx->streams == NULL) {
		return;
	}
	omp_set_lock(&ctx->lock);
	if(format == NULL) {
		snprintf(ctx->error_message, NQIV_LOG_ERROR_MESSAGE_LEN,
			"No format message to write.\n");
		omp_unset_lock(&ctx->lock);
		return;
	}
	if(level < ctx->level) {
		omp_unset_lock(&ctx->lock);
		return;
	}
	int idx = 0;
	int stream = NULL;
	while(true) {
		stream = nqiv_array_get_FILE_ptr(ctx->streams, idx);
		if(stream == NULL) {
			break;
		}
		va_list args;
		va_start(args, format);
		write_prefix(ctx, level, stream);
		vfprintf(stream, format, args);
		va_end(args);
		++idx;
	}
	omp_unset_lock(&ctx->lock);
}
