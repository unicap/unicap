SUBDIRS = sdl_display

all: subdirs

subdirs: $(patsubst %, _dir_%, $(SUBDIRS))

$(patsubst %, _dir_%, $(SUBDIRS)) : 
	$(MAKE) CFLAGS="$(CFLAGS)" MAYOR_VERSION="$(MAYOR_VERSION)" -C $(patsubst _dir_%, %, $@)

install: $(patsubst %, _install_%, $(SUBDIRS))

$(patsubst %, _install_%, $(SUBDIRS)) :
	$(MAKE) CFLAGS="$(CFLAGS)" MAYOR_VERSION="$(MAYOR_VERSION)" -C $(patsubst _install_%, %, $@) install

clean: $(patsubst %, _clean_%, $(SUBDIRS))

$(patsubst %, _clean_%, $(SUBDIRS)) :
	$(MAKE) CFLAGS="$(CFLAGS)" MAYOR_VERSION="$(MAYOR_VERSION)" -C $(patsubst _clean_%, %, $@) clean

tags: $(OBJS)
	etags *.c *.h
