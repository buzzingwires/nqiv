
#if defined(__unix__) || defined(__linux__) || defined(__gnu_linux__)
	#define _GNU_SOURCE
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <stdbool.h>
	#include <time.h>
	#include <assert.h>
	#include <errno.h>
	#include <limits.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include "platform.h"
	char* nqiv_realpath(const char* path, char* resolved_path)
	{
		return realpath(path, resolved_path);
	}
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
	bool nqiv_mkdir(char* path)
	{
		const int result = mkdir(path, 0777);
		return result == 0 || errno == EEXIST ? true : false;
	}

	bool nqiv_get_default_cfg(char* output, const size_t length)
	{
		size_t available_length = length;
		size_t position = 0;
		if( available_length <= 1 + strlen("/.config/nqiv.cfg") ) {
			return false;
		}
		available_length -= ( 1 + strlen("/.config/nqiv.cfg") );
		const char* raw_env = getenv("HOME");
		if(raw_env == NULL) {
			return false;
		}
		char base_path[PATH_MAX + 1] = {0};
		if( nqiv_realpath(raw_env, base_path) == NULL || strlen(base_path) > available_length ) {
			return false;
		}
		memcpy( output, base_path, strlen(base_path) );
		available_length -= strlen(base_path);
		position += strlen(base_path);
		memcpy( output + position, "/.config/nqiv.cfg", strlen("/.config/nqiv.cfg") );
		return true;
	}
#else
	#error "Currently, only Unix and Linux systems are supported."
#endif
