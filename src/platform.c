
#if defined(__unix__) || defined(__linux__) || defined(__gnu_linux__)
	#define _GNU_SOURCE
	#include <stdio.h>
	#include <stdlib.h>
	#include <stdbool.h>
	#include <time.h>
	#include <assert.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include "platform.h"
	char* nqiv_realpath(const char* path, char* resolved_path)
	{
		return realpath(path, resolved_path);
	}
	bool nqiv_fstat(FILE* f, nqiv_stat_data* data)
	{
		assert(data != NULL);
		struct stat s;
		const int fd = fileno(f);
		if(fd == -1) {
			return false;
		}
		if(fstat(fd, &s) != 0) {
			return false;
		}
		data->size = (size_t)(s.st_size);
		data->mtime = s.st_mtime;
		return true;
	}
#else
	#error "Currently, only Unix and Linux systems are supported."
#endif
