/*
 * VisPatch :  Quake level patcher for water visibility.
 *
 * Copyright (C) 1996-1997  Id Software, Inc.
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

#include "arch_def.h"
#include "compiler.h"
#include "q_stdinc.h"
#include "q_endian.h"
#include <sys/stat.h>
#if defined(PLATFORM_WINDOWS)
#include <windows.h>
#include <io.h>
#include <direct.h>
#endif	/* PLATFORM_WINDOWS */
#if defined(PLATFORM_UNIX)
#include <unistd.h>
#include <dirent.h>
#include <fnmatch.h>
#endif	/* PLATFORM_UNIX */
#include <ctype.h>
#include "utilslib.h"

/*============================================================================*/

/* platform dependant (v)snprintf function names: */
#if defined(PLATFORM_WINDOWS)
#define	snprintf_func		_snprintf
#define	vsnprintf_func		_vsnprintf
#else
#define	snprintf_func		snprintf
#define	vsnprintf_func		vsnprintf
#endif

int q_vsnprintf(char *str, size_t size, const char *format, va_list args)
{
	int		ret;

	ret = vsnprintf_func (str, size, format, args);

	if (ret < 0)
		ret = (int)size;
	if (size == 0)	/* no buffer */
		return ret;
	if ((size_t)ret >= size)
		str[size - 1] = '\0';

	return ret;
}

int q_snprintf (char *str, size_t size, const char *format, ...)
{
	int		ret;
	va_list		argptr;

	va_start (argptr, format);
	ret = q_vsnprintf (str, size, format, argptr);
	va_end (argptr);

	return ret;
}

static inline int q_isupper(int c)
{
	return (c >= 'A' && c <= 'Z');
}

static inline int q_tolower(int c)
{
	return ((q_isupper(c)) ? (c | ('a' - 'A')) : c);
}

int q_strcasecmp(const char * s1, const char * s2)
{
	const char * p1 = s1;
	const char * p2 = s2;
	char c1, c2;

	if (p1 == p2)
		return 0;

	do
	{
		c1 = q_tolower (*p1++);
		c2 = q_tolower (*p2++);
		if (c1 == '\0')
			break;
	} while (c1 == c2);

	return (int)(c1 - c2);
}

int q_strncasecmp(const char *s1, const char *s2, size_t n)
{
	const char * p1 = s1;
	const char * p2 = s2;
	char c1, c2;

	if (p1 == p2 || n == 0)
		return 0;

	do
	{
		c1 = q_tolower (*p1++);
		c2 = q_tolower (*p2++);
		if (c1 == '\0' || c1 != c2)
			break;
	} while (--n > 0);

	return (int)(c1 - c2);
}

char *q_strlwr (char *str)
{
	char	*c;
	c = str;
	while (*c)
	{
		*c = q_tolower(*c);
		c++;
	}
	return str;
}

char *q_strrev (char *str)
{
	char a, *b, *e;

	b = e = str;

	while (*e++)
		;
	e -= 2;

	while ( b < e )
	{
		a = *b;
		*b = *e;
		*e = a;
		b++;
		e--;
	}
	return str;
}


/*============================================================================*/

void Error (const char *error, ...)
{
	va_list argptr;

	fprintf (stderr, "*** ERROR: ***\n");
	va_start (argptr, error);
	vfprintf (stderr, error, argptr);
	va_end (argptr);
	fprintf (stderr, "\n");
	exit (1);
}

/*============================================================================*/

#if defined(PLATFORM_WINDOWS)

static HANDLE findhandle = INVALID_HANDLE_VALUE;
static WIN32_FIND_DATA finddata;
static char		findstr[256];

const char *Sys_FindFirstFile (const char *path, const char *pattern)
{
	if (findhandle != INVALID_HANDLE_VALUE)
		Error ("FindFirst without FindClose");
	q_snprintf (findstr, sizeof(findstr), "%s/%s", path, pattern);
	findhandle = FindFirstFile(findstr, &finddata);
	if (findhandle == INVALID_HANDLE_VALUE)
		return NULL;
	if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		return Sys_FindNextFile();
	return finddata.cFileName;
}

const char *Sys_FindNextFile (void)
{
	if (findhandle == INVALID_HANDLE_VALUE)
		return NULL;
	while (FindNextFile(findhandle, &finddata) != 0)
	{
		if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			continue;
		return finddata.cFileName;
	}
	return NULL;
}

void Sys_FindClose (void)
{
	if (findhandle != INVALID_HANDLE_VALUE)
	{
		FindClose(findhandle);
		findhandle = INVALID_HANDLE_VALUE;
	}
}

int Sys_filesize (const char *filename)
{
	HANDLE fh;
	WIN32_FIND_DATA data;
	int size;

	fh = FindFirstFile(filename, &data);
	if (fh == INVALID_HANDLE_VALUE)
		return -1;
	FindClose(fh);
	if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		return -1;
//	we're not dealing with gigabytes of files.
//	size should normally smaller than INT_MAX.
//	size = (data.nFileSizeHigh * (MAXDWORD + 1)) + data.nFileSizeLow;
	size = (int) data.nFileSizeLow;
	return size;
}

int Sys_getcwd (char *buf, size_t size)
{
	const int sz = (int) size;

	if (sz <= 0) return 0;
	if (_getcwd(buf, sz) == NULL)
		return 1;

	return 0;
}

#endif	/* PLATFORM_WINDOWS */


//============================================================================

#if defined(PLATFORM_UNIX)

static DIR		*finddir;
static struct dirent	*finddata;
static char		*findpath, *findpattern;
static char		matchpath[256];

const char *Sys_FindFirstFile (const char *path, const char *pattern)
{
	if (finddir)
		Error ("FindFirst without FindClose");

	finddir = opendir (path);
	if (!finddir)
		return NULL;

	findpattern = strdup (pattern);
	if (!findpattern)
	{
		Sys_FindClose();
		return NULL;
	}

	findpath = strdup (path);
	if (!findpath)
	{
		Sys_FindClose();
		return NULL;
	}

	if (*findpath != '\0')
	{
	/* searching under "/" won't be a good idea, for example.. */
		size_t siz = strlen(findpath) - 1;
		if (findpath[siz] == '/' || findpath[siz] == '\\')
			findpath[siz] = '\0';
	}

	return Sys_FindNextFile();
}

const char *Sys_FindNextFile (void)
{
	struct stat	test;

	if (!finddir)
		return NULL;

	while ((finddata = readdir(finddir)) != NULL)
	{
		if (!fnmatch (findpattern, finddata->d_name, FNM_PATHNAME))
		{
			q_snprintf(matchpath, sizeof(matchpath), "%s/%s", findpath, finddata->d_name);
			if ( (stat(matchpath, &test) == 0)
						&& S_ISREG(test.st_mode))
				return finddata->d_name;
		}
	}

	return NULL;
}

void Sys_FindClose (void)
{
	if (finddir != NULL)
	{
		closedir(finddir);
		finddir = NULL;
	}
	if (findpath != NULL)
	{
		free (findpath);
		findpath = NULL;
	}
	if (findpattern != NULL)
	{
		free (findpattern);
		findpattern = NULL;
	}
}

int Sys_filesize (const char *filename)
{
	struct stat status;

	if (stat(filename, &status) == -1)
		return -1;
	if (! S_ISREG(status.st_mode))
		return -1;

	return (int) status.st_size;
}

int Sys_getcwd (char *buf, size_t size)
{
	if (getcwd(buf, size) == NULL)
		return 1;

	return 0;
}

#endif	/* PLATFORM_UNIX */


/*============================================================================*/

/*========== BYTE ORDER STUFF ================================================*/

#if ENDIAN_RUNTIME_DETECT
#define __byteswap_func static
#else
#define __byteswap_func
#endif

int host_byteorder;
int host_bigendian; /* qboolean */

FUNC_NOINLINE FUNC_NOCLONE
unsigned int get_0x12345678 (void) {
	return 0x12345678;
	/*       U N I X  */
}

int DetectByteorder (void)
{
	volatile union {
		unsigned int i;
		unsigned char c[4];
	} bint;

	bint.i = get_0x12345678 ();

	/*
	BE_ORDER:  12 34 56 78
	           U  N  I  X

	LE_ORDER:  78 56 34 12
	           X  I  N  U

	PDP_ORDER: 34 12 78 56
	           N  U  X  I
	*/

	if (bint.c[0] == 0x12)
		return BIG_ENDIAN;
	if (bint.c[0] == 0x78)
		return LITTLE_ENDIAN;
	if (bint.c[0] == 0x34)
		return PDP_ENDIAN;

	return -1;
}

__byteswap_func
int LongSwap (int l)
{
	unsigned char	b1, b2, b3, b4;

	b1 = l & 255;
	b2 = (l>>8 ) & 255;
	b3 = (l>>16) & 255;
	b4 = (l>>24) & 255;

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

#if ENDIAN_RUNTIME_DETECT

__byteswap_func
int LongNoSwap (int l)
{
	return l;
}

int   (*BigLong) (int);
int   (*LittleLong) (int);

#endif /* ENDIAN_RUNTIME_DETECT */

void ByteOrder_Init (void)
{
	host_byteorder = DetectByteorder ();
	host_bigendian = (host_byteorder == BIG_ENDIAN);

#if ENDIAN_RUNTIME_DETECT
	switch (host_byteorder)
	{
	case BIG_ENDIAN:
		BigLong = LongNoSwap;
		LittleLong = LongSwap;
		break;

	case LITTLE_ENDIAN:
		BigLong = LongSwap;
		LittleLong = LongNoSwap;
		break;

	default:
		break;
	}
#endif /* ENDIAN_RUNTIME_DETECT */
}

/* call this from your main() */
void ValidateByteorder (void)
{
	const char	*endianism[] = { "BE", "LE", "PDP", "Unknown" };
	const char	*tmp;

	ByteOrder_Init ();
	switch (host_byteorder)
	{
	case BIG_ENDIAN:
		tmp = endianism[0];
		break;
	case LITTLE_ENDIAN:
		tmp = endianism[1];
		break;
	case PDP_ENDIAN:
		tmp = endianism[2];
		host_byteorder = -1;	/* not supported */
		break;
	default:
		tmp = endianism[3];
		break;
	}
	if (host_byteorder < 0)
		Error ("Unsupported byte order.");
//	printf("Detected byte order: %s\n", tmp);
#if !ENDIAN_RUNTIME_DETECT
	if (host_byteorder != BYTE_ORDER)
	{
		const char	*tmp2;

		switch (BYTE_ORDER)
		{
		case BIG_ENDIAN:
			tmp2 = endianism[0];
			break;
		case LITTLE_ENDIAN:
			tmp2 = endianism[1];
			break;
		case PDP_ENDIAN:
			tmp2 = endianism[2];
			break;
		default:
			tmp2 = endianism[3];
			break;
		}
		Error ("Detected byte order %s doesn't match compiled %s order!", tmp, tmp2);
	}
#endif	/* ENDIAN_RUNTIME_DETECT */
}

