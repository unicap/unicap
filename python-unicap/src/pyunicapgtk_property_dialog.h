#ifndef __PYUNICAPGTK_PROPERTY_DIALOG_H__
#define __PYUNICAPGTK_PROPERTY_DIALOG_H__

#include <Python.h>
#include <glib.h>


void pyunicapgtk_property_dialog_register_classes(PyObject *d);
void pyunicapgtk_property_dialog_add_constants(PyObject *module, const gchar *strip_prefix);

#endif// __PYUNICAPGTK_PROPERTY_DIALOG_H__
