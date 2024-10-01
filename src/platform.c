#include "platform.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <glib.h>

#if defined(__MINGW32__)
	#include <errno.h>
	#include <direct.h>
char* nqiv_realpath(const char* path, char* resolved_path)
{
	return _fullpath(resolved_path, path, PATH_MAX);
}
bool nqiv_mkdir(char* path)
{
	const int result = _mkdir(path);
	return result == 0 || errno == EEXIST ? true : false;
}
bool nqiv_chmod(const char* filename, uint16_t mode)
{
	(void)filename;
	(void)mode;
	return true; /* Windows doesn't do that */
}
#else
	#include <stdio.h>
	#include <stdint.h>
	#include <limits.h>
	#include <errno.h>
	#include <sys/types.h>
	#include <sys/stat.h>
char* nqiv_realpath(const char* path, char* resolved_path)
{
	return realpath(path, resolved_path);
}
bool nqiv_mkdir(char* path)
{
	const int result = mkdir(path, 0777);
	return result == 0 || errno == EEXIST ? true : false;
}
bool nqiv_chmod(const char* filename, uint16_t mode)
{
	return chmod(filename, mode) == 0;
}
#endif

#if defined(__unix__) || defined(__linux__) || defined(__gnu_linux__) || defined(__MINGW32__)
	#include <stdio.h>
	#include <stdbool.h>
	#include <time.h>
	#include <assert.h>
	#include <limits.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include "platform.h"
	#include "array.h"
bool nqiv_stat(const char* path, nqiv_stat_data* data)
{
	assert(data != NULL);
	struct stat s;
	if(stat(path, &s) != 0) {
		return false;
	}
	data->size = (size_t)(s.st_size);
	data->mtime = s.st_mtime;
	return true;
}
#endif

bool nqiv_write_path_from_env(char*       output,
                              const int   length,
                              const char* env_name,
                              const char* sub_path)
{
	assert(length < INT_MAX);
	nqiv_array builder;
	nqiv_array_inherit(&builder, output, sizeof(char), length);
	const char* raw_env = g_getenv(env_name);
	if(raw_env == NULL) {
		return false;
	}
	char base_path[PATH_MAX + 1] = {0};
	if(nqiv_realpath(raw_env, base_path) == NULL) {
		return false;
	}
	bool result = nqiv_array_push_str(&builder, base_path);
	assert(strlen(base_path) == 0
	       || strncmp(base_path + strlen(base_path) - 1, "/", strlen("/")) != 0);
	if(strlen(sub_path) == 0 || strncmp(sub_path, "/", strlen("/")) != 0) {
		result = result && nqiv_array_push_str(&builder, "/");
	}
	result = result && nqiv_array_push_str(&builder, sub_path);
	return result;
}

bool nqiv_get_default_cfg(char* output, const int length)
{
	nqiv_array builder;
	nqiv_array_inherit(&builder, output, sizeof(char), length);
	return nqiv_array_push_str(&builder, "~" NQIV_CFG_DIRECTORY NQIV_CFG_FILENAME);
}

bool nqiv_get_default_cfg_thumbnail_dir(char* output, const int length)
{
	nqiv_array builder;
	nqiv_array_inherit(&builder, output, sizeof(char), length);
	return nqiv_array_push_str(&builder, "~" NQIV_CFG_THUMBNAILS);
}

void nqiv_suggest_cfg_setup(const char* exe)
{
	char default_config_dir[PATH_MAX + 1] = {0};
	if(nqiv_write_path_from_env(default_config_dir, PATH_MAX, NQIV_CFG_ENV, NQIV_CFG_DIRECTORY)) {
		fprintf(stderr,
		        "Failed to load default config file path. Consider `" NQIV_CFG_CMD
		        NQIV_CFG_MKDIR " " NQIV_CFG_ESC "\"%s" NQIV_CFG_ESC "\" && " NQIV_CFG_ESC
		        "\"%s" NQIV_CFG_ESC "\" -c " NQIV_CFG_ESC "\"dumpcfg" NQIV_CFG_ESC
		        "\" > " NQIV_CFG_ESC "\"%s" NQIV_CFG_FILENAME NQIV_CFG_ESC "\"" NQIV_CFG_CMD_END "` to create it?\n",
		        default_config_dir, exe, default_config_dir);
	} else {
		fprintf(stderr, "Failed to get environment variable '" NQIV_CFG_ENV
		                "' to suggest config creation command. This usually shouldn't happen.\n");
	}
}

int nqiv_strlen(const char* str)
{
	const size_t len = strlen(str);
	assert(len <= INT_MAX);
	return (int)len;
}

int nqiv_strtoi(const char* str, char** endptr, int base)
{
	long int result = strtol(str, endptr, base);
	if(result > INT_MAX) {
		result = INT_MAX;
		errno = ERANGE;
	} else if(result < INT_MIN) {
		result = INT_MIN;
		errno = ERANGE;
	}
	return (int)result;
}

int nqiv_ptrdiff(const void* a, const void* b)
{
	assert(sizeof(void*) == sizeof(char*));
	const ptrdiff_t diff = (char*)a - (char*)b;
	assert(diff >= 0);
	assert(diff <= INT_MAX);
	return (int)diff;
}

bool nqiv_starts_with_home_tilde(const char* path)
{
	return strncmp(path, "~/", strlen("~/")) == 0;
}

bool nqiv_expand_path(char* output, const int length, const char* input)
{
	if(nqiv_starts_with_home_tilde(input)) {
		return nqiv_write_path_from_env(output, length, NQIV_CFG_ENV, input + strlen("~/"));
	} else {
		nqiv_array builder;
		nqiv_array_inherit(&builder, output, sizeof(char), length);
		return nqiv_array_push_str(&builder, input);
	}
}

FILE* nqiv_fopen(const char* filename, const char* mode)
{
	char path[PATH_MAX + 1] = {0};
	if(!nqiv_expand_path(path, PATH_MAX, filename)) {
		return NULL;
	}
	return fopen(path, mode);
}
