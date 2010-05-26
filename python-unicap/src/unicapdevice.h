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

#ifndef __UNICAPDEVICE_H__
#define __UNICAPDEVICE_H__

#include <Python.h>
#include <unicap.h>
#include <ucil.h>
#include <semaphore.h>
#include <unicap.h>

#define __HIDDEN__ __attribute__((visibility("hidden")))

enum{
   CALLBACK_NEW_FRAME = 0,
   CALLBACK_DEVICE_REMOVED = 1,
   MAX_CALLBACK
};


typedef struct 
{
   PyObject_HEAD
   PyObject *device_id;
   unicap_handle_t handle;
   ucil_video_file_object_t *vobj;
   int wait_buffer;
   PyObject *buffer;
   sem_t sem;
   sem_t lock;

   PyObject *callbacks[MAX_CALLBACK];
   PyObject *callback_data[MAX_CALLBACK];

}UnicapDevice;


void initunicapdevice(PyObject *m);

#endif//__UNICAPDEVICE_H__
