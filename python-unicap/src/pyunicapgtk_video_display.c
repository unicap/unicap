/* -- THIS FILE IS GENERATED - DO NOT EDIT *//* -*- Mode: C; c-basic-offset: 4 -*- */

#include <Python.h>



#line 3 "pyunicapgtk_video_display.override"
#include <Python.h>
#include <pygobject.h>
#include <pygtk/pygtk.h>
#include <unicap.h>
#include "unicapmodule.h"
#include "utils.h"
#include "unicapgtk.h"
#line 16 "pyunicapgtk_video_display.c"


/* ---------- types from other modules ---------- */
static PyTypeObject *_PyGtkAspectFrame_Type;
#define PyGtkAspectFrame_Type (*_PyGtkAspectFrame_Type)


/* ---------- forward type declarations ---------- */
PyTypeObject G_GNUC_INTERNAL PyUnicapgtkVideoDisplay_Type;

#line 27 "pyunicapgtk_video_display.c"



/* ----------- UnicapgtkVideoDisplay ----------- */

#line 28 "pyunicapgtk_video_display.override"
static int
_wrap_unicapgtk_video_display_new(PyGObject *self, PyObject *args, PyObject *kwargs)
{
   static char* kwlist[] = { "device_id", "device", NULL };
   unicap_device_t device_spec;
   unicap_device_t device;
   char *device_id = NULL;
   
   if( import_unicap() < 0 ){
      PyErr_SetString( PyExc_RuntimeError, "Could not import unicap module" );
      return -1;
   }

   if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				    "|O&:unicapgtk.VideoDisplay.__init__",
				    kwlist, conv_device_identifier, &device_id ))
      return -1;

   pygobject_constructv(self, 0, NULL);
   if( device_id ){
      unicap_void_device( &device );
      unicap_void_device( &device_spec );
      if( device_id )
	 strcpy( device_spec.identifier, device_id );

      unicap_reenumerate_devices( NULL );

      if( !SUCCESS( unicap_enumerate_devices( &device_spec, &device, 0 ) ) ){
	 PyErr_SetString( PyExc_RuntimeError, "failed to enumerate device" );
	 return -1;
      }
      if( !SUCCESS( unicapgtk_video_display_set_device( UNICAPGTK_VIDEO_DISPLAY( self->obj ), &device ) ) ){
	 PyErr_SetString( PyExc_RuntimeError, "failed to set device" );
	 return -1;
      }
   }

   return 0;
}
#line 73 "pyunicapgtk_video_display.c"


static PyObject *
_wrap_unicapgtk_video_display_start(PyGObject *self)
{
    int ret;

    
    ret = unicapgtk_video_display_start(UNICAPGTK_VIDEO_DISPLAY(self->obj));
    
    return PyBool_FromLong(ret);

}

static PyObject *
_wrap_unicapgtk_video_display_stop(PyGObject *self)
{
    
    unicapgtk_video_display_stop(UNICAPGTK_VIDEO_DISPLAY(self->obj));
    
    Py_INCREF(Py_None);
    return Py_None;
}

#line 69 "pyunicapgtk_video_display.override"
static PyObject *
_wrap_unicapgtk_video_display_get_device(PyGObject *self, PyObject *args, PyObject *kwargs)
{
   unicap_handle_t handle;
   PyObject *obj;   
   
   handle = unicapgtk_video_display_get_handle( UNICAPGTK_VIDEO_DISPLAY( self->obj ) );

   obj = UnicapDevice_new_from_handle( handle );

   return obj;
}
#line 111 "pyunicapgtk_video_display.c"


#line 83 "pyunicapgtk_video_display.override"
static PyObject *
_wrap_unicapgtk_video_display_set_format(PyGObject *self, PyObject *args, PyObject *kwargs)
{
   unicap_format_t format_spec;
   PyObject *fmt_obj;

   if( !PyArg_ParseTuple( args, "O", &fmt_obj ) )
   {
      return NULL;
   }
   
   parse_video_format( &format_spec, fmt_obj );
   format_spec.buffer_size = -1;

   format_spec.buffer_type =  UNICAP_BUFFER_TYPE_SYSTEM;
   
   if( !SUCCESS( unicapgtk_video_display_set_format( UNICAPGTK_VIDEO_DISPLAY( self->obj ), &format_spec ) ) )
   {
      PyErr_SetString( PyExc_IOError, "Failed to set video format on device" );
      return NULL;
   }
   
   Py_INCREF( Py_None );
   return( Py_None );   
}
#line 140 "pyunicapgtk_video_display.c"


#line 110 "pyunicapgtk_video_display.override"
static PyObject *
_wrap_unicapgtk_video_display_get_format( PyGObject *self, PyObject *args, PyObject *kwargs)
{
   PyObject *result = NULL;
   unicap_format_t format;
   
   unicapgtk_video_display_get_format( UNICAPGTK_VIDEO_DISPLAY( self->obj ), &format );
   
   result = build_video_format( &format );
   
   return result;
}   
#line 156 "pyunicapgtk_video_display.c"


static PyObject *
_wrap_unicapgtk_video_display_set_pause(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "state", NULL };
    int state;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"i:Unicapgtk.VideoDisplay.set_pause", kwlist, &state))
        return NULL;
    
    unicapgtk_video_display_set_pause(UNICAPGTK_VIDEO_DISPLAY(self->obj), state);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_unicapgtk_video_display_get_pause(PyGObject *self)
{
    int ret;

    
    ret = unicapgtk_video_display_get_pause(UNICAPGTK_VIDEO_DISPLAY(self->obj));
    
    return PyBool_FromLong(ret);

}

#line 156 "pyunicapgtk_video_display.override"
static PyObject *
_wrap_unicapgtk_video_display_set_device( PyGObject *self, PyObject *args, PyObject *kwargs)
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
   
   if( !SUCCESS( unicapgtk_video_display_set_device( UNICAPGTK_VIDEO_DISPLAY( self->obj ), &device ) ) ){
      PyErr_SetString( PyExc_IOError, "Failed to set device" );
      return NULL;
   }

   Py_INCREF( Py_None );
   return( Py_None );   
}
#line 217 "pyunicapgtk_video_display.c"


static PyObject *
_wrap_unicapgtk_video_display_get_still_image(PyGObject *self)
{
    GdkPixbuf *ret;

    
    ret = unicapgtk_video_display_get_still_image(UNICAPGTK_VIDEO_DISPLAY(self->obj));
    
    /* pygobject_new handles NULL checking */
    return pygobject_new((GObject *)ret);
}

static PyObject *
_wrap_unicapgtk_video_display_set_size(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "width", "height", NULL };
    int width, height;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"ii:Unicapgtk.VideoDisplay.set_size", kwlist, &width, &height))
        return NULL;
    
    unicapgtk_video_display_set_size(UNICAPGTK_VIDEO_DISPLAY(self->obj), width, height);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject *
_wrap_unicapgtk_video_display_set_scale_to_fit(PyGObject *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = { "scale", NULL };
    int scale;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs,"i:Unicapgtk.VideoDisplay.set_scale_to_fit", kwlist, &scale))
        return NULL;
    
    unicapgtk_video_display_set_scale_to_fit(UNICAPGTK_VIDEO_DISPLAY(self->obj), scale);
    
    Py_INCREF(Py_None);
    return Py_None;
}

#line 124 "pyunicapgtk_video_display.override"
static PyObject *
_wrap_unicapgtk_video_display_set_device_id( PyGObject *self, PyObject *args, PyObject *kwargs)
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
   
   if( !SUCCESS( unicapgtk_video_display_set_device( UNICAPGTK_VIDEO_DISPLAY( self->obj ), &device ) ) ){
      PyErr_SetString( PyExc_IOError, "Failed to set device" );
      return NULL;
   }

   Py_INCREF( Py_None );
   return( Py_None );   
}
#line 293 "pyunicapgtk_video_display.c"


static const PyMethodDef _PyUnicapgtkVideoDisplay_methods[] = {
    { "start", (PyCFunction)_wrap_unicapgtk_video_display_start, METH_NOARGS,
      (char *) "start()\nStarts capturing and displaying video." },
    { "stop", (PyCFunction)_wrap_unicapgtk_video_display_stop, METH_NOARGS,
      (char *) "stop()\nStops capturing video. The last video image capture will be displayed" },
    { "get_device", (PyCFunction)_wrap_unicapgtk_video_display_get_device, METH_VARARGS,
      (char *) "get_device()\nReturns currently used video device" },
    { "set_format", (PyCFunction)_wrap_unicapgtk_video_display_set_format, METH_VARARGS,
      (char *) "set_format(format)\nSets the video format.\n\nSee unicap.enumerate_formats for the definition of a video format." },
    { "get_format", (PyCFunction)_wrap_unicapgtk_video_display_get_format, METH_VARARGS,
      (char *) "get_format()\nReturns the current video format." },
    { "set_pause", (PyCFunction)_wrap_unicapgtk_video_display_set_pause, METH_VARARGS|METH_KEYWORDS,
      (char *) "set_pause(state)\nSets the pause state of the display.\n\nstate: True: The display gets paused; False: The display gets unpaused" },
    { "get_pause", (PyCFunction)_wrap_unicapgtk_video_display_get_pause, METH_NOARGS,
      (char *) "get_pause()\nReturns the pause state of the device\n\nReturns True when the display is paused; False otherwise" },
    { "set_device", (PyCFunction)_wrap_unicapgtk_video_display_set_device, METH_VARARGS,
      (char *) "set_device(device)\nSets the current video capture device\n\ndevice: A unicap.Device" },
    { "get_still_image", (PyCFunction)_wrap_unicapgtk_video_display_get_still_image, METH_NOARGS,
      (char *) "get_still_image()\nReturns a gdk.Pixbuf with the currently displayed image." },
    { "set_size", (PyCFunction)_wrap_unicapgtk_video_display_set_size, METH_VARARGS|METH_KEYWORDS,
      (char *) "set_size(width,height)\nSets the size of the display" },
    { "set_scale_to_fit", (PyCFunction)_wrap_unicapgtk_video_display_set_scale_to_fit, METH_VARARGS|METH_KEYWORDS,
      (char *) "set_scale_to_fit(scale)\nDetermines whether the display should be scaled to fill all available space.\n\nscale: When True, the display fills the available space; otherwise the video image has the size of the current video format." },
    { "set_device_id", (PyCFunction)_wrap_unicapgtk_video_display_set_device_id, METH_VARARGS|METH_KEYWORDS,
      NULL },
    { NULL, NULL, 0, NULL }
};

PyTypeObject G_GNUC_INTERNAL PyUnicapgtkVideoDisplay_Type = {
    PyObject_HEAD_INIT(NULL)
    0,                                 /* ob_size */
    "unicapgtk.VideoDisplay",                   /* tp_name */
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
    (struct PyMethodDef*)_PyUnicapgtkVideoDisplay_methods, /* tp_methods */
    (struct PyMemberDef*)0,              /* tp_members */
    (struct PyGetSetDef*)0,  /* tp_getset */
    NULL,                              /* tp_base */
    NULL,                              /* tp_dict */
    (descrgetfunc)0,    /* tp_descr_get */
    (descrsetfunc)0,    /* tp_descr_set */
    offsetof(PyGObject, inst_dict),                 /* tp_dictoffset */
    (initproc)_wrap_unicapgtk_video_display_new,             /* tp_init */
    (allocfunc)0,           /* tp_alloc */
    (newfunc)0,               /* tp_new */
    (freefunc)0,             /* tp_free */
    (inquiry)0              /* tp_is_gc */
};



/* ----------- functions ----------- */

const PyMethodDef pyunicapgtk_video_display_functions[] = {
    { NULL, NULL, 0, NULL }
};

/* initialise stuff extension classes */
void
pyunicapgtk_video_display_register_classes(PyObject *d)
{
    PyObject *module;

    if ((module = PyImport_ImportModule("gtk")) != NULL) {
        _PyGtkAspectFrame_Type = (PyTypeObject *)PyObject_GetAttrString(module, "AspectFrame");
        if (_PyGtkAspectFrame_Type == NULL) {
            PyErr_SetString(PyExc_ImportError,
                "cannot import name AspectFrame from gtk");
            return ;
        }
    } else {
        PyErr_SetString(PyExc_ImportError,
            "could not import gtk");
        return ;
    }


#line 397 "pyunicapgtk_video_display.c"
    pygobject_register_class(d, "UnicapgtkVideoDisplay", UNICAPGTK_TYPE_VIDEO_DISPLAY, &PyUnicapgtkVideoDisplay_Type, Py_BuildValue("(O)", &PyGtkAspectFrame_Type));
    pyg_set_object_has_new_constructor(UNICAPGTK_TYPE_VIDEO_DISPLAY);
}
