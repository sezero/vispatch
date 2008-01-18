
/* This modification runs under FreeBSD and Linux */

#ifndef _UNIX_H_
#define _UNIX_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <regex.h>

// Map the Archive bit to the other execute permission bit
#define _A_ARCH S_IXOTH

typedef struct FIND {
 // Fill in with data needed for dos emulation
 char name[256];
 mode_t attribute;
} FIND;

#endif

