EXAMPLEDIRS=GettingStarted SnapImage
DISTFILES=Makefile README

all:

TAGS:

install:

clean:

_dist:
	mkdir -p /tmp/python-unicap-$(VERSION)/examples
	list='$(EXAMPLEDIRS)'; for d in $$list; do \
		mkdir /tmp/python-unicap-$(VERSION)/examples/$$d ; \
		install -D $$d/*.py /tmp/python-unicap-$(VERSION)/examples/$$d/; \
	done
	list='$(DISTFILES)'; for f in $$list; do \
		install -D $$f /tmp/python-unicap-$(VERSION)/examples/; \
	done
