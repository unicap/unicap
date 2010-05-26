MAINTAINERCLEANFILES = Makefile.in
INCLUDES = -I../include 
AM_CPPFLAGS=-DINSTALL_PREFIX="\"$(prefix)\"" -DPKGLIBDIR="\"$(pkglibdir)\"" -DPKGVERSION="\"@pkg_version@\"" $(GLIB_CPPFLAGS) -Wno-multichar
AM_CFLAGS = @UNICAP_CFLAGS@ @GLIB_PACKAGE_CFLAGS@ @PANGO_PACKAGE_CFLAGS@ \
            @AVCODEC_PACKAGE_CFLAGS@ @THEORA_PACKAGE_CFLAGS@ @ALSA_PACKAGE_CFLAGS@ \
	    @GSTREAMER_PACKAGE_CFLAGS@ \
            -fno-strict-aliasing 

lib_LTLIBRARIES = libucil.la

libucil_la_LDFLAGS = -version-info @lt_major@:@lt_revision@:@lt_age@
libucil_la_LIBADD =  @UNICAP_LIBS@ @GLIB_PACKAGE_LIBS@ @PANGO_PACKAGE_LIBS@ \
		     @AVCODEC_PACKAGE_LIBS@ @THEORA_PACKAGE_LIBS@ @ALSA_PACKAGE_LIBS@ @PNG_PACKAGE_LIBS@ \
		     @GSTREAMER_PACKAGE_LIBS@ @GSTAPP_LIBS@ \
		     -lm -lgthread-2.0

if HAVE_AVCODEC
libucil_avcavi_sources=ucil_avc_avi.c ../include/ucil_avc_avi.h
else
libucil_avcavi_sources=
endif

if HAVE_THEORA
libucil_theora_sources=ucil_theora.c ../include/ucil_theora.h
else
libucil_theora_sources=
endif

if HAVE_ALSA
libucil_alsa_sources=ucil_alsa.c ../include/ucil_alsa.h
else
libucil_alsa_sources=
endif

if HAVE_PNG
libucil_png_sources=ucil_png.c ../include/ucil_png.h
else
libucil_png_sources=
endif

if HAVE_GSTREAMER
libucil_gstreamer_sources=ucil_gstreamer.c ../include/ucil_gstreamer.h
else
libucil_gstreamer_sources=
endif


libucil_la_SOURCES =   \
		ucil.c ../include/ucil.h \
		draw.c ../include/draw.h \
		colorspace.c ../include/colorspace.h \
		yuvops.c ../include/yuvops.h \
		rgbops.c ../include/rgbops.h \
		font.c  \
		video_file.c ../include/video_file.h \
		ucil_rawavi.c ../include/ucil_rawavi.h ../include/avi.h\
		ucil_audio.c ../include/ucil_audio.h \
		ucil_ppm.c ../include/ucil_ppm.h \
		$(libucil_avcodec_sources) \
		$(libucil_avcavi_sources) \
		$(libucil_theora_sources) \
		$(libucil_alsa_sources) \
		$(libucil_png_sources) \
		$(libucil_gstreamer_sources)



