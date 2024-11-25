/*
	q_stdinc.h
	includes the minimum necessary stdc headers,

	Copyright (C) 1996-1997  Id Software, Inc.
	Copyright (C) 2007-2011  O.Sezer <sezero@users.sourceforge.net>

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 2
	of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

	See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to:

		Free Software Foundation, Inc.
		51 Franklin St, Fifth Floor,
		Boston, MA  02110-1301  USA
*/

#ifndef QSTDINC_H
#define QSTDINC_H

#include <sys/types.h>
#include <stddef.h>
#include <limits.h>

#include <stdio.h>

#if 0
#include "q_stdint.h"
#endif

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#if !defined(_WIN32)
#include <strings.h>	/* strcasecmp and strncasecmp	*/
#endif	/* ! PLATFORM_WINDOWS */

/*==========================================================================*/

#ifndef NULL
#if defined(__cplusplus)
#define	NULL		0
#else
#define	NULL		((void *)0)
#endif
#endif

/* Make sure the types really have the right
 * sizes: These macros are from SDL headers.
 */
#define	COMPILE_TIME_ASSERT(name, x)	\
	typedef int dummy_ ## name[(x) * 2 - 1]

COMPILE_TIME_ASSERT(char, sizeof(char) == 1);
COMPILE_TIME_ASSERT(long, sizeof(long) >= 4);
COMPILE_TIME_ASSERT(int, sizeof(int) == 4);
COMPILE_TIME_ASSERT(short, sizeof(short) == 2);


/* Provide a substitute for offsetof() if we don't have one.
 * This variant works on most (but not *all*) systems...
 */
#ifndef offsetof
#define offsetof(t,m) ((size_t)&(((t *)0)->m))
#endif


/*==========================================================================*/

/* MAX_OSPATH (max length of a filesystem pathname, i.e. PATH_MAX)
 * Note: See GNU Hurd and others' notes about brokenness of this:
 * http://www.gnu.org/software/hurd/community/gsoc/project_ideas/maxpath.html
 * http://insanecoding.blogspot.com/2007/11/pathmax-simply-isnt.html */

#if defined(__DJGPP__) || defined(_MSDOS) || defined(__MSDOS__) || defined(__DOS__)
/* 256 is more than enough */
#if !defined(PATH_MAX)
#define PATH_MAX	256
#endif
#define MAX_OSPATH	256

#else

#if !defined(PATH_MAX)
/* equivalent values? */
#if defined(MAXPATHLEN)
#define PATH_MAX	MAXPATHLEN
#elif defined(_WIN32) && defined(_MAX_PATH)
#define PATH_MAX	_MAX_PATH
#elif defined(_WIN32) && defined(MAX_PATH)
#define PATH_MAX	MAX_PATH
#elif defined(__OS2__) && defined(CCHMAXPATH)
#define PATH_MAX	CCHMAXPATH
#else /* fallback */
#define PATH_MAX	1024
#endif
#endif	/* PATH_MAX */

#define MAX_OSPATH	PATH_MAX
#endif	/* MAX_OSPATH */

/*==========================================================================*/

#endif	/* QSTDINC_H */

