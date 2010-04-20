/*
  Copyright (C) 2009  Arne Caspari <arne@unicap-imaging.org>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <Python.h>
#include <structmember.h>
#include <unicap.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>
#include <glib.h>

#define UNICAP_MODULE
#include "unicapmodule.h"
#include "unicapdevice.h"
#include "unicapimagebuffer.h"

#include "utils.h"


static void new_frame_callback( unicap_event_t event, unicap_handle_t handle, unicap_data_buffer_t *buffer, UnicapDevice *self )
{
   
   if( self->wait_buffer )
   {
      PyObject *pybuffer = NULL;
      self->buffer = UnicapImageBuffer_new_from_buffer( buffer );
      pybuffer = self->buffer;
      if( !self->buffer )
      {
	 Py_INCREF( Py_None );
	 self->buffer = Py_None;
      }
      self->wait_buffer = 0;
      sem_post( &self->sem );
   }
   sem_wait( &self->lock );
   if( self->callbacks[CALLBACK_NEW_FRAME] ){
      PyObject *arglist;
      PyObject *result;
      PyObject *pybuffer = NULL;
      PyGILState_STATE gstate;
      
      gstate = PyGILState_Ensure();
      pybuffer = UnicapImageBuffer_new_from_buffer_no_copy( buffer );

      if( self->callback_data[ CALLBACK_NEW_FRAME ] ){
	 arglist = Py_BuildValue( "(OO)", pybuffer, self->callback_data[CALLBACK_NEW_FRAME] );
      } else {
	 arglist = Py_BuildValue( "(O)", pybuffer );
      }
      result = PyEval_CallObject( self->callbacks[CALLBACK_NEW_FRAME], arglist );
      Py_DECREF( arglist );
/*       if( result == NULL ) */
/* 	 PyErr_Clear(); */

      Py_XDECREF( result );
      Py_XDECREF( pybuffer );
      PyGILState_Release( gstate );
   }

   if( self->vobj )
   {
      ucil_encode_frame( self->vobj, buffer );
   }
   sem_post( &self->lock );
}
					      
   

static void UnicapDevice_dealloc( UnicapDevice *self )
{
   int i;
   Py_XDECREF( self->device_id );
   if( self->handle )
   {
      unicap_close( self->handle );
      self->handle = NULL;
   }
   for( i = 0; i < MAX_CALLBACK; i++ ){
      Py_XDECREF( self->callbacks[ i ] );
      Py_XDECREF( self->callback_data[ i ] );
   }
   
   self->ob_type->tp_free((PyObject*)self);
}

static PyObject *UnicapDevice_new( PyTypeObject *type, PyObject *args, PyObject *kwds )
{
   UnicapDevice *self;
   
   self = (UnicapDevice *)type->tp_alloc( type, 0 );
   if( self != NULL )
   {
      self->device_id = PyString_FromString( "" );
      if( !self->device_id )
      {
	 Py_DECREF( self );
	 return NULL;
      }

      self->handle = NULL;
   }
   return (PyObject*)self;
}

PyObject *UnicapDevice_new_from_handle( unicap_handle_t handle )
{
   UnicapDevice *self = NULL;
   
   self = (UnicapDevice *)UnicapDeviceType.tp_alloc( (PyTypeObject*)&UnicapDeviceType, 0 );
   if( self != NULL )
   {
      unicap_device_t device;
      unicap_get_device( handle, &device );
      self->device_id = PyString_FromString( device.identifier );
      if( !self->device_id )
      {
	 Py_DECREF( self );
	 return NULL;
      }
      
      self->handle = unicap_clone_handle( handle );
   }
   
   return (PyObject*)self;
}

static int conv_identifier( PyObject *object, char **target ){
   char *identifier = NULL;
   if( PyString_Check( object ) ){
      identifier = PyString_AsString( object );
   } else if( PyDict_Check( object ) ){
      PyObject *strobj;
      strobj = PyDict_GetItemString( object, "identifier" );
      if( strobj ){
	 identifier = PyString_AsString( strobj );
      }
   } else {
      PyErr_SetString( PyExc_TypeError, "expected string or Dict object" );
   }
   
   *target = identifier;
   return identifier != NULL;
}

static int UnicapDevice_init( UnicapDevice *self, PyObject *args, PyObject *kwds )
{
   const char *device_id;
   unicap_device_t dev_spec, device;

   static char *kwlist[] = { "device", NULL };

   if( !PyArg_ParseTupleAndKeywords( args, kwds, "O&", kwlist, conv_identifier, &device_id ) )
   {
      return -1;
   }
      
   unicap_void_device( &dev_spec );
   strcpy( dev_spec.identifier, device_id );
   if( !SUCCESS( unicap_enumerate_devices( &dev_spec, &device, 0 ) ) )
   {
      return -1;
   }
   
   if( !SUCCESS( unicap_open( &self->handle, &device ) ) )
   {
      return -1;
   }

   self->vobj = NULL;
   self->device_id = PyString_FromString( device.identifier );
   
   sem_init( &self->sem, 0, 0 );
   sem_init( &self->lock, 0, 1 );
   self->wait_buffer = 0;
   

   return 0;
}

int UnicapDevice_Check( PyObject *obj )
{
   return( PyObject_IsInstance( obj, &UnicapDeviceType ) == 1);
}
   

static PyMemberDef UnicapDevice_members[] =
{
   { "identifier", T_OBJECT_EX, offsetof( UnicapDevice, device_id ), 0, "Device Identifier" }, 
   { NULL }
};

static const char enumerate_formats__doc__[] = "\
Returns a list containing all video formats supported by the device.\n\
Each format description consists of a dictionary with at least the following keys:\n\
identifier: A string identifying the video format\n\
fourcc: A fourc character string describing the colour format\n\
bpp: Bits per pixel\n\
size: A tuple containing the width and height of the format\n\
min_size: A tuple containing the minimum width and height of the format\n\
max_size: A tuple containing the maximum width and height of the format\n\
\n\
Optional keys include:\n\
size_list: A list of (width,height) tuples containg all valid sizes for this format\
";
static PyObject *UnicapDevice_enumerate_formats( UnicapDevice *self )
{
   static PyObject *list = NULL;
   
   unicap_format_t format;
   int i;

   Py_BEGIN_ALLOW_THREADS;

   list = PyList_New(0);

   for( i = 0; SUCCESS( unicap_enumerate_formats( self->handle, NULL, &format, i ) ); i++ )
   {
      PyObject *obj = NULL;
      
      obj = build_video_format( &format );
      
      if( obj )
      {
	 PyList_Append( list, obj );
      } else{
	 printf( "Failed to build video format\n" );
      }
   }
   
   Py_END_ALLOW_THREADS;

   return list;
}      

unicap_handle_t UnicapDevice_get_handle( PyObject *_self )
{
   UnicapDevice *self = (UnicapDevice*)_self;
   return unicap_clone_handle( self->handle );
}


static const char set_format__doc__[] = "\
set_format(fmt)\n\n\
Sets a video format on the device.\n\
\n\
fmt: A dict containing a format description with the 'size' and 'fourcc' keys set to the desired values\
";
static PyObject *UnicapDevice_set_format( UnicapDevice *self, PyObject *args, PyObject *kwds )
{
   unicap_format_t format_spec, format;
   PyObject *fmt_obj;
   PyThreadState *save;

   if( !PyArg_ParseTuple( args, "O", &fmt_obj ) )
   {
      return NULL;
   }
   
   save = PyEval_SaveThread();
   parse_video_format( &format_spec, fmt_obj );
   Py_XDECREF( fmt_obj );
   format_spec.buffer_size = -1;
   if( !SUCCESS( unicap_enumerate_formats( self->handle, &format_spec, &format, 0 ) ) )
   {
      PyEval_RestoreThread(save);
      PyErr_SetString( UnicapException, "Failed to enumerate video formats" );
      return NULL;
   }

   format.buffer_type =  UNICAP_BUFFER_TYPE_SYSTEM;
   
   if( !SUCCESS( unicap_set_format( self->handle, &format ) ) )
   {
      PyEval_RestoreThread(save);
      PyErr_SetString( UnicapException, "Failed to set video format on device" );
      return NULL;
   }
   
   PyEval_RestoreThread(save);
   Py_INCREF( Py_None );
   return( Py_None );
}

static const char get_format__doc__[] = "\
Returns the current video format of the device.\
";
static PyObject *UnicapDevice_get_format( UnicapDevice *self, PyObject *args, PyObject *kwds )
{
   PyObject *result = NULL;
   unicap_format_t format;
   PyThreadState *save;

   save = PyEval_SaveThread();
   
   if( !SUCCESS( unicap_get_format( self->handle, &format ) ) )
   {
      PyEval_RestoreThread(save);
      PyErr_SetString( UnicapException, "Failed to get video format from device" );
      return NULL;
   }
   
   PyEval_RestoreThread(save);
   result = build_video_format( &format );
   
   return result;
}

static const char start_capture__doc__[] = "\
Starts video capturing.\
";
static PyObject *UnicapDevice_start_capture( UnicapDevice *self, PyObject *args, PyObject *kwds )
{
   PyObject *result = NULL;
   unicap_format_t format;
   PyThreadState *save;

   save = PyEval_SaveThread();

   unicap_get_format( self->handle, &format );
   format.buffer_type =  UNICAP_BUFFER_TYPE_SYSTEM;
   unicap_set_format( self->handle, &format );

   
   if( !SUCCESS( unicap_register_callback( self->handle, UNICAP_EVENT_NEW_FRAME, (unicap_callback_t)new_frame_callback, self ) ) )
   {
      PyEval_RestoreThread(save);
      PyErr_SetString( UnicapException, "Failed to register callback function" );
      return NULL;
   }


   if( !SUCCESS( unicap_start_capture( self->handle ) ) )
   {
      PyEval_RestoreThread(save);
      PyErr_SetString( UnicapException, "Failed to start video capture" );
      return NULL;
   }

   PyEval_RestoreThread(save);
   Py_INCREF( Py_None );
   result = Py_None;
   return result;
}

static const char stop_capture__doc__[] = "\
Stops video capturing.\
";

static PyObject *UnicapDevice_stop_capture( UnicapDevice *self, PyObject *args, PyObject *kwds )
{
   PyObject *result = NULL;
   PyThreadState *save;

   save = PyEval_SaveThread();
   
   if( !SUCCESS( unicap_stop_capture( self->handle ) ) )
   {
      PyEval_RestoreThread(save);
      PyErr_SetString( UnicapException, "Failed to stop video capture" );
      return NULL;
   }

   unicap_unregister_callback( self->handle, UNICAP_EVENT_NEW_FRAME );
   
   PyEval_RestoreThread(save);
   Py_INCREF( Py_None );
   result = Py_None;
   return result;
}

static const char wait_buffer__doc__[] = "\
wait_buffer(timeout)\n\n\
Acquires the next image from the video stream and returns it as an unicap.ImageBuffer object.\n\
\n\
timeout: Timeout to wait for the buffer in seconds.\
";
static PyObject *UnicapDevice_wait_buffer( UnicapDevice *self, PyObject *args, PyObject *kwds )
{
   double tmp = 1.0;
   struct timespec rel_timeout;
   struct timespec ts;
   PyObject *result;

   static char *kwlist[] = { "timeout", NULL };

   if( !PyArg_ParseTupleAndKeywords( args, kwds, "|d", kwlist, &tmp ) )
   {      
      return NULL;
   }
   
   rel_timeout.tv_sec = tmp;
   tmp -= rel_timeout.tv_sec;
   rel_timeout.tv_nsec = 1000000000 * tmp;
   clock_gettime( CLOCK_REALTIME, &ts );
   ts.tv_sec += rel_timeout.tv_sec;
   ts.tv_nsec += rel_timeout.tv_nsec;
   if( ts.tv_nsec > 1000000000 )
   {
      ts.tv_nsec -= 1000000000;
      ts.tv_sec++;
   }
   
   Py_BEGIN_ALLOW_THREADS;
   self->wait_buffer = 1;
   while( ( sem_timedwait( &self->sem, &ts ) ) == -1 && errno == EINTR )
   {
      continue;
   }
   Py_END_ALLOW_THREADS;
   
   result = self->buffer;
   self->buffer = NULL;

   if( !result )
   {
      PyErr_SetString( UnicapTimeoutException, "Operation timed out while waiting for buffer" );
   }
   
   return result;
}
   
static const char enumerate_properties__doc__[] = "\
Returns a list of all properties supported by the device.\n\
\n\
Each list entry is a dictionary with at least the following fields:\n\
identifier: A unique string specifying the property\n\
category: A string describing the property category\n\
unit: A string describing the property unit\n\
\n\
The property may have one of the following keys:\n\
value: A float value\n\
menu_item: A string value\n\
\n\
The property may have the following optional keys:\n\
range: A (min,max) tuple specifying the range in which the 'value' may be\n\
values: A list of float values specifying the allowed values for the 'value' key\n\
menu: A list of strings specifying the allowed values for the 'menu_item' key\n\
flags: A list of strings specifying flags that are set on the property\n\
capabilities: A list of string specifying the allowed flags for the property\n\
\n\
Possible flags are:\n\
'manual': Indicates manual operation of the property\n\
'auto': Indicates automatic setting of the property\n\
'one push': Indicates a one-time automatic action of the property\n\
'read only': Indicates that the property may only be read\n\
'write only': Indicates that the values read with get_property do not reflect the actual setting\
";
static PyObject *UnicapDevice_enumerate_properties( UnicapDevice *self )
{
   int i;
   PyObject *obj;
   unicap_property_t property;
   PyThreadState *save;
   
   obj = PyList_New(0);
   
   save = PyEval_SaveThread();

   for( i = 0; SUCCESS( unicap_enumerate_properties( self->handle, NULL, &property, i ) ); i++ )
   {
      PyObject *tmp;
      tmp = build_property( &property );
      if( !tmp )
	 goto err;
      if( PyList_Append( obj, tmp ) < 0 )
      {
	 Py_XDECREF( tmp );
	 goto err;
      }
   }
   
   PyEval_RestoreThread(save);
   return obj;
   
  err:
   PyEval_RestoreThread(save);
   Py_XDECREF( obj );
   return NULL;
}

static const char get_property__doc__[] = "\
get_property(property)\n\n\
Returns the current state of a device property.\n\
\n\
property: The unique identifier of the property or a dict with the property description as returned by enumerate_properties\
";
static PyObject *UnicapDevice_get_property( UnicapDevice *self, PyObject *args, PyObject *kwds )
{
   unicap_property_t prop_spec, property;
   char *ppty_id = NULL;
   PyObject *obj;   
   PyObject *dict = NULL;
   static char *kwlist[] = { "identifier", "data", NULL };
   PyThreadState *save;

   if( !PyArg_ParseTupleAndKeywords( args, kwds, "O&|O", kwlist, conv_identifier, &ppty_id, &dict ) )
   {
      return NULL;
   }

   unicap_void_property( &prop_spec );
   strcpy( prop_spec.identifier, ppty_id );


   save = PyEval_SaveThread();
   if( !SUCCESS( unicap_enumerate_properties( self->handle, &prop_spec, &property, 0 ) ) )
   {
      PyErr_SetString( UnicapException, "Failed to enumerate property" );
      goto err;
   }

   if( dict ){
      parse_property( &property, dict );
   }

   if( !SUCCESS( unicap_get_property( self->handle, &property ) ) )
   {
      PyErr_SetString( UnicapException, "Failed to get property" );
      goto err;
   }

   obj = build_property( &property );
   PyEval_RestoreThread(save);
   
   return obj;
   
  err:
   PyEval_RestoreThread(save);
   return NULL;
   
}

static const char set_property__doc__[] = "\
set_property(property)\n\n\
Sets a property.\n\
\n\
property: A dict describing a property, as returned by enumerate_properties\n\
";
static PyObject *UnicapDevice_set_property( UnicapDevice *self, PyObject *args, PyObject *kwds )
{
   unicap_property_t property, prop_spec;
   PyObject *obj;
   PyObject *tmp = NULL;
   const char *identifier = NULL;
   PyThreadState *save;
   
   if( !PyArg_ParseTuple( args, "O", &obj ) )
      return NULL;

   tmp = PyDict_GetItemString( obj, "identifier" );
   if( !tmp )
      return NULL;

   identifier = PyString_AsString( tmp );
   if( !identifier )
      return NULL;

   save = PyEval_SaveThread();
   
   unicap_void_property( &prop_spec );
   strcpy( prop_spec.identifier, identifier );
   if( !SUCCESS( unicap_enumerate_properties( self->handle, &prop_spec, &property, 0 ) ) )
   {
      PyEval_RestoreThread(save);
      PyErr_SetString( UnicapException, "Failed to enumerate property" );
      goto err;
   }
   
   if( parse_property( &property, obj ) != 0 )
   {
      PyEval_RestoreThread(save);
      PyErr_SetString( UnicapException, "Failed to parse property" );
      goto err;
   }
   
   if( !SUCCESS( unicap_set_property( self->handle, &property ) ) )
   {
      PyEval_RestoreThread(save);
      PyErr_SetString( UnicapException, "Failed to set property" );
      goto err;
   }

   PyEval_RestoreThread(save);
   Py_INCREF( Py_None );
   return( Py_None );

  err:
   PyEval_RestoreThread(save);
   return NULL;   
}

static const char set_callback__doc__[] = "\
set_callback(which,callback,data=None)\n\n\
Sets a callback function for device events.\n\
\n\
which: The event for which the callback should be set ( integer )\n\
callback: The callback function that should be called when the event triggers or None to unset the callback\n\
data: User parameter that should be passed to the callback function\n\
\n\
Valid events are:\n\
0: New frame event \n\
     def callback(buffer,{data})\n\
     \n\
     buffer: an unicap.ImageBuffer object\n\
     data: User parameter ( if any )\n\
";
static PyObject *UnicapDevice_set_callback( UnicapDevice *self, PyObject *args, PyObject *kwds )
{
   static char *kwlist[] = { "which", "callback", "data", NULL };
   PyObject *result = NULL;
   PyObject *func = NULL;
   PyObject *data = NULL;
   int which = 0;

   if( !PyArg_ParseTupleAndKeywords(args, kwds, "iO|O:set_callback", kwlist, &which, &func, &data)) 
      return NULL;

   if( ( which < 0 ) || ( which >= MAX_CALLBACK ) )      {
      PyErr_SetString( PyExc_ValueError, "invalid event" );
      return NULL;
   }
   
   if( func == Py_None ) {
      Py_XDECREF(self->callbacks[which]);
      Py_XDECREF(self->callback_data[which]);
      self->callbacks[which] = NULL;
      self->callback_data[which] = NULL;
   } else {
      if (!PyCallable_Check(func)) {
	 PyErr_SetString(PyExc_TypeError, "parameter must be callable");
	 return NULL;
      }
      
      Py_XINCREF(func);         /* Add a reference to new callback */
      Py_XINCREF(data);
      Py_XDECREF(self->callbacks[which]);  /* Dispose of previous callback */
      Py_XDECREF(self->callback_data[which] );
      self->callbacks[which] = func;       /* Remember new callback */
      self->callback_data[which] = data;
   }
      
   Py_INCREF(Py_None);
   result = Py_None;

   return result;
}

static const char record_video__doc__[] = "\
record_video(filename,codec,params=None)\n\n\
Starts recording of the video stream to a .AVI or .OGG file\n\
\n\
filename: Name of the video file to create\n\
codec: Codec specification\n\
params: Parameters to pass to the codec\n\
\n\
Currently supported codecs: \n\
'ogg/theora'\n\
'avi/raw'\n\
\n\
See the libucil documentation for more information on codecs and parameters.\n\
";

static PyObject *UnicapDevice_record_video( UnicapDevice *self, PyObject *args, PyObject *kwds )
{
   static char *kwlist[] = { "filename", "codec", "params", NULL };
   const char* filename;
   const char *codec;
   unicap_format_t format;
   PyObject *params=NULL;

   if( !PyArg_ParseTupleAndKeywords( args, kwds, "ss|O", kwlist, &filename, &codec, &params ) )
   {
      return NULL;
   }

   if( params )
   {
      int i;
      GParameter gparams[128];
      PyObject *keys;
      memset( gparams, 0x0, sizeof( gparams ) );
      
      if( !PyDict_Check( params ) )
      {
	 PyErr_SetString( PyExc_TypeError, "params must be a dictionary" );
	 return NULL;
      }
      
      keys = PyDict_Keys( params );
      if( PyObject_Length( keys ) > 128 )
      {
	 PyErr_SetString( UnicapException, "Invalid codec params" );
	 return NULL;
      }

      for( i = 0; i < PyObject_Length( keys ); i++ )
      {
	 PyObject *key = PyList_GetItem( keys, i );
	 if( !key )
	 {
	    return NULL;
	 }
	 if( !PyString_Check( key ) )
	 {
	    PyErr_SetString( PyExc_TypeError, "Key must be a string" );
	    return NULL;
	 }
	 
	 PyObject *param = PyDict_GetItem( params, key );
	 if( !param )
	 {
	    return NULL;
	 }
	 
	 gparams[i].name = PyString_AsString( key );
	 if( PyInt_Check( param ) )
	 {
	    g_value_init( &gparams[i].value, G_TYPE_INT );
	    g_value_set_int( &gparams[i].value, PyInt_AsLong( param ) );
	 }
	 else if( PyString_Check( param ) )
	 {
	    g_value_init( &gparams[i].value, G_TYPE_STRING );
	    g_value_set_string( &gparams[i].value, PyString_AsString( param ) );
	 }
	 else if( PyFloat_Check( param ) )
	 {
	    g_value_init( &gparams[i].value, G_TYPE_DOUBLE );
	    g_value_set_double( &gparams[i].value, PyFloat_AsDouble( param ) );
	 }
	 else
	 {
	    PyErr_SetString( PyExc_TypeError, "Invalid parameter type" );
	    return NULL;
	 }
      }
      

      Py_BEGIN_ALLOW_THREADS;
      unicap_get_format( self->handle, &format );
      Py_END_ALLOW_THREADS;
      sem_wait( &self->lock );
      self->vobj = ucil_create_video_filev( filename, &format, codec, i, gparams );
      sem_post( &self->lock );
   }
   else
   {
      sem_wait( &self->lock );
      self->vobj = ucil_create_video_file( filename, &format, codec, NULL );
      sem_wait( &self->lock );
   }

   Py_INCREF( Py_None );
   return( Py_None );
}

static const char stop_record__doc__[] = "\
Stops recording of video file.\
";
static PyObject *UnicapDevice_stop_record( UnicapDevice *self, PyObject *args, PyObject *kwds )
{
   sem_wait( &self->lock );
   if( self->vobj )
   {
      if( !SUCCESS( ucil_close_video_file( self->vobj ) ) )
      {
	 PyErr_SetString( UnicapException, "Failed to close the video file object" );
	 return NULL;
      }
   }
   self->vobj = NULL;
   sem_post( &self->lock );

   Py_INCREF( Py_None );
   return( Py_None );
}




static PyMethodDef UnicapDevice_methods[] = 
{
   { "enumerate_formats", (PyCFunction)UnicapDevice_enumerate_formats, METH_NOARGS, enumerate_formats__doc__ },
   { "set_format", (PyCFunction)UnicapDevice_set_format, METH_VARARGS, set_format__doc__ },
   { "get_format", (PyCFunction)UnicapDevice_get_format, METH_NOARGS, get_format__doc__ },
   { "start_capture", (PyCFunction)UnicapDevice_start_capture, METH_NOARGS, start_capture__doc__ },
   { "stop_capture", (PyCFunction)UnicapDevice_stop_capture, METH_NOARGS, stop_capture__doc__ },
   { "wait_buffer", (PyCFunction)UnicapDevice_wait_buffer, METH_VARARGS, wait_buffer__doc__ },
   { "enumerate_properties", (PyCFunction)UnicapDevice_enumerate_properties, METH_NOARGS, enumerate_properties__doc__ },
   { "get_property", (PyCFunction)UnicapDevice_get_property, METH_VARARGS, get_property__doc__ },
   { "set_property", (PyCFunction)UnicapDevice_set_property, METH_VARARGS, set_property__doc__ },
   { "record_video", (PyCFunction)UnicapDevice_record_video, METH_VARARGS | METH_KEYWORDS, record_video__doc__ },
   { "stop_record", (PyCFunction)UnicapDevice_stop_record, METH_NOARGS, stop_record__doc__ },
   { "set_callback", (PyCFunction)UnicapDevice_set_callback, METH_VARARGS, set_callback__doc__ },
   
   { NULL }
};

PyTypeObject UnicapDeviceType = 
{
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "unicap.Device",             /*tp_name*/
    sizeof(UnicapDevice),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)UnicapDevice_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "UnicapDevice objects",    /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    UnicapDevice_methods,      /* tp_methods */
    UnicapDevice_members,      /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)UnicapDevice_init,/* tp_init */
    0,                         /* tp_alloc */
    UnicapDevice_new,          /* tp_new */
};

   
void initunicapdevice(PyObject *m)
{
   if( PyType_Ready( &UnicapDeviceType ) < 0 )
   {
      return;
   }

   PyEval_InitThreads();
   Py_INCREF( &UnicapDeviceType );
   PyModule_AddObject( m, "Device", (PyObject*)&UnicapDeviceType );
   
}
