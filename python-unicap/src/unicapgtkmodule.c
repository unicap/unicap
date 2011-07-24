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
#include <pygobject.h>
#include <structmember.h>
#include <unicap.h>

/* #include "unicapgtkmodule.h" */
#include "pyunicapgtk_video_display.h"
#include "pyunicapgtk_property_dialog.h"
#include "pyunicapgtk_device_property.h"
#include "pyunicapgtk_device_selection.h"
#include "pyunicapgtk_video_format_selection.h"
#include "unicapimagebuffer.h"
#include "utils.h"

static PyObject *
pyunicapgtk_property_from_gpointer ( PyTypeObject *type, PyObject *args, PyObject *kwargs);



static PyMethodDef UnicapgtkMethods[] = 
{
	{ "property_from_gpointer", pyunicapgtk_property_from_gpointer, METH_VARARGS | METH_KEYWORDS, 
	  "Convert gpointer of type unicap_property_t to a python object" },
	{ NULL, NULL, 0, NULL }
};


static PyObject *
pyunicapgtk_property_from_gpointer ( PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	static char* kwlist[] = { "gpointer", NULL };
	UnicapImageBuffer *self;
	PyObject *gpointer = NULL;
	if (!PyArg_ParseTupleAndKeywords(args, kwargs,
					 "O:unicapgtk.ImageBuffer.__init__",
					 kwlist, &gpointer ))
		return NULL;
	
	PyObject *gobject_module = pygobject_init( -1, -1, -1 );
      
	 
	if( !PyObject_TypeCheck(gpointer, &PyGPointer_Type) ){
		PyErr_SetString( PyExc_ValueError, "Argument should be a gpointer" );
		Py_XDECREF( gpointer );
		return NULL;
	}
	
	unicap_property_t *property = pyg_pointer_get( gpointer, unicap_property_t );

	PyObject *obj = build_property (property);
	
	Py_XDECREF( gobject_module );

	return obj;
}




PyMODINIT_FUNC
initunicapgtk(void)
{
   PyObject *m, *d;

   init_pygobject();

   m = Py_InitModule( "unicapgtk", UnicapgtkMethods );
   d = PyModule_GetDict(m);
   if( m == NULL )
      return;

   pyunicapgtk_video_display_register_classes(d);
   pyunicapgtk_property_dialog_register_classes(d);
   pyunicapgtk_device_property_register_classes(d);
   pyunicapgtk_device_selection_register_classes(d);
   pyunicapgtk_video_format_selection_register_classes(d);
   

}
