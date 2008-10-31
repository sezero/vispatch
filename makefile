# GNU Makefile for vispatch binaries using GCC.
# $Id: makefile,v 1.2 2008-10-31 16:40:52 sezero Exp $

CC ?= gcc

OBJECTS:= utilslib.o \
	strlcat.o \
	strlcpy.o \
	vispatch.o

OPTIMIZATIONS:= -O2
#OPTIMIZATIONS:= -O -g

CFLAGS := -Wall -W -Wshadow $(OPTIMIZATIONS)
LDFLAGS:=

all: vispatch

.c.o:
	$(CC) $(CFLAGS) -c $*.c

vispatch: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS)

clean:
	rm -f *.o *.d

