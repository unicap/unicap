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

#include "utils.h"
#include "unicapmodule.h"

int parse_video_format( unicap_format_t *format, PyObject *obj )
{
   PyObject *tmp;

   unicap_void_format( format );
   
   tmp = PyDict_GetItemString( obj, "identifier" );
   if( tmp )
   {
      char *str;
      str = PyString_AsString( tmp );
      if( str )
      {
	 strcpy( format->identifier, str );
      }
      else
      {
	 return -1;
      }
   }
   tmp = PyDict_GetItemString( obj, "fourcc" );
   if( tmp )
   {
      char *str = PyString_AsString( tmp );
      if( str )
      {
	 format->fourcc = ((unsigned int)str[0]) | (((unsigned int)str[1])<<8) | (((unsigned int)str[2])<<16) | (((unsigned int)str[3])<<24);
      }
      else
      {
	 return -1;
      }
   }
   tmp = PyDict_GetItemString( obj, "bpp" );
   if( tmp )
   {
      if( !PyInt_Check( tmp ) )
      {
	 PyErr_SetString( PyExc_TypeError, "'bpp' must be of type Integer" );
	 return -1;
      }
      format->bpp = PyInt_AsLong( tmp );
   }
   tmp = PyDict_GetItemString( obj, "size" );
   if( tmp )
   {
      PyObject *t;
      if( !PyTuple_Check( tmp ) )
      {
	 PyErr_SetString( PyExc_TypeError, "'size' must be a tuple of Integers" );
	 return -1;
      }
      t = PyTuple_GetItem( tmp, 0 );
      if( !t )
	 return -1;
      if( !PyInt_Check( t ) )
      {
	 PyErr_SetString( PyExc_TypeError, "'size' must be a tuple of Integers" );	 
	 return -1;
      }
      format->size.width = PyInt_AsLong( t );
      
      t = PyTuple_GetItem( tmp, 1 );
      if( !t )
	 return -1;
      if( !PyInt_Check( t ) )
      {
	 PyErr_SetString( PyExc_TypeError, "'size' must be a tuple of Integers" );	 
	 return -1;
      }
      
      format->size.height = PyInt_AsLong( t );
   }

   format->buffer_size = format->size.width * format->size.height * format->bpp / 8;
   
   return 0;
}


PyObject *build_video_format( const unicap_format_t *format )
{
   PyObject *obj;
   char fourcc[4] = { (unsigned char ) format->fourcc & 0xff,
		      (unsigned char )(format->fourcc >> 8 ) & 0xff, 
		      (unsigned char )(format->fourcc >> 16) & 0xff, 
		      (unsigned char )(format->fourcc >> 24) & 0xff };
      
   // name: string, fourcc: 4 char string, bpp: int, size: (int,int), min_size: (int,int), max_size: (int,int), sizes: [(int,int)]
   obj = Py_BuildValue( "{s:s#,s:i,s:(ii)}", 
			"fourcc", fourcc, 4, 
			"bpp", format->bpp, 
			"size", format->size.width, format->size.height );
   if (strlen( format->identifier ))
      PyDict_SetItemString( obj, "identifier", PyString_FromString (format->identifier) );
   if ((format->min_size.width != -1 ) && ( format->min_size.width != -1 ) )
      PyDict_SetItemString( obj, "min_size", PyTuple_Pack( 2, PyInt_FromLong( format->min_size.width ), PyInt_FromLong( format->min_size.height ) ) );
   if ((format->max_size.width != -1 ) && ( format->max_size.width != -1 ) )
      PyDict_SetItemString( obj, "max_size", PyTuple_Pack( 2, PyInt_FromLong( format->max_size.width ), PyInt_FromLong( format->max_size.height ) ) );
   
      

   if( format->size_count ){
      PyObject *list;
      int i;

      list = PyList_New( 0 );
      if( !list )
      {
	 Py_DECREF( obj );
	 return NULL;
      }

      for( i = 0; i < format->size_count; i++ ){
	 PyObject *tmp;
	 
	 tmp = Py_BuildValue( "(ii)", format->sizes[i].width, format->sizes[i].height );
	 if( tmp )
	 {
	    if( PyList_Append( list, tmp ) != 0 )
	    {
	       return NULL;
	    }
	 } 
      }
      
      PyDict_SetItemString( obj, "size_list", list );
   }

   return obj;
}

static PyObject *build_property_flags_list( unsigned long long flags )
{
   PyObject *list = NULL;
   list = PyList_New(0);
   if( flags & UNICAP_FLAGS_MANUAL )
   {
      if( PyList_Append( list, Py_BuildValue( "s", "manual" ) ) != 0 )
	 goto err;
   }
   if( flags & UNICAP_FLAGS_AUTO )
   {
      if( PyList_Append( list, Py_BuildValue( "s", "auto" ) ) != 0 )
	 goto err;
   }
   if( flags & UNICAP_FLAGS_ONE_PUSH )
   {
      if( PyList_Append( list, Py_BuildValue( "s", "one push" ) ) != 0 )
	 goto err;
   }
   if( flags & UNICAP_FLAGS_READ_ONLY )
   {
      if( PyList_Append( list, Py_BuildValue( "s", "read only" ) ) != 0 )
	 goto err;
   }
   if( flags & UNICAP_FLAGS_WRITE_ONLY )
   {
      if( PyList_Append( list, Py_BuildValue( "s", "write only" ) ) != 0 )
	 goto err;
   }
   
   return list;
   
  err:
   Py_XDECREF( list );
   return NULL;
}


PyObject *build_property( const unicap_property_t *property )
{
   PyObject *obj;
   PyObject *tmp = NULL;
   
   // name: string, category: string, unit: string, (value: float)|(menu_item: string), 
   // (range: (float,float))|(values:[float,..])|(menu:[string,..]), flags: [string], capabilities: [string]

   obj = Py_BuildValue( "{s:s,s:s}", 
			"identifier", property->identifier, 
			"category", property->category );
   if( !obj )
   {
      return NULL;
   }
   
   if( strlen( property->unit ) > 0 )
   {
      if( PyDict_SetItemString( obj, "unit", Py_BuildValue( "s", property->unit ) ) != 0 )
	 goto err;
   }

   switch( property->type )
   {
      case UNICAP_PROPERTY_TYPE_RANGE:
      {
	 if( PyDict_SetItemString( obj, "value", Py_BuildValue( "d", property->value ) ) != 0 )
	    goto err;
	 if( PyDict_SetItemString( obj, "range", Py_BuildValue( "(d,d)", property->range.min, property->range.max ) ) != 0 )
	    goto err;
      }
      break;
      case UNICAP_PROPERTY_TYPE_VALUE_LIST:
      {
	 int i;
/* 	 if( !property->value_list ) */
/* 	    goto err; */

	 if( PyDict_SetItemString( obj, "value", Py_BuildValue( "d", property->value ) ) != 0 )
	    goto err;
	 tmp = PyList_New(0);
	 if( !tmp )
	    goto err;
	 for( i = 0; i < property->value_list.value_count; i++ )
	 {
	    if( PyList_Append( tmp, Py_BuildValue( "d", property->value_list.values[i] ) ) != 0 )
	       goto err;
	 }
	 if( PyDict_SetItemString( obj, "values", tmp ) )
	    goto err;

	 tmp = NULL;
      }
      break;
      
      case UNICAP_PROPERTY_TYPE_MENU:
      {
	 int i;
	 if( PyDict_SetItemString( obj, "menu_item", Py_BuildValue( "s", property->menu_item ) ) != 0 )
	    goto err;
	 
	 tmp = PyList_New(0);
	 if( !tmp) 
	    goto err;
	 
	 for( i = 0; i < property->menu.menu_item_count; i++ )
	 {
	    if( PyList_Append( tmp, Py_BuildValue( "s", property->menu.menu_items[i] ) ) != 0 )
	       goto err;
	 }
	 if( PyDict_SetItemString( obj, "menu_items", tmp ) )
	    goto err;

	 tmp = NULL;
      }
      break;

   case UNICAP_PROPERTY_TYPE_DATA:
   {
      PyObject *data = PyString_FromStringAndSize( (char*)property->property_data, property->property_data_size );
      PyDict_SetItemString( obj, "data", data );
   }
   break;
   
      
      default:
	 break;
   }
   
   tmp = build_property_flags_list( property->flags );
   if( !tmp )
      goto err;
   if( PyDict_SetItemString( obj, "flags", tmp ) != 0 )
      goto err;
   
   tmp = build_property_flags_list( property->flags_mask );
   if( !tmp )
      goto err;
   if( PyDict_SetItemString( obj, "flags_mask", tmp ) != 0 )
      goto err;

   return obj;
   
  err:
   Py_XDECREF(tmp);
   Py_XDECREF( obj );
   return NULL;
}

int parse_property( unicap_property_t *property, PyObject *obj )
{   
   PyObject *tmp = NULL;

   switch( property->type )
   {
      case UNICAP_PROPERTY_TYPE_RANGE:
      case UNICAP_PROPERTY_TYPE_VALUE_LIST:
      {
	 tmp = PyDict_GetItemString( obj, "value" );
	 if( !tmp )
	 {
	    PyErr_SetString( PyExc_ValueError, "Object is missing 'value' key" );
	    goto err;
	 }
	 if( PyFloat_Check( tmp ) )
	 {
	    property->value = PyFloat_AsDouble( tmp );
	 }
	 else if( PyInt_Check( tmp ) )
	 {
	    property->value = PyInt_AsLong( tmp );
	 }
	 else
	 {
	    goto err;
	 }

	 
      }
      break;
      
      case UNICAP_PROPERTY_TYPE_MENU:
      {
	 tmp = PyDict_GetItemString( obj, "menu_item" );
	 if( !tmp ){
	    PyErr_SetString( PyExc_ValueError, "Object is missing 'menu_item' key" );
	    goto err;
	 }
	 if( !PyString_Check( tmp ) )
	 {
	    goto err;
	 }
	 
	 strcpy( property->menu_item, PyString_AsString( tmp ) );
      }
      break;

   case UNICAP_PROPERTY_TYPE_DATA:
   {
      tmp = PyDict_GetItemString( obj, "data" );
      if( !tmp ){
	 PyErr_SetString( PyExc_ValueError, "Object is missing 'data' key" );
	 goto err;
      }
      if( !PyString_Check( tmp ) ){
	 goto err;
      }

      Py_ssize_t size;
      PyString_AsStringAndSize( tmp, (char**)&property->property_data, &size );
      property->property_data_size = size;
   }
   break;

   default:
      break;
   }

      

   return 0;

  err:
   return -1;
}

int conv_device_identifier( PyObject *object, char **target )
{
   char *identifier = NULL;
   if( PyString_Check( object ) ){
      identifier = PyString_AsString( object );
   } else if( PyDict_Check( object ) ){
      PyObject *strobj;
      strobj = PyDict_GetItemString( object, "identifier" );
      if( strobj ){
	 identifier = PyString_AsString( strobj );
      }
   } else if( UnicapDevice_Check( object ) ){
      PyObject *strobj;
      strobj = PyObject_GetAttrString( object, "identifier" );
      if( strobj ){
	 identifier = PyString_AsString( strobj );
      }
   } else {
      PyErr_SetString( PyExc_TypeError, "expected string or Dict object" );
   }
   
   *target = identifier;
   return identifier != NULL;
}

