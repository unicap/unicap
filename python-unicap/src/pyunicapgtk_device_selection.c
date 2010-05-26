/* -- THIS FILE IS GENERATED - DO NOT EDIT *//* -*- Mode: C; c-basic-offset: 4 -*- */

#include <Python.h>



#line 3 "pyunicapgtk_device_selection.override"
#include <Python.h>
#include <pygobject.h>
#include <pygtk/pygtk.h>
#include <unicap.h>
#include "unicapmodule.h"
#include "utils.h"
#include "unicapgtk.h"
#line 16 "pyunicapgtk_device_selection.c"


/* ---------- types from other modules ---------- */
static PyTypeObject *_PyGtkComboBox_Type;
#define PyGtkComboBox_Type (*_PyGtkComboBox_Type)
static PyTypeObject *_PyGtkWidget_Type;
#define PyGtkWidget_Type (*_PyGtkWidget_Type)


/* ---------- forward type declarations ---------- */
PyTypeObject G_GNUC_INTERNAL PyUnicapgtkDeviceSelection_Type;

#line 29 "pyunicapgtk_device_selection.c"



/* ----------- UnicapgtkDeviceSelection ----------- */

#line 20 "pyunicapgtk_device_selection.override"
static int
_wrap_unicapgtk_device_selection_new(PyGObject *self, PyObject *args, PyObject *kwargs)
{
   static char* kwlist[] = { "include_rescan", NULL };
   int include_rescan = FALSE;
   
   if( import_unicap() < 0 ){
      PyErr_SetString( PyExc_RuntimeError, "Could not import unicap module" );
      return -1;
   }

   if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				    "|i:unicapgtk.DeviceSelection.__init__",
				    kwlist, &include_rescan ))
      return -1;

   pygobject_construct( self, "include-rescan-entry", include_rescan, NULL );

   return 0;
}
#line 56 "pyunicapgtk_device_selection.c"


static PyObject *
_wrap_unicapgtk_device_selection_rescan(PyGObject *self)
{
    int ret;

    
    ret = unicapgtk_device_selection_rescan(UNICAPGTK_DEVICE_SELECTION(self->obj));
    
    return PyInt_FromLong(ret);
}

#line 42 "pyunicapgtk_device_selection.override"
static PyObject *
_wrap_unicapgtk_device_selection_set_device(PyGObject *self, PyObject *args, PyObject *kwargs)
{
   unicap_device_t device_spec;
   unicap_device_t device;
   char *identifier;
   static char* kwlist[] = { "device_id", NULL };
   
   unicap_void_device( &device_spec );
   
   if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				    "O&:unicapgtk.DeviceSelection.set_device",
				    kwlist, conv_device_identifier, &identifier )){
      return NULL;
   }
   
   strcpy( device_spec.identifier, identifier );
   
   if( !SUCCESS( unicap_enumerate_devices( &device_spec, &device, 0 ) ) ){
      PyErr_SetString( PyExc_RuntimeError, "Could not enumerate device" );
      printf( "::%s\n", device_spec.identifier );
      return NULL;
   }
   
   unicapgtk_device_selection_set_device( UNICAPGTK_DEVICE_SELECTION( self->obj ), &device );

   Py_INCREF( Py_None );
   return( Py_None );   
}
#line 100 "pyunicapgtk_device_selection.c"


static PyObject *
_wrap_unicapgtk_device_selection_set_label_fmt(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "fmt", NULL };
    char *fmt;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"s:Unicapgtk.DeviceSelection.set_label_fmt", kwlist, &fmt))
        return NULL;
    
    unicapgtk_device_selection_set_label_fmt(UNICAPGTK_DEVICE_SELECTION(self->obj), fmt);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static const PyMethodDef _PyUnicapgtkDeviceSelection_methods[] = {
    { "rescan", (PyCFunction)_wrap_unicapgtk_device_selection_rescan, METH_NOARGS,
      (char *) "rescan()\n"
"\n"
"Triggers a rescan of the available video capture devices." },
    { "set_device", (PyCFunction)_wrap_unicapgtk_device_selection_set_device, METH_VARARGS,
      (char *) "set_device(device_id)\n"
"\n"
"Sets the current device." },
    { "set_label_fmt", (PyCFunction)_wrap_unicapgtk_device_selection_set_label_fmt, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { NULL, NULL, 0, NULL }
};

PyTypeObject G_GNUC_INTERNAL PyUnicapgtkDeviceSelection_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                 /* ob_size */
    "unicapgtk.DeviceSelection",                   /* tp_name */
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
    (struct PyMethodDef*)_PyUnicapgtkDeviceSelection_methods, /* tp_methods */
    (struct PyMemberDef*)0,              /* tp_members */
    (struct PyGetSetDef*)0,  /* tp_getset */
    NULL,                              /* tp_base */
    NULL,                              /* tp_dict */
    (descrgetfunc)0,    /* tp_descr_get */
    (descrsetfunc)0,    /* tp_descr_set */
    offsetof(PyGObject, inst_dict),                 /* tp_dictoffset */
    (initproc)_wrap_unicapgtk_device_selection_new,             /* tp_init */
    (allocfunc)0,           /* tp_alloc */
    (newfunc)0,               /* tp_new */
    (freefunc)0,             /* tp_free */
    (inquiry)0              /* tp_is_gc */
};



/* ----------- functions ----------- */

static PyObject *
_wrap_unicapgtk_device_selection_get_type(PyObject *self)
{
    GType ret;

    
    ret = unicapgtk_device_selection_get_type();
    
    return pyg_type_wrapper_new(ret);
}

const PyMethodDef pyunicapgtk_device_selection_functions[] = {
    { "unicapgtk_device_selection_get_type", (PyCFunction)_wrap_unicapgtk_device_selection_get_type, METH_NOARGS,
      NULL },
    { NULL, NULL, 0, NULL }
};

/* initialise stuff extension classes */
void
pyunicapgtk_device_selection_register_classes(PyObject *d)
{
    PyObject *module;

    if ((module = PyImport_ImportModule("gtk")) != NULL) {
        _PyGtkComboBox_Type = (PyTypeObject *)PyObject_GetAttrString(module, "ComboBox");
        if (_PyGtkComboBox_Type == NULL) {
            PyErr_SetString(PyExc_ImportError,
                "cannot import name ComboBox from gtk");
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


#line 224 "pyunicapgtk_device_selection.c"
    pygobject_register_class(d, "UnicapgtkDeviceSelection", UNICAPGTK_TYPE_DEVICE_SELECTION, &PyUnicapgtkDeviceSelection_Type, Py_BuildValue("(O)", &PyGtkComboBox_Type));
    pyg_set_object_has_new_constructor(UNICAPGTK_TYPE_DEVICE_SELECTION);
}
