#ifndef NQIV_LOG_H
#define NQIV_LOG_H

#include <stdio.h>
#include <stdbool.h>

#include <omp.h>

#include "array.h"

/*
 * Since the logging module is devoted to producing feedback for nqiv, including
 * about fatal errors, it is to function in less than ideal conditions. In
 * particular, it is to avoid memory allocation after its initialization, and
 * generally be tolerant of errors in its own right. While it will record errors
 * to a fixed buffer, it will still make its best attempt to write messages.
 *
 * The logging function itself is protected by a mutex lock and can be used by
 * any thread. It is capable of simple filtering messages by log level and
 * writing to a relatively arbitrary number of streams (FILE*)
 *
 * Additionally, it has a custom formatting system for rendering prefixes. It
 * works as follows:
 *
 * - Arbitrary text is entered normally.
 * - # is a special character used to delimit formatting. This is chosen because
 *   it does not conflict with % used by strftime. A formatted section begins
 *   and ends with #
 * - ## with no contents produces a literal #
 * - #time:<strftime format># produces the local time formatted as specified.
 * - #level# prints the log level.
 *
 */

#define NQIV_LOG_PREFIX_FORMAT_LEN 255
#define NQIV_LOG_ERROR_MESSAGE_LEN 255
#define NQIV_LOG_STRFTIME_LEN      255

typedef enum nqiv_log_level
{
	NQIV_LOG_ANY = 0,
	NQIV_LOG_DEBUG = 10,
	NQIV_LOG_INFO = 20,
	NQIV_LOG_WARNING = 30,
	NQIV_LOG_ERROR = 40,
	NQIV_LOG_FINAL = NQIV_LOG_ERROR,
	NQIV_LOG_UNKNOWN = 999,
} nqiv_log_level;

extern const char* const nqiv_log_level_names[];

typedef struct nqiv_log_ctx
{
	omp_lock_t     lock;
	char           prefix_format[NQIV_LOG_PREFIX_FORMAT_LEN];
	char           error_message[NQIV_LOG_ERROR_MESSAGE_LEN];
	nqiv_log_level level; /* Allow this priority and higher. */
	nqiv_array*    streams;
} nqiv_log_ctx;

nqiv_log_level nqiv_log_level_from_string(const char* text);
void           nqiv_log_clear_error(nqiv_log_ctx* ctx);
bool           nqiv_log_has_error(const nqiv_log_ctx* ctx);
void           nqiv_log_set_prefix_format(nqiv_log_ctx* ctx, const char* fmt);
void           nqiv_log_destroy(nqiv_log_ctx* ctx);
void           nqiv_log_init(nqiv_log_ctx* ctx);
void           nqiv_log_add_stream(nqiv_log_ctx* ctx, const FILE* stream);
void nqiv_log_write(nqiv_log_ctx* ctx, const nqiv_log_level level, const char* format, ...);

#endif /* NQIV_LOG_H */
