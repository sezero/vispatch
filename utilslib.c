/*
 * VisPatch :  Quake level patcher for water visibility.
 *
 * Copyright (C) 1996-1997  Id Software, Inc.
 * Copyright (C) 1997-2006  Andy Bay <IMarvinTPA@bigfoot.com>
 * Copyright (C) 2006-2008  O. Sezer <sezero@users.sourceforge.net>
 *
 * $Id: utilslib.c,v 1.6 2011-02-18 07:10:02 sezero Exp $
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
#if defined(PLATFORM_UNIX) || defined(PLATFORM_DOS)
#include <unistd.h>
#include <dirent.h>
#include <fnmatch.h>
#endif	/* PLATFORM_UNIX */
#include <ctype.h>
#include "utilslib.h"

/*============================================================================*/

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

	while (*e)
		e++;
	e--;

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

static HANDLE  findhandle;
static WIN32_FIND_DATA finddata;

char *Sys_FindNextFile (void)
{
	BOOL	retval;

	if (!findhandle || findhandle == INVALID_HANDLE_VALUE)
		return NULL;

	retval = FindNextFile(findhandle,&finddata);
	while (retval)
	{
		if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			retval = FindNextFile(findhandle, &finddata);
			continue;
		}

		return finddata.cFileName;
	}

	return NULL;
}

char *Sys_FindFirstFile (const char *path, const char *pattern)
{
	char	tmp_buf[256];

	if (findhandle)
		Error ("Sys_FindFirst without FindClose");

	q_snprintf (tmp_buf, sizeof(tmp_buf), "%s/%s", path, pattern);
	findhandle = FindFirstFile(tmp_buf, &finddata);

	if (findhandle != INVALID_HANDLE_VALUE)
	{
		if (finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			return Sys_FindNextFile();
		else
			return finddata.cFileName;
	}

	return NULL;
}

void Sys_FindClose (void)
{
	if (findhandle != INVALID_HANDLE_VALUE)
		FindClose(findhandle);
	findhandle = NULL;
}

int Sys_filesize (const char *filename)
{
	struct stat status;

	if (stat(filename, &status) != 0)
		return(-1);

	return(status.st_size);
}

int Sys_getcwd (char *buf, size_t size)
{
	if (_getcwd(buf, size) == NULL)
		return 1;

	return 0;
}

#endif	/* PLATFORM_WINDOWS */


//============================================================================

#if defined(PLATFORM_UNIX) || defined(PLATFORM_DOS)

static DIR		*finddir;
static struct dirent	*finddata;
static char		*findpath, *findpattern;
static char		matchpath[256];

char *Sys_FindNextFile (void)
{
	struct stat	test;

	if (!finddir)
		return NULL;

	do {
		finddata = readdir(finddir);
		if (finddata != NULL)
		{
			if (!fnmatch (findpattern, finddata->d_name, FNM_PATHNAME))
			{
				q_snprintf(matchpath, sizeof(matchpath), "%s/%s", findpath, finddata->d_name);
				if ( (stat(matchpath, &test) == 0)
							&& S_ISREG(test.st_mode) )
					return finddata->d_name;
			}
		}
	} while (finddata != NULL);

	return NULL;
}

char *Sys_FindFirstFile (const char *path, const char *pattern)
{
	size_t	tmp_len;

	if (finddir)
		Error ("Sys_FindFirst without FindClose");

	finddir = opendir (path);
	if (!finddir)
		return NULL;

	tmp_len = strlen (pattern);
	findpattern = (char *) calloc (tmp_len + 1, sizeof(char));
	if (!findpattern)
		return NULL;
	strcpy (findpattern, pattern);
	tmp_len = strlen (path);
	findpath = (char *) calloc (tmp_len + 1, sizeof(char));
	if (!findpath)
		return NULL;
	strcpy (findpath, path);
	if (tmp_len)
	{
		--tmp_len;
		/* searching / won't be a good idea, for example.. */
		if (findpath[tmp_len] == '/' || findpath[tmp_len] == '\\')
			findpath[tmp_len] = '\0';
	}

	return Sys_FindNextFile();
}

void Sys_FindClose (void)
{
	if (finddir != NULL)
		closedir(finddir);
	if (findpath != NULL)
		free (findpath);
	if (findpattern != NULL)
		free (findpattern);
	finddir = NULL;
	findpath = NULL;
	findpattern = NULL;
}

int Sys_filesize (const char *filename)
{
	struct stat status;

	if (stat(filename, &status) == -1)
		return(-1);

	return(status.st_size);
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


int host_byteorder;

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

