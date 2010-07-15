MAINTAINERCLEANFILES = Makefile.in
INCLUDES = -I../include -I../../include -I../../common

if ENABLE_STATIC_CPI
noinst_LTLIBRARIES = libv4l2cpi.la
libv4l2cpi_la_LIBADD = @PTHREAD_LIBS@  @LIBV4L_LIBS@ -L../../common -lucutils
else
libcpi_LTLIBRARIES = libv4l2cpi.la
libv4l2cpi_la_LIBADD = @PTHREAD_LIBS@  @LIBV4L_LIBS@ -L../../src/.libs/ -lunicap -L../../common -lucutils
endif

libcpidir = $(libdir)/unicap$(pkg_version)/cpi


libv4l2cpi_la_LDFLAGS = -module -avoid-version

libv4l2cpi_la_CFLAGS = @LIBV4L_CFLAGS@

libv4l2cpi_la_SOURCES =  \
	v4l2.c v4l2.h \
	buffermanager.c buffermanager.h \
	tisuvccam.c tisuvccam.h uvcvideo.h uvc_compat.h \
	tiseuvccam.c tiseuvccam.h 

EXTRA_DIST = v4l2_i18n_strings.h