#ifndef __PYUNICAPGTK_DEVICE_SELECTION_H__
#define __PYUNICAPGTK_DEVICE_SELECTION_H__

#include <Python.h>
#include <glib.h>


void pyunicapgtk_device_selection_register_classes(PyObject *d);
void pyunicapgtk_device_selection_add_constants(PyObject *module, const gchar *strip_prefix);

#endif// __PYUNICAPGTK_DEVICE_SELECTION_H__
