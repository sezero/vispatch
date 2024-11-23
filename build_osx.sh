#!/bin/sh

OLDPATH=$PATH
MAKE_CMD=make

rm -f vispatch.ppc vispatch.x86 vispatch.x64
$MAKE_CMD -f Makefile.darwin distclean

# ppc
TARGET=powerpc-apple-darwin9
PATH=/opt/cross_osx-ppc/bin:$OLDPATH
CC=$TARGET-gcc
export PATH CC
$MAKE_CMD -f Makefile.darwin CROSS=$TARGET MACH_TYPE=ppc $* || exit 1
$TARGET-strip -S vispatch || exit 1
mv vispatch vispatch.ppc || exit 1
$MAKE_CMD -f Makefile.darwin distclean

# x86
TARGET=i686-apple-darwin9
PATH=/opt/cross_osx-x86/bin:$OLDPATH
CC=$TARGET-gcc
export PATH CC
$MAKE_CMD -f Makefile.darwin CROSS=$TARGET MACH_TYPE=x86 $* || exit 1
$TARGET-strip -S vispatch || exit 1
mv vispatch vispatch.x86 || exit 1
$MAKE_CMD -f Makefile.darwin distclean

# x86_64
TARGET=x86_64-apple-darwin9
PATH=/opt/cross_osx-x86_64/usr/bin:$OLDPATH
CC=$TARGET-gcc
export PATH CC
$MAKE_CMD -f Makefile.darwin CROSS=$TARGET MACH_TYPE=x86_64 $* || exit 1
$TARGET-strip -S vispatch || exit 1
mv vispatch vispatch.x64 || exit 1
$MAKE_CMD -f Makefile.darwin distclean

echo "$TARGET-lipo -create -o vispatch vispatch.ppc vispatch.x86 vispatch.x64"
$TARGET-lipo -create -o vispatch vispatch.ppc vispatch.x86 vispatch.x64 || exit 1
