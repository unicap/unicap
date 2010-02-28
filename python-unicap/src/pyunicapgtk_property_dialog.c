/* -- THIS FILE IS GENERATED - DO NOT EDIT *//* -*- Mode: C; c-basic-offset: 4 -*- */

#include <Python.h>



#line 3 "pyunicapgtk_property_dialog.override"
#include <Python.h>
#include <pygobject.h>
#include <pygtk/pygtk.h>
#include <unicap.h>
#include "unicapmodule.h"
#include "utils.h"
#include "unicapgtk.h"
#line 16 "pyunicapgtk_property_dialog.c"


/* ---------- types from other modules ---------- */
static PyTypeObject *_PyGtkDialog_Type;
#define PyGtkDialog_Type (*_PyGtkDialog_Type)


/* ---------- forward type declarations ---------- */
PyTypeObject G_GNUC_INTERNAL PyUnicapgtkPropertyDialog_Type;

#line 27 "pyunicapgtk_property_dialog.c"



/* ----------- UnicapgtkPropertyDialog ----------- */

#line 19 "pyunicapgtk_property_dialog.override"
static int
_wrap_unicapgtk_property_dialog_new(PyGObject *self, PyObject *args, PyObject *kwargs)
{
   static char* kwlist[] = { "device", NULL };
   unicap_device_t device_spec;
   unicap_device_t device;
   char *device_id = NULL;
   
   if( import_unicap() < 0 )
      return -1;

   if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				    "|O&:unicapgtk.PropertyDialog.__init__",
				    kwlist, conv_device_identifier, &device_id))
      return -1;

   pygobject_constructv(self, 0, NULL);
   unicap_void_device( &device );
   unicap_void_device( &device_spec );
   if( device_id )
      strcpy( device_spec.identifier, device_id );

   unicap_reenumerate_devices( NULL );
   
   if( !SUCCESS( unicap_enumerate_devices( &device_spec, &device, 0 ) ) ){
      PyErr_SetString( PyExc_RuntimeError, "failed to enumerate device" );
      return -1;
   }
   unicapgtk_property_dialog_set_device( UNICAPGTK_PROPERTY_DIALOG( self->obj ), &device );

   return 0;
}
#line 66 "pyunicapgtk_property_dialog.c"


static PyObject *
_wrap_unicapgtk_property_dialog_reset(PyGObject *self)
{
    
    unicapgtk_property_dialog_reset(UNICAPGTK_PROPERTY_DIALOG(self->obj));
    
    Py_INCREF(Py_None);
    return Py_None;
}

#line 97 "pyunicapgtk_property_dialog.override"
static PyObject *
_wrap_unicapgtk_property_dialog_set_device( PyGObject *self, PyObject *args, PyObject *kwargs)
{
   unicap_device_t dev_spec, device;
   char *identifier;

   static char *kwlist[] = { "device", NULL };

   if( !PyArg_ParseTupleAndKeywords( args, kwargs, "O&", kwlist, conv_device_identifier, &identifier ) )
   {
      PyErr_SetString( PyExc_ValueError, "Invalid argument" );
      return NULL;
   }

   unicap_void_device( &dev_spec );
   strcpy( dev_spec.identifier, identifier );
   if( !SUCCESS( unicap_enumerate_devices( &dev_spec, &device, 0 ) ) )
   {
      PyErr_SetString( PyExc_IOError, "Could not enumerate device" );
      return NULL;   
   }
   
   unicapgtk_property_dialog_set_device( UNICAPGTK_PROPERTY_DIALOG( self->obj ), &device );

   Py_INCREF( Py_None );
   return( Py_None );   
}
#line 107 "pyunicapgtk_property_dialog.c"


#line 68 "pyunicapgtk_property_dialog.override"
static PyObject *
_wrap_unicapgtk_property_dialog_set_device_id( PyGObject *self, PyObject *args, PyObject *kwargs)
{
   const char *device_id;
   unicap_device_t dev_spec, device;

   static char *kwlist[] = { "device_id", NULL };

   if( !PyArg_ParseTupleAndKeywords( args, kwargs, "s", kwlist, &device_id ) )
   {
      PyErr_SetString( PyExc_ValueError, "Invalid argument" );
      return NULL;
   }
   
   unicap_void_device( &dev_spec );
   strcpy( dev_spec.identifier, device_id );
   if( !SUCCESS( unicap_enumerate_devices( &dev_spec, &device, 0 ) ) )
   {
      PyErr_SetString( PyExc_IOError, "Could not enumerate device" );
      return NULL;   
   }
   
   unicapgtk_property_dialog_set_device( UNICAPGTK_PROPERTY_DIALOG( self->obj ), &device );

   Py_INCREF( Py_None );
   return( Py_None );   
}
#line 138 "pyunicapgtk_property_dialog.c"


static const PyMethodDef _PyUnicapgtkPropertyDialog_methods[] = {
    { "reset", (PyCFunction)_wrap_unicapgtk_property_dialog_reset, METH_NOARGS,
      NULL },
    { "set_device", (PyCFunction)_wrap_unicapgtk_property_dialog_set_device, METH_VARARGS,
      NULL },
    { "set_device_id", (PyCFunction)_wrap_unicapgtk_property_dialog_set_device_id, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { NULL, NULL, 0, NULL }
};

PyTypeObject G_GNUC_INTERNAL PyUnicapgtkPropertyDialog_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                 /* ob_size */
    "unicapgtk.PropertyDialog",                   /* tp_name */
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
    (struct PyMethodDef*)_PyUnicapgtkPropertyDialog_methods, /* tp_methods */
    (struct PyMemberDef*)0,              /* tp_members */
    (struct PyGetSetDef*)0,  /* tp_getset */
    NULL,                              /* tp_base */
    NULL,                              /* tp_dict */
    (descrgetfunc)0,    /* tp_descr_get */
    (descrsetfunc)0,    /* tp_descr_set */
    offsetof(PyGObject, inst_dict),                 /* tp_dictoffset */
    (initproc)_wrap_unicapgtk_property_dialog_new,             /* tp_init */
    (allocfunc)0,           /* tp_alloc */
    (newfunc)0,               /* tp_new */
    (freefunc)0,             /* tp_free */
    (inquiry)0              /* tp_is_gc */
};



/* ----------- functions ----------- */

static PyObject *
_wrap_unicapgtk_property_dialog_get_type(PyObject *self)
{
    GType ret;

    
    ret = unicapgtk_property_dialog_get_type();
    
    return pyg_type_wrapper_new(ret);
}

const PyMethodDef pyunicapgtk_property_dialog_functions[] = {
    { "unicapgtk_property_dialog_get_type", (PyCFunction)_wrap_unicapgtk_property_dialog_get_type, METH_NOARGS,
      NULL },
    { NULL, NULL, 0, NULL }
};

/* initialise stuff extension classes */
void
pyunicapgtk_property_dialog_register_classes(PyObject *d)
{
    PyObject *module;

    if ((module = PyImport_ImportModule("gtk")) != NULL) {
        _PyGtkDialog_Type = (PyTypeObject *)PyObject_GetAttrString(module, "Dialog");
        if (_PyGtkDialog_Type == NULL) {
            PyErr_SetString(PyExc_ImportError,
                "cannot import name Dialog from gtk");
            return ;
        }
    } else {
        PyErr_SetString(PyExc_ImportError,
            "could not import gtk");
        return ;
    }


#line 237 "pyunicapgtk_property_dialog.c"
    pygobject_register_class(d, "UnicapgtkPropertyDialog", UNICAPGTK_TYPE_PROPERTY_DIALOG, &PyUnicapgtkPropertyDialog_Type, Py_BuildValue("(O)", &PyGtkDialog_Type));
    pyg_set_object_has_new_constructor(UNICAPGTK_TYPE_PROPERTY_DIALOG);
}
