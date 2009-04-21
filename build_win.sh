#!/bin/sh
# This script is written with the cross-tools and instructions found at
# http://www.libsdl.org/extras/win32/cross/ in mind.  Change it to meet
# your needs and environment.

#TARGET=i686-pc-mingw32
TARGET=i386-mingw32msvc
PREFIX=/usr/local/cross-tools

PATH="$PREFIX/bin:$PATH"
export PATH

W32BUILD=1
export W32BUILD

CC="$TARGET-gcc"
AS="$TARGET-as"
AR="$TARGET-ar"
WINDRES="$TARGET-windres"
export CC WINDRES AS AR

STRIPPER="$TARGET-strip"

if [ "$1" = "strip" ]
then
$STRIPPER vispatch.exe
exit 0
fi

HOST_OS=`uname|sed -e s/_.*//|tr '[:upper:]' '[:lower:]'`

case "$HOST_OS" in
freebsd|openbsd|netbsd)
	MAKE_CMD=gmake
	;;
linux)
	MAKE_CMD=make
	;;
*)
	MAKE_CMD=make
	;;
esac

exec $MAKE_CMD -f makefile.mingw $*

