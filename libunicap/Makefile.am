SUBDIRS=include common src doc po cpi 

MAINTAINERCLEANFILES = Makefile.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libunicap.pc


EXTRA_DIST = \
intltool-extract.in \
intltool-merge.in 	\
intltool-update.in \
install-sh \
libunicap.pc.in 


unversioneddistdir: distdir
	mv $(PACKAGE)-$(VERSION) unicap-dist



