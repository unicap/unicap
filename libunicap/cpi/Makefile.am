# process this file with automake to create a Makefile.in

if BUILD_VID21394
DIR_VID21394 = vid21394
endif

if BUILD_DCAM
DIR_DCAM = dcam
endif

if BUILD_V4L
DIR_V4L = v4l
endif

if BUILD_V4L2
DIR_V4L2 = v4l2cpi
endif

if BUILD_EUVCCAM
DIR_EUVCCAM = euvccam
endif

if BUILD_THING
DIR_THING = thing
endif

SUBDIRS = $(DIR_VID21394) $(DIR_DCAM) $(DIR_V4L) $(DIR_V4L2) $(DIR_EUVCCAM) $(DIR_THING)
DIST_SUBDIRS = vid21394 dcam v4l v4l2cpi euvccam thing include
MAINTAINERCLEANFILES = Makefile.in 
