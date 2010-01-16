MAINTAINERCLEANFILES = Makefile.in
INCLUDES =  -I../include @GTK_PACKAGE_CFLAGS@ @UNICAP_PACKAGE_CFLAGS@
AM_CPPFLAGS = $(GTK_CPPFLAGS) -DSYSCONFDIR="\"$(sysconfdir)\"" -DBACKENDDIR="\"$(libdir)/unicap$(pkg_version)/backends\"" -DPKG_VERSION="\"@lt_major@\""

lib_LTLIBRARIES = libunicapgtk.la


libunicapgtk_la_LDFLAGS = -version-info @lt_major@:@lt_revision@:@lt_age@

libunicapgtk_la_SOURCES =  \
			  unicapgtk.c ../include/unicapgtk.h \
			  unicapgtk_video_display.c ../include/unicapgtk_video_display.h \
			  unicapgtk_device_property.c ../include/unicapgtk_device_property.h \
			  unicapgtk_video_format_selection.c ../include/unicapgtk_video_format_selection.h \
			  unicapgtk_device_selection.c ../include/unicapgtk_device_selection.h \
			  unicapgtk_property_dialog.c ../include/unicapgtk_property_dialog.h \
			  unicapgtk_priv.h \
			  backend_xv.c backend_xv.h \
			  backend_gtk.c backend_gtk.h backend.h 

libunicapgtk_la_LIBADD = \
			  @GTK_PACKAGE_LIBS@ \
			  @X_LIBS@ @X_XV_LIBS@ \
			  @UNICAP_PACKAGE_LIBS@
			  -lm

libunicapgtk_la_CFLAGS = \
			 -DUNICAP_LOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\"
