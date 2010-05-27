MAINTAINERCLEANFILES = Makefile.in
INCLUDES = -I../include -I../../include -I../../ -I../../common @LIBRAW1394_PACKAGE_CFLAGS@

if ENABLE_STATIC_CPI
noinst_LTLIBRARIES = libdcam.la
libdcam_la_LIBADD = @LIBRAW1394_PACKAGE_LIBS@ @PTHREAD_LIBS@ -L../../common -lucutils
else
libcpi_LTLIBRARIES = libdcam.la
libdcam_la_LIBADD = @LIBRAW1394_PACKAGE_LIBS@ @PTHREAD_LIBS@ -L../../src/.libs/ -lunicap -L../../common -lucutils
endif

libcpidir = $(libdir)/unicap$(pkg_version)/cpi

libdcam_la_CPPFLAGS = -D@LIBRAW1394_VERSION@
libdcam_la_CFLAGS = -fno-strict-aliasing -Wl,-z,defs

libdcam_la_LDFLAGS = -module -avoid-version

libdcam_la_SOURCES =  \
	1394util.c \
	dcam.c dcam.h \
	dcam_busreset.c dcam_busreset.h \
	dcam_capture.c dcam_capture.h \
	dcam_defs.h \
	dcam_format_table.h \
	dcam_functions.c dcam_functions.h \
	dcam_isoch_table.h \
	dcam_offsets.h \
	dcam_property.c dcam_property.h dcam_property_table.h \
	dcam_v_modes.c dcam_v_modes.h 


