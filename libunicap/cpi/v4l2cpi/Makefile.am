MAINTAINERCLEANFILES = Makefile.in
INCLUDES = -I../include -I../../include -I../../common

if ENABLE_STATIC_CPI
noinst_LTLIBRARIES = libv4l2cpi.la
else
libcpi_LTLIBRARIES = libv4l2cpi.la
endif

libcpidir = $(libdir)/unicap$(pkg_version)/cpi


# need to rename v4l2 plugin to avoid name clash with libv4l2
libv4l2cpi_la_LDFLAGS = 

libv4l2cpi_la_LIBADD = @PTHREAD_LIBS@  @LIBV4L_LIBS@ -L../../src/.libs/ -lunicap -L../../common -lucutils

libv4l2cpi_la_CFLAGS = @LIBV4L_CFLAGS@

libv4l2cpi_la_SOURCES =  \
	v4l2.c v4l2.h \
	tisuvccam.c tisuvccam.h uvcvideo.h uvc_compat.h \
	tiseuvccam.c tiseuvccam.h 

EXTRA_DIST = v4l2_i18n_strings.h