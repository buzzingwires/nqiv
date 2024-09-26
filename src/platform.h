#ifndef NQIV_PLATFORM_H
#define NQIV_PLATFORM_H

/*
 * A major philosophy of nqiv is that conditional compilation should be kept to
 * a minimum and that when it is necessary, it should primarily be restricted
 * to a module that provides platform-specific features with a standard
 * interface that matches the idioms of the wider project. Additionally, any
 * platform-related sanity checks may be performed here.
 *
 * Further, when this header is included, it should be included first since its
 * behavior can affect the behavior of other headers.
 */

#if !defined(__unix__) && !defined(__linux__) && !defined(__gnu_linux__) && !defined(__MINGW32__)
	#error "Currently, only Unix, Linux, and windows through MinGW are supported."
#else
	/* Standard library headers will have different functionality with this set. */
	#define _GNU_SOURCE
#endif

/* Platform-specific config paths and suggested commands to create them. */
#define NQIV_CFG_FILENAME "nqiv.cfg"
#if defined(__MINGW32__)
	#define NQIV_CFG_CMD        "cmd"
	#define NQIV_CFG_ESC        "\""
	#define NQIV_CFG_MKDIR      "md"
	#define NQIV_CFG_ENV        "USERPROFILE"
	#define NQIV_CFG_DIRECTORY  "\\AppData\\Roaming\\nqiv\\"
	#define NQIV_CFG_THUMBNAILS NQIV_CFG_DIRECTORY
	#define NQIV_CFG_PATHSEP    "\\"
#else
	#define NQIV_CFG_CMD        "sh -c"
	#define NQIV_CFG_ESC        "\\"
	#define NQIV_CFG_MKDIR      "mkdir -p"
	#define NQIV_CFG_ENV        "HOME"
	#define NQIV_CFG_DIRECTORY  "/.config/nqiv/"
	#define NQIV_CFG_THUMBNAILS "/.cache/"
	#define NQIV_CFG_PATHSEP    "/"
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
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
bool  nqiv_get_default_cfg(char* output, const int length);
bool  nqiv_get_default_cfg_thumbnail_dir(char* output, const int length);
void  nqiv_suggest_cfg_setup(const char* exe);
/* Output asserted to 0 to INT_MAX */
int   nqiv_strlen(const char* str);
/* Works like strtol but with a regular int. */
int   nqiv_strtoi(const char* str, char** endptr, int base);
/* Simple pointer arithmetic with result asserted to be within an 0 to INT_MAX */
int   nqiv_ptrdiff(const void* a, const void* b);
/* Get realpath and expand starting tilde to user directory. */
bool  nqiv_expand_path(char* output, const int length, const char* input);
/* Expand a path, open it, and return its file object. */
FILE* nqiv_fopen(const char* filename, const char* mode);

#endif /* NQIV_PLATFORM_H */
