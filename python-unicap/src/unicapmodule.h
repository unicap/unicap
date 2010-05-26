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

#ifndef __UNICAPMODULE_H__
#define __UNICAPMODULE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <unicap.h>
#include <Python.h>


#define pyunicap_device_new_from_handle_NUM    0
#define pyunicap_device_new_from_handle_RETURN PyObject *
#define pyunicap_device_new_from_handle_PROTO  ( unicap_handle_t handle )

#define pyunicap_device_get_handle_NUM         1
#define pyunicap_device_get_handle_RETURN      unicap_handle_t 
#define pyunicap_device_get_handle_PROTO       ( PyObject *self )

#define pyunicap_UnicapDeviceType_NUM          2

#define pyunicap_device_check_NUM              3
#define pyunicap_device_check_RETURN           int
#define pyunicap_device_check_PROTO            ( PyObject *obj )

#define pyunicap_API_POINTERS                  4

#ifdef UNICAP_MODULE
#ifndef __HIDDEN__
#define __HIDDEN__ __attribute__((visibility("hidden")))
#endif//__HIDDEN

__HIDDEN__ pyunicap_device_new_from_handle_RETURN UnicapDevice_new_from_handle pyunicap_device_new_from_handle_PROTO;
__HIDDEN__ pyunicap_device_get_handle_RETURN      UnicapDevice_get_handle      pyunicap_device_get_handle_PROTO;
__HIDDEN__ pyunicap_device_check_RETURN           UnicapDevice_Check_impl      pyunicap_device_check_PROTO;

#else
static void **pyunicap_API;

#define UnicapDevice_new_from_handle \
   (*(pyunicap_device_new_from_handle_RETURN (*)pyunicap_device_new_from_handle_PROTO) pyunicap_API[pyunicap_device_new_from_handle_NUM])
#define UnicapDevice_get_handle \
   (*(pyunicap_device_get_handle_RETURN (*)pyunicap_device_get_handle_PROTO) pyunicap_API[pyunicap_device_get_handle_NUM])
#define UnicapDeviceType \
   ((PyObject*)pyunicap_API[pyunicap_UnicapDeviceType_NUM])
#define UnicapDevice_Check \
   (*(pyunicap_device_check_RETURN (*)pyunicap_device_check_PROTO) pyunicap_API[pyunicap_device_check_NUM])


/* Return -1 and set exception on error, 0 on success. */
static int
import_unicap(void)
{
    PyObject *module = PyImport_ImportModule("unicap");

    if (module != NULL) {
        PyObject *c_api_object = PyObject_GetAttrString(module, "_C_API");
        if (c_api_object == NULL)
            return -1;
        if (PyCObject_Check(c_api_object))
            pyunicap_API = (void **)PyCObject_AsVoidPtr(c_api_object);
        Py_DECREF(c_api_object);
    }
    return 0;
}


#endif//UNICAP_MODULE




extern PyObject *UnicapException;
extern PyObject *UnicapTimeoutException;


#ifdef __cplusplus
}
#endif
#endif//__UNICAPMODULE_H__
