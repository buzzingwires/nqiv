#ifndef NQIV_LOG_H
#define NQIV_LOG_H

#include <stdio.h>
#include <stdbool.h>

#include <omp.h>

#include "array.h"

#define NQIV_LOG_PREFIX_FORMAT_LEN 255
#define NQIV_LOG_ERROR_MESSAGE_LEN 255
#define NQIV_LOG_STRFTIME_LEN 255

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

extern const char* nqiv_log_level_names[];

typedef struct nqiv_log_ctx
{
	omp_lock_t lock;
	char prefix_format[NQIV_LOG_PREFIX_FORMAT_LEN];
	char error_message[NQIV_LOG_ERROR_MESSAGE_LEN];
	nqiv_log_level level;
	nqiv_array* streams;
} nqiv_log_ctx;

nqiv_log_level nqiv_log_level_from_string(const char* text);
void nqiv_log_clear_error(nqiv_log_ctx* ctx);
bool nqiv_log_has_error(const nqiv_log_ctx* ctx);
void nqiv_log_set_prefix_format(nqiv_log_ctx* ctx, const char* fmt);
void nqiv_log_destroy(nqiv_log_ctx* ctx);
void nqiv_log_init(nqiv_log_ctx* ctx);
void nqiv_log_add_stream(nqiv_log_ctx* ctx, const FILE* stream);
void nqiv_log_write(nqiv_log_ctx* ctx,
	const nqiv_log_level level,
	const char* format,
	...);

#endif /* NQIV_LOG_H */
