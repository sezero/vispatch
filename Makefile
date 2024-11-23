# GNU Makefile for vispatch binaries using GCC.

CC ?= gcc

OBJECTS:= utilslib.o \
	strlcat.o \
	strlcpy.o \
	vispatch.o

OPTIMIZATIONS= -O2

CFLAGS = -g -Wall -W -Wshadow $(OPTIMIZATIONS)
LDFLAGS=

all: vispatch

.c.o:
	$(CC) $(CFLAGS) -c $*.c

vispatch: $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(LDFLAGS)

clean:
	rm -f *.o *.res *.d

