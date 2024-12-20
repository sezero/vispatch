VisPatch, version 1.4.7

-----------------------------------------------------------------

A WaterVIS utility for glquake

http://vispatch.sourceforge.net/
http://sourceforge.net/projects/vispatch/

-----------------------------------------------------------------

VisPatch is a tool for patching quake maps for transparent water
in glquake. Original Quake didn't have their maps water-vis'ed,
so people did that by themselves and prepared patch data files.
This tool is used for preparing and applying those patch files.

At the time this tool was written, re-vis'ing maps took a lot of
time, but applying a vispatch took less than minutes, so this was
a necessity.  Even today, if people don't want going into a
tiresome job of vising, this tool comes as a great convenience
because there are a lot of vispatch data files around.

This is a revised version of Andy Bay's 1.2a source code for unix
(linux, freebsd, ...), as well as windows.  It fixes a number of
compilation issues, crashes and some other bugs, and adds support
for big endian systems.  The source code is licensed under GPLv2,
and is maintained here with portability in mind.


-----------------------------------------------------------------

Links :

Homepage of Andy Bay, the original author:
  http://www.imarvintpa.com/

The (latest) website for WaterVIS:
  https://www.quake-info-pool.net/vispatch/


-----------------------------------------------------------------

Usage: vispatch <file> [arguments]

<file> : Level filename pattern, bsp or pak type, wildcards
	 are allowed, relative paths not allowed. See the
	 examples.

 -dir  : Specify the directory that the level files are in.

 -data : Specify the vis data file. Wildcards are not allowed.

 -new  : Tells the program not to overwrite the original level
	 files.  In case of pak files, the patched the levels
	 will be put into a new pak file. In case of standalone
	 maps files, the original maps are renamed to name.bak,
	 and the patched ones are saved as name.bsp .
	 Can not be used in combination with -extract.

 -extract: Retrieve all the vis data from the given file.
	 Can not be used in combination with -new.

IMPORTANT: All files must be of lowercase names for vispatch to
	 find and work with them.

Examples:

vispatch map25.bsp
  Patch and overwrite map25.bsp using vis data from the default
  data file vispatch.dat

vispatch map25.bsp -data map25.vis
  Use map25.vis as the data file instead of vispatch.dat

vispatch -dir hipnotic -data hipnotic.vis
  Patch levels in pak files under the directory named hipnotic,
  using  hipnotic.vis  as the data.

vispatch "*.bsp" -dir hipnotic -data hipnotic.vis
  Patch bsp levels NOT residing in pak files under the directory
  named hipnotic, using  hipnotic.vis  as the data.

vispatch somefile.bsp -extract
  Retrieve all vis data from somefile.bsp and put it into a file
  named vispatch.dat  (If you don't backup any previous one, it
  will append the data into it!)

vispatch somefile.bsp -data some.dat -extract
  Retrieve all vis data from the somefile.bsp and dump it into
  a file named some.dat instead of vispatch.dat

vispatch "*.bsp" -data some.dat -extract
  Retrieve all vis data from all bsp level files and put them
  into the file some.dat

vispatch -dir hipnotic -extract
  Retrieve all vis data from the levels residing in pak files in
  the directory hipnotic, and put them into vispatch.dat


-----------------------------------------------------------------

COPYRIGHT

  Copyright (C) 1997-2006  Andy Bay <IMarvinTPA@bigfoot.com>
  Copyright (C) 2006-2024  O. Sezer <sezero@users.sourceforge.net>

  VisPatch is free software;  you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published
  by the Free Software Foundation;  either version 2, or (at your
  option) any later version.  See the accompanying COPYING file for
  details.
