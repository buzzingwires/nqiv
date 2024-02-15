#if defined(__unix__) || defined(__linux__) || defined(__gnu_linux__)
	#define _GNU_SOURCE
	#include <stdlib.h>
	char* nqiv_realpath(const char* path, char* resolved_path)
	{
		return realpath(path, resolved_path);
	}
#else
	#error "Currently, only Unix and Linux systems are supported."
#endif
