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
#include <glib.h>

#define UNICAP_MODULE
#include "unicapmodule.h"
#include "unicapdevice.h"     
#include "unicapimagebuffer.h"

PyObject *UnicapException = NULL;
PyObject *UnicapTimeoutException = NULL;
extern PyTypeObject UnicapDeviceType;
extern PyTypeObject UnicapImageBufferType;

static PyObject *pyunicap_enumerate_devices(PyObject *self, PyObject *args)
{
   PyObject *list = NULL;
   unicap_device_t device;
   unicap_status_t status = STATUS_FAILURE;
   int i;
   
   list = PyList_New(0);

   status = unicap_reenumerate_devices( NULL );

   if( !SUCCESS( status ) && status != STATUS_NO_DEVICE )
   {
      PyErr_SetString( UnicapException, "Failed to enumerate video capture devices" );
      return NULL;
   }
   
   
   for( i = 0; SUCCESS( unicap_enumerate_devices( NULL, &device, i ) ); i++ )
   {
      PyObject *obj = NULL;
      obj = Py_BuildValue( "{s:s,s:s,s:s,s:L,s:i}", 
			   "identifier",  device.identifier, 
			   "vendor_name", device.vendor_name, 
			   "model_name",  device.model_name, 
			   "model_id",    device.model_id, 
			   "vendor_id",   device.vendor_id );
      PyList_Append( list, obj );
   }
   
   return list;
}


static PyMethodDef UnicapMethods[] = 
{
   { "enumerate_devices", pyunicap_enumerate_devices, METH_VARARGS, "Enumerate connected devices" },
   { NULL, NULL, 0, NULL }
};

PyMODINIT_FUNC
initunicap(void)
{
   PyObject *m;
   PyObject *c_api_object;

   static void *pyunicap_API[ pyunicap_API_POINTERS ];

   
   m = Py_InitModule( "unicap", UnicapMethods );
   if( m == NULL )
   {
      return;
   }

   g_type_init();

   UnicapException = PyErr_NewException( "unciap.Exception", NULL, NULL );
   Py_INCREF( UnicapException );
   PyModule_AddObject( m, "Exception", UnicapException );

   UnicapTimeoutException = PyErr_NewException( "unciap.TimeoutException", NULL, NULL );
   Py_INCREF( UnicapTimeoutException );
   PyModule_AddObject( m, "TimeoutException", UnicapTimeoutException );

   initunicapdevice( m );
   initunicapimagebuffer( m );

   pyunicap_API[ pyunicap_device_new_from_handle_NUM ] = (void*) UnicapDevice_new_from_handle;
   pyunicap_API[ pyunicap_device_get_handle_NUM ] = (void*) UnicapDevice_get_handle;
   pyunicap_API[ pyunicap_UnicapDeviceType_NUM ] = (void*) &UnicapDeviceType;
   pyunicap_API[ pyunicap_device_check_NUM ] = (void*) UnicapDevice_Check_impl;
   pyunicap_API[ pyunicap_UnicapImageBufferType_NUM ] = (void*) &UnicapImageBufferType;
   c_api_object = PyCObject_FromVoidPtr((void*)pyunicap_API, NULL );

   if( c_api_object != NULL ){
      PyModule_AddObject( m, "_C_API", c_api_object );
   }
   
}
