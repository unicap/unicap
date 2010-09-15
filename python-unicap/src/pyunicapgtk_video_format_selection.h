#ifndef __PYUNICAPGTK_VIDEO_FORMAT_SELECTION_H__
#define __PYUNICAPGTK_VIDEO_FORMAT_SELECTION_H__

#include <Python.h>
#include <glib.h>


void pyunicapgtk_video_format_selection_register_classes(PyObject *d);
void pyunicapgtk_video_format_selection_add_constants(PyObject *module, const gchar *strip_prefix);

#endif// __PYUNICAPGTK_VIDEO_FORMAT_SELECTION_H__
