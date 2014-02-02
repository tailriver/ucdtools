bin_PROGRAMS = ucdconv
lib_LIBRARIES = libucd.a
include_HEADERS = ucd.h

AM_CFLAGS = -Wall -ansi -pedantic

libucd_a_SOURCES = ucd.c ucd_reader.c ucd_writer.c
noinst_HEADERS = ucd_private.h

ucdconv_SOURCES = ucdconv.c
ucdconv_LDADD = libucd.a
