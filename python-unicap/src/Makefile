CODEGEN ?= pygobject-codegen-2.0
PYGTK_DEFSDIR=`pkg-config --variable=defsdir pygtk-2.0`

OVERRIDEFILES=	pyunicapgtk_video_display.override pyunicapgtk_property_dialog.override \
	      	pyunicapgtk_device_property.override pyunicapgtk_device_selection.override \
		pyunicapgtk_video_format_selection.override 

DEFSFILES=	unicap.defs\
		pyunicapgtk_video_display.defs \
		pyunicapgtk_property_dialog.defs \
		pyunicapgtk_device_property.defs \
		pyunicapgtk_device_selection.defs \
		pyunicapgtk_video_format_selection.defs 

GENCFILES=	pyunicapgtk_video_display.c \
		pyunicapgtk_property_dialog.c \
		pyunicapgtk_device_property.c \
		pyunicapgtk_device_selection.c \
		pyunicapgtk_video_format_selection.c 

DISTCFILES=	\
		unicapmodule.c \
		unicapmodule.h \
		unicapdevice.c \
		unicapdevice.h \
		unicapimagebuffer.c \
		unicapimagebuffer.h \
		unicapgtkmodule.c \
		unicapgtkmodule.h \
		utils.c \
		utils.h \
		pyunicapgtk_device_property.h \
		pyunicapgtk_device_selection.h \
		pyunicapgtk_property_dialog.h \
		pyunicapgtk_video_display.h \
		pyunicapgtk_video_format_selection.h

DISTFILES= 	\
		Makefile\
		setup.py

all: $(OVERRIDE) $(DEFSFILES) $(GENCFILES) build

.PHONY: build install clean TAGS dist

%.c: %.defs %.override
	$(CODEGEN) --register $(PYGTK_DEFSDIR)/gtk-types.defs --register $(PYGTK_DEFSDIR)/gdk-base-types.defs \
	--register unicap.defs \
	--override $*.override --prefix $* $*.defs > gen-$*.c \
	&& cp gen-$*.c $*.c \
	&& rm -f gen-$*.c

TAGS:
	etags $(DISTCFILES)

build:
	python setup.py build

install:
	if [ -n "$(DESTDIR)" ]; then \
		python setup.py install	--prefix=$(DESTDIR)/$(PREFIX) $(SETUPOPTS); \
	else \
		python setup.py install --prefix=$(PREFIX) $(SETUPOPTS); \
	fi

clean:
	rm -f $(GENCFILES)
	python setup.py clean

_dist:
	mkdir -p /tmp/python-unicap-$(VERSION)/src
	cp $(DISTCFILES) /tmp/python-unicap-$(VERSION)/src
	cp $(OVERRIDEFILES) /tmp/python-unicap-$(VERSION)/src
	cp $(DEFSFILES) /tmp/python-unicap-$(VERSION)/src
	cp $(DISTFILES) /tmp/python-unicap-$(VERSION)/src
