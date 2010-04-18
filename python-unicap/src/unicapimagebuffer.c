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
#include <ucil.h>

#include "unicapmodule.h"
#include "unicapimagebuffer.h"
#include "utils.h"

PyTypeObject UnicapImageBufferType;


struct fourcc_bpp_map
{
      unsigned int fourcc;
      int bpp;
};

struct fourcc_bpp_map fourcc_bpp_map[] =
{
   { UCIL_FOURCC('B','A','8','1'),  8 },
   { UCIL_FOURCC('B','G','R','3'), 24 },
   { UCIL_FOURCC('B','Y','8',' '),  8 },
   { UCIL_FOURCC('G','R','E','Y'),  8 },
   { UCIL_FOURCC('I','4','2','0'), 12 },
   { UCIL_FOURCC('R','A','W',' '),  8 },
   { UCIL_FOURCC('R','G','B', 0 ),  8 },
   { UCIL_FOURCC('R','G','B','3'), 24 },
   { UCIL_FOURCC('R','G','B','4'), 32 },
   { UCIL_FOURCC('B','G','R','4'), 32 },
   { UCIL_FOURCC('R','G','B','A'), 32 },
   { UCIL_FOURCC('R','G','G','B'), 16 }, 
   { UCIL_FOURCC('U','Y','V','Y'), 16 },
   { UCIL_FOURCC('Y','4','1','1'), 12 },
   { UCIL_FOURCC('Y','8','0','0'),  8 },
   { UCIL_FOURCC('Y','U','Y','2'), 16 },
   { UCIL_FOURCC('Y','U','Y','V'), 16 },
   { 0, -1 }
};
   

static int _pyunicap_ucil_get_bpp_from_fourcc( unsigned int fourcc )
{
   int i;
   int bpp = -1;
   

   for( i = 0; fourcc_bpp_map[i].bpp != -1; i++ )
   {
      if( fourcc == fourcc_bpp_map[i].fourcc )
      {
	 bpp = fourcc_bpp_map[i].bpp;
	 break;
      }
   }
   
   return bpp;
}


static void UnicapImageBuffer_dealloc( UnicapImageBuffer *self )
{
   Py_XDECREF( self->format );
   if( self->free_data ){
      free( self->buffer.data );
   }
   if( self->fobj )
      ucil_destroy_font_object( self->fobj );
   self->ob_type->tp_free( (PyObject*) self );
}

static PyObject *UnicapImageBuffer_new( PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
   static char* kwlist[] = { "format", NULL };
   UnicapImageBuffer *self;
   int width = -1;
   int height = -1;
   char *strfourcc = NULL;

   if( !PyArg_ParseTupleAndKeywords( args, kwargs, 
				     "|(iis)", kwlist, 
				     &width, &height, &strfourcc  ) ){
      return NULL;
   }

   self = (UnicapImageBuffer *)type->tp_alloc( type, 0 );
   if( self != NULL ){
      unicap_void_format( &self->buffer.format );
      
      if ((width!=-1) && (height!=-1) && (strfourcc!=NULL)){
	 self->buffer.format.fourcc = ((unsigned int)strfourcc[0]) | (((unsigned int)strfourcc[1])<<8) | (((unsigned int)strfourcc[2])<<16) | (((unsigned int)strfourcc[3])<<24);
	 self->buffer.format.bpp = _pyunicap_ucil_get_bpp_from_fourcc (self->buffer.format.fourcc);
	 if ( self->buffer.format.bpp == -1 ){
	    PyErr_SetString( PyExc_ValueError, "Unknown fourcc" );
	    return NULL;
	 }
	    
	 self->buffer.format.size.width = width;
	 self->buffer.format.size.height = height;
	 
	 
	 self->format = build_video_format( &self->buffer.format );
	 /* unicap_copy_format( &self->buffer.format, &format ); */
	 /* self->buffer.buffer_size = data_buffer->buffer_size; */
	 self->buffer.data = malloc( self->buffer.buffer_size );
	 /* memcpy( self->buffer.data, data_buffer->data, self->buffer.buffer_size ); */
	 self->fobj = NULL;
	 self->free_data = TRUE;
      } else {
	 self->format = PyDict_New();
	 if( !self->format ){
	    Py_XDECREF( self );
	    return NULL;
	 }
	 self->fobj = NULL;
	 self->free_data = TRUE;
      }
   }
   
   return (PyObject*) self;
}



static const char wrap_gpointer__doc__[] = "\
wrap_gpointer(ptr)\n\
Creates a new ImageBuffer instance from a gpointer.\n\
The gpointer should point to a C struct of (unicap_data_buffer_t*) type. \n\
This function is used to create an ImageBuffer instance from the gpointer that is passed to the signal handlers eg. of a unicapgtk.VideoDisplay widget\n\
The image data is not copied and no data buffer memory is allocated by this method so this ImageBuffer may only be used as long as \n\
the underlying unicap_data_buffer is valid ( ie. only inside the signal handler function ). \n\
\n\
ptr: a gpointer object pointing to a unicap_data_buffer_t *";
static PyObject *UnicapImageBuffer_wrap_gpointer( PyTypeObject *type, PyObject *args, PyObject *kwargs)
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
      
   unicap_data_buffer_t *data_buffer = pyg_pointer_get( gpointer, unicap_data_buffer_t );
   self = (UnicapImageBuffer *)type->tp_alloc( &UnicapImageBufferType, 0 );
   if( self != NULL ){
      memcpy( &self->buffer, data_buffer, sizeof( unicap_data_buffer_t ) );
      self->format = build_video_format( &data_buffer->format );
      self->time = ((double)data_buffer->fill_time.tv_sec) + ((double)data_buffer->fill_time.tv_usec / 1000000.0 );
      self->free_data = FALSE;
   }
      
   Py_XDECREF( gobject_module );
   
   return (PyObject*) self;
}

PyObject *UnicapImageBuffer_new_from_buffer_no_copy( const unicap_data_buffer_t *data_buffer )
{
   UnicapImageBuffer *self;
   self = (UnicapImageBuffer *)UnicapImageBufferType.tp_alloc( (PyTypeObject*)&UnicapImageBufferType, 0 );
   if( self != NULL )
   {
      self->format = build_video_format( &data_buffer->format );
      self->time = ((double)data_buffer->fill_time.tv_sec) + ((double)data_buffer->fill_time.tv_usec / 1000000.0 );
      unicap_copy_format( &self->buffer.format, &data_buffer->format );
      self->buffer.buffer_size = data_buffer->buffer_size;
      self->buffer.data = data_buffer->data;
      self->fobj = NULL;
      self->free_data = FALSE;
   }

   return (PyObject *)self;
}

PyObject *UnicapImageBuffer_new_from_buffer( const unicap_data_buffer_t *data_buffer )
{
   UnicapImageBuffer *self;
   self = (UnicapImageBuffer *)UnicapImageBufferType.tp_alloc( (PyTypeObject*)&UnicapImageBufferType, 0 );
   if( self != NULL )
   {
      self->format = build_video_format( &data_buffer->format );
      self->time = ((double)data_buffer->fill_time.tv_sec) + ((double)data_buffer->fill_time.tv_usec / 1000000.0 );
      unicap_copy_format( &self->buffer.format, &data_buffer->format );
      self->buffer.buffer_size = data_buffer->buffer_size;
      self->buffer.data = malloc( self->buffer.buffer_size );
      memcpy( self->buffer.data, data_buffer->data, self->buffer.buffer_size );
      self->fobj = NULL;
      self->free_data = TRUE;
   }

   return (PyObject *)self;
}

static const char load_file__doc__[] = " \
load_file(path)\n\
\n\
Returns: A new allocated image buffer\
";
static PyObject *UnicapImageBuffer_load_file(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
   static char* kwlist[] = { "path", NULL };
   char *path = NULL;
   
   if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				    "s:unicapgtk.ImageBuffer.weave",
				    kwlist, &path ))
      return NULL;
   
   unicap_data_buffer_t *destbuf;
   PyObject *obj;
   
   destbuf = ucil_read_file_alloc( path );
   obj = UnicapImageBuffer_new_from_buffer( destbuf );
   free( destbuf->data );
   free( destbuf );
   
   return (PyObject*)obj;
}

static const char weave__doc__[] = " \
weave(odd,even)\n\
\n\
Returns: A new allocated image buffer\
";
static PyObject *UnicapImageBuffer_weave(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
   static char* kwlist[] = { "odd", "even", NULL };
   PyObject *_odd;
   PyObject *_even;
   UnicapImageBuffer *odd;
   UnicapImageBuffer *even;
   PyObject *copy;
   
   if (!PyArg_ParseTupleAndKeywords(args, kwargs,
				    "OO:unicapgtk.ImageBuffer.weave",
				    kwlist, &_odd, &_even ))
      return NULL;
   
   if (!PyObject_TypeCheck( _odd, &UnicapImageBufferType )){
      PyErr_SetString( PyExc_ValueError, "'odd' must be a unicap.ImageBuffer" );
      return NULL;
   }
   if (!PyObject_TypeCheck( _even, &UnicapImageBufferType )){
      PyErr_SetString( PyExc_ValueError, "'even' must be a unicap.ImageBuffer" );
      return NULL;
   }
      
   odd = (UnicapImageBuffer*)_odd;
   even = (UnicapImageBuffer*)_even;
   
   unicap_data_buffer_t *destbuf;
   
   destbuf = ucil_weave_alloc( &odd->buffer, &even->buffer );
   copy = UnicapImageBuffer_new_from_buffer( destbuf );
   free( destbuf->data );
   free( destbuf );
   
   return (PyObject*)copy;
}

static const char copy__doc__[] = " \
copy()\n\
Creates a copy of the image buffer.\n\
\n\
Returns: A new allocated image buffer \
";

static PyObject *UnicapImageBuffer_copy( UnicapImageBuffer *self, PyObject *args, PyObject *kwds )
{
   UnicapImageBuffer *copy;

   copy = (UnicapImageBuffer *)UnicapImageBufferType.tp_alloc( (PyTypeObject*)&UnicapImageBufferType, 0 );
   if( copy != NULL ){
      copy->format = PyDict_Copy(self->format);
      if( !copy->format ){
	 Py_XDECREF( copy );
	 return NULL;
      }
      copy->time = self->time;
      unicap_copy_format( &copy->buffer.format, &self->buffer.format );
      copy->buffer.buffer_size = self->buffer.buffer_size;
      copy->buffer.data = malloc( self->buffer.buffer_size );
      if( !copy->buffer.data ){
	 Py_XDECREF( copy->format );
	 Py_XDECREF( copy );
	 return NULL;
      }
      memcpy( copy->buffer.data, self->buffer.data, self->buffer.buffer_size );
      memcpy( &copy->buffer.fill_time, &self->buffer.fill_time, sizeof( struct timeval ) );
      memcpy( &copy->buffer.duration, &self->buffer.duration, sizeof( struct timeval ) );
      memcpy( &copy->buffer.capture_start_time, &self->buffer.capture_start_time, sizeof( struct timeval ) );
      copy->buffer.type = self->buffer.type;
      copy->buffer.flags = self->buffer.flags;
      
      copy->fobj = NULL;
      copy->free_data = TRUE;
   }
   
   return (PyObject*) copy;
   
}





/* static int UnicapImageBuffer_init( UnicapImageBuffer *self, PyObject *args, PyObject *kwds ) */
/* { */
/*    static char *kwlist[] = { "format", NULL }; */
/*    PyObject *tmp; */
   
/*    if( !PyArg_ParseTupleAndKeywords( args, kwds, "|O", kwlist, &tmp ) ) */
/*       return -1; */

/*    if( tmp ) */
/*    { */
/*       Py_XDECREF( self->format ); */
/*       self->format = PyDict_Copy( tmp ); */
/*    } */
   
/*    return 0; */
/* } */

static PyObject *UnicapImageBuffer_repr( UnicapImageBuffer *self )
{
   PyObject *ret = NULL;

   if( self->format )
   {
      PyObject *format_repr;
      
      format_repr = PyObject_Repr( self->format );
      ret =  PyString_FromFormat( "<unicap.ImageBuffer object at %p: format = %s, buffer at %p>", 
      self, PyString_AsString( format_repr ), &self->buffer );
      Py_DECREF( format_repr );
   }
   else
   {
      ret = PyString_FromFormat( "<unicap.ImageBuffer object at %p>", self );
   }
   return ret;
}


static PyMemberDef UnicapImageBuffer_members[] = 
{
   { "format", T_OBJECT_EX, offsetof( UnicapImageBuffer, format ), 0, "Format" },
   { "time", T_DOUBLE, offsetof( UnicapImageBuffer, time ), 0, "time" },
/*    { "buffer", T_STRING, offsetof( UnicapImageBuffer, buffer ) + offsetof( unicap_data_buffer_t, data ), 0, "Buffer" }, */
   { NULL }
};

static const char convert__doc__[] = "\
convert(fourcc)\n\
Converts the buffer to a different color format.\n\
\n\
fourcc: A four character string describing the new color format ( eg. 'UYVY' )";
static PyObject *UnicapImageBuffer_convert( UnicapImageBuffer *self, PyObject *args, PyObject *kwds )
{
   PyObject *ret = NULL;
   char *str;
   unicap_data_buffer_t dest_buffer;

   static char *kwlist[] = { "fourcc", NULL };
   
   if( !PyArg_ParseTupleAndKeywords( args, kwds, "s", kwlist, &str ) )
      return NULL;
   
   unicap_copy_format( &dest_buffer.format, &self->buffer.format );
   if( str )
      dest_buffer.format.fourcc = ((unsigned int)str[0]) | (((unsigned int)str[1])<<8) | (((unsigned int)str[2])<<16) | (((unsigned int)str[3])<<24);
   else
      return NULL;
   
   dest_buffer.format.bpp = _pyunicap_ucil_get_bpp_from_fourcc( dest_buffer.format.fourcc );
   if( dest_buffer.format.bpp == -1 ){
      PyErr_SetString( PyExc_RuntimeError, "Could not convert to target format" );
      return NULL;
   }
   
   dest_buffer.format.buffer_size = dest_buffer.buffer_size = dest_buffer.format.size.width * dest_buffer.format.size.height * dest_buffer.format.bpp / 8;
   dest_buffer.data = malloc( dest_buffer.buffer_size );
   
   if( !SUCCESS( ucil_convert_buffer( &dest_buffer, &self->buffer ) ) ){
      PyErr_SetString( PyExc_RuntimeError, "Could not convert to target format" );
      ret =  NULL;
      goto out;
   }
   
   ret = UnicapImageBuffer_new_from_buffer( &dest_buffer );
   if( !ret ){
      PyErr_SetString( PyExc_RuntimeError, "Failed to allocate new buffer" );
      ret = NULL; 
      goto out;
   }
   
  out:
   free( dest_buffer.data );
   return ret;
}


static const char tostring__doc__[] = "Returns a binary string representation of image data";
static PyObject *UnicapImageBuffer_tostring( UnicapImageBuffer *self, PyObject *args, PyObject *kwds )
{
   PyObject *str;
   
   str = PyString_FromStringAndSize( (char *)self->buffer.data, self->buffer.buffer_size );
   
   return str;
}

static const char draw_line__doc__[] = "\
draw_line(start,end,color,alpha=255)\n\
Draws a line between two points.\n\
\n\
start: (x,y) tuple where the line starts\n\
end: (x,y) tuple where the line ends\n\
color: (r,g,b) tuple for the color of the line ( with r,b,g in {0..255} )\n\
alpha: Opacity value ( 0 == fully transparent, 255 = opaque )\
";
static PyObject *UnicapImageBuffer_draw_line( UnicapImageBuffer *self, PyObject *args, PyObject *kwds )
{
   static char *kwlist[] = { "start", "end", "color", "alpha", NULL };
   ucil_color_t scolor, dcolor;
   int r,g,b;
   int a = 255;
   int sx,sy;
   int ex,ey;
   
   
   if( !PyArg_ParseTupleAndKeywords( args, kwds, 
				     "(ii)(ii)(iii)|i", kwlist, 
				     &sx, &sy, &ex, &ey, &r, &g, &b, &a  ) ){
      return NULL;
   }

   scolor.rgb32.r = r;
   scolor.rgb32.g = g;
   scolor.rgb32.b = b;
   scolor.rgb32.a = a;
   if( a == 255 )
      scolor.colorspace = UCIL_COLORSPACE_RGB24;
   else
      scolor.colorspace = UCIL_COLORSPACE_RGB32;
   
   dcolor.colorspace = ucil_get_colorspace_from_fourcc( self->buffer.format.fourcc );
   ucil_convert_color( &dcolor, &scolor );
   
   ucil_draw_line( &self->buffer, &dcolor, sx, sy, ex, ey );

   Py_INCREF( Py_None );
   return( Py_None );
}


static const char draw_rect__doc__[] = "\
draw_rect(pos1,pos2,color,fill=False,alpha=255)\n\
Draws a rectangle.\n\
\n\
pos1: (x,y) tuple containing the first corner\n\
pos2: (x,y) tuple containing the second corner\n\
color: (r,g,b) tuple for the color of the line ( with r,b,g in {0..255} )\n\
fill: when True, fill the rectangle with color\n\
alpha: Opacity value ( 0 == fully transparent, 255 = opaque )\
";
static PyObject *UnicapImageBuffer_draw_rect( UnicapImageBuffer *self, PyObject *args, PyObject *kwds )
{
   static char *kwlist[] = { "pos1", "pos2", "color", "fill", "alpha", NULL };
   ucil_color_t scolor, dcolor;
   int r,g,b;
   int a = 255;
   int sx,sy;
   int ex,ey;
   int fill = FALSE;
   
   
   if( !PyArg_ParseTupleAndKeywords( args, kwds, 
				     "(ii)(ii)(iii)|ii", kwlist, 
				     &sx, &sy, &ex, &ey, &r, &g, &b, &a, &fill  ) )
      return NULL;

   scolor.rgb32.r = r;
   scolor.rgb32.g = g;
   scolor.rgb32.b = b;
   scolor.rgb32.a = a;
   if( a == 255 )
      scolor.colorspace = UCIL_COLORSPACE_RGB24;
   else
      scolor.colorspace = UCIL_COLORSPACE_RGB32;
   
   dcolor.colorspace = ucil_get_colorspace_from_fourcc( self->buffer.format.fourcc );
   ucil_convert_color( &dcolor, &scolor );
   
   if( fill )
      ucil_draw_rect( &self->buffer, &dcolor, sx, sy, ex, ey );
   else
      ucil_draw_box( &self->buffer, &dcolor, sx, sy, ex, ey );

   Py_INCREF( Py_None );
   return( Py_None );
}

static const char draw_circle__doc__[] = "\
draw_circle(pos,size,color,alpha=255)\n\
Draws a circle.\n\
\n\
pos: (x,y) tuple containing the center position\n\
size: Diameter in pixels\n\
color: (r,g,b) tuple for the color of the circle ( with r,b,g in {0..255} )\n\
alpha: Opacity value ( 0 == fully transparent, 255 = opaque )\
";
static PyObject *UnicapImageBuffer_draw_circle( UnicapImageBuffer *self, PyObject *args, PyObject *kwds )
{
   static char *kwlist[] = { "pos", "size", "color", "alpha", NULL };
   ucil_color_t scolor, dcolor;
   int r,g,b;
   int a = 255;
   int x,y,size;
   
   
   if( !PyArg_ParseTupleAndKeywords( args, kwds, 
				     "(ii)i(iii)|i", kwlist, 
				     &x, &y, &size, &r, &g, &b, &a ) ){
      return NULL;
   }

   scolor.rgb32.r = r;
   scolor.rgb32.g = g;
   scolor.rgb32.b = b;
   scolor.rgb32.a = a;
   if( a == 255 )
      scolor.colorspace = UCIL_COLORSPACE_RGB24;
   else
      scolor.colorspace = UCIL_COLORSPACE_RGB32;
   
   dcolor.colorspace = ucil_get_colorspace_from_fourcc( self->buffer.format.fourcc );
   ucil_convert_color( &dcolor, &scolor );
   
   ucil_draw_circle( &self->buffer, &dcolor, x, y, size );

   Py_INCREF( Py_None );
   return( Py_None );
}

static const char set_font__doc__[]="\
set_font(size,font=None)\n\
Sets the font for text drawing operations.\n\
\n\
size: size of the font in pixels\n\
font: font name or None for default font\
";
static PyObject *UnicapImageBuffer_set_font( UnicapImageBuffer *self, PyObject *args, PyObject *kwds )
{
   static char *kwlist[] = { "size", "font", NULL };
   const char *font = NULL;
   int size;
   
   if( !PyArg_ParseTupleAndKeywords( args, kwds, "i|s", kwlist, &size, &font ) )
      return NULL;
   
   if( self->fobj )
      ucil_destroy_font_object( self->fobj );

   self->fobj = ucil_create_font_object( size, font );
   
   if( !self->fobj ){
      PyErr_SetString( PyExc_RuntimeError, "Failed to create font object" );
      return NULL;
   }
   
   Py_INCREF( Py_None );
   return( Py_None );
}

static const char draw_text__doc__[] = "\
draw_text(pos,text,color,alpha=255)\n\
Draws a text string.\n\
\n\
pos: (x,y) tuple containing the position\n\
text: A string to draw\n\
color: (r,g,b) tuple for the color of the text ( with r,b,g in {0..255} )\n\
alpha: Opacity value ( 0 == fully transparent, 255 = opaque )\
";
static PyObject *UnicapImageBuffer_draw_text( UnicapImageBuffer *self, PyObject *args, PyObject *kwds )
{
   static char *kwlist[] = { "pos", "text", "color", "alpha", NULL };
   ucil_color_t scolor, dcolor;
   int r,g,b;
   int a = 255;
   int x,y;
   char *text = NULL;
   
   
   if( !PyArg_ParseTupleAndKeywords( args, kwds, 
				     "(ii)s(iii)|i", kwlist, 
				     &x, &y, &text, &r, &g, &b, &a ) ){
      return NULL;
   }

   scolor.rgb32.r = r;
   scolor.rgb32.g = g;
   scolor.rgb32.b = b;
   scolor.rgb32.a = a;
   if( a == 255 )
      scolor.colorspace = UCIL_COLORSPACE_RGB24;
   else
      scolor.colorspace = UCIL_COLORSPACE_RGB32;
   
   dcolor.colorspace = ucil_get_colorspace_from_fourcc( self->buffer.format.fourcc );
   ucil_convert_color( &dcolor, &scolor );

   if( !self->fobj )
      self->fobj = ucil_create_font_object( 12, NULL );

   ucil_draw_text( &self->buffer, &dcolor, self->fobj, text, x, y );

   Py_INCREF( Py_None );
   return( Py_None );
}


static const char copy_region__doc__[] = " \
copy_region(rect)\n\
Creates a new image buffer with a copy of a region.\n\
\n\
rect: A (x,y,width,height)-Tuple describing the region to copy\
\n\
Returns: A new allocated image buffer\
";

static PyObject *UnicapImageBuffer_copy_region( UnicapImageBuffer *self, PyObject *args, PyObject *kwds )
{
   PyObject *copy;
   static char *kwlist[] = { "rect", NULL };
   unicap_rect_t rect;
   unicap_data_buffer_t *destbuf;
   
   if( !PyArg_ParseTupleAndKeywords( args, kwds, 
				     "(iiii)", kwlist, 
				     &rect.x, &rect.y, &rect.width, &rect.height ) ){
      return NULL;
   }

   destbuf = ucil_copy_region_alloc( &self->buffer, &rect );
   copy = UnicapImageBuffer_new_from_buffer( destbuf );
   free( destbuf->data );
   free( destbuf );
   
   return (PyObject*) copy;
   
}

static const char copy_field__doc__[] = " \
copy_field()\n\
Creates a new image buffer with a copy of a field ( odd or even lines ) of this buffer.\n\
\n\
Returns: A new allocated image buffer\
";

static PyObject *UnicapImageBuffer_copy_field( UnicapImageBuffer *self, PyObject *args, PyObject *kwds )
{
   PyObject *copy;
   static char *kwlist[] = { "type", NULL };
   int field_type;
   unicap_data_buffer_t *destbuf;
   
   if( !PyArg_ParseTupleAndKeywords( args, kwds, 
				     "i", kwlist, 
				     &field_type ) ){
      return NULL;
   }

   destbuf = ucil_copy_field_alloc( &self->buffer, field_type );
   copy = UnicapImageBuffer_new_from_buffer( destbuf );
   free( destbuf->data );
   free( destbuf );
   
   return (PyObject*) copy;
   
}

static const char copy_color_plane__doc__[] = " \
copy_color_plane()\n\
Creates a new image buffer with a copy of a color plane from this buffer.\n\
\n\
Returns: A new allocated image buffer\
";

static PyObject *UnicapImageBuffer_copy_color_plane( UnicapImageBuffer *self, PyObject *args, PyObject *kwds )
{
   PyObject *copy;
   static char *kwlist[] = { "plane", NULL };
   int plane;
   unicap_data_buffer_t *destbuf;
   
   if( !PyArg_ParseTupleAndKeywords( args, kwds, 
				     "i", kwlist, 
				     &plane ) ){
      return NULL;
   }

   destbuf = ucil_copy_color_plane_alloc( &self->buffer, plane );
   copy = UnicapImageBuffer_new_from_buffer( destbuf );
   free( destbuf->data );
   free( destbuf );
   
   return (PyObject*) copy;
   
}

static const char save__doc__[] = " \
save(path)\n\
\n\
";

static PyObject *UnicapImageBuffer_save( UnicapImageBuffer *self, PyObject *args, PyObject *kwds )
{
   static char *kwlist[] = { "path", NULL };
   char *path = NULL;
   
   if( !PyArg_ParseTupleAndKeywords( args, kwds, 
				     "s", kwlist, 
				     &path ) ){
      return NULL;
   }

   ucil_write_file( path, NULL, &self->buffer );
   
   Py_INCREF( Py_None );
   return( Py_None );   
}

static const char get_pixel__doc__[] = " \
get_pixel((x,y))\n\
\n\
";
static PyObject *UnicapImageBuffer_get_pixel( UnicapImageBuffer *self, PyObject *args, PyObject *kwds )
{
   static char *kwlist[] = { "pos", NULL };
   int x,y;
   
   if( !PyArg_ParseTupleAndKeywords( args, kwds, 
				     "(ii)", kwlist, 
				     &x, &y ) ){
      return NULL;
   }

   ucil_color_t color;
   
   ucil_get_pixel( &self->buffer, &color, x, y );
   
   PyObject *obj = NULL;
   switch (color.colorspace){
   case UCIL_COLORSPACE_Y8:
      obj = PyTuple_Pack(1, PyInt_FromLong(color.y8.y));
      break;
   case UCIL_COLORSPACE_RGB24:
      obj = PyTuple_Pack(3, PyInt_FromLong(color.rgb24.r), PyInt_FromLong(color.rgb24.g), PyInt_FromLong(color.rgb24.b));
      break;
   case UCIL_COLORSPACE_RGB32:
      obj = PyTuple_Pack(4, PyInt_FromLong(color.rgb32.r), PyInt_FromLong(color.rgb32.g), PyInt_FromLong(color.rgb32.b), PyInt_FromLong(color.rgb32.a));
      break;
      
   case UCIL_COLORSPACE_YUV:
      obj = PyTuple_Pack(3, PyInt_FromLong(color.yuv.y), PyInt_FromLong(color.yuv.u), PyInt_FromLong(color.yuv.v));
      break;
   default:
      break;
   }
   
   return obj;
   
}

static const char set_pixel__doc__[] = " \
set_pixel((x,y), color)\n\
\n\
";
static PyObject *UnicapImageBuffer_set_pixel( UnicapImageBuffer *self, PyObject *args, PyObject *kwds )
{
   static char *kwlist[] = { "pos", "color", NULL };
   int x,y;
   PyObject *colobj;
   
   if( !PyArg_ParseTupleAndKeywords( args, kwds, 
				     "(ii)O", kwlist, 
				     &x, &y, &colobj ) ){
      return NULL;
   }
   
   ucil_color_t color;
   color.colorspace = ucil_get_colorspace_from_fourcc( self->buffer.format.fourcc );
   if( color.colorspace == UCIL_COLORSPACE_UNKNOWN )
      return NULL;
   
   switch( color.colorspace ){
   case UCIL_COLORSPACE_Y8:
      color.y8.y = PyInt_AsLong( PyTuple_GetItem( colobj, 0 ));
      break;
   case UCIL_COLORSPACE_RGB24:
      color.rgb24.r = PyInt_AsLong( PyTuple_GetItem( colobj, 0));
      color.rgb24.g = PyInt_AsLong( PyTuple_GetItem( colobj, 1));
      color.rgb24.b = PyInt_AsLong( PyTuple_GetItem( colobj, 2));
      break;
   case UCIL_COLORSPACE_RGB32:
      color.rgb32.r = PyInt_AsLong( PyTuple_GetItem( colobj, 0));
      color.rgb32.g = PyInt_AsLong( PyTuple_GetItem( colobj, 1));
      color.rgb32.b = PyInt_AsLong( PyTuple_GetItem( colobj, 2));
      color.rgb32.a = PyInt_AsLong( PyTuple_GetItem( colobj, 3));
      break;
   case UCIL_COLORSPACE_YUV:
      color.yuv.y = PyInt_AsLong( PyTuple_GetItem( colobj, 0 ));
      color.yuv.u = PyInt_AsLong( PyTuple_GetItem( colobj, 1 ));
      color.yuv.v = PyInt_AsLong( PyTuple_GetItem( colobj, 2 ));
      break;
   default:
      return NULL;
   }
   
   ucil_set_pixel( &self->buffer, &color, x, y);
   
   Py_INCREF( Py_None );
   return( Py_None );   
}


static PyMethodDef UnicapImageBuffer_methods[] = 
{
   { "wrap_gpointer", (PyCFunction)UnicapImageBuffer_wrap_gpointer, METH_VARARGS | METH_CLASS, wrap_gpointer__doc__ },
   { "load_file", (PyCFunction)UnicapImageBuffer_load_file, METH_VARARGS | METH_CLASS, load_file__doc__ },   
   { "weave", (PyCFunction)UnicapImageBuffer_weave, METH_VARARGS | METH_CLASS, weave__doc__ },
   { "copy", (PyCFunction)UnicapImageBuffer_copy, METH_VARARGS, copy__doc__ },
   { "convert", (PyCFunction)UnicapImageBuffer_convert, METH_VARARGS, convert__doc__ },   
   { "tostring", (PyCFunction)UnicapImageBuffer_tostring, METH_NOARGS, tostring__doc__ },
   { "draw_line", (PyCFunction)UnicapImageBuffer_draw_line, METH_VARARGS, draw_line__doc__ },
   { "draw_rect", (PyCFunction)UnicapImageBuffer_draw_rect, METH_VARARGS, draw_rect__doc__ },
   { "draw_circle", (PyCFunction)UnicapImageBuffer_draw_circle, METH_VARARGS, draw_circle__doc__ },
   { "set_font", (PyCFunction)UnicapImageBuffer_set_font, METH_VARARGS, set_font__doc__ },
   { "draw_text", (PyCFunction)UnicapImageBuffer_draw_text, METH_VARARGS, draw_text__doc__ },
   { "copy_region", (PyCFunction)UnicapImageBuffer_copy_region, METH_VARARGS, copy_region__doc__ },
   { "copy_field", (PyCFunction)UnicapImageBuffer_copy_field, METH_VARARGS, copy_field__doc__ },
   { "copy_color_plane", (PyCFunction)UnicapImageBuffer_copy_color_plane, METH_VARARGS, copy_color_plane__doc__ },
   { "save", (PyCFunction)UnicapImageBuffer_save, METH_VARARGS, save__doc__ },
   { "get_pixel", (PyCFunction)UnicapImageBuffer_get_pixel, METH_VARARGS, get_pixel__doc__ },
   { "set_pixel", (PyCFunction)UnicapImageBuffer_set_pixel, METH_VARARGS, set_pixel__doc__ },
   
   { NULL }
};


PyTypeObject UnicapImageBufferType = 
{
   PyObject_HEAD_INIT(NULL)
   0,                         /*ob_size*/
   "unicap.ImageBuffer",      /*tp_name*/
   sizeof(UnicapImageBuffer), /*tp_basicsize*/
   0,                         /*tp_itemsize*/
   (destructor)UnicapImageBuffer_dealloc, /*tp_dealloc*/
   0,                         /*tp_print*/
   0,                         /*tp_getattr*/
   0,                         /*tp_setattr*/
   0,                         /*tp_compare*/
   (reprfunc)UnicapImageBuffer_repr,    /*tp_repr*/
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
   "UnicapImageBuffer objects",/* tp_doc */
   0,		               /* tp_traverse */
   0,		               /* tp_clear */
   0,		               /* tp_richcompare */
   0,		               /* tp_weaklistoffset */
   0,		               /* tp_iter */
   0,		               /* tp_iternext */
   UnicapImageBuffer_methods, /* tp_methods */
   UnicapImageBuffer_members, /* tp_members */
   0,                         /* tp_getset */
   0,                         /* tp_base */
   0,                         /* tp_dict */
   0,                         /* tp_descr_get */
   0,                         /* tp_descr_set */
   0,                         /* tp_dictoffset */
   0,/* tp_init */
   0,                         /* tp_alloc */
   UnicapImageBuffer_new,     /* tp_new */
};

void initunicapimagebuffer( PyObject *m )
{
   if( PyType_Ready( &UnicapImageBufferType ) < 0 )
   {
      return;
   }
   
   Py_INCREF( &UnicapImageBufferType );
   PyModule_AddObject( m, "ImageBuffer", (PyObject*)&UnicapImageBufferType );
}

