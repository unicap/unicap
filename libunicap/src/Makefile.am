MAINTAINERCLEANFILES = Makefile.in
INCLUDES = -I../include -I../cpi/include
AM_CPPFLAGS=-DINSTALL_PREFIX="\"$(prefix)\"" -DPKGLIBDIR="\"$(libdir)/unicap$(pkg_version)\"" -DPKG_VERSION="\"@pkg_version@\""
lib_LTLIBRARIES = libunicap.la

if ENABLE_STATIC_CPI
libunicap_la_LDFLAGS = -version-info @lt_major@:@lt_revision@:@lt_age@ @RT_LIBS@ -static
else
libunicap_la_LDFLAGS = -version-info @lt_major@:@lt_revision@:@lt_age@ @RT_LIBS@
endif

libunicap_la_SOURCES =   \
	unicap.c 	 \
	../include/unicap_private.h \
	unicap_cache.c   \
        check_match.c	 \
	../include/check_match.h    \
	unicap_helpers.c \
	../include/unicap_helpers.h

if BUILD_V4L2
LIBV4L2=cpi/v4l2cpi/libv4l2cpi.la
endif
if BUILD_V4L
LIBV4L=cpi/v4l/libv4l.la
endif
if BUILD_DCAM
LIBDCAM=cpi/dcam/libdcam.la
endif
if BUILD_VID21394
LIBVID21394=cpi/vid21394/libvid21394.la
endif

if ENABLE_STATIC_CPI
libunicap_la_LIBADD = \
	@DL_LIBS@ \
	@M_LIBS@ \
	../$(LIBV4L2) ../$(LIBV4L) ../$(LIBDCAM) ../$(LIBVID21394)
else
libunicap_la_LIBADD = \
	@DL_LIBS@ \
	@M_LIBS@
endif


