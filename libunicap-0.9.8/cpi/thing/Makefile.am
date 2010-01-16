MAINTAINERCLEANFILES = Makefile.in
INCLUDES = -I../include -I../../include

libcpi_LTLIBRARIES = libthing.la

libcpidir = $(libdir)/unicap/cpi


libthing_la_LDFLAGS = -version-info @lt_major@:@lt_revision@:@lt_age@

libthing_la_LIBADD = @PTHREAD_LIBS@

libthing_la_SOURCES =  \
	queue.c \
	thing.c thing.h

