#ifndef NQIV_PLATFORM_H
#define NQIV_PLATFORM_H

#if !defined(__unix__) && !defined(__linux__) && !defined(__gnu_linux__) && !defined(__MINGW32__)
	#error "Currently, only Unix, Linux, and windows through MinGW are supported."
#else
	#define _GNU_SOURCE
#endif

#define NQIV_CFG_FILENAME "nqiv.cfg"
#if defined(__MINGW32__)
	#define NQIV_CFG_MKDIR      "md"
	#define NQIV_CFG_ENV        "USERPROFILE"
	#define NQIV_CFG_DIRECTORY  "\\AppData\\Roaming\\nqiv\\"
	#define NQIV_CFG_THUMBNAILS NQIV_CFG_DIRECTORY
#else
	#define NQIV_CFG_MKDIR      "mkdir -p"
	#define NQIV_CFG_ENV        "HOME"
	#define NQIV_CFG_DIRECTORY  "/.config/nqiv/"
	#define NQIV_CFG_THUMBNAILS "/.cache/"
#endif

#include <stdint.h>
#include <stdbool.h>
#include <time.h>

typedef struct nqiv_stat_data
{
	size_t size;
	time_t mtime;
} nqiv_stat_data;

char* nqiv_realpath(const char* path, char* resolved_path);
bool  nqiv_stat(const char* path, nqiv_stat_data* data);
bool  nqiv_mkdir(char* path);
bool  nqiv_chmod(const char* filename, uint16_t mode);
bool  nqiv_get_default_cfg(char* output, const size_t length);
bool  nqiv_get_default_cfg_thumbnail_dir(char* output, const size_t length);
void  nqiv_suggest_cfg_setup(const char* exe);

#endif /* NQIV_PLATFORM_H */
