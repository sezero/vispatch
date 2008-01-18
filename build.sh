#!/bin/sh

#gcc -march=i386 -DPLATFORM_UNIX -Wall -Wshadow -o vispatch vispatch.c
gcc -DPLATFORM_UNIX -Wall -Wshadow -o vispatch vispatch.c

strip vispatch
#strip -R .comment vispatch

