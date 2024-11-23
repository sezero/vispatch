/*
 * VisPatch :  Quake level patcher for water visibility.
 *
 * Copyright (C) 1997-2006  Andy Bay <IMarvinTPA@bigfoot.com>
 * Copyright (C) 2006-2011  O. Sezer <sezero@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to:
 *
 * Free Software Foundation, Inc.
 * 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
*/

#ifndef LIBUTILS_H
#define LIBUTILS_H

/* strlcpy and strlcat */
#include "strl_fn.h"

/* locale-insensitive strcasecmp replacement functions: */
extern int q_strcasecmp (const char * s1, const char * s2);

extern int q_snprintf (char *str, size_t size, const char *format, ...) FUNC_PRINTF(3,4);
extern int q_vsnprintf(char *str, size_t size, const char *format, va_list args) FUNC_PRINTF(3,0);

extern char *q_strlwr (char *str);
extern char *q_strrev (char *str);

extern int Sys_getcwd (char *buf, size_t size);
extern int Sys_filesize (const char *filename);

/* simplified findfirst/findnext implementation */
extern const char *Sys_FindFirstFile (const char *path, const char *pattern);
extern const char *Sys_FindNextFile (void);
extern void Sys_FindClose (void);

FUNC_NORETURN
extern void Error (const char *error, ...) FUNC_PRINTF(1,2);

extern void ValidateByteorder (void);		/* call this from your main() */

#endif	/* LIBUTILS_H */
