/* unicap
 *
 * Copyright (C) 2004 Arne Caspari ( arne_caspari@users.sourceforge.net )
 *
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

#include "config.h"

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <string.h>
#include <X11/X.h>
#include <X11/Xlib.h>

#include "backend.h"
#include "backend_gtk.h"
#include "ucil.h"


#if UNICAPGTK_DEBUG
#define DEBUG
#endif
#include <debug.h>

#include <sys/time.h>
#include <time.h>
#include <semaphore.h>

#define NUM_BUFFERS 2


struct backend_data
{
      GtkWidget *widget;
      guchar *image_data[ NUM_BUFFERS ];
      struct timeval fill_times[ NUM_BUFFERS ];
      gint current_buffer;
      unicap_format_t format;
      GdkPixbuf *pixbuf;

      gint crop_x;
      gint crop_y;
      gint crop_w;
      gint crop_h;

      Display *display;
      GdkWindow *overlay_window;
      GdkGC *overlay_gc;
      guint exposure_timer;

      gboolean scale_to_fit;
      gint output_width;
      gint output_height;
      
      gboolean pause_state;
      
      unicapgtk_color_conversion_callback_t color_conversion_cb;
      void                                 *color_conversion_data;

      sem_t sema;
};

gint backend_gtk_get_merit()
{
   return BACKEND_MERIT_GTK;
}

gboolean backend_gtk_init( GtkWidget *widget, unicap_format_t *format, gpointer *_data, GError **err )
{
   struct backend_data *data;
   int i;
   GdkGCValues values;
   GdkColormap *colormap;
   
   data = g_new0( struct backend_data, 1 );

   g_memmove( &data->format, format, sizeof( unicap_format_t ) );

   for( i = 0; i < NUM_BUFFERS; i++ )
   {
      data->image_data[ i ] = g_malloc0( format->size.width * format->size.height * 3 );
      g_assert( data->image_data );
   }

   data->crop_x = 0;
   data->crop_y = 0;
   data->crop_w = format->size.width;
   data->crop_h = format->size.height;
   data->output_width = format->size.width;
   data->output_height = format->size.height;
   data->pixbuf = NULL;
 
   data->widget = widget;
   data->display = GDK_DISPLAY_XDISPLAY(gtk_widget_get_display( widget ) );

   *_data = data;
   
   sem_init( &data->sema, 0, 1 );

   data->overlay_window = widget->window;

   values.foreground.red = 0;
   values.foreground.green = 0;
   values.foreground.blue = 0;

   colormap = gtk_widget_get_colormap( widget );
   gdk_rgb_find_color( colormap, &values.foreground );

   data->overlay_gc = gdk_gc_new_with_values( data->overlay_window, 
					      &values, GDK_GC_FOREGROUND );

   gdk_window_show( data->overlay_window );


   return TRUE;
}

void backend_gtk_destroy( gpointer _data )
{
   struct backend_data *data = _data;
   int i;
   
   backend_gtk_lock( data );

   g_object_unref( data->overlay_gc );

   if( data->pixbuf )
   {
      g_object_unref( data->pixbuf );
   }
   
   for( i = 0; i < NUM_BUFFERS; i++ )
   {
      g_free( data->image_data[ i ] );
   }

   backend_gtk_unlock( data );

   g_free( data );
}


void backend_gtk_update_image( gpointer _data, unicap_data_buffer_t *data_buffer, GError **err )
{
   unicap_data_buffer_t tmp_buffer;
   struct backend_data *data = _data;
   int tmp;

   if( !data_buffer )
   {
      g_warning( "update_image: data_buffer == NULL!\n" );
      return;
   }

   if( /* ( data_buffer->format.fourcc != data->format.fourcc ) ||  */
       ( data_buffer->format.size.width != data->format.size.width ) || 
       ( data_buffer->format.size.height != data->format.size.height ) /* || */
/*        ( data_buffer->format.bpp != data->format.bpp  )*/ )
   {
      g_warning( "update_image: data_buffer format missmatch %dx%d <> %dx%d\n", 
		 data_buffer->format.size.width, data_buffer->format.size.height, 
		 data->format.size.width, data->format.size.height );      
      return;
   }

   unicap_copy_format( &tmp_buffer.format, &data_buffer->format );
   tmp_buffer.format.bpp = 24;
   tmp_buffer.format.fourcc = UCIL_FOURCC( 'R', 'G', 'B', '3' );

   tmp = ( data->current_buffer + 1 ) % NUM_BUFFERS;   
   memcpy( &data->fill_times[tmp], &data_buffer->fill_time, sizeof( struct timeval ) );
   tmp_buffer.data = data->image_data[tmp];
   tmp_buffer.buffer_size = data->format.size.width * data->format.size.height * 3;
   if( !data->color_conversion_cb )
   {
      ucil_convert_buffer( &tmp_buffer, data_buffer );
   }
   else
   {
      if( !data->color_conversion_cb( &tmp_buffer, data_buffer, data->color_conversion_data ) )
      {
	 ucil_convert_buffer( &tmp_buffer, data_buffer );
      } 
   }
}

static void scale_image( gpointer _data )
{
   struct backend_data *data = _data;
   GdkPixbuf *imgpixbuf;

   if( data->pixbuf )
   {
      g_object_unref( data->pixbuf );
      data->pixbuf = NULL;
   }
   
   imgpixbuf = gdk_pixbuf_new_from_data( data->image_data[ data->current_buffer ] + ( data->crop_y * data->format.size.width * 3 + data->crop_x * 3 ), 
					 GDK_COLORSPACE_RGB, 
					 0, 
					 8, 
					 data->crop_w, 
					 data->crop_h, 
					 data->format.size.width * 3, 
					 NULL, 
					 NULL );
   data->pixbuf = gdk_pixbuf_scale_simple( imgpixbuf, 
				    data->output_width, 
				    data->output_height, 
				    GDK_INTERP_TILES );
}


void backend_gtk_display_image( gpointer _data )
{
   struct backend_data *data = _data;

   data->current_buffer = ( data->current_buffer + 1 ) % NUM_BUFFERS;
   
   if( data->scale_to_fit )
   {
      if( ( data->format.size.width != data->output_width ) &&
	  ( data->format.size.height != data->output_height ) )
      {
	 if( data->image_data[ data->current_buffer ] )
	 {
	    scale_image( _data );
	 }
      }
   }

   backend_gtk_redraw( _data );
}

void backend_gtk_size_allocate( gpointer _data, GtkWidget *widget, GtkAllocation *allocation )
{
   struct backend_data *data = _data;

   if( data->scale_to_fit )
   {
      data->output_width = allocation->width;
      data->output_height = allocation->height;
      if( data->pause_state )
      {
	 scale_image( _data );
	 backend_gtk_redraw( _data );
      }
   }
}

void backend_gtk_set_crop( gpointer _data, int crop_x, int crop_y, int crop_w, int crop_h )
{
   struct backend_data *data = _data;
   
   data->crop_x = crop_x;
   data->crop_y = crop_y;
   data->crop_w = crop_w;
   data->crop_h = crop_h;
}

void backend_gtk_redraw( gpointer _data )
{
   struct backend_data *data = _data;
   
   

   if( ( data->crop_w == data->output_width ) &&
       ( data->crop_h == data->output_height ) )
   {
      if( data->image_data[ data->current_buffer ] )
      {
	 gdk_draw_rgb_image( data->overlay_window, 
			     data->overlay_gc, 
			     0,0,  
			     data->crop_w, data->crop_h, 
			     GDK_RGB_DITHER_NONE, 
			     data->image_data[ data->current_buffer ] + ( data->crop_y * data->format.size.width * 3 + data->crop_x * 3 ), 
			     data->format.size.width * 3 );      
      }
   }
   else
   {
      if( data->pixbuf )
      {
	 gdk_draw_pixbuf( data->overlay_window, 
			  data->overlay_gc, 
			  data->pixbuf, 
			  0, 0,
			  0, 0, 
			  -1,-1,
			  GDK_RGB_DITHER_NONE, 
			  0, 0 );
      }
   }  
}

void backend_gtk_expose_event( gpointer _data, GtkWidget *da, GdkEventExpose *event )
{
   backend_gtk_redraw( _data );
   
}

void backend_gtk_get_image_data( gpointer _data, unicap_data_buffer_t *data_buffer, int b )
{
   struct backend_data *data = _data;
   int tmp;

   unicap_void_format( &data_buffer->format );
   data_buffer->format.fourcc = UCIL_FOURCC( 'R', 'G', 'B', '3' );
   data_buffer->format.bpp = 24;
   data_buffer->format.size.width = data->format.size.width;
   data_buffer->format.size.height = data->format.size.height;
   data_buffer->format.buffer_size = data->format.size.width * data->format.size.height * 3;
   data_buffer->buffer_size = data_buffer->format.buffer_size;
   tmp = ( data->current_buffer + 1 ) % NUM_BUFFERS;
   data_buffer->data = data->image_data[ tmp ];
   memcpy( &data_buffer->fill_time, &data->fill_times[tmp], sizeof( struct timeval ) );   
}

void backend_gtk_lock( gpointer _data )
{
   struct backend_data *data = _data;
   sem_wait( &data->sema );
}

void backend_gtk_unlock( gpointer _data )
{
   struct backend_data *data = _data;
   sem_post( &data->sema );
}

void backend_gtk_set_scale_to_fit( gpointer _data, gboolean scale_to_fit )
{
   struct backend_data *data = _data;

   data->scale_to_fit = scale_to_fit;
   if( !scale_to_fit )
   {
      data->output_width = data->format.size.width;
      data->output_height = data->format.size.height;
      if( data->pixbuf )
      {
	 g_object_unref( data->pixbuf );
	 data->pixbuf = NULL;
      }
   }
}

guint backend_gtk_get_flags( gpointer _data )
{
   return BACKEND_FLAGS_SCALING_SUPPORTED;
}

void backend_gtk_set_pause_state( gpointer _data, gboolean state )
{
   struct backend_data *data = _data;

   data->pause_state = state;
}

   
void backend_gtk_set_color_conversion_callback( void *_backend_data, 
					       unicapgtk_color_conversion_callback_t cb,
					       void *data )
{
   struct backend_data *backend_data = _backend_data;
   backend_data->color_conversion_cb = cb;
   backend_data->color_conversion_data = data;
}
