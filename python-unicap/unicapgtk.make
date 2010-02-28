PYGTK_DEFSDIR=/usr/share/pygtk/2.0/defs

pyunicapgtk_video_display.c: pyunicapgtk_video_display.defs pyunicapgtk_video_display.override
	pygobject-codegen-2.0 --register $(PYGTK_DEFSDIR)/gtk-types.defs --register $(PYGTK_DEFSDIR)/gdk-base-types.defs \
	--register unicap.defs \
	--override $*.override --prefix $* $*.defs > gen-$*.c \
	&& cp gen-$*.c $*.c \
	&& rm -f gen-$*.c
