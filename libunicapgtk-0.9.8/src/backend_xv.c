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

#include <stdlib.h>
#include <linux/types.h>
#include <stdlib.h>
#include <linux/types.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/time.h>
#include <time.h>

#include <X11/Xlib.h>
#include <X11/extensions/XShm.h>
#include <X11/extensions/Xvlib.h>
#include <X11/X.h>

#include <sys/ipc.h>
#include <sys/shm.h>

#include "unicap.h"
#include "ucil.h"
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <semaphore.h>


#include <unicapgtk.h>
#include "backend.h"
#include "backend_xv.h"
#if UNICAPGTK_DEBUG
#define DEBUG
#endif
#include <debug.h>

#define MAX_IMAGE_BUFFERS 12
#define MAX_XV_PORTS 64

static int g_used_ports[MAX_XV_PORTS];
static int g_initialized = 0;

static gboolean match_fourcc( unsigned int a, unsigned int b );
static gboolean lookup_fcc( unsigned int fcc, unsigned int *list );


typedef struct _xv_fourcc
{
      unsigned int fourcc;
      char guid[16];
} xv_fourcc_t;

struct xv_backend_data
{
      XvAdaptorInfo *p_adaptor_info;
      unsigned int num_adaptors;
      XvPortID port_id;
		
      unsigned int xv_mode_id;	
		
      XvImage *image[MAX_IMAGE_BUFFERS];
      unicap_data_buffer_t data_buffers[MAX_IMAGE_BUFFERS];
      int use_shm;
      XShmSegmentInfo shminfo[MAX_IMAGE_BUFFERS];
		
      int width;
      int height;
      
      gint output_width;
      gint output_height;
      gint crop_x;
      gint crop_y;
      gint crop_w;
      gint crop_h;

      int window_width;
      int window_height;
		
      Atom atom_colorkey;
      Atom atom_brightness;
      Atom atom_hue;
      Atom atom_contrast;
		
      int brightness_min;
      int brightness_max;
      int hue_min;
      int hue_max;
      int contrast_max;
      int contrast_min;

      unsigned int colorkey;

      volatile int current_buffer;
      volatile int next_display_buffer;
		
      Display *display;
      Drawable drawable;
      GC       gc;
      GdkGC   *gdkgc;

      GtkWidget *widget;
      GdkWindow *overlay_window;
      GC overlay_gc;

      gboolean scale_to_fit;

      gint redisplay_timerid;
      gboolean pause_state;

      sem_t sema;

      int used_port;

      unicap_new_frame_callback_t new_frame_callback;
      void * new_frame_callback_data;
      unicap_handle_t new_frame_callback_handle;

      unicapgtk_color_conversion_callback_t color_conversion_cb;
      void                                 *color_conversion_data;

};


static XvImage *allocate_image( Display *display, 
				XvPortID port_id, 
				unsigned int xv_mode_id,
				XShmSegmentInfo *shminfo, 
				int width, 
				int height, 
				int *use_shm )
{
   XvImage *image = 0;
   if( shminfo )
   {
      image = XvShmCreateImage( display, 
				port_id, 
				xv_mode_id, 
				(char*)NULL, 
				width,
				height, 
				shminfo );
	
      if( image )
      {
	 shminfo->shmid = shmget( IPC_PRIVATE, image->data_size, IPC_CREAT | 0777 );
	 if( shminfo->shmid == -1 )
	 {
	    return 0;
	 }
	 shminfo->shmaddr = image->data = shmat( shminfo->shmid, 0, 0 );
	 shmctl(shminfo->shmid, IPC_RMID, 0);

	 if( shminfo->shmaddr == ( void * ) -1 )
	 {
	    return 0;
	 }
	
	 shminfo->readOnly = False;
	 if( !XShmAttach( display, shminfo ) )
	 {
	    shmdt( shminfo->shmaddr );
	    XFree( image );
	    return 0;
	 }
	
	 *use_shm = 1;
      }
   }
   else
   {
      image = XvCreateImage( display, 
			     port_id, 
			     xv_mode_id, 
			     (char*)NULL, 
			     width,
			     height );
      if( image )
      {
	 g_assert( image->data_size );
	 image->data = malloc( image->data_size );
      }

      *use_shm = 0;
   }
  
   return image;
}


gint backend_xv_get_merit()
{
   return BACKEND_MERIT_XV;
}


static gboolean redraw_timeout( gpointer _data )
{
   backend_xv_redraw( _data );
   return TRUE;
}

static GdkGC *create_gc( GtkWidget *widget, int r, int g, int b )
{
   GdkGCValues values;
   GdkColormap *colormap;
   GdkGC *gc;

   values.foreground.red = ( r << 8 ) | r;
   values.foreground.green = ( g << 8 ) | g ;
   values.foreground.blue = ( b << 8 ) | b;
   values.foreground.pixel = 0;

   colormap = gtk_widget_get_colormap( widget );
   gdk_rgb_find_color( colormap, &values.foreground );

   gc = gdk_gc_new_with_values( widget->window,
				&values, GDK_GC_FOREGROUND );

   return gc;
}


static void set_colorkey( struct xv_backend_data *data,  unsigned int r, unsigned int g, unsigned int b )
{
   unsigned int val = ( ((r & 0xff) << 16) | ((g & 0xff) << 8) | (b & 0xff) );

   if( data->atom_colorkey )
   {
      XvSetPortAttribute( data->display,
			  data->port_id,
			  data->atom_colorkey,
			  val );
      XvGetPortAttribute( data->display,
			  data->port_id, 
			  data->atom_colorkey,
			  (int*)&data->colorkey );
   }
   else
   {
      data->colorkey = val;
   }
}


void backend_xv_size_allocate( gpointer _data, GtkWidget *widget, GtkAllocation *allocation )
{
   struct xv_backend_data *data = _data;

   if( data->scale_to_fit )
   {      
      data->output_width = allocation->width;
      data->output_height = allocation->height;
   }
}

unsigned int yuy2_equiv_fccs[] =
{
   UCIL_FOURCC( 'Y', 'U', 'Y', '2' ), 
   UCIL_FOURCC( 'Y', 'U', 'Y', 'V' ), 
   0
};

unsigned int *equiv_fccs[] =
{
   yuy2_equiv_fccs, 
   NULL
};

static gboolean lookup_fcc( unsigned int fcc, unsigned int *list )
{
   int i;
   
   for( i = 0; list[i]; i++ )
   {
      if( list[i] == fcc )
      {
	 return TRUE;
      }
   }
   
   return FALSE;
}

static gboolean match_fourcc( unsigned int a, unsigned int b )
{
   int i;
   
   if( a == b )
   {
      return TRUE;
   }
   
   for( i = 0; equiv_fccs[i]; i++ )
   {
      if( lookup_fcc( a, equiv_fccs[i] ) && lookup_fcc( b, equiv_fccs[i] ) )
      {
	 return TRUE;
      }
   }
   
   return FALSE;
}



gboolean backend_xv_init( GtkWidget *widget, unicap_format_t *format, guint32 req_fourcc, gpointer *_data, GError **err )  
{
   unsigned int version, release;
   unsigned int request_base, event_base, error_base;
	
   int i;
   Display *dpy;
   struct xv_backend_data *data;
   int use_shm = 1;
   unsigned int target_fourcc = format->fourcc;

   int numPorts;

   GKeyFile *keyfile;
   gchar *keyfile_path;
   gchar *userconf_path;
   gchar *cval;

   unsigned int r,g,b;
   XGCValues values;

   TRACE( "Backend xv: init\n" );

   if( !g_initialized )
   {
      TRACE( "Initialize Xv port list\n" );
      memset( g_used_ports, 0x0, sizeof( g_used_ports ) );
      g_initialized = 1;
   }

   g_assert( GTK_WIDGET_REALIZED( widget ) );   

   data = g_new0( struct xv_backend_data, 1 );
   dpy = GDK_DISPLAY_XDISPLAY(gtk_widget_get_display( widget ) );
   if( XvQueryExtension( dpy, 
			 &version, &release, &request_base, &event_base, &error_base ) != Success )
   {
      TRACE( "XvQueryExtension failed\n" );
      free( data );
      return FALSE;
   }
   if( XvQueryAdaptors( dpy, DefaultRootWindow( dpy ), &data->num_adaptors, &data->p_adaptor_info ) != Success )
   {
      TRACE( "XvQueryAdaptors failed\n" );
      free( data );
      return FALSE;
   }
   if( data->num_adaptors == 0 )
   {
      XFree( data->p_adaptor_info );
      free( data );
      return FALSE;
   }	

   data->port_id = -1;

   userconf_path = unicapgtk_get_user_config_path( );
   keyfile_path = g_build_path( G_DIR_SEPARATOR_S, userconf_path, "unicapgtk.conf", NULL );
   g_free( userconf_path );
/*    if( !g_file_test( keyfile_path, G_FILE_TEST_EXISTS ) ) */
/*    { */
/*       g_free( keyfile_path ); */
/*       keyfile_path = g_build_path( G_DIR_SEPARATOR_S, SYSCONFDIR, "unicap", "unicapgtk.conf", NULL ); */
/*    } */
   
   keyfile = g_key_file_new();
   g_key_file_load_from_file( keyfile, keyfile_path, 0, NULL );
   g_free( keyfile_path );

   if( !req_fourcc )
   {
      cval = g_key_file_get_value( keyfile, "display_backend_xv", "force_fourcc", NULL );
      if( cval )
      {
	 req_fourcc = cval[0] + (cval[1]<<8) + (cval[2]<<16) + (cval[3]<<24);
	 g_free( cval );
      }
   }
   
   if( req_fourcc )
   {
/*       if( ucil_conversion_supported( req_fourcc, target_fourcc ) ) */
/*       { */
      target_fourcc = req_fourcc;
/*       } */
   }

   if( !req_fourcc && !ucil_conversion_supported( target_fourcc, format->fourcc ) )
   {
      g_key_file_free( keyfile );
      TRACE( "No conversion from format->fourcc[%08x] to target_fourcc[%08x]\n", format->fourcc, target_fourcc );
      XFree( data->p_adaptor_info );
      free( data );
      return FALSE;
   }
   
   numPorts = 0;
   for( i = 0; i < data->num_adaptors; i++ )
   {
      numPorts += data->p_adaptor_info[i].num_ports;
   }   

   for( i = 0; ( i < numPorts ) && ( data->port_id == -1 ) ; i++ )
   {

      XvAttribute *at;
      int num_attributes;
      int attribute;

      XvImageFormatValues *xvimage_formats;
      int num_xvimage_formats;
      int f;
      XvPortID tmp_port_id = -1;

      XvEncodingInfo *encodings;
      unsigned int num_encodings;

      tmp_port_id = data->p_adaptor_info[0].base_id + i;
      TRACE( "port id: %08x\n:", (unsigned int )tmp_port_id );  


      XvQueryEncodings( dpy, tmp_port_id, &num_encodings, &encodings );
      if( encodings && num_encodings )
      {
	 int i;
	 int size_match = 0;
	 
	 for( i = 0; i < num_encodings; i++ )
	 {
	    if( !strcmp( encodings[i].name, "XV_IMAGE" ) )
	    {
	       if( ( encodings[i].width >= format->size.width ) && 
		   ( encodings[i].height >= format->size.height ) )
	       {
		  size_match = 1;
		  break;
	       }
	    }
	 }

	 if( !size_match )
	 {
	    // no encoding is big enough for this format
	    continue;
	 }
      }
      
	 
	 
      xvimage_formats = XvListImageFormats( dpy, 
					    tmp_port_id, 
					    &num_xvimage_formats );

      for( f = 0; f < num_xvimage_formats; f++ )
      {
	 char imageName[5] = {0, 0, 0, 0, 0};
	 memcpy(imageName, &(xvimage_formats[f].id), 4);
	 TRACE( "      id: 0x%x", xvimage_formats[f].id);
	 if( isprint( imageName[0]) && isprint(imageName[1]) &&
	     isprint( imageName[2]) && isprint(imageName[3])) 
	 {
	    DBGOUT(" (%s)\n", imageName);
	 } else {
	    DBGOUT("\n");
	 }
	 if( match_fourcc( xvimage_formats[f].id, target_fourcc ) )
	 {
	    data->xv_mode_id = xvimage_formats[f].id;
	    data->port_id = tmp_port_id;
	    TRACE( " using fourcc for display: %08x\n", xvimage_formats[f].id );
	    break;
	 }
      }

      if( !match_fourcc( data->xv_mode_id, target_fourcc ) )
      {
	 for( f = 0; f < num_xvimage_formats; f++ )
	 {
	    if( ucil_conversion_supported( target_fourcc, xvimage_formats[f].id ) )
	    {
	       target_fourcc = data->xv_mode_id = xvimage_formats[f].id;
	       data->port_id = tmp_port_id;
	    }
	 }
      }
      
      XFree( xvimage_formats );

      if( target_fourcc && !match_fourcc( data->xv_mode_id, target_fourcc ) )
      {
	 continue;
      }

      if( !g_used_ports[i] && ( data->port_id != -1 ) )
      {
	 if( XvGrabPort( dpy, tmp_port_id, CurrentTime ) 
	     != Success )
	 {
	    TRACE( "can not grab port: %d\n", (unsigned int)tmp_port_id );
	    data->port_id = -1;
	    continue;
	 }
	 TRACE( "lock port: %d\n", i );
	 g_used_ports[i] = 1;
	 data->used_port = i;
      }
      else
      {
	 TRACE( "port %d is in use!\n", i );
	 data->port_id = -1;
	 continue;
      }
      
      at = XvQueryPortAttributes( dpy, tmp_port_id, &num_attributes );
/*       TRACE( "num_attributes: %d\n", num_attributes ); */
      for( attribute = 0; attribute < num_attributes; attribute++ )
      {
	 
	 if( !strcmp( at[attribute].name, "XV_COLORKEY" ) )
	 {
	    int val;
	    Atom atom;
	    atom = (Atom)XInternAtom( dpy, at[attribute].name, 0 );
	    XvGetPortAttribute( dpy, 
				tmp_port_id, 
				atom, 
				&val );
	    data->colorkey = val;
	    data->atom_colorkey = atom;
	    TRACE( "found xv_colorkey\n" );
	 }
	 
	 if( !strcmp( at[attribute].name, "XV_DOUBLE_BUFFER" ) )
	 {
	    Atom _atom;
	    _atom = XInternAtom( dpy, at[attribute].name, 0 );
	    XvSetPortAttribute( dpy, tmp_port_id, _atom, 1 );
	    TRACE( "Xv: DOUBLE_BUFFER available\n" );
	 }

	 cval = g_key_file_get_value( keyfile, "display_backend_xv", at[attribute].name, NULL );
	 if( cval )
	 {
	    int val;
	    Atom atom;
	    atom = (Atom)XInternAtom( dpy, at[attribute].name, 0 );
	    val = atoi( cval );
	    XvSetPortAttribute( dpy, tmp_port_id, atom, val );
	    TRACE( "Set Attribute for: %s to %d\n", at[attribute].name, val );
	 }
      }
      XFree( at );
   }

   if( data->port_id == -1 )
   {
      TRACE( "No suitable port found\n" );
      XFree( data->p_adaptor_info );
      free( data );
      return FALSE;
   }

   TRACE( "Using port: %d\n", (unsigned int )data->port_id );

   data->display = dpy;
   data->width = format->size.width;
   data->height = format->size.height;
   data->output_width = data->width;
   data->output_height = data->height;
   data->crop_x = 0;
   data->crop_y = 0;
   data->crop_w = data->width;
   data->crop_h = data->height;
   data->widget = widget;

   data->current_buffer = -1;
   
   set_colorkey( data, 0x5, 0x5, 0xfe );

   if( data->atom_colorkey )
   {
      r = ( data->colorkey >> 16 ) & 0xff;
      g = ( data->colorkey >>  8 ) & 0xff;
      b = ( data->colorkey ) & 0xff;
      data->gdkgc = create_gc( widget, r, g, b );
   }
   else
   {
      data->gdkgc = create_gc( widget, 0xff, 0xff, 0 );
   }

   for( i = 0; i < MAX_IMAGE_BUFFERS; i++ )
   {
      memset( &data->shminfo[i], 0, sizeof( XShmSegmentInfo ) );
   
      data->image[i] = allocate_image( data->display, 
				       data->port_id,
				       data->xv_mode_id, 
				       &data->shminfo[i], 
				       data->width, 
				       data->height, 
				       &use_shm );
      memset( &data->data_buffers[i], 0, sizeof( unicap_data_buffer_t ) );
      if( !data->image[i] )
      {
	 g_assert( "Failed to allocate image" == FALSE );
      }
   }

   data->use_shm = use_shm;

   *_data = data;

   XSync( dpy, FALSE );

   g_key_file_free( keyfile );

   sem_init( &data->sema, 0, 1 );

   data->overlay_window = widget->window;

   data->overlay_gc = XCreateGC( data->display,
				 GDK_WINDOW_XWINDOW( data->overlay_window ),
				 0, &values );
   g_assert( data->overlay_gc );

   gtk_widget_set_double_buffered( widget, FALSE );

   gdk_window_show( data->overlay_window );

   return TRUE;
}

void backend_xv_destroy( gpointer _data )
{
   struct xv_backend_data *data = _data;
   
   int i;

   TRACE( "xv_destroy\n" );

   backend_xv_lock( data );

   XSync( data->display, FALSE );

   XFreeGC( data->display, data->overlay_gc );
   if( data->gdkgc )
   {
      g_object_unref( data->gdkgc );
   }
 
   for( i = 0; i < MAX_IMAGE_BUFFERS; i++ )
   {
      shmdt( data->shminfo[i].shmaddr );
      XFree( data->image[i] );
   }

   XvUngrabPort( data->display, data->port_id, CurrentTime );

   XFree( data->p_adaptor_info );

   g_used_ports[data->used_port] = 0;

   backend_xv_unlock( data );

   g_free( data );
}


void backend_xv_set_crop( gpointer _data, int crop_x, int crop_y, int crop_w, int crop_h )
{
   struct xv_backend_data *data = _data;
   
   data->crop_x = crop_x;
   data->crop_y = crop_y;
   data->crop_w = crop_w;
   data->crop_h = crop_h;
}

void backend_xv_redraw( gpointer _data )
{
   struct xv_backend_data *data = _data;

   if( data->atom_colorkey )
   {
      int width, height;
      
      width = data->widget->allocation.width < data->output_width ? data->widget->allocation.width : data->output_width;
      height = data->widget->allocation.height < data->output_height ? data->widget->allocation.height : data->output_height;
      
/*       gdk_draw_rectangle( data->widget->window, */
/* 			  data->gdkgc, */
/* 			  TRUE, */
/* 			  0, 0, */
/* 			  width, height ); */
/*       printf( "draw_rectangle\n" ); */
      
   }


   if( data->current_buffer >= 0 )
   {
      if( data->use_shm )
      {
	 XvShmPutImage( data->display,
			data->port_id,
			GDK_WINDOW_XWINDOW( data->overlay_window ),
			data->overlay_gc,
			data->image[data->current_buffer],
			data->crop_x, data->crop_y, data->crop_w, data->crop_h,
			0, 0, data->output_width, data->output_height,
			1 );
      }
      else
      {
	 
	 XvPutImage( data->display,
		     data->port_id,
		     GDK_WINDOW_XWINDOW( data->overlay_window ),
		     data->overlay_gc,
		     data->image[data->current_buffer],
		     data->crop_x, data->crop_y, data->crop_w, data->crop_h,
		     0, 0, data->output_width, data->output_height );
      }
   }
}

void backend_xv_update_image( gpointer _data, unicap_data_buffer_t *data_buffer, GError **err )
{
   struct xv_backend_data *data = _data;
   int start, tmp, i;
   gboolean nr_unlocked_buffers = 0;

   start = ( data->current_buffer + 1 ) % MAX_IMAGE_BUFFERS;
   // find the first buffer not locked
   for( tmp = start; 
	( tmp != data->current_buffer ) && ( ( data->data_buffers[tmp].flags & UNICAP_FLAGS_BUFFER_LOCKED ) || ( tmp == ( data->current_buffer ) ) ); 
	tmp = ( tmp+1 ) % MAX_IMAGE_BUFFERS )
   {
   }

   if( tmp == MAX_IMAGE_BUFFERS )
   {
      TRACE( "No free buffers! ( Should never get here !!! )\n" );
      return;
   }   
   
   

   unicap_copy_format( &data->data_buffers[tmp].format, &data_buffer->format );
   data->data_buffers[tmp].data = ( unsigned char * )data->image[tmp]->data;
   if( data_buffer->format.fourcc != data->xv_mode_id )
   {
      data->data_buffers[tmp].format.fourcc = data->xv_mode_id;
      if( !data->color_conversion_cb )
      {
	 ucil_convert_buffer( &data->data_buffers[tmp], data_buffer );
      }
      else
      {
	 if( !data->color_conversion_cb( &data->data_buffers[tmp], data_buffer, data->color_conversion_data ) )
	 {
	    ucil_convert_buffer( &data->data_buffers[tmp], data_buffer );
	 }  
      }
   }
   else
   {
      if( !data->color_conversion_cb )
      {
	 g_memmove( data->data_buffers[tmp].data, 
		    data_buffer->data, 
		    data_buffer->buffer_size );
      }
      else
      {
	 if( !data->color_conversion_cb( &data->data_buffers[tmp], data_buffer, data->color_conversion_data ) )
	 {
	    g_memmove( data->data_buffers[tmp].data, 
		       data_buffer->data, 
		       data_buffer->buffer_size );
	 }
      }	 
   }
   data->data_buffers[tmp].format.fourcc = data->xv_mode_id;
   data->data_buffers[tmp].buffer_size = data->data_buffers[tmp].format.buffer_size =
      data_buffer->format.size.width * data_buffer->format.size.height * data->data_buffers[tmp].format.bpp / 8;   
   
   memcpy( &data->data_buffers[tmp].fill_time, &data_buffer->fill_time, sizeof( struct timeval) );

/*    printf( "tmp: %d current: %d  fourcc: %08x   bpp: %d\n", tmp, data->current_buffer, data->data_buffers[tmp].format.fourcc,  */
/* 	   data->data_buffers[tmp].format.bpp); */

   for( i = 0; i < MAX_IMAGE_BUFFERS; i++ )
   {
      if( !(data->data_buffers[i].flags & UNICAP_FLAGS_BUFFER_LOCKED ) )
      {
	 nr_unlocked_buffers++;
      }
   }

/*    printf( "nr: %d\n", nr_unlocked_buffers ); */

   if( data->new_frame_callback && ( nr_unlocked_buffers > 2 ) )
   {
      data->new_frame_callback( UNICAP_EVENT_NEW_FRAME, data->new_frame_callback_handle, &data->data_buffers[tmp], data->new_frame_callback_data );
   }

   data->next_display_buffer = tmp;   
}

void backend_xv_display_image( gpointer _data )
{
   struct xv_backend_data *data = _data;

/*    printf( "disp: next = %d curr = %d\n", data->next_display_buffer, data->current_buffer ); */
   data->current_buffer = data->next_display_buffer;
   backend_xv_redraw( _data );
}

void backend_xv_expose_event( gpointer _data, GtkWidget *da, GdkEventExpose *event )
{
   struct xv_backend_data *data = _data;

   if( data->gdkgc )
   {
      gdk_draw_rectangle( da->window,
			  data->gdkgc,
			  TRUE,
			  event->area.x, event->area.y,
			  event->area.width, event->area.height );
   }
   

   backend_xv_redraw( _data );
/*    { */
/*       int width, height; */
/*       GdkGC *foogc = create_gc( data->widget, 0xff, 0xff, 0 ); */
      
/*       width = data->widget->allocation.width < data->output_width ? data->widget->allocation.width : data->output_width; */
/*       height = data->widget->allocation.height < data->output_height ? data->widget->allocation.height : data->output_height; */
/*       gdk_draw_rectangle( data->widget->window, */
/* 			  foogc, */
/* 			  TRUE, */
/* 			  5, 5, */
/* 			  width-10, height-10 ); */
/*       g_object_unref( foogc ); */
/*    } */
}

void backend_xv_get_image_data( gpointer _data, unicap_data_buffer_t *data_buffer, int b )
{
   struct xv_backend_data *data = _data;

   if( b == 0 )
   {
      memcpy( data_buffer, &data->data_buffers[data->current_buffer], sizeof( unicap_data_buffer_t ) );
   }
   else
   {
      memcpy( data_buffer, &data->data_buffers[data->next_display_buffer], sizeof( unicap_data_buffer_t ) );
   }
   
   sprintf( data_buffer->format.identifier, "%c%c%c%c", 
	    ( data_buffer->format.fourcc >> 0 ) & 0xff, 
	    ( data_buffer->format.fourcc >> 8 ) & 0xff, 
	    ( data_buffer->format.fourcc >> 16 ) & 0xff, 
	    ( data_buffer->format.fourcc >> 24 ) & 0xff );
}

void backend_xv_lock( gpointer _data )
{
   struct xv_backend_data *data = _data;
   sem_wait( &data->sema );
}

void backend_xv_unlock( gpointer _data )
{
   struct xv_backend_data *data = _data;
   sem_post( &data->sema );
}

void backend_xv_set_scale_to_fit( gpointer _data, gboolean scale_to_fit )
{
   struct xv_backend_data *data = _data;

   data->scale_to_fit = scale_to_fit;
   if( !scale_to_fit )
   {
      data->output_width = data->width;
      data->output_height = data->height;
   }
   else
   {
      data->output_width = data->widget->allocation.width;
      data->output_height = data->widget->allocation.height;
   }
   
}


void backend_xv_set_pause_state( gpointer _data, gboolean state )
{
   struct xv_backend_data *data = _data;

   if( data->redisplay_timerid )
   {
      g_source_remove( data->redisplay_timerid );
      data->redisplay_timerid = 0;
   }

   TRACE( "set pause: %d\n", state );
   
   if( state )
   {
      data->redisplay_timerid = g_timeout_add( 300, (GSourceFunc)redraw_timeout, _data );
   }

   data->pause_state = state;
   
}

guint backend_xv_get_flags( gpointer _data )
{
   return BACKEND_FLAGS_SCALING_SUPPORTED;
}

unicap_new_frame_callback_t backend_xv_set_new_frame_callback( void *_backend_data, 
							       unicap_new_frame_callback_t cb, 
							       unicap_handle_t handle, 
							       void *data )
{
   unicap_new_frame_callback_t old_cb;
   struct xv_backend_data *backend_data = ( struct xv_backend_data * )_backend_data;
   
   old_cb = backend_data->new_frame_callback;

   backend_data->new_frame_callback = NULL;
   backend_data->new_frame_callback_data = data;
   backend_data->new_frame_callback_handle = handle;
   backend_data->new_frame_callback = cb;
   
   return old_cb;
}

void backend_xv_set_color_conversion_callback( void *_backend_data, 
					       unicapgtk_color_conversion_callback_t cb,
					       void *data )
{
   struct xv_backend_data *backend_data = ( struct xv_backend_data * )_backend_data;
   backend_data->color_conversion_cb = cb;
   backend_data->color_conversion_data = data;
}
