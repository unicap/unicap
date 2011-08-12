INCLUDES = -I../include -I../../include -I../../ -I../../common

if ENABLE_STATIC_CPI
noinst_LTLIBRARIES=libaravis_cpi.la
libaravis_cpi_la_LIBADD=-lpthread @ARAVIS_PACKAGE_LIBS@
else
libcpi_LTLIBRARIES=libaravis_cpi.la
libaravis_cpi_la_LIBADD=-lpthread @ARAVIS_PACKAGE_LIBS@ -L../../src/.libs/ -lunicap
endif
libcpidir = $(libdir)/unicap$(pkg_version)/cpi

libaravis_cpi_la_CFLAGS=-O2 -g @ARAVIS_PACKAGE_CFLAGS@
libaravis_cpi_la_LDFLAGS=-module -avoid-version

libaravis_cpi_la_SOURCES= \
	aravis_cpi.c aravis_cpi.h \
	aravis_tools.c aravis_tools.h \
	aravis_properties.c aravis_properties.h \
	logging.c logging.h

