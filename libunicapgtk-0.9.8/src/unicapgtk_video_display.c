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
#include <stdio.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <string.h>

#include <sys/time.h>
#include <time.h>

#include <unicap.h>
#include <ucil.h>


#if UNICAPGTK_DEBUG
#define DEBUG
#endif
#include <debug.h>

#include "unicapgtk.h"
#include "unicapgtk_priv.h"
#include "unicapgtk_video_display.h"
#include "backend.h"
#include "backend_gtk.h"
#include "backend_xv.h"

#define TIMEOUT_UNSET 0

#define DATA_BUFFERS 8

enum {
   UNICAPGTK_VIDEO_DISPLAY_PREDISPLAY_SIGNAL,
   UNICAPGTK_VIDEO_DISPLAY_POSTDISPLAY_SIGNAL,
   LAST_SIGNAL
};

enum
{
   PROP_0 = 0,
   PROP_SCALE_TO_FIT, 
   PROP_BACKEND, 
   PROP_FOURCC,
   PROP_STORE_ORIGINAL,
};

typedef enum 
{
   BACKEND_UNDECIDED = 0, 
   BACKEND_XV, 
   BACKEND_GTK, 
   BACKEND_LAST
} backend_type_t;

struct _UnicapgtkVideoDisplay
{
      GtkAspectFrame  parent_instance;
      
      GtkWidget *drawing_area;
      GdkGC *gc;
      
      GdkPixbuf *still_image_pixbuf;
      
      gint display_timeout_tag;
      gboolean scale_to_fit;
      gboolean capture_running;
      volatile gboolean pause;
      
      unicap_handle_t unicap_handle;
      unicap_device_t device;
      unicap_format_t format;
      unicap_data_buffer_t data_buffer[DATA_BUFFERS];
      unicap_data_buffer_t still_image_buffer;
      unicap_data_buffer_t pause_buffer;
   unicap_data_buffer_t stored_buffer;

      volatile gboolean new_frame;
      unicap_new_frame_callback_t new_frame_callback;
      void * new_frame_callback_data;
      unsigned int new_frame_callback_flags;
      struct timeval disp_time;

      unicapgtk_color_conversion_callback_t color_conversion_cb;
      void                                 *color_conversion_data;

      unsigned int backend_fourcc;
      
      int crop_x;
      int crop_y;
      int crop_width;
      int crop_height;
      
      int display_width;
      int display_height;

      backend_type_t requested_backend;
      gchar *backend;
   gboolean store_original_buffer;
      backend_update_image_func_t backend_update_image;
      backend_redraw_func_t backend_redraw;
      backend_destroy_func_t backend_destroy;
      backend_set_output_size_func_t backend_set_output_size;
      backend_expose_event_t backend_expose;
      backend_get_image_data_t backend_get_image_data;
      backend_lock_t backend_lock;
      backend_unlock_t backend_unlock;
      backend_display_image_t backend_display_image;
      backend_size_allocate_t backend_size_allocate;
      backend_set_crop_t backend_set_crop;
      backend_set_scale_to_fit_t backend_set_scale_to_fit;
      backend_set_pause_state_t backend_set_pause_state;
      backend_get_flags_t backend_get_flags;
      backend_set_color_conversion_callback_t backend_set_color_conversion_callback;
      
      
      gpointer backend_data;
};


static void unicapgtk_video_display_class_init          ( UnicapgtkVideoDisplayClass *klass);
static void unicapgtk_video_display_init                ( UnicapgtkVideoDisplay      *ugtk);
static void unicapgtk_video_display_set_property        ( GObject       *object, 
                                                          guint          property_id, 
                                                          const GValue  *value, 
                                                          GParamSpec    *pspec );
static void unicapgtk_video_display_get_property        ( GObject       *object, 
                                                          guint          property_id, 
                                                          GValue        *value, 
                                                          GParamSpec    *pspec );
static void unicapgtk_video_display_destroy             ( GtkObject     *object );
static void unicapgtk_video_display_compute_child_allocation( GtkFrame  *frame,
                                                              GtkAllocation *child_allocation );
static gboolean tlwindow_configure_event( GtkWidget *window, GdkEventConfigure *event, UnicapgtkVideoDisplay *ugtk );
static void da_hierarchy_changed_cb( GtkWidget *da, GtkWidget *widget2, UnicapgtkVideoDisplay *ugtk );
static void da_configure_cb ( GtkWidget *da, GdkEventConfigure *event, UnicapgtkVideoDisplay *ugtk );
static gboolean da_expose_event( GtkWidget *da, GdkEventExpose *event, UnicapgtkVideoDisplay *ugtk );
static gboolean da_realize_cb( GtkWidget *da, UnicapgtkVideoDisplay *ugtk );
static void set_backend( UnicapgtkVideoDisplay *ugtk );
static void new_frame_cb( unicap_event_t event, unicap_handle_t handle, unicap_data_buffer_t *buffer, UnicapgtkVideoDisplay *ugtk );

static GtkWidgetClass *parent_class = NULL;
static guint unicapgtk_video_display_signals[LAST_SIGNAL] = { 0 };

GType
unicapgtk_video_display_get_type (void)
{
   static GType ugtk_type = 0;

   if( !ugtk_type )
   {
      static const GTypeInfo ugtk_info =
	 {
	    sizeof( UnicapgtkVideoDisplayClass ),
	    NULL, /* base_init */
	    NULL, /* base_finalize */
	    ( GClassInitFunc ) unicapgtk_video_display_class_init,
	    NULL, /* class_finalize */
	    NULL, /* class_data */
	    sizeof( UnicapgtkVideoDisplay ),
	    0,
	    ( GInstanceInitFunc ) unicapgtk_video_display_init,
	 };
	  
      ugtk_type = g_type_register_static( GTK_TYPE_ASPECT_FRAME, "UnicapgtkVideoDisplay", &ugtk_info, 0 );
   }

   return ugtk_type;
}

static void unicapgtk_video_display_class_init( UnicapgtkVideoDisplayClass *klass )
{
   GObjectClass *object_class = G_OBJECT_CLASS( klass );
   GtkObjectClass *gtk_object_class = GTK_OBJECT_CLASS( klass );
   GtkFrameClass *frame_class = GTK_FRAME_CLASS( klass );

   parent_class = g_type_class_peek_parent( klass );

   object_class->set_property = unicapgtk_video_display_set_property;
   object_class->get_property = unicapgtk_video_display_get_property;

   gtk_object_class->destroy  = unicapgtk_video_display_destroy;

   frame_class->compute_child_allocation = unicapgtk_video_display_compute_child_allocation;

   /**
    * UnicapgtkVideoDisplay::unicapgtk_video_display_predisplay:
    * 
    */
   unicapgtk_video_display_signals[UNICAPGTK_VIDEO_DISPLAY_PREDISPLAY_SIGNAL] = 
      g_signal_new( "unicapgtk_video_display_predisplay",
		    G_TYPE_FROM_CLASS(klass),
		    G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		    G_STRUCT_OFFSET(UnicapgtkVideoDisplayClass, unicapgtk_video_display),
		    NULL, 
		    NULL,                
		    g_cclosure_marshal_VOID__POINTER,
		    G_TYPE_NONE, 1, 
		    G_TYPE_POINTER );

/*    unicapgtk_video_display_signals[UNICAPGTK_VIDEO_DISPLAY_POSTDISPLAY_SIGNAL] =  */
/*       g_signal_new( "unicapgtk_video_display_postdisplay", */
/* 		    G_TYPE_FROM_CLASS(klass), */
/* 		    G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION, */
/* 		    G_STRUCT_OFFSET(UnicapgtkVideoDisplayClass, unicapgtk_video_display), */
/* 		    NULL,  */
/* 		    NULL,                 */
/* 		    g_cclosure_marshal_VOID__POINTER, */
/* 		    G_TYPE_NONE, 1, */
/* 		    G_TYPE_POINTER ); */

/*    unicapgtk_video_display_signals[UNICAPGTK_VIDEO_DISPLAY_DISPLAY_PIXBUF_SIGNAL] =  */
/*       g_signal_new( "unicapgtk_video_display_display_pixbuf", */
/* 		    G_TYPE_FROM_CLASS(klass), */
/* 		    G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION, */
/* 		    G_STRUCT_OFFSET(UnicapgtkVideoDisplayClass, unicapgtk_video_display), */
/* 		    NULL,  */
/* 		    NULL,                 */
/* 		    g_cclosure_marshal_VOID__POINTER, */
/* 		    G_TYPE_NONE, 1, */
/* 		    G_TYPE_POINTER ); */


   g_object_class_install_property (object_class,
				    PROP_SCALE_TO_FIT,
				    g_param_spec_boolean ("scale-to-fit",
                                                          NULL, NULL,
                                                          FALSE,
                                                          G_PARAM_READWRITE));
   g_object_class_install_property (object_class,
				    PROP_BACKEND,
				    g_param_spec_string ("backend",
                                                          NULL, NULL,
                                                          NULL,
                                                          G_PARAM_READWRITE ) );
   g_object_class_install_property (object_class,
				    PROP_FOURCC,
				    g_param_spec_uint ("backend_fourcc",
						       NULL, NULL,
						       0, 0xffffffff, 0, 
						       G_PARAM_READWRITE ) );
   g_object_class_install_property (object_class,
				    PROP_STORE_ORIGINAL,
				    g_param_spec_boolean ("store_original",
                                                          NULL, NULL,
                                                          FALSE,
                                                          G_PARAM_READWRITE));
}

static void unicapgtk_video_display_set_property( GObject *object, 
						  guint property_id, 
						  const GValue *value, 
						  GParamSpec *pspec )
{
   UnicapgtkVideoDisplay *ugtk = UNICAPGTK_VIDEO_DISPLAY( object );

   switch( property_id )
   {
      case PROP_SCALE_TO_FIT:
	 unicapgtk_video_display_set_scale_to_fit( ugtk,
						   g_value_get_boolean( value ) );
	 break;

      case PROP_BACKEND:
      {
	 const gchar *str = g_value_get_string( value );
	 backend_type_t t = BACKEND_UNDECIDED;

	 if( !strcmp( str, "gtk" ) )
	 {
	    t = BACKEND_GTK;
	 }
	 else if( !strcmp( str, "xv" ) )
	 {
	    t = BACKEND_XV;
	 }
	 ugtk->requested_backend = t;
         set_backend( ugtk );
      }
      break;

      case PROP_FOURCC:
      {
	 ugtk->backend_fourcc = g_value_get_uint( value );
         set_backend( ugtk );
      }
      break;

   case PROP_STORE_ORIGINAL:
   {
      ugtk->store_original_buffer = g_value_get_boolean( value );
   }
   break;
   

      default:
	 G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, pspec );
	 break;
   }
}

static void unicapgtk_video_display_get_property( GObject *object, 
						  guint property_id, 
						  GValue *value, 
						  GParamSpec *pspec )
{
   UnicapgtkVideoDisplay *ugtk = UNICAPGTK_VIDEO_DISPLAY( object );

   switch( property_id )
   {
      case PROP_SCALE_TO_FIT:
         g_value_set_boolean( value, ugtk->scale_to_fit );
         break;

      case PROP_BACKEND:
	 g_value_set_string( value, ugtk->backend );
	 break;

      case PROP_FOURCC:
	 g_value_set_uint( value, ugtk->backend_fourcc );
	 break;

   case PROP_STORE_ORIGINAL:
      g_value_set_boolean( value, ugtk->store_original_buffer );
      break;
	 
      default:
	 G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, pspec );
	 break;
   }
}


static void unicapgtk_video_display_destroy( GtkObject *object )
{
   UnicapgtkVideoDisplay *ugtk = UNICAPGTK_VIDEO_DISPLAY( object );

/*    g_object_unref( ugtk->gc ); */
   if( ugtk->capture_running )
   {
      unicapgtk_video_display_stop( ugtk );
   }

   if( ugtk->unicap_handle )
   {
      unicap_close( ugtk->unicap_handle );
      ugtk->unicap_handle = NULL;
   }

   if( ugtk->backend )
   {
      ugtk->backend_destroy( ugtk->backend_data );
      ugtk->backend = NULL;
   }

   GTK_OBJECT_CLASS (parent_class)->destroy (object);
}


static void unicapgtk_video_display_compute_child_allocation( GtkFrame  *frame, GtkAllocation *child_allocation )
{
   UnicapgtkVideoDisplay *ugtk = UNICAPGTK_VIDEO_DISPLAY( frame );
   
   GTK_FRAME_CLASS( parent_class )->compute_child_allocation( frame, child_allocation );
   
   if( child_allocation->width < 0 )
   {
      child_allocation->width = ugtk->crop_width;
   }
   if( child_allocation->height < 0 )
   {
      child_allocation->height = ugtk->crop_height;
   }
   
   if( ugtk->scale_to_fit )
   {
      return;
   }

   if( child_allocation->width > ugtk->crop_width )
   {
      child_allocation->x += GTK_ASPECT_FRAME( frame )->xalign *
	 (child_allocation->width - ugtk->crop_width);
      child_allocation->width = ugtk->crop_width;
   }
   if( child_allocation->height > ugtk->crop_height )
   {
      child_allocation->y += GTK_ASPECT_FRAME( frame )->yalign *
	 (child_allocation->height - ugtk->crop_height);
      child_allocation->height = ugtk->crop_height;
   }
}

static 
void da_size_allocate( GtkWidget *da, GtkAllocation *allocation, UnicapgtkVideoDisplay * ugtk )
{
   if( ugtk->backend )
   {
      if( ugtk->scale_to_fit )
      {
	 ugtk->backend_size_allocate( ugtk->backend_data, da, allocation );
      }
   }
}


static 
void unicapgtk_video_display_init( UnicapgtkVideoDisplay *ugtk )
{
   ugtk->backend_fourcc = 0;

   gtk_aspect_frame_set( GTK_ASPECT_FRAME( ugtk ), 0.5, 0.5, 1.0, TRUE );
   gtk_frame_set_shadow_type( GTK_FRAME( ugtk ), GTK_SHADOW_NONE );

   ugtk->drawing_area = gtk_drawing_area_new( );
   gtk_widget_add_events( ugtk->drawing_area, 
			  GDK_ALL_EVENTS_MASK );
   gtk_container_add( GTK_CONTAINER( ugtk ), ugtk->drawing_area );	
   gtk_widget_show( ugtk->drawing_area );


   g_signal_connect( ugtk->drawing_area, "hierarchy-changed",
		     G_CALLBACK( da_hierarchy_changed_cb ), ugtk );
   g_signal_connect( ugtk->drawing_area, "configure-event",
		     G_CALLBACK( da_configure_cb ),
                     ugtk );
   g_signal_connect( ugtk->drawing_area, "unmap",
                     G_CALLBACK(gtk_widget_destroyed),
                     &ugtk->gc );

   g_signal_connect( ugtk->drawing_area, "expose-event", 
		     G_CALLBACK( da_expose_event ), 
		     ugtk );
   g_signal_connect( ugtk->drawing_area, "size-allocate", 
		     G_CALLBACK( da_size_allocate ), 
		     ugtk );

   ugtk->unicap_handle = NULL;  
   ugtk->scale_to_fit = FALSE;
   ugtk->display_timeout_tag = TIMEOUT_UNSET;

}

/*
  Initializes object structures. Looks for first matching video format and resizes the drawing area accordingly.

  Input: ugtk
  
  Output: 

  Returns:
*/
static void new( UnicapgtkVideoDisplay *ugtk )
{
   unicap_format_t format_spec, format;
   unicap_void_format( &format_spec );
   format_spec.fourcc = UCIL_FOURCC( 'U', 'Y', 'V', 'Y' );
	
   ugtk->capture_running = FALSE;
   ugtk->pause = TRUE;
   ugtk->display_timeout_tag = TIMEOUT_UNSET;
   ugtk->still_image_pixbuf = NULL;
   ugtk->backend = NULL;   

   if( !SUCCESS( unicap_enumerate_formats( ugtk->unicap_handle, 
					   &format_spec, 
					   &format, 
					   0 ) ) )
   {
      if( !SUCCESS( unicap_enumerate_formats( ugtk->unicap_handle, 
					      NULL, 
					      &format, 
					      0 ) ) )
      {
	 TRACE( "Failed to get video format\n" );
	 return;
      }
   }
	
   if( ( format.size.width == 0 ) || 
       ( format.size.height == 0 ) )
   {
      if( format.size_count )
      {
	 format.size.width = format.sizes[ format.size_count - 1 ].width;
	 format.size.height = format.sizes[ format.size_count -1 ].height;
      }
   }


   if( !unicapgtk_video_display_set_format( ugtk, &format ) )
   {
      TRACE( "Failed to set video format\n" );
      return;
   }
   if( !SUCCESS( unicap_get_format( ugtk->unicap_handle, 
				    &ugtk->format ) ) )
   {
      TRACE( "Failed to get video format\n" );
      return;
   }

   TRACE( "format: FOURCC = %c%c%c%c\n", format.fourcc, format.fourcc>>8, format.fourcc>>16, 
	  format.fourcc>>24 );

}

/**
 * unicapgtk_video_display_new:
 *
 * Creates a new #UnicapgtkVideoDisplay
 *
 * Returns: a new #UnicapgtkVideoDisplay
 */
GtkWidget* unicapgtk_video_display_new( void )
{
   return g_object_new( UNICAPGTK_TYPE_VIDEO_DISPLAY, NULL );
}

/**
 * unicapgtk_video_display_new_by_device:
 * @device_spec: the device to be used by this video display widget
 *
 * Creates a new #UnicapgtkVideoDisplay and opens the given device to
 * operate with. 
 *
 * Returns: a new #UnicapgtkVideoDisplay
 */
GtkWidget* unicapgtk_video_display_new_by_device( unicap_device_t *device_spec )
{
   UnicapgtkVideoDisplay *ugtk;

   ugtk = g_object_new( UNICAPGTK_TYPE_VIDEO_DISPLAY, NULL );

   if( SUCCESS( unicap_enumerate_devices( device_spec, &ugtk->device, 0 ) ) )
   {
      unicap_open( &ugtk->unicap_handle, &ugtk->device );
   }
	
   if( ugtk->unicap_handle )
   {
      new( ugtk );
   }

   return GTK_WIDGET( ugtk );
}

/**
 * unicapgtk_video_display_new_by_handle:
 * @handle: a handle of the device to be used by this video display
 * widget
 *
 * Creates a new #UnicapgtkVideoDisplay and uses the handle to control
 * the video device. The handle gets cloned by this call. 
 *
 * Returns: a new #UnicapgtkVideoDisplay
 */
GtkWidget *unicapgtk_video_display_new_by_handle( unicap_handle_t handle )
{
   UnicapgtkVideoDisplay *ugtk;

   ugtk = g_object_new( UNICAPGTK_TYPE_VIDEO_DISPLAY, NULL );

   ugtk->unicap_handle = unicap_clone_handle( handle );
	
   new( ugtk );
	
   return GTK_WIDGET( ugtk );
}


static void da_hierarchy_changed_cb( GtkWidget *da, GtkWidget *widget2, UnicapgtkVideoDisplay *ugtk )
{
   GtkWidget *window;
   
   window = gtk_widget_get_toplevel( da );
   if( window )
   {
      g_signal_connect( G_OBJECT( window ), "configure-event", G_CALLBACK( tlwindow_configure_event ), ugtk );
   }
}

static gboolean tlwindow_configure_event( GtkWidget *window, GdkEventConfigure *event, UnicapgtkVideoDisplay *ugtk )
{
   gtk_widget_queue_draw( GTK_WIDGET( ugtk ) );
   return FALSE;
}

static void da_configure_cb ( GtkWidget *da, GdkEventConfigure *event, UnicapgtkVideoDisplay *ugtk )
{
   if( ugtk->scale_to_fit )
   {
     unicapgtk_video_display_set_size( ugtk, event->width, event->height );
   }
}

static gboolean da_expose_event( GtkWidget *da, GdkEventExpose *event, UnicapgtkVideoDisplay *ugtk )
{
   if( !ugtk->backend )
   {
      return TRUE;
   }

   // fill the drawing area with black
   ugtk->backend_expose( ugtk->backend_data, da, event );
   
   
   return TRUE;
}

static gint timeout_cb( UnicapgtkVideoDisplay *ugtk )
{
   unicap_data_buffer_t data_buffer;


   if( ugtk->pause )
   {
      if( !strcmp( ugtk->backend, "xv" ) )
      {
	 gdk_threads_enter();
	 ugtk->backend_update_image( ugtk->backend_data, &ugtk->pause_buffer, NULL );
	 ugtk->backend_get_image_data( ugtk->backend_data, &data_buffer, 0 );
	 
	 g_signal_emit( G_OBJECT( ugtk ),
			unicapgtk_video_display_signals[ UNICAPGTK_VIDEO_DISPLAY_PREDISPLAY_SIGNAL ],
			0,
			(gpointer)&data_buffer );
	 ugtk->backend_display_image( ugtk->backend_data );
	 gdk_threads_leave();
      }

      return TRUE;
   }
   else
   {
      ugtk->backend_lock( ugtk->backend_data );
      if( ugtk->new_frame )
      {
	 long long usec;
	 gdk_threads_enter();
	 ugtk->backend_get_image_data( ugtk->backend_data, &data_buffer, 1 );

	 usec = ( ugtk->disp_time.tv_sec - data_buffer.fill_time.tv_sec ) * 1000000LL;
	 usec += ( ugtk->disp_time.tv_usec - data_buffer.fill_time.tv_usec );

/* 	 if( usec <= 0 ) */
/* 	 { */
	 ugtk->disp_time.tv_sec = data_buffer.fill_time.tv_sec;
	 ugtk->disp_time.tv_usec = data_buffer.fill_time.tv_usec;
	 g_signal_emit( G_OBJECT( ugtk ),
			unicapgtk_video_display_signals[ UNICAPGTK_VIDEO_DISPLAY_PREDISPLAY_SIGNAL],
			0,
			(gpointer)&data_buffer );

	 ugtk->backend_display_image( ugtk->backend_data );
/* 	 } */
/* 	 else */
/* 	 { */
/* 	    TRACE( "drop old\n" ); */
/* 	 } */
	 
	 
	 ugtk->new_frame = FALSE;
	 gdk_threads_leave();
      }
      ugtk->backend_unlock( ugtk->backend_data );
   }

   return TRUE;
}

static void new_frame_cb( unicap_event_t event, unicap_handle_t handle, unicap_data_buffer_t *buffer, UnicapgtkVideoDisplay *ugtk )
{
   if( !ugtk->pause )
   {
      ugtk->new_frame = TRUE;
      if( ugtk->new_frame_callback )
      {
	 ugtk->new_frame_callback( event, handle, buffer, ugtk->new_frame_callback_data );
      }

      if( ugtk->backend )
      {
	 ugtk->backend_lock( ugtk->backend_data );
	 if( !ugtk->backend_update_image ){
	    TRACE( "!!!!\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n" );
	    ugtk->backend_unlock( ugtk->backend_data );
	    return;
	 }
	    
	 ugtk->backend_update_image( ugtk->backend_data, buffer, NULL );
	 ugtk->backend_unlock( ugtk->backend_data );
      }
   }
}


/**
 * unicapgtk_video_display_start:
 * @ugtk: an #UnicapgtkVideoDisplay
 *
 * Calls #unicap_start_capture on the device and starts displaying the
 * live video stream. 
 *
 * Returns: TRUE when the live display could be successfully started.
*/
gboolean unicapgtk_video_display_start( UnicapgtkVideoDisplay *ugtk )
{
   unicap_format_t tmp_format;
   
   if( !ugtk->unicap_handle )
   {
      g_warning( "Implement me: Correct warning message - handle not set\n" );
      return FALSE;
   }

   if( ugtk->capture_running )
   {
/*       g_warning( "video_display_start: Display already started!\n" ); */
      return TRUE;
   }

   if( ugtk->display_timeout_tag != TIMEOUT_UNSET )
   {
      g_source_remove( ugtk->display_timeout_tag );
      ugtk->display_timeout_tag = TIMEOUT_UNSET;
   }

   if( ugtk->still_image_buffer.data )   //
   {
      free( ugtk->still_image_buffer.data );
      ugtk->still_image_buffer.data = NULL;
   }

   if( !strcmp( ugtk->format.identifier, "" ) )
   {
/*       g_warning( "Video format not set" ); */
      return FALSE;
   }

   ugtk->capture_running = TRUE;

   if( !SUCCESS( unicap_get_format( ugtk->unicap_handle, &tmp_format ) ) )
   {
/*       g_warning( "Failed to get current video format\n" ); */
      return FALSE;
   }
   
   tmp_format.buffer_type = UNICAP_BUFFER_TYPE_SYSTEM;
   
   if( !SUCCESS( unicap_set_format( ugtk->unicap_handle, &tmp_format ) ) )
   {
      g_warning( "Failed to activate SYSTEM_BUFFERS\n" );
      return FALSE;
   }   

   ugtk->disp_time.tv_sec = 0;
   ugtk->disp_time.tv_usec = 0;

   unicap_unregister_callback( ugtk->unicap_handle, UNICAP_EVENT_NEW_FRAME );
   if( !SUCCESS( unicap_register_callback( ugtk->unicap_handle, UNICAP_EVENT_NEW_FRAME, (unicap_callback_t)new_frame_cb, ugtk ) ) ){
      TRACE( "Failed to register callback\n" );
      return FALSE;
   }

   ugtk->display_timeout_tag = g_timeout_add( 30, (GtkFunction)timeout_cb, ugtk );

   if( !SUCCESS( unicap_start_capture( ugtk->unicap_handle ) ) )
   {
      g_warning( "Failed to start video capture\n" );
      return FALSE;
   }
   
   ugtk->pause = FALSE;

   return TRUE;
}

/**
 * unicapgtk_video_display_stop:
 * @ugtk: an #UnicapgtkVideoDisplay
 *
 * Calls #unicap_stop_capture on the device and stops displaying the
 * live video image.
 */
void unicapgtk_video_display_stop( UnicapgtkVideoDisplay *ugtk )
{
   int cbuf;
   if( !ugtk->capture_running )
   {
      return;
   }

   ugtk->capture_running = FALSE;

   if( ugtk->display_timeout_tag != TIMEOUT_UNSET )
   {
      g_source_remove( ugtk->display_timeout_tag );
      ugtk->display_timeout_tag = TIMEOUT_UNSET;
   }
	
   unicap_stop_capture( ugtk->unicap_handle );

   if( ugtk->still_image_buffer.data )   //
   {
      free( ugtk->still_image_buffer.data );
      ugtk->still_image_buffer.data = NULL;
   }
	
   for( cbuf = 0; cbuf < DATA_BUFFERS; cbuf++ )
   {
      if( ugtk->data_buffer[cbuf].type == UNICAP_BUFFER_TYPE_USER )
      {
	 g_free( ugtk->data_buffer[cbuf].data );
      }
   }
   
   // todo: redraw current image as RGB buffer
}

/**
 * unicapgtk_video_display_get_device:
 * @ugtk: an #UnicapgtkVideoDisplay
 * 
 * Returns the device currently used by the video display. The
 * returned data is owned by the library and should not be freed by
 * the application.
 * 
 * Returns: an #unicap_device_t
 */
unicap_device_t *unicapgtk_video_display_get_device( UnicapgtkVideoDisplay *ugtk )
{
   return &ugtk->device;
}

/**
 * unicapgtk_video_display_get_handle:
 * @ugtk: an #UnicapgtkVideoDisplay
 * 
 * Returns a handle to the device currently used by the display. This
 * is a clone of the handle used by the display so #unicap_close must
 * be called on this handle by the calling application.
 *
 * Returns: a cloned #unicap_handle_t
 */
unicap_handle_t unicapgtk_video_display_get_handle( UnicapgtkVideoDisplay *ugtk )
{
   return unicap_clone_handle( ugtk->unicap_handle );
}


/**
 * unicapgtk_video_display_set_format:
 * @ugtk: an #UnicapgtkVideoDisplay
 * @format_spec: a #unicap_format_t
 *
 * Sets the video format used by the display. If a handle is set on
 * the display ( ie. the display controls a device ), the video format
 * is also changed and only video formats supported by the device can
 * be set. 
 * 
 * Returns: TRUE when the format change was successfull. 
 */
gboolean unicapgtk_video_display_set_format( UnicapgtkVideoDisplay *ugtk, 
					     unicap_format_t *format_spec )
{
   unicap_format_t format;
   gboolean old_capture_state;
   gboolean status = TRUE;
   

   old_capture_state = ugtk->capture_running;
   if( ugtk->capture_running )
   {
      unicapgtk_video_display_stop( ugtk );
   }

   if( ugtk->unicap_handle )
   {
      unicap_format_t _format_spec;
      
      unicap_void_format( &_format_spec );

      strcpy( _format_spec.identifier, format_spec->identifier );
      _format_spec.fourcc = format_spec->fourcc;
      _format_spec.bpp = format_spec->bpp;
      _format_spec.size.width = format_spec->size.width;
      _format_spec.size.height = format_spec->size.height;

      if( !SUCCESS( unicap_enumerate_formats( ugtk->unicap_handle, 
					      &_format_spec, 
					      &format, 
					      0 ) ) )
      {
	 TRACE( "Failed to enumerate video format\n" );
	 TRACE( "Spec = %s  %08x %dx%d\n", _format_spec.identifier, _format_spec.fourcc, _format_spec.size.width, _format_spec.size.height );
	 return FALSE;
      }
	
      format.size.x      = format_spec->size.x;
      format.size.y      = format_spec->size.y;
      format.size.width  = format_spec->size.width;
      format.size.height = format_spec->size.height;

      if( !SUCCESS( unicap_set_format( ugtk->unicap_handle, &format ) ) )
      {
	 TRACE( "Failed to set video format\n" );
      }
      unicap_get_format( ugtk->unicap_handle, &ugtk->format );
   }
   else
   {
      unicap_copy_format( &format, format_spec );
      unicap_copy_format( &ugtk->format, format_spec );
   }
   
   TRACE( "format: FOURCC = %c%c%c%c size: %d x %d\n", format.fourcc, format.fourcc>>8, format.fourcc>>16, 
	  format.fourcc>>24, 
	  format.size.width, 
	  format.size.height);


   ugtk->crop_x = ugtk->crop_y = 0;
   ugtk->crop_width = format.size.width;
   ugtk->crop_height = format.size.height;

   ugtk->display_width = format.size.width;
   ugtk->display_height = format.size.height;

   if ( ugtk->scale_to_fit)
   {
      if( format.size.width > 0 && format.size.height > 0 )
      {
	 gtk_aspect_frame_set( GTK_ASPECT_FRAME( ugtk ), 0.5, 0.5,
			       (gdouble) format.size.width /
			       (gdouble) format.size.height,
			       FALSE );
      }

      gtk_widget_queue_resize( GTK_WIDGET( ugtk->drawing_area ) );
   }
   else
   {
      gtk_widget_set_size_request( GTK_WIDGET( ugtk->drawing_area ),
                                   format.size.width,
                                   format.size.height );
   }

   if( GTK_WIDGET_REALIZED( ugtk->drawing_area ) )
   {
      da_realize_cb( ugtk->drawing_area, ugtk );
   }
   else
   {
      g_signal_connect_after( G_OBJECT( ugtk->drawing_area ), "realize", G_CALLBACK(da_realize_cb), ugtk );
   }

   if( old_capture_state )
   {
      status = unicapgtk_video_display_start( ugtk );
   }

   return status;
}

void unicapgtk_video_display_get_format( UnicapgtkVideoDisplay *ugtk, unicap_format_t *format )
{
   unicap_void_format( format );
   if( ugtk->unicap_handle )
   {
      unicap_get_format( ugtk->unicap_handle, format );
   }
}

/** 
 * unicapgtk_video_display_set_pause:
 * @ugtk: an UnicapgtkVideoDisplay
 * @state: TRUE to put the device into pause state; FALSE to put it
 * into running state
 * 
 * Pauses or unpauses the video display. If the display is controling
 * a device, the device is not stopped or paused by this call. When
 * the video display is paused, no new frames are retrieved from the
 * device and the display is constantly updated with the last image
 * retrieved from the device. 
 * 
 * Buffer display events are still called. This way an application can
 * still draw an overlay onto the image. 
 * 
 */
void unicapgtk_video_display_set_pause( UnicapgtkVideoDisplay *ugtk, gboolean state )
{
   unicap_data_buffer_t data_buffer;

   
   if( ugtk->capture_running )
   {
      if( state )
      {
	 if( ugtk->backend )
	 {
	    ugtk->backend_get_image_data( ugtk->backend_data, &data_buffer, 0 );
	    unicap_copy_format( &ugtk->pause_buffer.format, &data_buffer.format );
	    ugtk->pause_buffer.buffer_size = data_buffer.format.buffer_size;
	    ugtk->pause_buffer.data = malloc( ugtk->pause_buffer.format.buffer_size );
	    memcpy( ugtk->pause_buffer.data, data_buffer.data, ugtk->pause_buffer.format.buffer_size );
	 }
      }
      else
      {
	 if( ugtk->backend )
	 {
	    free( ugtk->pause_buffer.data );
	 }
      }   
   }

   ugtk->pause = state ? TRUE : FALSE;

/*    if( ugtk->backend_set_pause_state ) */
/*    { */
/*       ugtk->backend_set_pause_state( ugtk->backend_data, ugtk->pause ); */
/*    } */
}


/**
 * unicapgtk_video_display_get_pause
 * @ugtk: an #UnicapgtkVideoDisplay
 * 
 * Retrieve current pause state
 * 
 * Returns: TRUE when the display is currently paused
 */
gboolean unicapgtk_video_display_get_pause( UnicapgtkVideoDisplay *ugtk )
{
   return ugtk->pause;
}

/**
 * unicapgtk_video_display_set_crop:
 * @ugtk: an #UnicagtkVideoDisplay
 * @crop: cropping specification
 *
 * Set a cropping window. The video display will only display the
 * region specified in the cropping window. 
 */
void unicapgtk_video_display_set_crop( UnicapgtkVideoDisplay *ugtk, 
				       UnicapgtkVideoDisplayCropping *crop )
{
   crop->crop_x = MAX( crop->crop_x, 0 );
   crop->crop_x = MIN( crop->crop_x, ugtk->format.size.width );
   crop->crop_width = MAX( crop->crop_width, 0 );
   crop->crop_width = MIN( crop->crop_width, ugtk->format.size.width );

   crop->crop_y = MAX( crop->crop_y, 0 );
   crop->crop_y = MIN( crop->crop_y, ugtk->format.size.height );
   crop->crop_height = MAX( crop->crop_height, 0 );
   crop->crop_height = MIN( crop->crop_height, ugtk->format.size.height );

   ugtk->crop_x = crop->crop_x;
   ugtk->crop_y = crop->crop_y;
   ugtk->crop_width = crop->crop_width;
   ugtk->crop_height = crop->crop_height;

   if( ugtk->backend ){
      ugtk->backend_set_crop( ugtk->backend_data, ugtk->crop_x, ugtk->crop_y, ugtk->crop_width, ugtk->crop_height );
   }

   if (! ugtk->scale_to_fit)
   {
      gtk_widget_set_size_request( ugtk->drawing_area, 
                                   crop->crop_width, 
                                   crop->crop_height );
   }
}

/**
 * unicapgtk_video_display_get_crop
 * @ugtk: an #UnicapgtkVideoDisplay
 * @crop: pointer to where to store the #UnicapgtkVideoDisplayCropping
 * structure
 * 
 * Retrieves the current cropping window. 
 * 
 */
void unicapgtk_video_display_get_crop( UnicapgtkVideoDisplay *ugtk, 
				       UnicapgtkVideoDisplayCropping *crop )
{
   crop->crop_x = ugtk->crop_x;
   crop->crop_y = ugtk->crop_y;
   crop->crop_width = ugtk->crop_width;
   crop->crop_height = ugtk->crop_height;
}


/** 
 * unicapgtk_video_display_set_device:
 * @ugtk: an #UnicapgtkVideoDisplay
 * @device_spec: an #unicap_device_t
 * 
 * Set a new device on the display. 
 * 
 * Returns: TRUE when the new device could be successfully set. 
 */
gboolean unicapgtk_video_display_set_device( UnicapgtkVideoDisplay *ugtk, unicap_device_t *device_spec )
{	
   unicap_status_t status;
   gboolean capture_running;

   capture_running = ugtk->capture_running;

   unicapgtk_video_display_stop( ugtk );

   if( ugtk->unicap_handle )
   {
      unicap_close( ugtk->unicap_handle );
      ugtk->unicap_handle = 0;
   }

   status = unicap_enumerate_devices( device_spec, &ugtk->device, 0 );
   if( SUCCESS( status ) )
   {
      status = unicap_open( &ugtk->unicap_handle, &ugtk->device );
   }	
	
   if( ugtk->unicap_handle )
   {
      new( ugtk );
   }
   else
   {
      TRACE( "FAILED!\n" );
   }

   if( capture_running )
   {
      unicapgtk_video_display_start( ugtk );
   }
	
   return SUCCESS( status );
}

/**
 * unicapgtk_video_display_set_handle: 
 * @ugtk: an #UnicapgtkVideoDisplay
 * @handle: a #unicap_handle_t
 * 
 * Set a new device on the display, specified by a handle. The handle
 * gets cloned by this call. 
 * 
 * Returns: TRUE when the device change was successful. 
 */
gboolean unicapgtk_video_display_set_handle( UnicapgtkVideoDisplay *ugtk, unicap_handle_t handle )
{	
   unicap_status_t status;
   unicap_format_t format;
   gboolean capture_running;

   capture_running = ugtk->capture_running;
   if( ugtk->capture_running )
   {
      unicapgtk_video_display_stop( ugtk );
   }

   if( ugtk->unicap_handle )
   {
      unicap_close( ugtk->unicap_handle );
      ugtk->unicap_handle = 0;
   }
   
   if( ugtk->backend )
   {
      ugtk->backend_destroy( ugtk->backend_data );
      ugtk->backend = NULL;
   }      
	
   ugtk->unicap_handle = unicap_clone_handle( handle );


   status = unicap_get_device( handle, &ugtk->device );
	
   if( ugtk->unicap_handle )
   {
      ugtk->capture_running = FALSE;
      ugtk->pause = FALSE;
      ugtk->display_timeout_tag = TIMEOUT_UNSET;
      ugtk->still_image_pixbuf = NULL;
      ugtk->backend = NULL;   
   }
   else
   {
      TRACE( "FAILED!\n" );
   }

   status = unicap_get_format( handle, &format );
   
   if( SUCCESS( status ) )
   {
      unicapgtk_video_display_set_format( ugtk, &format );
   }
   

   if( capture_running )
   {
      unicapgtk_video_display_start( ugtk );
   }
	
   return SUCCESS( status );
}

/**
 * unicapgtk_video_display_get_still_image:
 * @ugtk: an #UnicapgtkVideoDisplay
 * 
 * Retrieves a still image from the live video stream. 
 * 
 * Returns: a new allocated #GdkPixbuf
 */
GdkPixbuf *unicapgtk_video_display_get_still_image( UnicapgtkVideoDisplay *ugtk )
{
   GdkPixbuf *pixbuf = NULL;
   
   if( ugtk->backend )
   {
      unicap_data_buffer_t data_buffer;
      unicap_data_buffer_t target_buffer;	 
      gboolean converted = FALSE;
      
      ugtk->backend_lock( ugtk->backend_data );
      ugtk->backend_get_image_data( ugtk->backend_data, &data_buffer, 0 );

      target_buffer.buffer_size = target_buffer.format.buffer_size = data_buffer.format.size.width *
	 data_buffer.format.size.height * 3;
      target_buffer.data = g_malloc( target_buffer.buffer_size );
      target_buffer.format.fourcc = UCIL_FOURCC( 'R', 'G', 'B', 0 );
      target_buffer.format.size.width = data_buffer.format.size.width;
      target_buffer.format.size.height = data_buffer.format.size.height;
      
      if( ugtk->color_conversion_cb )
      {
	 converted = ugtk->color_conversion_cb( &target_buffer, &data_buffer, ugtk->color_conversion_data );
      }
      if( !converted )
      {
	 ucil_convert_buffer( &target_buffer, &data_buffer );
      }

      pixbuf = gdk_pixbuf_new_from_data( target_buffer.data,
					 GDK_COLORSPACE_RGB,
					 0, // alpha
					 8, // bps
					 target_buffer.format.size.width,
					 target_buffer.format.size.height,
					 target_buffer.format.size.width * 3,
					 (GdkPixbufDestroyNotify)g_free,
					 NULL );
      ugtk->backend_unlock( ugtk->backend_data );
   }
   
   return pixbuf;
}

/**
 * unicapgtk_video_display_get_data_buffer:
 * @ugtk: an #UnicapgtkVideoDisplay
 * @dest_buffer: a pointer where to store the new data_buffer
 * 
 * Retrieves a still image from the live video stream. 
 */
void unicapgtk_video_display_get_data_buffer( UnicapgtkVideoDisplay *ugtk, unicap_data_buffer_t **dest_buffer )
{
   unicap_data_buffer_t data_buffer;
   unicap_data_buffer_t *new_buffer;

   new_buffer = malloc( sizeof( unicap_data_buffer_t ) );
   
   ugtk->backend_lock( ugtk->backend_data );
   ugtk->backend_get_image_data( ugtk->backend_data, &data_buffer, 0 );
   unicap_copy_format( &new_buffer->format, &data_buffer.format );
   new_buffer->data = malloc( data_buffer.format.buffer_size );
   new_buffer->buffer_size = data_buffer.format.buffer_size;
   memcpy( new_buffer->data, data_buffer.data, data_buffer.format.buffer_size );
   ugtk->backend_unlock( ugtk->backend_data );
   
   *dest_buffer = new_buffer;
}

/**
 * unicapgtk_video_display_set_still_image: 
 * @ugtk: an #UnicapgtkVideoDisplay
 * @buffer: a data_buffer to be displayed
 * 
 * Sets a still image. If the video display is currently running, it
 * is put to pause state by this call. 
 * 
 */
void unicapgtk_video_display_set_still_image( UnicapgtkVideoDisplay *ugtk, unicap_data_buffer_t *buffer )
{
   if( !ugtk->pause )
   {
      unicapgtk_video_display_set_pause( ugtk, TRUE );
   }

   if( buffer )
   {
      if( ugtk->backend )
      {
	 ugtk->backend_update_image( ugtk->backend_data, buffer, NULL );
	 ugtk->backend_display_image( ugtk->backend_data );


	 free( ugtk->pause_buffer.data );

	 unicap_copy_format( &ugtk->pause_buffer.format, &buffer->format );
	 ugtk->pause_buffer.buffer_size = buffer->format.buffer_size;
	 ugtk->pause_buffer.data = malloc( ugtk->pause_buffer.format.buffer_size );
	 memcpy( ugtk->pause_buffer.data, buffer->data, ugtk->pause_buffer.format.buffer_size );
	 g_signal_emit( G_OBJECT( ugtk ),
			unicapgtk_video_display_signals[ UNICAPGTK_VIDEO_DISPLAY_PREDISPLAY_SIGNAL ],
			0,
			(gpointer)&ugtk->pause_buffer );
      }
      else
      {
	 // Widget is not yet realized
	 memset( &ugtk->still_image_buffer, 0x0, sizeof( unicap_data_buffer_t ) );
	 ugtk->still_image_buffer.data = g_malloc( buffer->format.buffer_size );
	 ugtk->still_image_buffer.buffer_size = buffer->format.buffer_size;
	 unicap_copy_format( &ugtk->still_image_buffer.format, &buffer->format );
	 g_memmove( ugtk->still_image_buffer.data, buffer->data, buffer->format.buffer_size );
      }
   }

   gtk_widget_queue_draw( GTK_WIDGET(ugtk) );
}

void unicapgtk_video_display_set_data_buffer( UnicapgtkVideoDisplay *ugtk, unicap_data_buffer_t *buffer )
{
   if( ugtk->backend )
   {
      ugtk->backend_update_image( ugtk->backend_data, buffer, NULL );
      ugtk->backend_display_image( ugtk->backend_data );
   }
   else
   {
      // Widget is not yet realized
      g_warning( "video display not realized" );
/*       memset( &ugtk->still_image_buffer, 0x0, sizeof( unicap_data_buffer_t ) ); */
/*       ugtk->still_image_buffer.data = g_malloc( buffer->format.buffer_size ); */
/*       ugtk->still_image_buffer.buffer_size = buffer->format.buffer_size; */
/*       unicap_copy_format( &ugtk->still_image_buffer.format, &buffer->format ); */
/*       g_memmove( ugtk->still_image_buffer.data, buffer->data, buffer->format.buffer_size ); */
   }
/*    gtk_widget_queue_draw( GTK_WIDGET(ugtk) ); */
}

/**
 * unicapgtk_video_display_set_size:
 * @ugtk: an #UnicapgtkVideoDisplay
 * @width: new width
 * @height: new height
 * 
 * Sets the output size of the display. Video data that is to be
 * displayed will be scaled to this size. 
 */
void unicapgtk_video_display_set_size( UnicapgtkVideoDisplay *ugtk, 
				       gint width, gint height )
{
   ugtk->display_width = width;
   ugtk->display_height = height;
}

/**
 * unicapgtk_video_display_set_scale_to_fit: 
 * @ugtk: an #UnicapgtkVideoDisplay
 * @scale: TRUE if the display should scale to fit
 * 
 * If scale_to_fit is set, the video display will get scaled to the
 * size allocated to the widget. 
 */
void unicapgtk_video_display_set_scale_to_fit( UnicapgtkVideoDisplay *ugtk,
                                               gboolean               scale )
{
   g_return_if_fail( IS_UNICAPGTK_VIDEO_DISPLAY( ugtk ) );

   if( scale == ugtk->scale_to_fit )
   {
      return;
   }

   ugtk->scale_to_fit = scale ? TRUE : FALSE;

   if( ugtk->backend )
   {
      ugtk->backend_set_scale_to_fit( ugtk->backend_data, scale );
   }

   if ( ugtk->scale_to_fit )
   {
      unicapgtk_video_display_set_size( ugtk,
                                        GTK_WIDGET (ugtk->drawing_area)->allocation.width,
                                        GTK_WIDGET (ugtk->drawing_area)->allocation.height );
      gtk_widget_set_size_request( GTK_WIDGET( ugtk->drawing_area ), 1, 1 );
      gtk_aspect_frame_set( GTK_ASPECT_FRAME( ugtk ), 0.5, 0.5, (float)ugtk->crop_width / (float)ugtk->crop_height, FALSE );
   }
   else
   {
      gtk_widget_set_size_request( GTK_WIDGET( ugtk->drawing_area ),
                                   ugtk->crop_width, ugtk->crop_height );
      gtk_aspect_frame_set( GTK_ASPECT_FRAME( ugtk ), 0.5, 0.5, 1.0, TRUE );
      unicapgtk_video_display_set_size( ugtk,
					ugtk->crop_width, ugtk->crop_height );
   }

   

   g_object_notify( G_OBJECT( ugtk ), "scale-to-fit" );
}


static gboolean da_realize_cb( GtkWidget *da, UnicapgtkVideoDisplay *ugtk )
{
   GdkGCValues values;
   GdkColormap *colormap;

   set_backend( ugtk );

   if( ugtk->still_image_buffer.data )
   {
      if( ugtk->backend )
      {
	 unicapgtk_video_display_set_still_image( ugtk, &ugtk->still_image_buffer );
      }

      g_free( ugtk->still_image_buffer.data );
      ugtk->still_image_buffer.data = NULL;
   }

   values.foreground.red = 0;
   values.foreground.green = 0;
   values.foreground.blue = 0;

   colormap = gtk_widget_get_colormap( ugtk->drawing_area );
   gdk_rgb_find_color( colormap, &values.foreground );

   ugtk->gc = gdk_gc_new_with_values( da->window,
				      &values, GDK_GC_FOREGROUND );



   return FALSE;
   
}


static void set_backend( UnicapgtkVideoDisplay *ugtk )
{
   int old_running = ugtk->capture_running;
/*    printf( "sb\n" ); */
   if( ugtk->capture_running ){
      unicap_unregister_callback( ugtk->unicap_handle, UNICAP_EVENT_NEW_FRAME );
   }
   if( ugtk->backend )
   {
      ugtk->backend_destroy( ugtk->backend_data );
      ugtk->backend = NULL;
   }
/*    printf( "backend = NULL\n" ); */
   ugtk->backend_destroy = NULL;
   ugtk->backend_update_image = NULL;
   ugtk->backend_redraw = NULL;
   ugtk->backend_set_output_size = NULL;
   ugtk->backend_expose = NULL;

   if( ! GTK_WIDGET_REALIZED( ugtk ))
   {
      if( old_running ){
	 unicap_register_callback( ugtk->unicap_handle, UNICAP_EVENT_NEW_FRAME, (unicap_callback_t)new_frame_cb, ugtk );
      }
/*       printf( "--setbackend (!realized)\n" ); */
      return;
   }

   if( (ugtk->requested_backend == BACKEND_GTK ) || !backend_xv_init( ugtk->drawing_area, &ugtk->format, ugtk->backend_fourcc, 
								      &ugtk->backend_data, NULL ) )
   {
      if( ( ugtk->requested_backend == BACKEND_XV ) || !backend_gtk_init( ugtk->drawing_area, &ugtk->format, &ugtk->backend_data, NULL ) )
      {
	 if( old_running ){
	    unicap_register_callback( ugtk->unicap_handle, UNICAP_EVENT_NEW_FRAME, (unicap_callback_t)new_frame_cb, ugtk );
	 }
/* 	 printf( "--setbackend (!initialized)\n" );	  */
	 return;
      }
      else
      {
	 ugtk->backend_destroy = backend_gtk_destroy;
	 ugtk->backend_update_image = backend_gtk_update_image;
	 ugtk->backend_redraw = backend_gtk_redraw;
	 ugtk->backend_size_allocate = backend_gtk_size_allocate;
	 ugtk->backend_set_crop = backend_gtk_set_crop;
	 ugtk->backend_expose = backend_gtk_expose_event;
	 ugtk->backend_get_image_data = backend_gtk_get_image_data;
	 ugtk->backend_lock = backend_gtk_lock;
	 ugtk->backend_unlock = backend_gtk_unlock;
	 ugtk->backend_display_image = backend_gtk_display_image;
	 ugtk->backend_set_scale_to_fit = backend_gtk_set_scale_to_fit;
	 ugtk->backend_set_pause_state = backend_gtk_set_pause_state;
	 ugtk->backend_get_flags = backend_gtk_get_flags;
	 ugtk->backend_set_color_conversion_callback = backend_gtk_set_color_conversion_callback;
	 ugtk->backend = "gtk";
	 ugtk->backend_fourcc = 0;
      }
   }
   else
   {
      ugtk->backend_destroy = backend_xv_destroy;
      ugtk->backend_update_image = backend_xv_update_image;
      ugtk->backend_redraw = backend_xv_redraw;
      ugtk->backend_size_allocate = backend_xv_size_allocate;
      ugtk->backend_set_crop = backend_xv_set_crop;
      ugtk->backend_expose = backend_xv_expose_event;
      ugtk->backend_get_image_data = backend_xv_get_image_data;
      ugtk->backend_lock = backend_xv_lock;
      ugtk->backend_unlock = backend_xv_unlock;
      ugtk->backend_display_image = backend_xv_display_image;
      ugtk->backend_set_scale_to_fit = backend_xv_set_scale_to_fit;
      ugtk->backend_set_pause_state = backend_xv_set_pause_state;
      ugtk->backend_get_flags = backend_xv_get_flags;
      ugtk->backend_set_color_conversion_callback = backend_xv_set_color_conversion_callback;
      ugtk->backend = "xv";
   }
   
   ugtk->backend_set_scale_to_fit( ugtk->backend_data, ugtk->scale_to_fit );
   ugtk->backend_set_color_conversion_callback( ugtk->backend_data, ugtk->color_conversion_cb, ugtk->color_conversion_data );

   g_object_notify( G_OBJECT( ugtk ), "backend" );

   if( old_running ){
      unicap_register_callback( ugtk->unicap_handle, UNICAP_EVENT_NEW_FRAME, (unicap_callback_t)new_frame_cb, ugtk );
   }

/*    printf( "--setbackend\n" ); */
}

/**
 * unicapgtk_video_display_set_new_frame_callback:
 * @ugtk: an #UnicapgtkVideoDisplay
 * @flags: flags
 * @cb: a callback function
 * @data: a pointer to user data that is passed to the callback
 * function
 * 
 * Registers a callback function which is called in the unicap capture
 * thread context. 
 *
 * Returns: old callback 
 */
unicap_new_frame_callback_t unicapgtk_video_display_set_new_frame_callback( UnicapgtkVideoDisplay *ugtk, 
									    UnicapgtkCallbackFlags flags,  
									    unicap_new_frame_callback_t cb, void *data )
{
   unicap_new_frame_callback_t old_cb = NULL;
   
   if( flags & UNICAPGTK_CALLBACK_FLAGS_XV_CB )
   {
      if( ugtk->backend )
      {
	 old_cb = backend_xv_set_new_frame_callback( ugtk->backend_data, cb, ugtk->unicap_handle, data );
      }
   }
   else
   {
      old_cb = ugtk->new_frame_callback;
      
      ugtk->new_frame_callback = NULL;
      ugtk->new_frame_callback_data = data;
      
      ugtk->new_frame_callback = cb;
      ugtk->new_frame_callback_flags = flags;
   }
   
   return old_cb;
}

unicapgtk_color_conversion_callback_t unicapgtk_video_display_set_color_conversion_callback( UnicapgtkVideoDisplay *ugtk, 
											     unicapgtk_color_conversion_callback_t cb, 
											     void *data )
{
   unicapgtk_color_conversion_callback_t old_cb = ugtk->color_conversion_cb;
   
   if( ugtk->backend )
   {
      ugtk->backend_lock( ugtk->backend_data );
   }
   ugtk->color_conversion_cb = cb;
   ugtk->color_conversion_data = data;
   if( ugtk->backend )
   {
      ugtk->backend_set_color_conversion_callback( ugtk->backend_data, ugtk->color_conversion_cb, ugtk->color_conversion_data );
      ugtk->backend_unlock( ugtk->backend_data );
   }
   
   return old_cb;
}
