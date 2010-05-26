/* -- THIS FILE IS GENERATED - DO NOT EDIT *//* -*- Mode: C; c-basic-offset: 4 -*- */

#include <Python.h>



#line 3 "pyunicapgtk_device_property.override"
#include <Python.h>
#include <pygobject.h>
#include <pygtk/pygtk.h>
#include <unicap.h>
#include "unicapmodule.h"
#include "utils.h"
#include "unicapgtk.h"
#line 16 "pyunicapgtk_device_property.c"


/* ---------- types from other modules ---------- */
static PyTypeObject *_PyGtkHBox_Type;
#define PyGtkHBox_Type (*_PyGtkHBox_Type)
static PyTypeObject *_PyGtkWidget_Type;
#define PyGtkWidget_Type (*_PyGtkWidget_Type)


/* ---------- forward type declarations ---------- */
PyTypeObject G_GNUC_INTERNAL PyUnicapgtkDeviceProperty_Type;

#line 29 "pyunicapgtk_device_property.c"



/* ----------- UnicapgtkDeviceProperty ----------- */

#line 22 "pyunicapgtk_device_property.override"
static int
_wrap_unicapgtk_device_property_new(PyGObject *self, PyObject *args, PyObject *kwargs)
{
   static char* kwlist[] = { "property", NULL };
   PyObject *propertyobj = NULL;
   unicap_property_t property;
   
   unicap_void_property( &property );

   if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				    "O:unicapgtk.DeviceProperty.__init__",
				    kwlist, &propertyobj ))
      return -1;

   pygobject_constructv(self, 0, NULL);

   if( parse_property( &property, propertyobj ) < 0 ){
      PyErr_SetString( PyExc_ValueError, "Invalid property specification" );
      return -1;
   }
   
   unicapgtk_device_property_set( UNICAPGTK_DEVICE_PROPERTY( self->obj ), &property );

   return 0;
}
#line 61 "pyunicapgtk_device_property.c"


#line 49 "pyunicapgtk_device_property.override"
static int
_wrap_unicapgtk_device_property_set(PyGObject *self, PyObject *args, PyObject *kwargs)
{
   static char* kwlist[] = { "property", NULL };
   PyObject *propertyobj = NULL;
   unicap_property_t property;
   
   unicap_void_property( &property );

   if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				    "O:unicapgtk.DeviceProperty.__init__",
				    kwlist, &propertyobj ))
      return -1;

   if( parse_property( &property, propertyobj ) < 0 ){
      PyErr_SetString( PyExc_ValueError, "Invalid property specification" );
      return -1;
   }
   
   unicapgtk_device_property_set( UNICAPGTK_DEVICE_PROPERTY( self->obj ), &property );

   return 0;
}
#line 88 "pyunicapgtk_device_property.c"


static PyObject *
_wrap_unicapgtk_device_property_get_label(PyGObject *self)
{
    GtkWidget *ret;

    
    ret = unicapgtk_device_property_get_label(UNICAPGTK_DEVICE_PROPERTY(self->obj));
    
    /* pygobject_new handles NULL checking */
    return pygobject_new((GObject *)ret);
}

static PyObject *
_wrap_unicapgtk_device_property_set_label(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "label", NULL };
    PyGObject *label;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"O!:Unicapgtk.DeviceProperty.set_label", kwlist, &PyGtkWidget_Type, &label))
        return NULL;
    
    unicapgtk_device_property_set_label(UNICAPGTK_DEVICE_PROPERTY(self->obj), GTK_WIDGET(label->obj));
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_unicapgtk_device_property_redraw(PyGObject *self)
{
    
    unicapgtk_device_property_redraw(UNICAPGTK_DEVICE_PROPERTY(self->obj));
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_unicapgtk_device_property_update(PyGObject *self)
{
    
    unicapgtk_device_property_update(UNICAPGTK_DEVICE_PROPERTY(self->obj));
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_unicapgtk_device_property_redraw_sensitivity(PyGObject *self)
{
    
    unicapgtk_device_property_redraw_sensitivity(UNICAPGTK_DEVICE_PROPERTY(self->obj));
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_unicapgtk_device_property_update_sensitivity(PyGObject *self)
{
    
    unicapgtk_device_property_update_sensitivity(UNICAPGTK_DEVICE_PROPERTY(self->obj));
    
    Py_INCREF(Py_None);
    return Py_None;
}

static const PyMethodDef _PyUnicapgtkDeviceProperty_methods[] = {
    { "set", (PyCFunction)_wrap_unicapgtk_device_property_set, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { "get_label", (PyCFunction)_wrap_unicapgtk_device_property_get_label, METH_NOARGS,
      NULL },
    { "set_label", (PyCFunction)_wrap_unicapgtk_device_property_set_label, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { "redraw", (PyCFunction)_wrap_unicapgtk_device_property_redraw, METH_NOARGS,
      NULL },
    { "update", (PyCFunction)_wrap_unicapgtk_device_property_update, METH_NOARGS,
      NULL },
    { "redraw_sensitivity", (PyCFunction)_wrap_unicapgtk_device_property_redraw_sensitivity, METH_NOARGS,
      NULL },
    { "update_sensitivity", (PyCFunction)_wrap_unicapgtk_device_property_update_sensitivity, METH_NOARGS,
      NULL },
    { NULL, NULL, 0, NULL }
};

PyTypeObject G_GNUC_INTERNAL PyUnicapgtkDeviceProperty_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                 /* ob_size */
    "unicapgtk.DeviceProperty",                   /* tp_name */
    sizeof(PyGObject),          /* tp_basicsize */
    0,                                 /* tp_itemsize */
    /* methods */
    (destructor)0,        /* tp_dealloc */
    (printfunc)0,                      /* tp_print */
    (getattrfunc)0,       /* tp_getattr */
    (setattrfunc)0,       /* tp_setattr */
    (cmpfunc)0,           /* tp_compare */
    (reprfunc)0,             /* tp_repr */
    (PyNumberMethods*)0,     /* tp_as_number */
    (PySequenceMethods*)0, /* tp_as_sequence */
    (PyMappingMethods*)0,   /* tp_as_mapping */
    (hashfunc)0,             /* tp_hash */
    (ternaryfunc)0,          /* tp_call */
    (reprfunc)0,              /* tp_str */
    (getattrofunc)0,     /* tp_getattro */
    (setattrofunc)0,     /* tp_setattro */
    (PyBufferProcs*)0,  /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,                      /* tp_flags */
    NULL,                        /* Documentation string */
    (traverseproc)0,     /* tp_traverse */
    (inquiry)0,             /* tp_clear */
    (richcmpfunc)0,   /* tp_richcompare */
    offsetof(PyGObject, weakreflist),             /* tp_weaklistoffset */
    (getiterfunc)0,          /* tp_iter */
    (iternextfunc)0,     /* tp_iternext */
    (struct PyMethodDef*)_PyUnicapgtkDeviceProperty_methods, /* tp_methods */
    (struct PyMemberDef*)0,              /* tp_members */
    (struct PyGetSetDef*)0,  /* tp_getset */
    NULL,                              /* tp_base */
    NULL,                              /* tp_dict */
    (descrgetfunc)0,    /* tp_descr_get */
    (descrsetfunc)0,    /* tp_descr_set */
    offsetof(PyGObject, inst_dict),                 /* tp_dictoffset */
    (initproc)_wrap_unicapgtk_device_property_new,             /* tp_init */
    (allocfunc)0,           /* tp_alloc */
    (newfunc)0,               /* tp_new */
    (freefunc)0,             /* tp_free */
    (inquiry)0              /* tp_is_gc */
};



/* ----------- functions ----------- */

static PyObject *
_wrap_unicapgtk_device_property_get_type(PyObject *self)
{
    GType ret;

    
    ret = unicapgtk_device_property_get_type();
    
    return pyg_type_wrapper_new(ret);
}

const PyMethodDef pyunicapgtk_device_property_functions[] = {
    { "unicapgtk_device_property_get_type", (PyCFunction)_wrap_unicapgtk_device_property_get_type, METH_NOARGS,
      NULL },
    { NULL, NULL, 0, NULL }
};

/* initialise stuff extension classes */
void
pyunicapgtk_device_property_register_classes(PyObject *d)
{
    PyObject *module;

    if ((module = PyImport_ImportModule("gtk")) != NULL) {
        _PyGtkHBox_Type = (PyTypeObject *)PyObject_GetAttrString(module, "HBox");
        if (_PyGtkHBox_Type == NULL) {
            PyErr_SetString(PyExc_ImportError,
                "cannot import name HBox from gtk");
            return ;
        }
        _PyGtkWidget_Type = (PyTypeObject *)PyObject_GetAttrString(module, "Widget");
        if (_PyGtkWidget_Type == NULL) {
            PyErr_SetString(PyExc_ImportError,
                "cannot import name Widget from gtk");
            return ;
        }
    } else {
        PyErr_SetString(PyExc_ImportError,
            "could not import gtk");
        return ;
    }


#line 268 "pyunicapgtk_device_property.c"
    pygobject_register_class(d, "UnicapgtkDeviceProperty", UNICAPGTK_TYPE_DEVICE_PROPERTY, &PyUnicapgtkDeviceProperty_Type, Py_BuildValue("(O)", &PyGtkHBox_Type));
    pyg_set_object_has_new_constructor(UNICAPGTK_TYPE_DEVICE_PROPERTY);
}
