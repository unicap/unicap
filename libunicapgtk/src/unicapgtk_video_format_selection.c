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

#include <stdlib.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <string.h>

#include "config.h"

#if UNICAPGTK_DEBUG
#define DEBUG
#endif
#include "debug.h"

#include "unicapgtk_video_format_selection.h"


enum {
   UNICAPGTK_VIDEO_FORMAT_CHANGED_SIGNAL,
   LAST_SIGNAL
};

static void unicapgtk_video_format_selection_class_init          (UnicapgtkVideoFormatSelectionClass *klass);
static void unicapgtk_video_format_selection_init                (UnicapgtkVideoFormatSelection      *ugtk);
static void unicapgtk_video_format_selection_destroy             ( GtkObject     *object );
static void format_id_changed_cb                                 (GtkWidget *combo_box, gpointer *user_data );
static void size_changed_cb                                      (GtkWidget *combo_box, gpointer *user_data );

static guint unicapgtk_video_format_selection_signals[LAST_SIGNAL] = { 0 };


static unicap_rect_t standard_sizes[] = 
{
   { 0,0, 320, 240 }, 
   { 0,0, 384, 288 }, 
   { 0,0, 640, 480 }, 
   { 0,0, 720, 480 }, 
   { 0,0, 720, 576 }, 
   { 0,0, 768, 576 }, 
   { 0,0, 1024, 720 },
   { 0,0, 1360, 1024 }
};
   

GType
unicapgtk_video_format_selection_get_type (void)
{
   static GType ugtk_type = 0;

   if( !ugtk_type )
   {
      static const GTypeInfo ugtk_info =
	 {
	    sizeof( UnicapgtkVideoFormatSelectionClass ),
	    NULL, /* base_init */
	    NULL, /* base_finalize */
	    ( GClassInitFunc ) unicapgtk_video_format_selection_class_init,
	    NULL, /* class_finalize */
	    NULL, /* class_data */
	    sizeof( UnicapgtkVideoFormatSelection ),
	    0,
	    ( GInstanceInitFunc ) unicapgtk_video_format_selection_init,
	 };
	  
      ugtk_type = g_type_register_static( GTK_TYPE_HBOX, "UnicapgtkVideo_Format_Selection", &ugtk_info, 0 );
   }

   return ugtk_type;
}

static void unicapgtk_video_format_selection_class_init( UnicapgtkVideoFormatSelectionClass *klass )
{
   GtkObjectClass *gtk_object_class = GTK_OBJECT_CLASS( klass );

   gtk_object_class->destroy  = unicapgtk_video_format_selection_destroy;

   unicapgtk_video_format_selection_signals[UNICAPGTK_VIDEO_FORMAT_CHANGED_SIGNAL] = 
      g_signal_new( "unicapgtk_video_format_changed",
		    G_TYPE_FROM_CLASS(klass),
		    G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		    G_STRUCT_OFFSET(UnicapgtkVideoFormatSelectionClass, unicapgtk_video_format_selection),
		    NULL, 
		    NULL,                
		    g_cclosure_marshal_VOID__POINTER,
		    G_TYPE_NONE, 1, 
		    G_TYPE_POINTER );
}

static void unicapgtk_video_format_selection_destroy             ( GtkObject     *object )
{
   UnicapgtkVideoFormatSelection *ugtk = UNICAPGTK_VIDEO_FORMAT_SELECTION( object );
	
   if( ugtk->unicap_handle )
   {
      unicap_close( ugtk->unicap_handle );
      ugtk->unicap_handle = NULL;
   }
}


static void unicapgtk_video_format_selection_init( UnicapgtkVideoFormatSelection *ugtk )
{
   gtk_box_set_homogeneous( GTK_BOX( ugtk ), FALSE);
   gtk_box_set_spacing( GTK_BOX( ugtk ), 6);
   ugtk->fourcc_menu = gtk_combo_box_new_text( );
   gtk_widget_show( ugtk->fourcc_menu );
   gtk_box_pack_start( GTK_BOX( ugtk ), ugtk->fourcc_menu, TRUE, TRUE, 0);

	
   ugtk->unicap_handle = NULL;
  
}

static void new( UnicapgtkVideoFormatSelection *ugtk )
{
   unicap_format_t format;
   int i, sel = 0;
	
   /*
     Get current video format
   */
   if( !SUCCESS( unicap_get_format( ugtk->unicap_handle, &ugtk->format ) ) )
   {
      TRACE( "Failed to get video format\n" );
      unicap_void_format( &ugtk->format );
   }
/*    else */
/*    { */
/*       // Set format since some cameras are not initialized properly.  */
/*       unicap_set_format( ugtk->unicap_handle, &ugtk->format ); */
/*    } */

   if( ugtk->size_menu )
   {
      gtk_widget_destroy( ugtk->size_menu );
      ugtk->size_menu = 0;
   }
   if( ugtk->fourcc_menu )
   {
      gtk_widget_destroy( ugtk->fourcc_menu );
      ugtk->fourcc_menu = 0;
   }
	
   ugtk->fourcc_menu = gtk_combo_box_new_text();
   gtk_box_pack_start( GTK_BOX( ugtk ), ugtk->fourcc_menu, TRUE, TRUE, 0 );
   gtk_widget_show( ugtk->fourcc_menu );

   // format identifier menu
   for( i = 0; SUCCESS( unicap_enumerate_formats( ugtk->unicap_handle, 0, &format, i ) ); i++ )
   {
      gtk_combo_box_append_text( GTK_COMBO_BOX( ugtk->fourcc_menu ), format.identifier );
		
      if( !strcmp( ugtk->format.identifier, format.identifier ) )
      {
	 sel = i;
      }
   }
   if( i == 0 )
   {
      gtk_combo_box_append_text( GTK_COMBO_BOX( ugtk->fourcc_menu ), "N/A" );
      gtk_widget_set_sensitive( ugtk->fourcc_menu, FALSE );
   }
   else
   {
      gtk_widget_set_sensitive( ugtk->fourcc_menu, TRUE );
   }

   g_signal_connect( G_OBJECT( ugtk->fourcc_menu ), 
		     "changed", 
		     ( GCallback ) format_id_changed_cb, 
		     ( gpointer ) ugtk );	

   gtk_combo_box_set_active( GTK_COMBO_BOX( ugtk->fourcc_menu ), sel );
}


GtkWidget* unicapgtk_video_format_selection_new( )
{
  return g_object_new( UNICAPGTK_TYPE_VIDEO_FORMAT_SELECTION, NULL );
}

GtkWidget *unicapgtk_video_format_selection_new_by_handle( unicap_handle_t handle )
{
   UnicapgtkVideoFormatSelection *ugtk;
	
   ugtk = g_object_new( UNICAPGTK_TYPE_VIDEO_FORMAT_SELECTION, NULL );
	
   ugtk->unicap_handle = unicap_clone_handle( handle );
	
   new( ugtk );

   return GTK_WIDGET( ugtk );
}

GtkWidget *unicapgtk_video_format_selection_new_by_device( unicap_device_t *device_spec )
{
   UnicapgtkVideoFormatSelection *ugtk;
   unicap_device_t device;
	
   ugtk = g_object_new( UNICAPGTK_TYPE_VIDEO_FORMAT_SELECTION, NULL );
	
   if( SUCCESS( unicap_enumerate_devices( device_spec, &device, 0 ) ) )
   {
      unicap_open( &ugtk->unicap_handle, &device );
   }

   if( ugtk->unicap_handle )
   {
      new( ugtk );
   }
	
   return GTK_WIDGET( ugtk );
}


unicap_device_t *unicapgtk_video_format_selection_get_device( UnicapgtkVideoFormatSelection *ugtk )
{
   return &ugtk->device;
}

unicap_handle_t unicapgtk_video_format_selection_get_handle( UnicapgtkVideoFormatSelection *ugtk )
{
   return unicap_clone_handle( ugtk->unicap_handle );
}

void unicapgtk_video_format_selection_set_handle( UnicapgtkVideoFormatSelection *ugtk, unicap_handle_t handle )
{
   if( ugtk->unicap_handle )
   {
      unicap_close( ugtk->unicap_handle );
      ugtk->unicap_handle = NULL;
   }
	
   if( handle )
   {
      ugtk->unicap_handle = unicap_clone_handle( handle );

      if( ugtk->unicap_handle )
      {
	 new( ugtk );
      }
   }
}

gboolean unicapgtk_video_format_selection_set_device( UnicapgtkVideoFormatSelection *ugtk, unicap_device_t *device_spec )
{
   unicap_device_t device;
   unicap_status_t status;

   if( ugtk->unicap_handle )
   {
      unicap_close( ugtk->unicap_handle );
      ugtk->unicap_handle = 0;
   }

   status = unicap_enumerate_devices( device_spec, &device, 0 );
   if( SUCCESS( status ) )
   {
      status = unicap_open( &ugtk->unicap_handle, &device );
   }

   if( ugtk->unicap_handle )
   {
      new( ugtk );
   }

   return SUCCESS( status );
}


unicap_format_t *unicapgtk_video_format_selection_get_format( UnicapgtkVideoFormatSelection *ugtk )
{
   return &ugtk->format;
}

//
// +++ Callbacks +++
//

static GtkWidget *create_size_combo( )
{
   GtkListStore *store;
   GtkCellRenderer *cell;
   GtkWidget *combo;

   combo = gtk_combo_box_new( );

   store = gtk_list_store_new( 3, 
			       G_TYPE_STRING, // Format description
			       G_TYPE_INT,    // Width
			       G_TYPE_INT );  // Height
   gtk_combo_box_set_model( GTK_COMBO_BOX( combo ), GTK_TREE_MODEL( store ) );
   g_object_unref( store );

   cell = g_object_new( GTK_TYPE_CELL_RENDERER_TEXT, 
			"xalign", 0.0, NULL );
   gtk_cell_layout_pack_start( GTK_CELL_LAYOUT( combo ), cell, TRUE );
   gtk_cell_layout_set_attributes( GTK_CELL_LAYOUT (combo), cell,
				   "text", 0, NULL);
   
   return combo;
}

static void size_combo_add( GtkWidget *combo, int width, int height )
{
   GtkTreeIter iter;
   GtkListStore *store;
   gchar string[64];
   
   g_snprintf( string, sizeof( string ), "%d x %d", width, height );

   store = GTK_LIST_STORE( gtk_combo_box_get_model( GTK_COMBO_BOX( combo ) ) );
   
   gtk_list_store_append( store, &iter );
   gtk_list_store_set( store, &iter, 
		       0, string, 
		       1, width, 
		       2, height, 
		       -1 );	       
}

static void format_id_changed_cb( GtkWidget *combo_box, gpointer *user_data )
{
   UnicapgtkVideoFormatSelection *ugtk = ( UnicapgtkVideoFormatSelection *)user_data;
   unicap_format_t format, spec;
   int sel = -1;
   gchar *id;
	
   if( ugtk->size_menu )
   {
      gtk_widget_destroy( ugtk->size_menu );
   }

   unicap_void_format( &spec );
   id = gtk_combo_box_get_active_text( GTK_COMBO_BOX( combo_box ) );

   g_return_if_fail(id);
		
   strcpy( spec.identifier, id );

   unicap_enumerate_formats( ugtk->unicap_handle, &spec, &format, 0 );	

   if( format.size_count )
   {
      int i;

      ugtk->size_menu = create_size_combo( );
      gtk_box_pack_start( GTK_BOX( ugtk ), ugtk->size_menu, 0, 0, 2 );
      gtk_widget_show( ugtk->size_menu );
		
      for( i = 0; i < format.size_count; i++ )
      {
	 size_combo_add( ugtk->size_menu, format.sizes[i].width, format.sizes[i].height );

	 if( ( ugtk->format.size.width == format.sizes[i].width ) &&
	     ( ugtk->format.size.height == format.sizes[i].height ) )
	 {
	    sel = i;
	 }			
      }
		
      g_signal_connect( G_OBJECT( ugtk->size_menu ), 
			"changed", 
			( GCallback ) size_changed_cb, 
			( gpointer ) ugtk );
		
      if( sel != -1 )
      {
	 gtk_combo_box_set_active( GTK_COMBO_BOX( ugtk->size_menu ), sel );
/* 			size_changed_cb( ugtk->size_menu, ugtk ); */			
      }
   }
   else if( ( format.min_size.width < format.max_size.width ) && 
	    ( format.min_size.height < format.max_size.height ) )
   {
      int i;
      int row = 0;

      ugtk->size_menu = create_size_combo( );
      gtk_box_pack_start( GTK_BOX( ugtk ), ugtk->size_menu, 0, 0, 2 );
      gtk_widget_show( ugtk->size_menu );
		
      for( i = 0; i < G_N_ELEMENTS( standard_sizes ); i++ )
      {
	 if( ( standard_sizes[i].width >= format.min_size.width ) &&
	     ( standard_sizes[i].width < format.max_size.width ) && 
	     ( standard_sizes[i].height >= format.min_size.height ) &&
	     ( standard_sizes[i].height < format.max_size.height ) )
	 {
	    size_combo_add( ugtk->size_menu, standard_sizes[i].width, standard_sizes[i].height );

            if( ( ugtk->format.size.width == standard_sizes[i].width ) &&
                ( ugtk->format.size.height == standard_sizes[i].height ) )
              {
                sel = row;
              }

            row++;
	 }
      }

      size_combo_add( ugtk->size_menu, format.max_size.width, format.max_size.height );

      if( sel < 0 )
      {
          sel = row;
      }
      row++;
		
      g_signal_connect( G_OBJECT( ugtk->size_menu ), 
			"changed", 
			( GCallback ) size_changed_cb, 
			( gpointer ) ugtk );
		
      if( sel != -1 )
      {
	 gtk_combo_box_set_active( GTK_COMBO_BOX( ugtk->size_menu ), sel );
      }
      
   }
   else
   {
      // No size count and no size range
      g_signal_emit( G_OBJECT( ugtk ), 
		     unicapgtk_video_format_selection_signals[ UNICAPGTK_VIDEO_FORMAT_CHANGED_SIGNAL ], 
		     0, 
		     (gpointer)&format );
   }
}

static void size_changed_cb( GtkWidget  *combo_box, gpointer *user_data )
{
   UnicapgtkVideoFormatSelection *ugtk = ( UnicapgtkVideoFormatSelection *)user_data;

   unicap_format_t format;
   unicap_format_t real_format;
   gchar *id;
   gint width, height;
   GtkTreeIter iter;
   

   unicap_void_format( &format );

   id = gtk_combo_box_get_active_text( GTK_COMBO_BOX( ugtk->fourcc_menu ) );
   g_assert( id );

   strcpy( format.identifier, id );

   if( gtk_combo_box_get_active_iter( GTK_COMBO_BOX( combo_box ), &iter ) )
   {
      GtkTreeModel *model = gtk_combo_box_get_model( GTK_COMBO_BOX( combo_box ) );
      gtk_tree_model_get( model, &iter, 
			      1, &width, 
			      2, &height, 
			      -1 );
      
      format.size.width = width;
      format.size.height = height;
      TRACE( "size changed : %dx%d [%s]\n", format.size.width, format.size.height, id );

      if( SUCCESS( unicap_enumerate_formats( ugtk->unicap_handle, &format, &real_format, 0 ) ) )
      {
	 memcpy( &ugtk->format, &format, sizeof( unicap_format_t ) );
	 real_format.size.width = width;
	 real_format.size.height = height;
	 g_signal_emit( G_OBJECT( ugtk ), 
			unicapgtk_video_format_selection_signals[ UNICAPGTK_VIDEO_FORMAT_CHANGED_SIGNAL ], 
			0, 
			(gpointer)&real_format );
      }
      else
      {
	 TRACE( "enumerate failed\n" );
      }
   }
}
