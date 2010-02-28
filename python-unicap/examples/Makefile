EXAMPLEDIRS=GettingStarted SnapImage
DISTFILES=Makefile README

all:

TAGS:

install:

clean:

_dist:
	mkdir -p /tmp/pyunicap-$(VERSION)/examples
	list='$(EXAMPLEDIRS)'; for d in $$list; do \
		mkdir /tmp/pyunicap-$(VERSION)/examples/$$d ; \
		install -D $$d/*.py /tmp/pyunicap-$(VERSION)/examples/$$d/; \
	done
	list='$(DISTFILES)'; for f in $$list; do \
		install -D $$f /tmp/pyunicap-$(VERSION)/examples/; \
	done
