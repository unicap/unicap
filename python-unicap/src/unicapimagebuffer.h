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

#ifndef __UNICAPIMAGEBUFFER_H__
#define __UNICAPIMAGEBUFFER_H__

#include <Python.h>
#include <unicap.h>
#include <ucil.h>

typedef struct
{
   PyObject_HEAD
   PyObject *gobject_module;
   PyObject *format;
   unicap_data_buffer_t buffer;
   ucil_font_object_t *fobj;
   int free_data;
}UnicapImageBuffer;

extern PyTypeObject UnicapImageBufferType;

void initunicapimagebuffer( PyObject *m );

PyObject *UnicapImageBuffer_new_from_buffer( const unicap_data_buffer_t *data_buffer );

#endif//__UNICAPIMAGEBUFFER_H__
