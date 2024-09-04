#include "platform.h"

#if defined(__MINGW32__)
	#include <stdlib.h>
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
#else
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
	assert(data != NULL);
	struct stat s;
	if(stat(path, &s) != 0) {
		return false;
	}
	data->size = (size_t)(s.st_size);
	data->mtime = s.st_mtime;
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

#if defined(__MINGW32__)
bool nqiv_get_default_cfg(char* output, const size_t length)
{
	return nqiv_write_path_from_env(output, length, "USERPROFILE",
	                                "\\AppData\\Roaming\\nqiv\\nqiv.cfg");
}
#else
bool nqiv_get_default_cfg(char* output, const size_t length)
{
	return nqiv_write_path_from_env(output, length, "HOME", "/.config/nqiv.cfg");
}
#endif
