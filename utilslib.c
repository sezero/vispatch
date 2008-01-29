/*
 * VisPatch :  Quake level patcher for water visibility.
 *
 * Copyright (C) 1997-2006  Andy Bay <IMarvinTPA@bigfoot.com>
 * Copyright (C) 2006-2007  O. Sezer <sezero@users.sourceforge.net>
 *
 * $Id: utilslib.c,v 1.2 2008-01-29 18:00:10 sezero Exp $
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
#include <io.h>
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

void ValidateByteorder (void)
{
	const char	*endianism[] = { "BE", "LE", "PDP", "Unknown" };
	const char	*tmp;

	ByteOrder_Init ();
	if (host_byteorder < 0)
		Error ("%s: Unsupported byte order.", __thisfunc__);
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
		break;
	default:
		tmp = endianism[3];
		break;
	}
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

/*============================================================================*/

#if defined(PLATFORM_WINDOWS)

static long	findhandle;
static struct _finddata_t	finddata;

char *Sys_FindNextFile (void)
{
	int		retval;

	if (!findhandle || findhandle == -1)
		return NULL;

	retval = _findnext (findhandle, &finddata);
	while (retval != -1)
	{
		if (finddata.attrib & _A_SUBDIR)
		{
			retval = _findnext (findhandle, &finddata);
			continue;
		}

		return finddata.name;
	}

	return NULL;
}

char *Sys_FindFirstFile (const char *path, const char *pattern)
{
	char	tmp_buf[256];

	if (findhandle)
		Error ("Sys_FindFirst without FindClose");

	q_snprintf (tmp_buf, sizeof(tmp_buf), "%s%s", path, pattern);
	findhandle = _findfirst (tmp_buf, &finddata);

	if (findhandle != -1)
	{
		if (finddata.attrib & _A_SUBDIR)
			return Sys_FindNextFile();
		else
			return finddata.name;
	}

	return NULL;
}

void Sys_FindClose (void)
{
	if (findhandle != -1)
		_findclose (findhandle);
	findhandle = 0;
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

