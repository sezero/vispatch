#!/bin/sh

# used for building with djgpp cross toolchain

PREFIX=/usr/local/cross-djgpp
TARGET=i586-pc-msdosdjgpp

PATH="$PREFIX/bin:$PREFIX/$TARGET/bin:$PATH"
export PATH

DOSBUILD=1
export DOSBUILD

CC="$TARGET-gcc"
AS="$TARGET-as"
AR="$TARGET-ar"
export CC AS AR

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

$MAKE_CMD -f makefile.djgpp $*

