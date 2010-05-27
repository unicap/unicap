MAINTAINERCLEANFILES = Makefile.in
INCLUDES = -I../include -I../../include -I../../common

if ENABLE_STATIC_CPI
noinst_LTLIBRARIES = libv4l.la
libv4l_la_LIBADD = @PTHREAD_LIBS@ -L../../common -lucutils
else
libcpi_LTLIBRARIES = libv4l.la
libv4l_la_LIBADD = @PTHREAD_LIBS@ -L../../src/.libs/ -lunicap -L../../common -lucutils
endif

libcpidir = $(libdir)/unicap$(pkg_version)/cpi


libv4l_la_LDFLAGS = -module -avoid-version

libv4l_la_SOURCES =  \
	v4l.c v4l.h 
