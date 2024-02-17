#ifndef NQIV_PLATFORM_H
#define NQIV_PLATFORM_H

#include <stdbool.h>
#include <time.h>

typedef struct nqiv_stat_data
{
	size_t size;
	time_t mtime;
} nqiv_stat_data;

char* nqiv_realpath(const char *path, char *resolved_path);
bool nqiv_fstat(FILE* f, nqiv_stat_data* data);
bool nqiv_mkdir(char* path);

#endif /* NQIV_PLATFORM_H */
