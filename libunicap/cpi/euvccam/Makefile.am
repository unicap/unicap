INCLUDES = -I../include -I../../include -I../../ -I../../common

if ENABLE_STATIC_CPI
noinst_LTLIBRARIES=libeuvccam_cpi.la
libeuvccam_cpi_la_LIBADD=-lpthread 
else
libcpi_LTLIBRARIES=libeuvccam_cpi.la
libeuvccam_cpi_la_LIBADD=-lpthread -L../../src/.libs/ -lunicap
endif
libcpidir = $(libdir)/unicap$(pkg_version)/cpi

#libeuvccam_cpi_la_CFLAGS=-msse2 -O2 -g
libeuvccam_cpi_la_CFLAGS=-O2 -g
libeuvccam_cpi_la_LDFLAGS=-module -avoid-version

libeuvccam_cpi_la_SOURCES= \
	euvccam_cpi.c euvccam_cpi.h \
	euvccam_device.c euvccam_device.h \
	euvccam_capture.c euvccam_capture.h \
	euvccam_devspec.c euvccam_devspec.h \
	euvccam_colorproc.c euvccam_colorproc.h \
	euvccam_usb.c euvccam_usb.h \
	debayer.c debayer.h \
	queue.c queue.h \
	logging.c logging.h

