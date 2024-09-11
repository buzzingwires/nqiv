#include "platform.h"

#include <assert.h>

#if defined(__MINGW32__)
	#include <stdlib.h>
	#include <string.h>
	#include <errno.h>
	#include <direct.h>
	#include <winbase.h>
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
	return true; /* Windows doesn't do that */
}
bool nqiv_rename(const char* to, const char* from)
{
	assert(strcmp(to, from) != 0);
	return MoveFile(from, to) != 0;
}
#else
	#include <stdio.h>
	#include <stdint.h>
	#include <string.h>
	#include <stdlib.h>
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
bool nqiv_rename(const char* to, const char* from)
{
	assert(strcmp(to, from) != 0);
	return rename(from, to) != -1;
}
#endif

#if defined(__unix__) || defined(__linux__) || defined(__gnu_linux__) || defined(__MINGW32__)
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
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
	struct stat s;
	if(stat(path, &s) != 0) {
		return false;
	}
	if(data != NULL) {
		data->size = (size_t)(s.st_size);
		data->mtime = s.st_mtime;
	}
	return true;
}
bool nqiv_write_path_from_env(char*        output,
                              const size_t length,
                              const char*  env_name,
                              const char*  sub_path)
{
	assert(length < INT_MAX);
	nqiv_array builder;
	nqiv_array_inherit(&builder, output, sizeof(char), length);
	const char* raw_env = getenv(env_name);
	if(raw_env == NULL) {
		return false;
	}
	char base_path[PATH_MAX + 1] = {0};
	if(nqiv_realpath(raw_env, base_path) == NULL) {
		return false;
	}
	return nqiv_array_push_str(&builder, base_path) && nqiv_array_push_str(&builder, sub_path);
}
#endif

bool nqiv_get_default_cfg(char* output, const size_t length)
{
	return nqiv_write_path_from_env(output, length, NQIV_CFG_ENV,
	                                NQIV_CFG_DIRECTORY NQIV_CFG_FILENAME);
}

bool nqiv_get_default_cfg_thumbnail_dir(char* output, const size_t length)
{
	return nqiv_write_path_from_env(output, length, NQIV_CFG_ENV, NQIV_CFG_THUMBNAILS);
}

void nqiv_suggest_cfg_setup(const char* exe)
{
	char default_config_dir[PATH_MAX + 1] = {0};
	if(nqiv_write_path_from_env(default_config_dir, PATH_MAX, NQIV_CFG_ENV, NQIV_CFG_DIRECTORY)) {
		fprintf(stderr,
		        "Failed to load default config file path. Consider `" NQIV_CFG_MKDIR
		        " \'%s\' && %s -c \'dumpcfg\' > \'%s" NQIV_CFG_FILENAME "\'` to create it?\n",
		        default_config_dir, exe, default_config_dir);
	} else {
		fprintf(stderr, "Failed to get environment variable '" NQIV_CFG_ENV
		                "' to suggest config creation command. This usually shouldn't happen.\n");
	}
}
