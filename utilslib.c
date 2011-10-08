/*
 * VisPatch :  Quake level patcher for water visibility.
 *
 * Copyright (C) 1996-1997  Id Software, Inc.
 * Copyright (C) 1997-2006  Andy Bay <IMarvinTPA@bigfoot.com>
 * Copyright (C) 2006-2011  O. Sezer <sezero@users.sourceforge.net>
 *
 * $Id: utilslib.c,v 1.13 2011-10-08 12:33:03 sezero Exp $
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
#if defined(PLATFORM_DOS)
#include <unistd.h>
#include <dos.h>
#include <io.h>
#include <dir.h>
#include <fcntl.h>
#endif	/* PLATFORM_DOS */
#if defined(PLATFORM_UNIX)
#include <unistd.h>
#include <dirent.h>
#include <fnmatch.h>
#endif	/* PLATFORM_UNIX */
#include <ctype.h>
#include "utilslib.h"

/*============================================================================*/

#if defined(__DJGPP__) &&	\
  (!defined(__DJGPP_MINOR__) || __DJGPP_MINOR__ < 4)
/* DJGPP < v2.04 doesn't have [v]snprintf().  */
/* to ensure a proper version check, include stdio.h
 * or go32.h which includes sys/version.h since djgpp
 * versions >= 2.02 and defines __DJGPP_MINOR__ */
#include "djlib/vsnprntf.c"
#endif	/* __DJGPP_MINOR__ < 4 */

int q_vsnprintf(char *str, size_t size, const char *format, va_list args)
{
	int		ret;

	ret = vsnprintf_func (str, size, format, args);

	if (ret < 0)
		ret = (int)size;

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

char *q_strlwr (char *str)
{
	char	*c;
	c = str;
	while (*c)
	{
		*c = tolower(*c);
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

char *Sys_FindFirstFile (const char *path, const char *pattern)
{
	char	tmp_buf[256];
	if (findhandle != INVALID_HANDLE_VALUE)
		Error ("Sys_FindFirst without FindClose");
	q_snprintf (tmp_buf, sizeof(tmp_buf), "%s/%s", path, pattern);
	findhandle = FindFirstFile(tmp_buf, &finddata);
	if (findhandle == INVALID_HANDLE_VALUE)
		return NULL;
	if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		return Sys_FindNextFile();
	return finddata.cFileName;
}

char *Sys_FindNextFile (void)
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
	if (_getcwd(buf, size) == NULL)
		return 1;

	return 0;
}

#endif	/* PLATFORM_WINDOWS */


//============================================================================

#if defined(PLATFORM_DOS)

static struct ffblk	finddata;
static int		findhandle = -1;

char *Sys_FindFirstFile (const char *path, const char *pattern)
{
	char	tmp_buf[256];

	if (findhandle == 0)
		Error ("Sys_FindFirst without FindClose");

	q_snprintf (tmp_buf, sizeof(tmp_buf), "%s/%s", path, pattern);
	memset (&finddata, 0, sizeof(finddata));

	findhandle = findfirst(tmp_buf, &finddata, FA_ARCH | FA_RDONLY);
	if (findhandle == 0)
		return finddata.ff_name;

	return NULL;
}

char *Sys_FindNextFile (void)
{
	if (findhandle != 0)
		return NULL;

	if (findnext(&finddata) == 0)
		return finddata.ff_name;

	return NULL;
}

void Sys_FindClose (void)
{
	findhandle = -1;
}

int Sys_filesize (const char *filename)
{
	struct ffblk	f;

	if (findfirst(filename, &f, FA_ARCH | FA_RDONLY) != 0)
		return -1;

	return (int) f.ff_fsize;
}

int Sys_getcwd (char *buf, size_t size)
{
	if (getcwd(buf, size) == NULL)
		return 1;

	return 0;
}

#endif	/* PLATFORM_DOS */


//============================================================================

#if defined(PLATFORM_UNIX)

static DIR		*finddir;
static struct dirent	*finddata;
static char		*findpath, *findpattern;
static char		matchpath[256];

char *Sys_FindFirstFile (const char *path, const char *pattern)
{
	if (finddir)
		Error ("Sys_FindFirst without FindClose");

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

char *Sys_FindNextFile (void)
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
#define	__byteswap_func	static
#else
#define	__byteswap_func
#endif	/* ENDIAN_RUNTIME_DETECT */

#if ENDIAN_RUNTIME_DETECT
/*
# warning "Byte order will be detected at runtime"
*/
#elif defined(ENDIAN_ASSUMED_UNSAFE)
# warning "Cannot determine byte order:"
# if (ENDIAN_ASSUMED_UNSAFE == LITTLE_ENDIAN)
#    warning "Using LIL endian as an UNSAFE default."
# elif (ENDIAN_ASSUMED_UNSAFE == BIG_ENDIAN)
#    warning "Using BIG endian as an UNSAFE default."
# endif
# warning "Revise the macros in q_endian.h for this"
# warning "machine or use runtime detection !!!"
#endif	/* ENDIAN_ASSUMED_UNSAFE */


int	host_byteorder;
int	host_bigendian;	/* bool */

int DetectByteorder (void)
{
	int	i = 0x12345678;
		/*    U N I X */

	/*
	BE_ORDER:  12 34 56 78
		   U  N  I  X

	LE_ORDER:  78 56 34 12
		   X  I  N  U

	PDP_ORDER: 34 12 78 56
		   N  U  X  I
	*/

	if ( *(char *)&i == 0x12 )
		return BIG_ENDIAN;
	else if ( *(char *)&i == 0x78 )
		return LITTLE_ENDIAN;
	else if ( *(char *)&i == 0x34 )
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

int	(*BigLong) (int);
int	(*LittleLong) (int);

#endif	/* ENDIAN_RUNTIME_DETECT */

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
#endif	/* ENDIAN_RUNTIME_DETECT */
}

void ValidateByteorder (void)
{
	const char	*endianism[] = { "BE", "LE", "PDP", "Unknown" };
	int		i;

	ByteOrder_Init ();
	switch (host_byteorder)
	{
	case BIG_ENDIAN:
		i = 0; break;
	case LITTLE_ENDIAN:
		i = 1; break;
	case PDP_ENDIAN:
		host_byteorder = -1;	/* not supported */
		i = 2; break;
	default:
		i = 3; break;
	}
	if (host_byteorder < 0)
		Error ("Unsupported byte order.");
	/*
	printf("Detected byte order: %s\n", endianism[i]);
	*/
#if !ENDIAN_RUNTIME_DETECT
	if (host_byteorder != BYTE_ORDER)
	{
		int		j;

		switch (BYTE_ORDER)
		{
		case BIG_ENDIAN:
			j = 0; break;
		case LITTLE_ENDIAN:
			j = 1; break;
		case PDP_ENDIAN:
			j = 2; break;
		default:
			j = 3; break;
		}
		Error ("Detected byte order %s doesn't match compiled %s order!",
			endianism[i], endianism[j]);
	}
#endif	/* ENDIAN_RUNTIME_DETECT */
}

