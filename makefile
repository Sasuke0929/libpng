# makefile for libpng
# Copyright (C) 1995 Guy Eric Schalnat, Group 42, Inc.
# For conditions of distribution and use, see copyright notice in png.h

CC=gcc
CFLAGS=-I../zlib -O3
LDFLAGS=-L. -L../zlib/ -lpng -lgz -lm

RANLIB=ranlib
#RANLIB=echo

# where make install puts libpng.a and png.h
prefix=/usr/local

OBJS = png.o pngrcb.o pngrutil.o pngtrans.o pngwutil.o \
	pngread.o pngstub.o pngwrite.o pngrtran.o pngwtran.o

all: libpng.a pngtest

libpng.a: $(OBJS)
	ar rc $@  $(OBJS)
	$(RANLIB) $@

pngtest: pngtest.o libpng.a
	cc -o pngtest $(CCFLAGS) pngtest.o $(LDFLAGS)

install: libpng.a
	-@mkdir $(prefix)/include
	-@mkdir $(prefix)/lib
	cp png.h $(prefix)/include
	chmod 644 $(prefix)/include/png.h
	cp libpng.a $(prefix)/lib
	chmod 644 $(prefix)/lib/libpng.a

clean:
	rm -f *.o libpng.a pngtest pngout.png

# DO NOT DELETE THIS LINE -- make depend depends on it.

pngrcb.o: png.h
pngread.o: png.h
pngrtran.o: png.h
pngrutil.o: png.h
pngstub.o: png.h
pngtest.o: png.h
pngtrans.o: png.h
pngwrite.o: png.h
pngwtran.o: png.h
pngwutil.o: png.h
