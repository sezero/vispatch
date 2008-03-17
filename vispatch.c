/*
 * VisPatch :  Quake level patcher for water visibility.
 *
 * Copyright (C) 1997-2006  Andy Bay <IMarvinTPA@bigfoot.com>
 * Copyright (C) 2006-2007  O. Sezer <sezero@users.sourceforge.net>
 *
 * $Id: vispatch.c,v 1.5 2008-03-17 21:33:28 sezero Exp $
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
#include "utilslib.h"
#include "vispatch.h"

static visdat_t		*visdat;
static pakentry_t	NewPakEnt[MAX_FILES_IN_PACK];
static unsigned int	NPcnt, numvis;
static int		usepak = 0;
static int		mode = 0;	/* 0: patch, 2: patch without overwriting, 1: extract	*/

static FILE	*InFile, *OutFile, *fVIS;

#define		TEMP_FILE_NAME		"~vistmp.tmp"
static char	Path[256],	/* where we shall work: getcwd(), changed by -dir	*/
		Path2[256],	/* temporary filename buffer				*/
		TempFile[256],	/* name of our temporary file on disk			*/
		VIS[256] = "vispatch.dat",	/* vis data filename. changed by -data	*/
		FoutPak[256] = "pak*.pak",	/* name of the output pak file		*/
		File[256] = "pak*.pak",		/* filename pattern we'll be globbing	*/
		CurName[VISPATCH_IDLEN+6];	/* name of the currently processed file	*/


/*============================================================================*/

/* Functions used for vis data loading: */
static void loadvis (FILE *fp);
static void freevis (void);

/* Functions used for patching process: */
static int BSPFix (int InitOFFS);
static int PakFix (int Offset);
static int OthrFix(int Offset, int Length);
static int ChooseLevel(char *FileSpec, int Offset, int length);

/* Functions used for extraction process: */
static int PakNew (int Offset);
static int BSPNew (int Offset);
static int ChooseFile (char *FileSpec, int Offset, int length);

/*============================================================================*/

#define CLOSE_ALL {			\
	if (InFile)			\
		fclose(InFile);		\
	if (OutFile)			\
		fclose(OutFile);	\
	if (fVIS)			\
		fclose(fVIS);		\
}


#define FWRITE_ERROR {							\
	CLOSE_ALL;							\
	remove(TempFile);						\
	freevis();							\
	Error("Errors during fwrite: Not enough disk space?");		\
}


/*============================================================================*/

static void usage (void)
{
	printf("Usage: vispatch <file> [arguments]\n\n");
/*
	printf(" -h, -help  or  --help : Prints this message.\n\n");
*/
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


/*============================================================================*/

#if defined(PLATFORM_WINDOWS)

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

#endif	/* PLATFORM_WINDOWS */


int main (int argc, char **argv)
{
	int	ret = 0, tmp;
	char	*testname;

	printf("VisPatch %d.%d.%d by Andy Bay\n", VP_VER_MAJ, VP_VER_MID, VP_VER_MIN);
	printf("Revised and fixed by O.Sezer\n");

	ValidateByteorder ();

	if (argc > 1)
	{
		for (tmp = 1; tmp < argc; tmp++)
		{
			q_strlwr(argv[tmp]);
			if (strcmp(argv[tmp], "-?"    ) == 0 ||
			    strcmp(argv[tmp], "-h"    ) == 0 ||
			    strcmp(argv[tmp], "-help" ) == 0 ||
			    strcmp(argv[tmp], "--help") == 0 )
				usage();
		}
	}

	if ( Sys_getcwd(Path,sizeof(Path)) != 0)
		Error ("Unable to determine current working directory");
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
						Error ("You must specify the filename of visdata after -data");
					q_strlcpy (VIS, argv[tmp], sizeof(VIS));
					argv[tmp][0] = 0;
				}
				else if (strcmp(argv[tmp],"-dir") == 0)
				{
					argv[tmp][0] = 0;
					if (++tmp == argc)
						Error ("You must specify a directory name after -dir");
					q_strlcpy (Path, argv[tmp], sizeof(Path) - 1);
					Path[sizeof(Path) - 1] = 0;
					argv[tmp][0] = 0;
					printf("Will look into %s as the pak/bsp directory..\n", Path);
				}
				else if (strcmp(argv[tmp],"-extract") == 0)
				{
					if (mode == 2)
						Error ("-extract and -new cannot be used together");
					mode = 1;
					argv[tmp][0] = 0;
				}
				else if (strcmp(argv[tmp],"-new") == 0)
				{
					if (mode == 1)
						Error ("-extract and -new cannot be used together");
					mode = 2;
					argv[tmp][0] = 0;
				}
			}
			else
			{
				q_strlcpy (File, argv[tmp], sizeof(File));
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
		q_snprintf(FoutPak, sizeof(FoutPak), "%s/pak%i.pak", Path, tmp);
		printf("The new pak file will be called %s.\n", FoutPak);
	}

	q_snprintf(TempFile, sizeof(TempFile), "%s/%s", Path, TEMP_FILE_NAME);
//	printf("%s", TempFile);

	if (mode == 0 || mode == 2)	/* we are patching */
	{
		int chk = 0;

		fVIS = fopen(VIS, "rb");
		if (!fVIS)
			Error ("couldn't find the vis source file.");

		loadvis(fVIS);

		OutFile = fopen(TempFile, "w+b");
		if (!OutFile)
			Error ("couldn't create an output file");

		testname = Sys_FindFirstFile(Path, File);

		while (testname != NULL)
		{
			q_strlcpy (File, testname, sizeof(File));
			q_snprintf(Path2, sizeof(Path2), "%s/%s", Path, File);

			InFile = fopen(Path2, "rb");
			if (!InFile)
				Error ("couldn't find the level file.");

			if (mode == 0)
			{
			/* we may be working with multiple pak files,
			 * clean any garbage from the previous runs.
			 */
				memset(NewPakEnt, 0, MAX_FILES_IN_PACK * sizeof(pakentry_t));
			}

			chk = ChooseLevel(File, 0, 100000);

			if (chk < 0)
			{
				FWRITE_ERROR;
			}

			if (mode == 0)		/* patching with overwrite enabled */
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
					Error ("couldn't create an output file");
			}
			else if (usepak == 1)	/* mode 2: writing into a new pakfile */
			{
				fclose(InFile);
				InFile = NULL;
			}
			else if (chk > 0)	/* mode 2: writing into new bsp files */
			{
			//	printf("%i\n", chk);
				fclose(OutFile);
				fclose(InFile);
				InFile = NULL;
				q_snprintf(Path2, sizeof(Path2), "%s/%s", Path, File);
				q_strlcpy (File, Path2, sizeof(File));
				q_strrev(File);
				File[0] = 'k';
				File[1] = 'a';
				File[2] = 'b';
				q_strrev(File);
				remove(File);
				q_snprintf(Path2, sizeof(Path2), "%s/%s", Path, CurName);
				rename(Path2, File);
				rename(TempFile, Path2);
				OutFile = fopen(TempFile, "w+b");
				if (!OutFile)
					Error ("couldn't create an output file");
			}
			else	/* mode 2: ??? */
			{
				fclose(OutFile);
				fclose(InFile);
				InFile = NULL;
				OutFile = fopen(TempFile, "w+b");
				if (!OutFile)
					Error ("couldn't create an output file");
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
	else if (mode == 1)	/* we are extracting */
	{
		if (Sys_filesize(VIS) == -1)
			fVIS = fopen(VIS, "wb");
		else
			fVIS = fopen(VIS, "r+b");

		if (!fVIS)
			Error ("couldn't open the vis data file.");

		testname = Sys_FindFirstFile(Path, File);

		while (testname != NULL)
		{
			q_strlcpy (File, testname, sizeof(File));
			q_snprintf(Path2, sizeof(Path2), "%s/%s", Path, File);

			InFile = fopen(Path2, "r+b");
			if (!InFile)
				Error ("couldn't find the level file.");

			ret = ChooseFile(File, 0, 0);

			if (ret < 0)
			{
				FWRITE_ERROR;
			}

			testname = Sys_FindNextFile();
		}
		Sys_FindClose();
	}

	remove (TempFile);

	return 0;
}


/*============================================================================*/

/* Functions used for patching process:
 * ChooseLevel, PakFix, BspFix, OthrFix
 */

static int ChooseLevel (char *FileSpec, int Offset, int length)
{
	int tmp = 0;

//	printf("Looking at file %s %i %i.\n", FileSpec, length, mode);
	if ( strstr(q_strlwr(FileSpec),".pak") )
	{
		printf("Looking at file %s.\n", FileSpec);
		usepak = 1;
		tmp = PakFix(Offset);
	}
	else if (length > 50000 && strstr(q_strlwr(FileSpec),".bsp"))
	{
		printf("Looking at file %s.\n", FileSpec);
		q_strlcpy (CurName, FileSpec, sizeof(CurName));
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

static int PakFix (int Offset)
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
		q_strlcpy (NewPakEnt[NPcnt].filename, PakEnt[pakwalk].filename, sizeof(NewPakEnt[0].filename));
		q_strlcpy (CurName, PakEnt[pakwalk].filename, sizeof(CurName));

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
//	test = fwrite(&NewPakEnt[0], sizeof(pakentry_t), NPcnt, OutFile);
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

static int OthrFix (int Offset, int Length)
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

static int BSPFix (int InitOFFS)
{
	int		tmp, good;
	size_t		test, count;
	int		here;
	dheader_t	bspheader;
	unsigned char	*cpy;
	char	VisName[VISPATCH_IDLEN+6];
	char	pad[4] = { 0, 0, 0, 0 };

	fflush(OutFile);
	NewPakEnt[NPcnt].offset = LittleLong( ftell(OutFile) );
	tmp = LittleLong(NewPakEnt[NPcnt].size);
	if (tmp == 0)
		NewPakEnt[NPcnt].size = LittleLong( Sys_filesize(File) );

	fseek(InFile, InitOFFS, SEEK_SET);
	test = fread(&bspheader, sizeof(dheader_t), 1, InFile);
	if (test == 0)
		return 0;

	printf("Version of bsp file is: %d\n", LittleLong(bspheader.version));
	printf("Vis info is at %d and is %d long.\n", LittleLong(bspheader.visilist.offset), LittleLong(bspheader.visilist.size));
	test = fwrite(&bspheader, sizeof(dheader_t), 1, OutFile);
	if (test == 0)
		return -1;

/* swap the header */
	for (count = 0 ; count < sizeof(dheader_t)/4 ; count++)
		((int *)&bspheader)[count] = LittleLong ( ((int *)&bspheader)[count]);

	q_strlcpy (VisName, CurName, sizeof(VisName));
	q_strrev (VisName);
	q_strlcat (VisName, "/", sizeof(VisName));
	count = strcspn(VisName, "\\/");
	memset(VisName+count, 0, sizeof(VisName)-count);
	q_strrev (VisName);
	good = 0;
	for (count = 0; count < numvis; count++)
	{
		if (!q_strcasecmp(visdat[count].File, VisName))
		{
			good = 1;
			printf("Name: %s Size: %d %lu\n", VisName, visdat[count].vislen, (unsigned long)count);
			bspheader.visilist.size = visdat[count].vislen;
			bspheader.leaves.size	= visdat[count].leaflen;
			break;
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
					Error ("%s: Error: negative size!", __thisfunc__);
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
		/* else: Individual file and it doesn't matter. */
		return 0;
	}

/*	write the lumps not in the structrue order, but using the
	bspfile.c::WriteBSPFile() order of iD Software. align the
	offset at 4 bytes as in bspfile.c::AddLump()		*/

	cpy = (unsigned char *)malloc(bspheader.planes.size);
	fseek(InFile, InitOFFS+bspheader.planes.offset, SEEK_SET);
	fread(cpy, 1, bspheader.planes.size, InFile);
	bspheader.planes.offset = ftell(OutFile)-LittleLong(NewPakEnt[NPcnt].offset);
	test = fwrite(cpy, bspheader.planes.size, 1, OutFile);
	free(cpy);
	if (test == 0)
		return -1;
	tmp = (bspheader.planes.size + 3) & ~3;
	if (tmp > bspheader.planes.size)
	{
		tmp -= bspheader.planes.size;
		fwrite(pad, tmp, 1, OutFile);
	}

	bspheader.leaves.offset = ftell(OutFile)-LittleLong(NewPakEnt[NPcnt].offset);
	test = fwrite(visdat[count].leafdata, bspheader.leaves.size, 1, OutFile);
	if (test == 0)
		return -1;
	tmp = (bspheader.leaves.size + 3) & ~3;
	if (tmp > bspheader.leaves.size)
	{
		tmp -= bspheader.leaves.size;
		fwrite(pad, tmp, 1, OutFile);
	}

	cpy = (unsigned char *)malloc(bspheader.vertices.size);
	fseek(InFile, InitOFFS+bspheader.vertices.offset, SEEK_SET);
	fread(cpy, 1, bspheader.vertices.size, InFile);
	bspheader.vertices.offset = ftell(OutFile)-LittleLong(NewPakEnt[NPcnt].offset);
	test = fwrite(cpy, bspheader.vertices.size, 1, OutFile);
	free(cpy);
	if (test == 0)
		return -1;
	tmp = (bspheader.vertices.size + 3) & ~3;
	if (tmp > bspheader.vertices.size)
	{
		tmp -= bspheader.vertices.size;
		fwrite(pad, tmp, 1, OutFile);
	}

	cpy = (unsigned char *)malloc(bspheader.nodes.size);
	fseek(InFile, InitOFFS+bspheader.nodes.offset, SEEK_SET);
	fread(cpy, 1, bspheader.nodes.size, InFile);
	bspheader.nodes.offset = ftell(OutFile)-LittleLong(NewPakEnt[NPcnt].offset);
	test = fwrite(cpy, bspheader.nodes.size, 1, OutFile);
	free(cpy);
	if (test == 0)
		return -1;
	tmp = (bspheader.nodes.size + 3) & ~3;
	if (tmp > bspheader.nodes.size)
	{
		tmp -= bspheader.nodes.size;
		fwrite(pad, tmp, 1, OutFile);
	}

	cpy = (unsigned char *)malloc(bspheader.texinfo.size);
	fseek(InFile, InitOFFS+bspheader.texinfo.offset, SEEK_SET);
	fread(cpy, 1, bspheader.texinfo.size, InFile);
	bspheader.texinfo.offset = ftell(OutFile)-LittleLong(NewPakEnt[NPcnt].offset);
	test = fwrite(cpy, bspheader.texinfo.size, 1, OutFile);
	free(cpy);
	if (test == 0)
		return -1;
	tmp = (bspheader.texinfo.size + 3) & ~3;
	if (tmp > bspheader.texinfo.size)
	{
		tmp -= bspheader.texinfo.size;
		fwrite(pad, tmp, 1, OutFile);
	}

	cpy = (unsigned char *)malloc(bspheader.faces.size);
	fseek(InFile, InitOFFS+bspheader.faces.offset, SEEK_SET);
	fread(cpy, 1, bspheader.faces.size, InFile);
	bspheader.faces.offset = ftell(OutFile)-LittleLong(NewPakEnt[NPcnt].offset);
	test = fwrite(cpy, bspheader.faces.size, 1, OutFile);
	free(cpy);
	if (test == 0)
		return -1;
	tmp = (bspheader.faces.size + 3) & ~3;
	if (tmp > bspheader.faces.size)
	{
		tmp -= bspheader.faces.size;
		fwrite(pad, tmp, 1, OutFile);
	}

	cpy = (unsigned char *)malloc(bspheader.clipnodes.size);
	fseek(InFile, InitOFFS+bspheader.clipnodes.offset, SEEK_SET);
	fread(cpy, 1, bspheader.clipnodes.size, InFile);
	bspheader.clipnodes.offset = ftell(OutFile)-LittleLong(NewPakEnt[NPcnt].offset);
	test = fwrite(cpy, bspheader.clipnodes.size, 1, OutFile);
	free(cpy);
	if (test == 0)
		return -1;
	tmp = (bspheader.clipnodes.size + 3) & ~3;
	if (tmp > bspheader.clipnodes.size)
	{
		tmp -= bspheader.clipnodes.size;
		fwrite(pad, tmp, 1, OutFile);
	}

	cpy = (unsigned char *)malloc(bspheader.lface.size);
	fseek(InFile, InitOFFS+bspheader.lface.offset, SEEK_SET);
	fread(cpy, 1, bspheader.lface.size, InFile);
	bspheader.lface.offset = ftell(OutFile)-LittleLong(NewPakEnt[NPcnt].offset);
	test = fwrite(cpy, bspheader.lface.size, 1, OutFile);
	free(cpy);
	if (test == 0)
		return -1;
	tmp = (bspheader.lface.size + 3) & ~3;
	if (tmp > bspheader.lface.size)
	{
		tmp -= bspheader.lface.size;
		fwrite(pad, tmp, 1, OutFile);
	}

	cpy = (unsigned char *)malloc(bspheader.ledges.size);
	fseek(InFile, InitOFFS+bspheader.ledges.offset, SEEK_SET);
	fread(cpy, 1, bspheader.ledges.size, InFile);
	bspheader.ledges.offset = ftell(OutFile)-LittleLong(NewPakEnt[NPcnt].offset);
	test = fwrite(cpy, bspheader.ledges.size, 1, OutFile);
	free(cpy);
	if (test == 0)
		return -1;
	tmp = (bspheader.ledges.size + 3) & ~3;
	if (tmp > bspheader.ledges.size)
	{
		tmp -= bspheader.ledges.size;
		fwrite(pad, tmp, 1, OutFile);
	}

	cpy = (unsigned char *)malloc(bspheader.edges.size);
	fseek(InFile, InitOFFS+bspheader.edges.offset, SEEK_SET);
	fread(cpy, 1, bspheader.edges.size, InFile);
	bspheader.edges.offset = ftell(OutFile)-LittleLong(NewPakEnt[NPcnt].offset);
	test = fwrite(cpy, bspheader.edges.size, 1, OutFile);
	free(cpy);
	if (test == 0)
		return -1;
	tmp = (bspheader.edges.size + 3) & ~3;
	if (tmp > bspheader.edges.size)
	{
		tmp -= bspheader.edges.size;
		fwrite(pad, tmp, 1, OutFile);
	}

	cpy = (unsigned char *)malloc(bspheader.models.size);
	fseek(InFile, InitOFFS+bspheader.models.offset, SEEK_SET);
	fread(cpy, 1, bspheader.models.size, InFile);
	bspheader.models.offset = ftell(OutFile)-LittleLong(NewPakEnt[NPcnt].offset);
	test = fwrite(cpy, bspheader.models.size, 1, OutFile);
	free(cpy);
	if (test == 0)
		return -1;
	tmp = (bspheader.models.size + 3) & ~3;
	if (tmp > bspheader.models.size)
	{
		tmp -= bspheader.models.size;
		fwrite(pad, tmp, 1, OutFile);
	}

	cpy = (unsigned char *)malloc(bspheader.lightmaps.size);
	fseek(InFile, InitOFFS+bspheader.lightmaps.offset, SEEK_SET);
	fread(cpy, 1, bspheader.lightmaps.size, InFile);
	bspheader.lightmaps.offset = ftell(OutFile)-LittleLong(NewPakEnt[NPcnt].offset);
	test = fwrite(cpy, bspheader.lightmaps.size, 1, OutFile);
	free(cpy);
	if (test == 0)
		return -1;
	tmp = (bspheader.lightmaps.size + 3) & ~3;
	if (tmp > bspheader.lightmaps.size)
	{
		tmp -= bspheader.lightmaps.size;
		fwrite(pad, tmp, 1, OutFile);
	}

	bspheader.visilist.offset = ftell(OutFile) - LittleLong(NewPakEnt[NPcnt].offset);
	test = fwrite(visdat[count].visdata, bspheader.visilist.size, 1, OutFile);
	if (test == 0)
		return -1;
	tmp = (bspheader.visilist.size + 3) & ~3;
	if (tmp > bspheader.visilist.size)
	{
		tmp -= bspheader.visilist.size;
		fwrite(pad, tmp, 1, OutFile);
	}

	cpy = (unsigned char *)malloc(bspheader.entities.size);
	fseek(InFile, InitOFFS+bspheader.entities.offset, SEEK_SET);
	fread(cpy, 1, bspheader.entities.size, InFile);
	bspheader.entities.offset = ftell(OutFile)-LittleLong(NewPakEnt[NPcnt].offset);
	test = fwrite(cpy, bspheader.entities.size, 1, OutFile);
	free(cpy);
	if (test == 0)
		return -1;
	tmp = (bspheader.entities.size + 3) & ~3;
	if (tmp > bspheader.entities.size)
	{
		tmp -= bspheader.entities.size;
		fwrite(pad, tmp, 1, OutFile);
	}

	cpy = (unsigned char *)malloc(bspheader.miptex.size);
	fseek(InFile, InitOFFS+bspheader.miptex.offset, SEEK_SET);
	fread(cpy, 1, bspheader.miptex.size, InFile);
	bspheader.miptex.offset = ftell(OutFile)-LittleLong(NewPakEnt[NPcnt].offset);
	test = fwrite(cpy, bspheader.miptex.size, 1, OutFile);
	free(cpy);
	if (test == 0)
		return -1;
	tmp = (bspheader.miptex.size + 3) & ~3;
	if (tmp > bspheader.miptex.size)
	{
		tmp -= bspheader.miptex.size;
		fwrite(pad, tmp, 1, OutFile);
	}

	here = ftell(OutFile);
	fflush(OutFile);

/* swap the header */
	for (count = 0 ; count < sizeof(dheader_t)/4 ; count++)
		((int *)&bspheader)[count] = LittleLong ( ((int *)&bspheader)[count]);

	fseek(OutFile, LittleLong(NewPakEnt[NPcnt].offset), SEEK_SET);
	test = fwrite(&bspheader, sizeof(dheader_t), 1, OutFile);
	if (test == 0)
		return -1;

	fseek(OutFile, here, SEEK_SET);
	NewPakEnt[NPcnt].size = LittleLong( ftell(OutFile) - LittleLong(NewPakEnt[NPcnt].offset) );

	return 1;
}


/*============================================================================*/

/* Functions used for extraction process:
 * ChooseFile, PakNew, BspNew
 */

static int ChooseFile (char *FileSpec, int Offset, int length)
{
	int tmp = 0;

	if (length == 0 && strstr(q_strlwr(FileSpec),".pak"))
	{
		printf("Looking at file %s.\n", FileSpec);
		tmp = PakNew(Offset);
	}

	if (strstr(q_strlwr(FileSpec),".bsp"))
	{
		printf("Looking at file %s.\n", FileSpec);
		q_strlcpy (CurName, FileSpec, sizeof(CurName));
		tmp = BSPNew(Offset);
	}

	return tmp;
}

static int PakNew (int Offset)
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

		q_strlcpy (NewPakEnt[NPcnt].filename, PakEnt[pakwalk].filename, sizeof(NewPakEnt[0].filename));
		q_strlcpy (CurName, PakEnt[pakwalk].filename, sizeof(CurName));

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

static int BSPNew (int InitOFFS)
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

/* swap the header */
	for (count = 0 ; count < sizeof(dheader_t)/4 ; count++)
		((int *)&bspheader)[count] = LittleLong ( ((int *)&bspheader)[count]);

	printf("Version of bsp file is:  %d\n", bspheader.version);
	printf("Vis info is at %d and is %d long\n", bspheader.visilist.offset, bspheader.visilist.size);
	printf("Leaf info is at %d and is %d long\n", bspheader.leaves.offset, bspheader.leaves.size);

/*	Map with no vis data: no need to do this. (And if
	we do, we shall fail at the fwrite test below.)	*/
	if (bspheader.visilist.size == 0)
	{
		printf("Vis info size = 0.  Skipping...\n");
		return 1;
	}

	q_strlcpy (VisName, CurName, sizeof(VisName));
	q_strrev (VisName);
	q_strlcat (VisName, "/", sizeof(VisName));
	count = strcspn(VisName, "/\\");
	memset(VisName+count, 0, sizeof(VisName)-count);
	q_strrev (VisName);

	cpy = (unsigned char *)malloc(bspheader.visilist.size);
	fseek(InFile, InitOFFS+bspheader.visilist.offset, SEEK_SET);
	fread(cpy, 1, bspheader.visilist.size, InFile);
	len = Sys_filesize(VIS);
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


/*============================================================================*/

/* Functions used for vis data loading: */

static void loadvis(FILE *fp)
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
		Error ("Not enough memory!");

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
			free(visdat);
			Error ("Not enough memory!");
		}
		fread(visdat[tmp].visdata, 1, visdat[tmp].vislen, fp);

		fread(&visdat[tmp].leaflen, 1, sizeof(int), fp);
		visdat[tmp].leaflen = LittleLong(visdat[tmp].leaflen);
		visdat[tmp].leafdata = (unsigned char *)malloc(visdat[tmp].leaflen);
		if (visdat[tmp].leafdata == 0)
		{
			free(visdat);
			Error ("Not enough memory!");
		}
		fread(visdat[tmp].leafdata, 1, visdat[tmp].leaflen, fp);
	}

	numvis = count;
}

static void freevis (void)
{
	unsigned int		tmp;

	for (tmp = 0; tmp < numvis; tmp++)
	{
		free(visdat[tmp].visdata);
		free(visdat[tmp].leafdata);
	}
	free(visdat);
}

