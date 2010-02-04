MAINTAINERCLEANFILES = Makefile.in
INCLUDES = @UNICAP_PACKAGE_CFLAGS@
bin_PROGRAMS = xv_display	

xv_display_SOURCES = xv_display.c xv.c xv.h
xv_display_LDADD =   $(UNICAP_PACKAGE_LIBS) \
		     $(X_XV_LIBS) \
		     $(X_LIBS) 

EXTRA_DIST = install-sh config.sub config.guess
# headers to be installed
# pkginclude_HEADERS =

