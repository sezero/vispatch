/*
 * VisPatch :  Quake level patcher for water visibility.
 *
 * Copyright (C) 1997-2006  Andy Bay <IMarvinTPA@bigfoot.com>
 * Copyright (C) 2006-2007  O. Sezer <sezero@users.sourceforge.net>
 *
 * $Id: utilslib.h,v 1.2 2010-01-11 18:46:13 sezero Exp $
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


#ifndef __QLIBUTILS_H
#define __QLIBUTILS_H

#if defined(PLATFORM_WINDOWS) && !defined(F_OK)
// values for the mode argument of access(). MS does not define them
#define	R_OK	4		/* Test for read permission.  */
#define	W_OK	2		/* Test for write permission.  */
#define	X_OK	1		/* Test for execute permission.  */
#define	F_OK	0		/* Test for existence.  */
#endif

/* strlcpy and strlcat : */
#include "strl_fn.h"

#if HAVE_STRLCAT && HAVE_STRLCPY
/* use native library functions */
#define q_strlcat	strlcat
#define q_strlcpy	strlcpy
#else
/* use our own copies of strlcpy and strlcat taken from OpenBSD */
extern size_t q_strlcpy (char *dst, const char *src, size_t size);
extern size_t q_strlcat (char *dst, const char *src, size_t size);
#endif

#if defined(PLATFORM_WINDOWS)
#define q_strcasecmp	_stricmp
#define q_strncasecmp	_strnicmp
#else
#define q_strcasecmp	strcasecmp
#define q_strncasecmp	strncasecmp
#endif

/* snprintf, vsnprintf : always use our versions. */
/* platform dependant (v)snprintf function names: */
#if defined(PLATFORM_WINDOWS)
#define	snprintf_func		_snprintf
#define	vsnprintf_func		_vsnprintf
#else
#define	snprintf_func		snprintf
#define	vsnprintf_func		vsnprintf
#endif

extern int q_snprintf (char *str, size_t size, const char *format, ...) __attribute__((__format__(__printf__,3,4)));
extern int q_vsnprintf(char *str, size_t size, const char *format, va_list args);

extern char *q_strlwr (char *str);
extern char *q_strupr (char *str);
extern char *q_strrev (char *str);

extern int Sys_getcwd (char *buf, size_t size);
extern int Sys_filesize (const char *filename);

/* simplified findfirst/findnext implementation */
extern char *Sys_FindFirstFile (const char *path, const char *pattern);
extern char *Sys_FindNextFile (void);
extern void Sys_FindClose (void);

extern void Error (const char *error, ...) __attribute__((__format__(__printf__,1,2), __noreturn__));

extern void ValidateByteorder (void);		/* call this from your main() */

#endif	/* __QLIBUTILS_H */

