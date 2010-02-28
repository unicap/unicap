VERSION = 0.2
CODEGEN ?= pygobject-codegen-2.0
PYTHON ?= python
PREFIX ?= /usr



SUBDIRS=src examples
DISTFILES=Makefile README ChangeLog
REQMODULES=pygtk-2.0 libunicap libunicapgtk
REQBINARIES=$(CODEGEN) $(PYTHON)

export VERSION
export CODEGEN
export PYTHON
export DESTDIR
export PREFIX
export SETUPOPTS

all: .external-deps
	@list='$(SUBDIRS)'; for d in $$list; do        \
		make  -C $$d || exit 1;   \
	done

.PHONY: install clean TAGS dist

TAGS:
	@list='$(SUBDIRS)'; for d in $$list; do        \
		make -C $$d TAGS \
	done

.external-deps:
	@list='$(REQMODULES)'; for mod in $$list; do \
		if ! pkg-config --exists $$mod; then \
			echo "Required module '$$mod' not found!"; \
			echo "See the README file for more information"; \
			exit 1; \
		fi; \
	done

	@list='$(REQBINARIES)'; for bin in $$list; do \
		if [ -z `which $$bin` ]; then \
			echo "'$$bin' is required to build this package"; \
			echo "See the README file for more information"; \
			exit 1; \
		fi; \
	done

install:
	@list='$(SUBDIRS)'; for d in $$list; do        \
		make -C $$d install || exit 1; \
	done

clean:
	@list='$(SUBDIRS)'; for d in $$list; do        \
		make -C $$d clean ; \
	done

distclean:
	clean

dist:
	rm -rf /tmp/pyunicap-$(VERSION)
	mkdir /tmp/pyunicap-$(VERSION)
	list='$(SUBDIRS)'; for d in $$list; do        \
		make -C $$d _dist; \
	done 
	cp $(DISTFILES) /tmp/pyunicap-$(VERSION)/
	tar -C /tmp -c -z -f pyunicap-$(VERSION).tar.gz pyunicap-$(VERSION)
