#ifndef __PYUNICAPGTK_VIDEO_DISPLAY_H__
#define __PYUNICAPGTK_VIDEO_DISPLAY_H__

#include <Python.h>
#include <glib.h>


void pyunicapgtk_video_display_register_classes(PyObject *d);
void pyunicapgtk_video_display_add_constants(PyObject *module, const gchar *strip_prefix);

#endif// __PYUNICAPGTK_VIDEO_DISPLAY_H__
