MAINTAINERCLEANFILES = Makefile.in
INCLUDES = -I../include -I../../include @LIBRAW1394_PACKAGE_CFLAGS@ -I. -I../../common

if ENABLE_STATIC_CPI
noinst_LTLIBRARIES = libvid21394.la
libvid21394_la_LIBADD = @LIBRAW1394_PACKAGE_LIBS@ @PTHREAD_LIBS@ -L../../common -lucutils
else
libcpi_LTLIBRARIES = libvid21394.la
libvid21394_la_LIBADD = @LIBRAW1394_PACKAGE_LIBS@ @PTHREAD_LIBS@  -L../../src/.libs/ -lunicap -L../../common -lucutils
endif

libcpidir = $(libdir)/unicap$(pkg_version)/cpi

libvid21394_la_LDFLAGS = -module -avoid-version

libvid21394_la_SOURCES = 1394util.c \
			 Fcp.h \
			 vid21394.h \
			 vid21394_base.c \
			 vid21394_base.h \
			 vid21394_cpi.c \
			 vid21394_cpi.h \
			 visca.c visca.h visca_private.h\
			 visca_property_table.h



libvid21394_la_CFLAGS = -fno-strict-aliasing