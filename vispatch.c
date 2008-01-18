/*
 * Copyright (C) 1997-2006  Andy Bay <IMarvinTPA@bigfoot.com>
 * Copyright (C) 2006  O. Sezer <sezero@users.sourceforge.net>
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

/*
 * VisPatch :  Quake level patcher for water visibility.
 *
 * Compiling for Linux / Unix :
 * gcc -DPLATFORM_UNIX -Wall -Wshadow -o vispatch vispatch.c
 *
 * Compiling for Windows:
 * If cross-compiling on linux, use the included build_win.sh
 * script. If compiling on native windows, then:
 * gcc -Wall -Wshadow -mconsole -o vispatch.exe vispatch.c
 *
 */

#include "vispatch.h"
#include <sys/stat.h>
#if defined(_WIN32)
#include <io.h>
#endif
#if defined(PLATFORM_UNIX)
#include <unistd.h>
#include <dirent.h>
#include <fnmatch.h>
#endif


visdat_t	*visdat;
pakentry_t	NewPakEnt[MAX_FILES_IN_PACK];
unsigned int	NPcnt, numvis;
int		usepak = 0;
int		mode = 0;	/* 0: patch, 2: patch without overwriting, 1: extract	*/

FILE		*InFile, *OutFile, *fVIS;

#define		TEMP_FILE_NAME		"~vistmp.tmp"
char		Path[256],	/* where we shall work: getcwd(), changed by -dir	*/
		Path2[256],	/* temporary filename buffer				*/
		TempFile[256],	/* name of our temporary file on disk			*/
		VIS[256] = "vispatch.dat",	/* vis data filename. changed by -data	*/
		FoutPak[256] = "pak*.pak",	/* name of the output pak file		*/
		File[256] = "pak*.pak",		/* filename pattern we'll be globbing	*/
		CurName[VISPATCH_IDLEN+6];	/* name of the currently processed file	*/


//============================================================================

#define CLOSE_ALL {			\
	if (InFile)			\
		fclose(InFile);		\
	if (OutFile)			\
		fclose(OutFile);	\
	if (fVIS)			\
		fclose(fVIS);		\
}


#define FWRITE_ERROR {							\
	printf("Errors during fwrite: Not enough disk space??\n");	\
	CLOSE_ALL;							\
	remove(TempFile);						\
	freevis();							\
	exit (2);							\
}


//============================================================================

#include "strlcat.c"
#include "strlcpy.c"

#if HAVE_STRLCAT && HAVE_STRLCPY
// use native library functions
#define strlcat_	strlcat
#define strlcpy_	strlcpy
#endif


//============================================================================

#ifdef _WIN32

#define strlwr_(a)		strlwr((a))
#define strrev_(a)		strrev((a))
#define strcasecmp_(a,b)	strcmpi((a),(b))

/* From MinGW runtime/init.c :
 * [...] GetMainArgs (used below) takes a fourth argument
 * which is an int that controls the globbing of the command line. If
 * _CRT_glob is non-zero the command line will be globbed (e.g. *.*
 * expanded to be all files in the startup directory). In the mingw32
 * library a _CRT_glob variable is defined as being -1, enabling
 * this command line globbing by default. To turn it off and do all
 * command line processing yourself (and possibly escape bogons in
 * MS's globbing code) include a line in one of your source modules
 * defining _CRT_glob and setting it to zero, like this:
 *  int _CRT_glob = 0;
 */
int	_CRT_glob = 0;

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

char *Sys_FindFirstFile (char *path, char *pattern)
{
	char	tmp_buf[256];

	if (findhandle)
	{
		printf("Error: Sys_FindFirst without FindClose\n");
		exit (2);
	}

	snprintf (tmp_buf, sizeof(tmp_buf), "%s%s", path, pattern);
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

int Sys_filesize (char *filename)
{
	struct stat status;

	if (stat(filename, &status) != 0)
		return(-1);

	return(status.st_size);
}

int Sys_getcwd (char *buf, size_t size)
{
	if ( !( _getcwd(buf, size) ) )
		return 1;

	return ( strlcat_(buf, "\\", size) >= size );
}


#endif	// _WIN32


//============================================================================

#ifdef PLATFORM_UNIX

#define strcasecmp_(a,b)	strcasecmp((a),(b))

char *strlwr_ (char *str)
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

char *strrev_ (char *s)
{
	char a, *b, *e;

	b = e = s;

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
	return s;
}


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
				snprintf(matchpath, sizeof(matchpath), "%s/%s", findpath, finddata->d_name);
				if ( (stat(matchpath, &test) == 0)
							&& S_ISREG(test.st_mode) )
					return finddata->d_name;
			}
		}
	} while (finddata != NULL);

	return NULL;
}

char *Sys_FindFirstFile (char *path, char *pattern)
{
	size_t	tmp_len;

	if (finddir)
	{
		printf("Error: Sys_FindFirst without FindClose\n");
		exit (2);
	}

	finddir = opendir (path);
	if (!finddir)
		return NULL;

	tmp_len = strlen (pattern);
	findpattern = (char *) malloc (tmp_len + 1);
	if (!findpattern)
		return NULL;
//	strcpy (findpattern, pattern);
	memcpy (findpattern, pattern, tmp_len);
	findpattern[tmp_len] = '\0';
	tmp_len = strlen (path);
	findpath = (char *) malloc (tmp_len + 1);
	if (!findpath)
		return NULL;
//	strcpy (findpath, path);
	memcpy (findpath, path, tmp_len);
	findpath[tmp_len] = '\0';
	if (findpath[tmp_len-1] == '/' || findpath[tmp_len-1] == '\\')
		findpath[tmp_len-1] = '\0';

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

int Sys_filesize (char *filename)
{
	struct stat status;

	if (stat(filename, &status) == -1)
		return(-1);

	return(status.st_size);
}

int Sys_getcwd (char *buf, size_t size)
{
	if ( !( getcwd(buf, size) ) )
		return 1;

	return ( strlcat_(buf, "/", size) >= size );
}

#endif	// PLATFORM_UNIX


//============================================================================

short ShortSwap (short l)
{
	unsigned char	b1, b2;

	b1 = l&255;
	b2 = (l>>8)&255;

	return (b1<<8) + b2;
}

int LongSwap (int l)
{
	unsigned char	b1, b2, b3, b4;

	b1 = l&255;
	b2 = (l>>8)&255;
	b3 = (l>>16)&255;
	b4 = (l>>24)&255;

	return ((int)b1<<24) + ((int)b2<<16) + ((int)b3<<8) + b4;
}

float FloatSwap (float f)
{
	union
	{
		float	f;
		unsigned char	b[4];
	} dat1, dat2;

	dat1.f = f;
	dat2.b[0] = dat1.b[3];
	dat2.b[1] = dat1.b[2];
	dat2.b[2] = dat1.b[1];
	dat2.b[3] = dat1.b[0];
	return dat2.f;
}

// endianness stuff: <sys/types.h> is supposed
// to succeed in locating the correct endian.h
// this BSD style may not work everywhere, eg. on WIN32
#if !defined(BYTE_ORDER) || !defined(LITTLE_ENDIAN) || !defined(BIG_ENDIAN) || (BYTE_ORDER != LITTLE_ENDIAN && BYTE_ORDER != BIG_ENDIAN)
#undef BYTE_ORDER
#undef LITTLE_ENDIAN
#undef BIG_ENDIAN
#define LITTLE_ENDIAN 1234
#define BIG_ENDIAN 4321
#endif
// assumptions in case we don't have endianness info
#ifndef BYTE_ORDER
#if defined(_WIN32)
#define BYTE_ORDER LITTLE_ENDIAN
#else
#if defined(__sun)
#if defined(__i386) || defined(__amd64)
#define BYTE_ORDER LITTLE_ENDIAN
#else
#define BYTE_ORDER BIG_ENDIAN
#endif
#else
#define BYTE_ORDER LITTLE_ENDIAN
#warning "Unable to determine CPU endianess. Defaulting to little endian"
#endif
#endif
#endif

#if BYTE_ORDER == BIG_ENDIAN
#define BigShort(s) (s)
#define LittleShort(s) ShortSwap((s))
#define BigLong(l) (l)
#define LittleLong(l) LongSwap((l))
#define BigFloat(f) (f)
#define LittleFloat(f) FloatSwap((f))
#else
// BYTE_ORDER == LITTLE_ENDIAN
#define BigShort(s) ShortSwap((s))
#define LittleShort(s) (s)
#define BigLong(l) LongSwap((l))
#define LittleLong(l) (l)
#define BigFloat(f) FloatSwap((f))
#define LittleFloat(f) (f)
#endif

// end of endianness stuff


//============================================================================

void usage (void)
{
	printf("Usage: vispatch <file> [arguments]\n\n");
//	printf(" -h, -help  or  --help : Prints this message.\n\n");
	printf("<file> : Level filename pattern, bsp or pak type, wildcards\n");
	printf("\t are allowed, relative paths not allowed. See the\n");
	printf("\t examples.\n\n");
	printf(" -dir  : Specify the directory that the level files are in.\n\n");
	printf(" -data : Specify the vis data file. Wildcards are not allowed.\n\n");
	printf(" -new  : Tells the program not to overwrite the original level\n");
	printf("\t files.  In case of pak files, the patched the levels\n");
	printf("\t will be put into a new pak file. In case of standalone\n");
	printf("\t maps files, the original maps are renamed to name.bak,\n");
	printf("\t and the patched ones are saved as name.bsp .\n");
	printf("\t Can not be used in combination with -extract.\n\n");
	printf(" -extract: Retrieve all the vis data from the given file.\n");
	printf("\t Can not be used in combination with -new.\n\n");
#if 1
	printf("See the vispatch.txt file for further information.\n\n");
#else
	printf("Press Enter to continue...\n");

	while (('\n' != getchar())) { }

	printf("Examples:\n");
	printf("vispatch map25.bsp\n");
	printf("  Patch and overwrite map25.bsp using vis data from the default\n");
	printf("  data file vispatch.dat\n\n");
	printf("vispatch map25.bsp -data map25.vis\n");
	printf("  Use map25.vis as the data file instead of vispatch.dat\n\n");
	printf("vispatch -dir hipnotic -data hipnotic.vis\n");
	printf("  Patch pak or bsp type level files under the directory named\n");
	printf("  hipnotic, using  hipnotic.vis  as the data.\n\n");
	printf("vispatch somefile.ext -extract\n");
	printf("  Retrieve all vis data from somefile.ext and put it into a file\n");
	printf("  named vispatch.dat  (You must backup any previous one!.)\n\n");
	printf("vispatch somefile.ext -data some.dat -extract\n");
	printf("  Retrieve all vis data from the somefile.ext and dump it into\n");
	printf("  a file named some.dat instead of vispatch.dat\n\n");
	printf("vispatch -dir hipnotic -extract\n");
	printf("  Retrieve all vis data from the files in the directory\n");
	printf("  hipnotic, and dump it into a file named vispatch.dat\n");
#endif
	exit (0);
}


//============================================================================

int main (int argc, char **argv)
{
	int	ret = 0, tmp;
	char	*testname;

	printf("VisPatch %s by Andy Bay\n", VERS);
	printf("Revised and fixed by O.Sezer\n");

	if (argc > 1)
	{
		for (tmp = 1; tmp < argc; tmp++)
		{
			strlwr_(argv[tmp]);
			if (strcmp(argv[tmp], "-?"    ) == 0 ||
			    strcmp(argv[tmp], "-h"    ) == 0 ||
			    strcmp(argv[tmp], "-help" ) == 0 ||
			    strcmp(argv[tmp], "--help") == 0 )
				usage();
		}
	}

	if ( Sys_getcwd(Path,sizeof(Path)) != 0)
	{
		printf("Unable to determine current working directory\n");
		exit (2);
	}
	printf("Current directory: %s\n", Path);

	if (argc > 1)
	{
		for (tmp = 1; tmp < argc; tmp++)
		{
			if (argv[tmp][0] == '-' || argv[tmp][0] == '/')
			{
				if (argv[tmp][0] == '/')
					argv[tmp][0] = '-';

				if (strcmp(argv[tmp],"-data") == 0)
				{
					argv[tmp][0] = 0;
					if (++tmp == argc)
					{
						printf("You must specify the filename of visdata after -data\n");
						exit (1);
					}
					strlcpy_ (VIS, argv[tmp], sizeof(VIS));
					argv[tmp][0] = 0;
				}
				else if (strcmp(argv[tmp],"-dir") == 0)
				{
					argv[tmp][0] = 0;
					if (++tmp == argc)
					{
						printf("You must specify a directory name after -dir\n");
						exit (1);
					}
					strlcpy_ (Path, argv[tmp], sizeof(Path)-1);
					// -2 : reserve space for a trailing dir separator
					Path[sizeof(Path)-2] = 0;
					argv[tmp][0] = 0;
					printf("Will look into %s as the pak/bsp directory..\n", Path);
#	ifdef PLATFORM_UNIX
					// include the path separator at the end
					if (Path[strlen(Path)-1] != '/')
						strlcat_ (Path, "/", sizeof(Path));
#	endif
#	ifdef _WIN32
					// include the path separator at the end
					if (Path[strlen(Path)-1] != '\\')
						strlcat_ (Path, "\\", sizeof(Path));
#	endif
				}
				else if (strcmp(argv[tmp],"-extract") == 0)
				{
					if (mode == 2)
					{
						printf("-extract and -new cannot be used together\n");
						exit (1);
					}
					mode = 1;
					argv[tmp][0] = 0;
				}
				else if (strcmp(argv[tmp],"-new") == 0)
				{
					if (mode == 1)
					{
						printf("-extract and -new cannot be used together\n");
						exit (1);
					}
					mode = 2;
					argv[tmp][0] = 0;
				}
			}
			else
			{
				strlcpy_ (File, argv[tmp], sizeof(File));
			}
		}
	}

	printf("VisPatch is in mode %i\n", mode);
	printf("Will use %s as the Vis-data source\n", VIS);
	if (mode == 1)
	{
		printf("Will extract Vis data to %s, auto-append.\n", VIS);
	}
	if (mode == 2)
	{
		testname = Sys_FindFirstFile(Path, "pak*.pak");
		tmp = 0;
		while (testname != NULL)
		{
			tmp++;
			testname = Sys_FindNextFile();
		}
		Sys_FindClose();
		snprintf(FoutPak, sizeof(FoutPak), "%spak%i.pak", Path, tmp);
		printf("The new pak file will be called %s.\n", FoutPak);
	}

	snprintf(TempFile, sizeof(TempFile), "%s%s", Path, TEMP_FILE_NAME);
//	printf("%s", TempFile);

	if (mode == 0 || mode == 2)	// we are patching
	{
		int chk = 0;

		fVIS = fopen(VIS, "rb");
		if (!fVIS)
		{
			printf("couldn't find the vis source file.\n");
			exit (2);
		}

		loadvis(fVIS);

		OutFile = fopen(TempFile, "w+b");
		if (!OutFile)
		{
			printf("couldn't create an output file\n");
			exit (2);
		}

		testname = Sys_FindFirstFile(Path, File);

		while (testname != NULL)
		{
			strlcpy_ (File, testname, sizeof(File));

			strlcpy_ (Path2, Path, sizeof(Path2));
			strlcat_ (Path2, File, sizeof(Path2));

			InFile = fopen(Path2, "rb");
			if (!InFile)
			{
				printf("couldn't find the level file.\n");
				exit (2);
			}

			if (mode == 0)
			{
			// we may be working with multiple pak files,
			// clean any garbage from the previous run.
				memset(NewPakEnt, 0, MAX_FILES_IN_PACK * sizeof(pakentry_t));
			}

			chk = ChooseLevel(File, 0, 100000);

			if (chk < 0)
				FWRITE_ERROR;

			if (mode == 0)		// patching with overwrite enabled
			{
				NPcnt = 0;
				fclose(OutFile);
				fclose(InFile);
				InFile = NULL;

				if (chk > 0)
				{
					remove(Path2);
					rename(TempFile, Path2);
				}

				OutFile = fopen(TempFile, "w+b");
				if (!OutFile)
				{
					printf("couldn't create an output file\n");
					exit (2);
				}
			}
			else if (usepak == 1)	// mode 2: writing into a new pakfile
			{
				fclose(InFile);
				InFile = NULL;
			}
			else if (chk > 0)	// mode 2: writing into new bsp files
			{
			//	printf("%i\n", chk);
				fclose(OutFile);
				fclose(InFile);
				InFile = NULL;
				strlcpy_ (Path2, Path, sizeof(Path2));
				strlcat_ (Path2, File, sizeof(Path2));
				strlcpy_ (File, Path2, sizeof(File));
				strrev_(File);
				File[0] = 'k';
				File[1] = 'a';
				File[2] = 'b';
				strrev_(File);
				remove(File);
				strlcpy_ (Path2, Path, sizeof(Path2));
				strlcat_ (Path2, CurName, sizeof(Path2));
				rename(Path2, File);
				rename(TempFile, Path2);
				OutFile = fopen(TempFile, "w+b");
				if (!OutFile)
				{
					printf("couldn't create an output file\n");
					exit (2);
				}
			}
			else	// mode 2: ???
			{
				fclose(OutFile);
				fclose(InFile);
				InFile = NULL;
				OutFile = fopen(TempFile, "w+b");
				if (!OutFile)
				{
					printf("couldn't create an output file\n");
					exit (2);
				}
			}

			testname = Sys_FindNextFile();
		}
		Sys_FindClose();
		CLOSE_ALL;
	//	printf("%s\n", FoutPak);
		if (mode == 2 && usepak == 1)
		{
			rename(TempFile, FoutPak);
		}
		freevis();

	}
	else if (mode == 1)	// we are extracting
	{
		if (Sys_filesize(VIS) == -1)
			fVIS = fopen(VIS, "wb");
		else
			fVIS = fopen(VIS, "r+b");

		if (!fVIS)
		{
			printf("couldn't open the vis data file.\n");
			exit (2);
		}

		testname = Sys_FindFirstFile(Path, File);

		while (testname != NULL)
		{
			strlcpy_ (File, testname, sizeof(File));
			strlcpy_ (Path2, Path, sizeof(Path2));
			strlcat_ (Path2, File, sizeof(Path2));

			InFile = fopen(Path2, "r+b");
			if (!InFile)
			{
				printf("couldn't find the level file.\n");
				exit (2);
			}

			ret = ChooseFile(File, 0, 0);

			if (ret < 0)
				FWRITE_ERROR;

			testname = Sys_FindNextFile();
		}
		Sys_FindClose();
	}

	remove (TempFile);

	return 0;
}


//============================================================================

// Functions used for patching process:
// ChooseLevel, PakFix, BspFix, OthrFix

int ChooseLevel (char *FileSpec, int Offset, int length)
{
	int tmp = 0;

//	printf("Looking at file %s %i %i.\n", FileSpec, length, mode);
	if ( strstr(strlwr_(FileSpec),".pak") )
	{
		printf("Looking at file %s.\n", FileSpec);
		usepak = 1;
		tmp = PakFix(Offset);
	}
	else if (length > 50000 && strstr(strlwr_(FileSpec),".bsp"))
	{
		printf("Looking at file %s.\n", FileSpec);
		strlcpy_ (CurName, FileSpec, sizeof(CurName));
		tmp = BSPFix(Offset);
	}
	else if (mode == 0 && Offset > 0)
	{
		tmp = OthrFix(Offset, length);
	}
	else if (mode == 2 && Offset > 0)
	{
		NPcnt--;
	}

//	if (tmp == 0)
//		printf("Did not process the file!\n");

	return tmp;
}

int PakFix (int Offset)
{
	pakheader_t	Pak;
	pakentry_t	*PakEnt;
	int		numentry;
	int		ugh, pakwalk;
	int		ret = 0;
	size_t		test;

	test = fwrite(&Pak, sizeof(pakheader_t), 1, OutFile);
	if (test == 0)
		return -1;

	fseek(InFile, Offset, SEEK_SET);
	fread(&Pak, sizeof(pakheader_t), 1, InFile);
	Pak.dirsize = LittleLong(Pak.dirsize);
	Pak.diroffset = LittleLong(Pak.diroffset);
	numentry = Pak.dirsize / sizeof(pakentry_t);
	PakEnt = (pakentry_t *)malloc(numentry*sizeof(pakentry_t));
	fseek(InFile, Offset+Pak.diroffset, SEEK_SET);
	fread(PakEnt, sizeof(pakentry_t), numentry, InFile);

	for (pakwalk = 0; pakwalk < numentry; pakwalk++)
	{
		PakEnt[pakwalk].size = LittleLong(PakEnt[pakwalk].size);
		PakEnt[pakwalk].offset = LittleLong(PakEnt[pakwalk].offset);
		strlcpy_ (NewPakEnt[NPcnt].filename, PakEnt[pakwalk].filename, sizeof(NewPakEnt[0].filename));
		strlcpy_ (CurName, PakEnt[pakwalk].filename, sizeof(CurName));

		NewPakEnt[NPcnt].size = LittleLong(PakEnt[pakwalk].size);
		ret = ChooseLevel(PakEnt[pakwalk].filename, Offset+PakEnt[pakwalk].offset, PakEnt[pakwalk].size);

		if (ret < 0)
		{
			free(PakEnt);
			return ret;
		}

		NPcnt++;
	}
	free(PakEnt);
//	fseek(OutFile,0,SEEK_END);
	fflush(OutFile);
	Pak.diroffset = ftell(OutFile);
//	printf("PAK diroffset = %i, entries = %i\n", Pak.diroffset, NPcnt);
	Pak.dirsize = NPcnt * sizeof(pakentry_t);
	ugh = ftell(OutFile);
// NewPakEnt is an array: pass either NewPakEnt itself or &NewPakEnt[0] to fwrite()
//	test = fwrite(&NewPakEnt, sizeof(pakentry_t), NPcnt, OutFile);
	test = fwrite(NewPakEnt, sizeof(pakentry_t), NPcnt, OutFile);

	if (test < NPcnt)
		return -1;

	fflush(OutFile);
//	chsize(fileno(OutFile), ftell(OutFile));
	fseek(OutFile, 0, SEEK_SET);
	Pak.dirsize = LittleLong(Pak.dirsize);
	Pak.diroffset = LittleLong(Pak.diroffset);
	test = fwrite(&Pak, sizeof(pakheader_t), 1, OutFile);

	if (test == 0)
		return -1;

	fseek(OutFile, ugh, SEEK_SET);

	return numentry;
}

int OthrFix (int Offset, int Length)
{
	int		test;
	void		*cpy;

	fseek(InFile, Offset, SEEK_SET);
	NewPakEnt[NPcnt].offset = LittleLong( ftell(OutFile) );
	NewPakEnt[NPcnt].size = LittleLong( Length );
	cpy = malloc(Length);
	fread(cpy, Length, 1, InFile);
	test = fwrite(cpy, Length, 1, OutFile);
	free(cpy);

	if (test == 0)
		return -1;

	return 1;
}

int BSPFix (int InitOFFS)
{
	int		tmp, good;
	size_t		test, count;
	int		here;
	dheader_t	bspheader;
	unsigned char	*cpy;
	char	VisName[VISPATCH_IDLEN+6];

	fflush(OutFile);
	NewPakEnt[NPcnt].offset = LittleLong( ftell(OutFile) );
	tmp = LittleLong(NewPakEnt[NPcnt].size);
	if (tmp == 0)
		NewPakEnt[NPcnt].size = LittleLong( Sys_filesize(File) );

//	printf("Start: %i\n", LittleLong(NewPakEnt[NPcnt].offset));

	fseek(InFile, InitOFFS, SEEK_SET);
	test = fread(&bspheader, sizeof(dheader_t), 1, InFile);
	if (test == 0)
		return 0;

	printf("Version of bsp file is: %d\n", LittleLong(bspheader.version));
	printf("Vis info is at %d and is %d long.\n", LittleLong(bspheader.visilist.offset), LittleLong(bspheader.visilist.size));
	test = fwrite(&bspheader, sizeof(dheader_t), 1, OutFile);
	if (test == 0)
		return -1;

// swap the header
	for (count = 0 ; count < sizeof(dheader_t)/4 ; count++)
		((int *)&bspheader)[count] = LittleLong ( ((int *)&bspheader)[count]);

	strlcpy_ (VisName, CurName, sizeof(VisName));
	strrev_ (VisName);
	strlcat_ (VisName, "/", sizeof(VisName));
	count = strcspn(VisName, "\\/");
	memset(VisName+count, 0, sizeof(VisName)-count);
	strrev_ (VisName);
	good = 0;
	here = ftell(OutFile);
	bspheader.visilist.offset = ftell(OutFile) - LittleLong(NewPakEnt[NPcnt].offset);
//	printf("%s %s %i\n", VisName, CurName, good);
	for (count = 0; count < numvis; count++)
	{
		////("%s  ",
		if (!strcasecmp_ (visdat[count].File, VisName))
		{
			good = 1;
			printf("Name: %s Size: %d %u\n", VisName, visdat[count].vislen, count);
			fseek(OutFile, here, SEEK_SET);
			bspheader.visilist.size = visdat[count].vislen;
			test = fwrite(visdat[count].visdata, bspheader.visilist.size, 1, OutFile);
			if (test == 0)
				return -1;

			fflush(OutFile);
			bspheader.leaves.size	= visdat[count].leaflen;
			bspheader.leaves.offset = ftell(OutFile)-LittleLong(NewPakEnt[NPcnt].offset);
			test = fwrite(visdat[count].leafdata, bspheader.leaves.size, 1, OutFile);
			if (test == 0)
				return -1;
		}
	}

	if (good == 0)
	{
		if (usepak == 1)
		{
			fseek(InFile, InitOFFS, SEEK_SET);
			fseek(OutFile, LittleLong(NewPakEnt[NPcnt].offset), SEEK_SET);
			if (mode == 0)
			{
				char *cc;
				tmp = LittleLong(NewPakEnt[NPcnt].size);
				if (tmp < 0)
				{
					printf ("%s: Error: negative size\n", __FUNCTION__);
					exit (3);
				}
				cc = (char *)malloc(tmp);
				fread(cc, tmp, 1, InFile);
				test = fwrite(cc, tmp, 1, OutFile);
				free(cc);
				if (test == 0)
					return -1;
				return 1;
			}
			else
			{
				NPcnt--;
				return 0;
			}
		}
		else
		{
			return 0;	//Individual file and it doesn't matter.
		}

//	printf("not good\n");
/*	cpy = (unsigned char *)malloc(bspheader.visilist.size);
	fseek(InFile, InitOFFS+bspheader.visilist.offset, SEEK_SET);
	fread(cpy, 1, bspheader.visilist.size, InFile);
	fwrite(cpy, bspheader.visilist.size, 1, OutFile);
	free(cpy);

	cpy = (unsigned char *)malloc(bspheader.leaves.size);
	fseek(InFile, InitOFFS+bspheader.leaves.offset, SEEK_SET);
	fread(cpy, 1, bspheader.leaves.size, InFile);
	bspheader.leaves.offset = ftell(OutFile)-LittleLong(NewPakEnt[NPcnt].offset);
	fwrite(cpy, bspheader.leaves.size, 1, OutFile);
	free(cpy);
*/
//	printf("K: %i\n", ftell(OutFile));

	}

	cpy = (unsigned char *)malloc(bspheader.entities.size);
	fseek(InFile, InitOFFS+bspheader.entities.offset, SEEK_SET);
	fread(cpy, 1, bspheader.entities.size, InFile);
	bspheader.entities.offset = ftell(OutFile)-LittleLong(NewPakEnt[NPcnt].offset);
	test = fwrite(cpy, bspheader.entities.size, 1, OutFile);
	free(cpy);
	if (test == 0)
		return -1;

//	printf("A: %i %i\n", bspheader.entities.offset, ftell(OutFile));

	cpy = (unsigned char *)malloc(bspheader.planes.size);
	fseek(InFile, InitOFFS+bspheader.planes.offset, SEEK_SET);
	fread(cpy, 1, bspheader.planes.size, InFile);
	bspheader.planes.offset = ftell(OutFile)-LittleLong(NewPakEnt[NPcnt].offset);
	test = fwrite(cpy, bspheader.planes.size, 1, OutFile);
	free(cpy);
	if (test == 0)
		return -1;

//	printf("B: %i\n", ftell(OutFile));

	cpy = (unsigned char *)malloc(bspheader.miptex.size);
	fseek(InFile, InitOFFS+bspheader.miptex.offset, SEEK_SET);
	fread(cpy, 1, bspheader.miptex.size, InFile);
	bspheader.miptex.offset = ftell(OutFile)-LittleLong(NewPakEnt[NPcnt].offset);
	test = fwrite(cpy, bspheader.miptex.size, 1, OutFile);
	free(cpy);
	if (test == 0)
		return -1;

//	printf("C: %i\n", ftell(OutFile));

	cpy = (unsigned char *)malloc(bspheader.vertices.size);
	fseek(InFile, InitOFFS+bspheader.vertices.offset, SEEK_SET);
	fread(cpy, 1, bspheader.vertices.size, InFile);
	bspheader.vertices.offset = ftell(OutFile)-LittleLong(NewPakEnt[NPcnt].offset);
	test = fwrite(cpy, bspheader.vertices.size, 1, OutFile);
	free(cpy);
	if (test == 0)
		return -1;

	cpy = (unsigned char *)malloc(bspheader.nodes.size);
	fseek(InFile, InitOFFS+bspheader.nodes.offset, SEEK_SET);
	fread(cpy, 1, bspheader.nodes.size, InFile);
	bspheader.nodes.offset = ftell(OutFile)-LittleLong(NewPakEnt[NPcnt].offset);
	test = fwrite(cpy, bspheader.nodes.size, 1, OutFile);
	free(cpy);
	if (test == 0)
		return -1;

	cpy = (unsigned char *)malloc(bspheader.texinfo.size);
	fseek(InFile, InitOFFS+bspheader.texinfo.offset, SEEK_SET);
	fread(cpy, 1, bspheader.texinfo.size, InFile);
	bspheader.texinfo.offset = ftell(OutFile)-LittleLong(NewPakEnt[NPcnt].offset);
	test = fwrite(cpy, bspheader.texinfo.size, 1, OutFile);
	free(cpy);
	if (test == 0)
		return -1;

//	printf("G: %i\n", ftell(OutFile));

	cpy = (unsigned char *)malloc(bspheader.faces.size);
	fseek(InFile, InitOFFS+bspheader.faces.offset, SEEK_SET);
	fread(cpy, 1, bspheader.faces.size, InFile);
	bspheader.faces.offset = ftell(OutFile)-LittleLong(NewPakEnt[NPcnt].offset);
	test = fwrite(cpy, bspheader.faces.size, 1, OutFile);
	free(cpy);
	if (test == 0)
		return -1;

//	printf("H: %i\n", ftell(OutFile));

	cpy = (unsigned char *)malloc(bspheader.lightmaps.size);
	fseek(InFile, InitOFFS+bspheader.lightmaps.offset, SEEK_SET);
	fread(cpy, 1, bspheader.lightmaps.size, InFile);
	bspheader.lightmaps.offset = ftell(OutFile)-LittleLong(NewPakEnt[NPcnt].offset);
	test = fwrite(cpy, bspheader.lightmaps.size, 1, OutFile);
	free(cpy);
	if (test == 0)
		return -1;

//	printf("I: %i\n", ftell(OutFile));

	cpy = (unsigned char *)malloc(bspheader.clipnodes.size);
	fseek(InFile, InitOFFS+bspheader.clipnodes.offset, SEEK_SET);
	fread(cpy, 1, bspheader.clipnodes.size, InFile);
	bspheader.clipnodes.offset = ftell(OutFile)-LittleLong(NewPakEnt[NPcnt].offset);
	test = fwrite(cpy, bspheader.clipnodes.size, 1, OutFile);
	free(cpy);
	if (test == 0)
		return -1;
//	printf("J: %i\n", ftell(OutFile));


	cpy = (unsigned char *)malloc(bspheader.lface.size);
	fseek(InFile, InitOFFS+bspheader.lface.offset, SEEK_SET);
	fread(cpy, 1, bspheader.lface.size, InFile);
	bspheader.lface.offset = ftell(OutFile)-LittleLong(NewPakEnt[NPcnt].offset);
	test = fwrite(cpy, bspheader.lface.size, 1, OutFile);
	free(cpy);
	if (test == 0)
		return -1;

//	printf("L: %i\n", ftell(OutFile));

	cpy = (unsigned char *)malloc(bspheader.edges.size);
	fseek(InFile, InitOFFS+bspheader.edges.offset, SEEK_SET);
	fread(cpy, 1, bspheader.edges.size, InFile);
	bspheader.edges.offset = ftell(OutFile)-LittleLong(NewPakEnt[NPcnt].offset);
	test = fwrite(cpy, bspheader.edges.size, 1, OutFile);
	free(cpy);
	if (test == 0)
		return -1;

//	printf("M: %i\n", ftell(OutFile));

	cpy = (unsigned char *)malloc(bspheader.ledges.size);
	fseek(InFile, InitOFFS+bspheader.ledges.offset, SEEK_SET);
	fread(cpy, 1, bspheader.ledges.size, InFile);
	bspheader.ledges.offset = ftell(OutFile)-LittleLong(NewPakEnt[NPcnt].offset);
	test = fwrite(cpy, bspheader.ledges.size, 1, OutFile);
	free(cpy);
	if (test == 0)
		return -1;

//	printf("N: %i\n", ftell(OutFile));

	cpy = (unsigned char *)malloc(bspheader.models.size);
	fseek(InFile, InitOFFS+bspheader.models.offset, SEEK_SET);
	fread(cpy, 1, bspheader.models.size, InFile);
	bspheader.models.offset = ftell(OutFile)-LittleLong(NewPakEnt[NPcnt].offset);
	test = fwrite(cpy, bspheader.models.size, 1, OutFile);
	free(cpy);
	if (test == 0)
		return -1;

	here = ftell(OutFile);
//	printf("O: %i\n", here);
	fflush(OutFile);

// swap the header
	for (count = 0 ; count < sizeof(dheader_t)/4 ; count++)
		((int *)&bspheader)[count] = LittleLong ( ((int *)&bspheader)[count]);

	fseek(OutFile, LittleLong(NewPakEnt[NPcnt].offset), SEEK_SET);
	test = fwrite(&bspheader, sizeof(dheader_t), 1, OutFile);
	if (test == 0)
		return -1;

	fseek(OutFile, here, SEEK_SET);
	NewPakEnt[NPcnt].size = LittleLong( ftell(OutFile) - LittleLong(NewPakEnt[NPcnt].offset) );

//	printf("End: %i\n", ftell(OutFile));

	return 1;
}


//============================================================================

// Functions used for extraction process:
// ChooseFile, PakNew, BspNew

int ChooseFile (char *FileSpec, int Offset, int length)
{
	int tmp = 0;

	if (length == 0 && strstr(strlwr_(FileSpec),".pak"))
	{
		printf("Looking at file %s.\n", FileSpec);
		tmp = PakNew(Offset);
	}

	if (strstr(strlwr_(FileSpec),".bsp"))
	{
		printf("Looking at file %s.\n", FileSpec);
		strlcpy_ (CurName, FileSpec, sizeof(CurName));
		tmp = BSPNew(Offset);
	}

	return tmp;
}

int PakNew (int Offset)
{
	pakheader_t	Pak;
	pakentry_t	*PakEnt;
	int		ret = 0;
	int		pakwalk;
	int		numentry;

	fseek(InFile, Offset, SEEK_SET);
	fread(&Pak, sizeof(pakheader_t), 1, InFile);
	Pak.dirsize = LittleLong(Pak.dirsize);
	Pak.diroffset = LittleLong(Pak.diroffset);
	numentry = Pak.dirsize / sizeof(pakentry_t);
	PakEnt = (pakentry_t *)malloc(numentry*sizeof(pakentry_t));
	fseek(InFile, Offset+Pak.diroffset, SEEK_SET);
	fread(PakEnt, sizeof(pakentry_t), numentry, InFile);

	for (pakwalk = 0; pakwalk < numentry; pakwalk++)
	{
		PakEnt[pakwalk].size = LittleLong(PakEnt[pakwalk].size);
		PakEnt[pakwalk].offset = LittleLong(PakEnt[pakwalk].offset);

		strlcpy_ (NewPakEnt[NPcnt].filename, PakEnt[pakwalk].filename, sizeof(NewPakEnt[0].filename));
		strlcpy_ (CurName, PakEnt[pakwalk].filename, sizeof(CurName));

		NewPakEnt[NPcnt].size = LittleLong(PakEnt[pakwalk].size);
		ret = ChooseFile(PakEnt[pakwalk].filename, Offset+PakEnt[pakwalk].offset, PakEnt[pakwalk].size);
		if (ret < 0)
		{
			free(PakEnt);
			FWRITE_ERROR;
		}
		NPcnt++;
	}
	free(PakEnt);

	return numentry;
}

int BSPNew (int InitOFFS)
{
	size_t		count, test;
	int	tes, len;
	dheader_t	bspheader;
	unsigned char	*cpy;
	char	VisName[VISPATCH_IDLEN+6];

	fseek(InFile, InitOFFS, SEEK_SET);
	test = fread(&bspheader, sizeof(dheader_t), 1, InFile);
	if (test == 0)
		return 0;

// swap the header
	for (count = 0 ; count < sizeof(dheader_t)/4 ; count++)
		((int *)&bspheader)[count] = LittleLong ( ((int *)&bspheader)[count]);

	printf("Version of bsp file is:  %d\n", bspheader.version);
	printf("Vis info is at %d and is %d long\n", bspheader.visilist.offset, bspheader.visilist.size);
	printf("Leaf info is at %d and is %d long\n", bspheader.leaves.offset, bspheader.leaves.size);

/*	If we don't perform the following check,
	we shall fail at the fwrite test below.	*/
	if (bspheader.visilist.size == 0)
	{
		printf("Vis info size = 0.  Skipping...\n");
		return 1;
	}

	strlcpy_ (VisName, CurName, sizeof(VisName));
	strrev_ (VisName);
	strlcat_ (VisName, "/", sizeof(VisName));
	count = strcspn(VisName, "/\\");
	memset(VisName+count, 0, sizeof(VisName)-count);
	strrev_ (VisName);

	cpy = (unsigned char *)malloc(bspheader.visilist.size);
	fseek(InFile, InitOFFS+bspheader.visilist.offset, SEEK_SET);
	fread(cpy, 1, bspheader.visilist.size, InFile);
	len = Sys_filesize(VIS);
//	printf("%i\n", len);
	if (len > -1)
		fseek(fVIS, 0, SEEK_END);

	test = fwrite(&VisName, 1, VISPATCH_IDLEN, fVIS);
	if (test == 0)
		return -1;

	tes = bspheader.visilist.size + bspheader.leaves.size + 8;
	tes = LittleLong(tes);
	test = fwrite(&tes, sizeof(int), 1, fVIS);
	if (test == 0)
		return -1;

	tes = LittleLong(bspheader.visilist.size);
	test = fwrite(&tes, sizeof(int), 1, fVIS);
	if (test == 0)
		return -1;

	test = fwrite(cpy, bspheader.visilist.size, 1, fVIS);
	free(cpy);
	if (test == 0)
		return -1;

	cpy = (unsigned char *)malloc(bspheader.leaves.size);
	fseek(InFile, InitOFFS+bspheader.leaves.offset, SEEK_SET);
	fread(cpy, 1, bspheader.leaves.size, InFile);
	tes = LittleLong(bspheader.leaves.size);
	test = fwrite(&tes, sizeof(int), 1, fVIS);
	if (test == 0)
	{
		free(cpy);
		return -1;
	}

	test = fwrite(cpy, bspheader.leaves.size, 1, fVIS);
	free(cpy);
	if (test == 0)
		return -1;

	return 1;
}


//============================================================================

// Functions used for vis data loading:

void loadvis(FILE *fp)
{
	unsigned int	count = 0, tmp;
	char		Name[VISPATCH_IDLEN];
	int		go, len;

	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	while (ftell(fp) < len)
	{
		count++;
		fread(Name, 1, VISPATCH_IDLEN, fp);
		fread(&go, 1, sizeof(int), fp);
		go = LittleLong(go);
		fseek(fp, go, SEEK_CUR);
	}

	visdat = (visdat_t *)malloc(sizeof(visdat_t)*count);
	if (visdat == 0)
	{
		printf("Ack, not enough memory!\n");
		exit (2);
	}

	fseek(fp, 0, SEEK_SET);

	for (tmp = 0; tmp < count; tmp++)
	{
		fread(visdat[tmp].File, 1, VISPATCH_IDLEN, fp);
		fread(&visdat[tmp].len, 1, sizeof(int), fp);
		fread(&visdat[tmp].vislen, 1, sizeof(int), fp);
		visdat[tmp].len = LittleLong(visdat[tmp].len);
		visdat[tmp].vislen = LittleLong(visdat[tmp].vislen);
	//	printf("%i\n", visdat[tmp].vislen);
		visdat[tmp].visdata = (unsigned char *)malloc(visdat[tmp].vislen);
		if (visdat[tmp].visdata == 0)
		{
			printf("Ack, not enough memory!\n");
			free(visdat);
			exit (2);
		}
		fread(visdat[tmp].visdata, 1, visdat[tmp].vislen, fp);

		fread(&visdat[tmp].leaflen, 1, sizeof(int), fp);
		visdat[tmp].leaflen = LittleLong(visdat[tmp].leaflen);
		visdat[tmp].leafdata = (unsigned char *)malloc(visdat[tmp].leaflen);
		if (visdat[tmp].leafdata == 0)
		{
			printf("Ack, not enough memory!\n");
			free(visdat);
			exit (2);
		}
		fread(visdat[tmp].leafdata, 1, visdat[tmp].leaflen, fp);
	}

	numvis = count;
}

void freevis (void)
{
	unsigned int		tmp;

	for (tmp = 0; tmp < numvis; tmp++)
	{
		free(visdat[tmp].visdata);
		free(visdat[tmp].leafdata);
	}
	free(visdat);
}

