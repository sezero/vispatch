#!/bin/sh

# This script is written with the cross-tools and instructions found at
# http://www.libsdl.org/extras/win32/cross/ in mind.  Change it to meet
# your needs and environment.

PREFIX=/usr/local/cross-tools
TARGET=i386-mingw32msvc
PATH="$PREFIX/bin:$PREFIX/$TARGET/bin:$PATH"
export PATH
SENDARGS="WINBUILD=1 CC=$TARGET-gcc NASM=nasm WINDRES=$TARGET-windres MINGWDIR=$PREFIX/$TARGET"
STRIPPER=$TARGET-strip

$TARGET-gcc -march=i386 -Wall -Wshadow -mconsole -o vispatch.exe vispatch.c

$TARGET-strip vispatch.exe

