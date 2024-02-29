
#if defined(__unix__) || defined(__linux__) || defined(__gnu_linux__)
	#define _GNU_SOURCE
	#include <stdio.h>
	#include <stdlib.h>
	#include <stdbool.h>
	#include <time.h>
	#include <assert.h>
	#include <errno.h>
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
#else
	#error "Currently, only Unix and Linux systems are supported."
#endif
