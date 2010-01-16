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
#include <string.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <libintl.h>
#include <ctype.h>

#include "config.h"

#include "unicapgtk.h"
#include "unicapgtk_priv.h"
#include "unicapgtk_property_dialog.h"
#include "unicapgtk_device_property.h"

#if UNICAPGTK_DEBUG
#define DEBUG
#endif
#include "debug.h"

enum
{
   PROP_0 = 0,
   PROP_UPDATE_INTERVAL
};

enum {
   UNICAPGTK_PROPERTY_DIALOG_CHANGED_SIGNAL,
   LAST_SIGNAL
};

struct ppty_relation
{
      unicap_property_t property;
      GtkWidget *widget;
};



static void unicapgtk_property_dialog_class_init          (UnicapgtkPropertyDialogClass *klass);
static void unicapgtk_property_dialog_init                (UnicapgtkPropertyDialog      *ugtk);
static void unicapgtk_property_dialog_finalize            (GObject                      *object);
static void unicapgtk_property_dialog_destroy             (GtkObject                    *object);
static void unicapgtk_property_dialog_set_property        ( GObject       *object, 
							    guint          property_id, 
							    const GValue  *value, 
							    GParamSpec    *pspec );
static void unicapgtk_property_dialog_get_property        ( GObject       *object, 
							    guint          property_id, 
							    GValue        *value, 
							    GParamSpec    *pspec );

static gboolean property_update_timeout_cb( UnicapgtkPropertyDialog *ugtk );
static void update_lock_destroy_cb( gpointer _id );
static gboolean save_device_defaults( UnicapgtkPropertyDialog *ugtk, gboolean overwrite );
static void load_device_defaults( UnicapgtkPropertyDialog *ugtk );
static void apply_property_filter( UnicapgtkPropertyDialog *ugtk );


/* static guint unicapgtk_property_dialog_signals[LAST_SIGNAL] = { 0 }; */


G_DEFINE_TYPE( UnicapgtkPropertyDialog, unicapgtk_property_dialog, GTK_TYPE_DIALOG );


static gchar *uri_escape_string (const gchar *unescaped)
{
   static const gchar hex[16] = "0123456789ABCDEF";
   
   GString    *string;
   const char *end;
   guchar      c;
   
   g_return_val_if_fail (unescaped != NULL, NULL);

   string = g_string_sized_new (strlen (unescaped));
   
   end = unescaped + strlen (unescaped);
   
   while ((c = *unescaped) != 0)
   {
      if (g_ascii_isalnum (c) ||
          c == '-' ||
          c == '.' ||
          c == '_' ||
          c == '~')
      {
         g_string_append_c (string, c);
         unescaped++;
      }
      else
      {
         g_string_append_c (string, '%');
         g_string_append_c (string, hex[((guchar)c) >> 4]);
         g_string_append_c (string, hex[((guchar)c) & 0xf]);
         unescaped++;
      }
   }
   
   return g_string_free (string, FALSE);
}





static void unicapgtk_property_dialog_class_init( UnicapgtkPropertyDialogClass *klass )
{
   GObjectClass *object_class = G_OBJECT_CLASS( klass );
   GtkObjectClass *gtk_object_class = GTK_OBJECT_CLASS( klass );

   unicapgtk_initialize();

   object_class->finalize = unicapgtk_property_dialog_finalize;
   gtk_object_class->destroy = unicapgtk_property_dialog_destroy;

   object_class->set_property = unicapgtk_property_dialog_set_property;
   object_class->get_property = unicapgtk_property_dialog_get_property;

/*    unicapgtk_property_dialog_signals[UNICAPGTK_PROPERTY_DIALOG_CHANGED_SIGNAL] =  */
/*       g_signal_new( "unicapgtk_property_dialog_changed", */
/* 		    G_TYPE_FROM_CLASS(klass), */
/* 		    G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION, */
/* 		    G_STRUCT_OFFSET(UnicapgtkPropertyDialogClass, unicapgtk_property_dialog), */
/* 		    NULL,  */
/* 		    NULL,                 */
/* 		    g_cclosure_marshal_VOID__POINTER, */
/* 		    G_TYPE_NONE, 1,  */
/* 		    G_TYPE_POINTER ); */

   g_object_class_install_property (object_class,
				    PROP_UPDATE_INTERVAL,
				    g_param_spec_int ("update-interval",
						      NULL, NULL,
						      0, 99999,
						      100,
						      G_PARAM_READWRITE ));

}

static void unicapgtk_property_dialog_set_property( GObject *object, 
						    guint property_id, 
						    const GValue *value, 
						    GParamSpec *pspec )
{
   UnicapgtkPropertyDialog *ugtk = UNICAPGTK_PROPERTY_DIALOG( object );

   switch( property_id )
   {
      case PROP_UPDATE_INTERVAL:
	 ugtk->update_interval = g_value_get_int( value );
         if( ugtk->update_timeout_id )
         {
            g_source_remove( ugtk->update_timeout_id );
            ugtk->update_timeout_id = 0;
         }
	 if( ugtk->update_interval > 0 )
	 {
	    g_timeout_add( ugtk->update_interval, (GSourceFunc)property_update_timeout_cb, ugtk );
	 }
	 break;

      default:
	 G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, pspec );
	 break;
   }
}

static void unicapgtk_property_dialog_get_property( GObject *object, 
						    guint property_id, 
						    GValue *value, 
						    GParamSpec *pspec )
{
   UnicapgtkPropertyDialog *ugtk = UNICAPGTK_PROPERTY_DIALOG( object );

   switch( property_id )
   {
      case PROP_UPDATE_INTERVAL:
         g_value_set_int( value, ugtk->update_interval );
         break;

      default:
	 G_OBJECT_WARN_INVALID_PROPERTY_ID( object, property_id, pspec );
	 break;
   }
}

static void unicapgtk_property_dialog_finalize( GObject *object )
{
   UnicapgtkPropertyDialog *ugtk = (UnicapgtkPropertyDialog *)object;

   g_list_foreach( ugtk->size_groups, (GFunc)g_object_unref, NULL );
   g_list_free( ugtk->size_groups );

   G_OBJECT_CLASS( unicapgtk_property_dialog_parent_class )->finalize( object );
}

static void unicapgtk_property_dialog_destroy( GtkObject *object )
{
   UnicapgtkPropertyDialog *ugtk = (UnicapgtkPropertyDialog *)object;
   GList *entry;

   for( entry = ugtk->property_list; entry; entry = entry->next )
   {
      struct ppty_relation *rel;
      guint timeout_id = 0;

      rel = (struct ppty_relation *) entry->data;
      

      timeout_id = (guint)g_object_get_data( G_OBJECT( rel->widget ), "update lock" );
      if( timeout_id )
      {
	 g_source_remove( timeout_id );
      }
      
      g_free( entry->data );
   }

   g_list_free( ugtk->property_list );
   ugtk->property_list = NULL;
   ugtk->update_entry = NULL;

   if( ugtk->unicap_handle )
   {
      unicap_close( ugtk->unicap_handle );
      ugtk->unicap_handle = NULL;
   }

   GTK_OBJECT_CLASS( unicapgtk_property_dialog_parent_class )->destroy( object );
}


static void format_label( gchar *string, gint maxlen )
{
   gchar *c;
   
   for( c = string; c != NULL; c = g_strstr_len( c, maxlen, " " ) )
   {
      if( *c == ' ' )
      {
	 c++;
      }
      
      if( *c )
      {
	 *c = g_ascii_toupper( *c );
      }
   }
}


static gboolean remove_update_lock_timeout_cb( GtkWidget *widget )
{
   g_object_set_data( G_OBJECT( widget ), "update lock", NULL );
   
   return FALSE;
}


static gboolean property_update_timeout_cb( UnicapgtkPropertyDialog *ugtk )
{
   struct ppty_relation *rel;

   if( !ugtk->update_entry )
   {
      return TRUE;
   }
   
   rel = ugtk->update_entry->data;

   if( !g_object_get_data( G_OBJECT( rel->widget ), "update lock" ) )
   {
      unicapgtk_device_property_update( UNICAPGTK_DEVICE_PROPERTY( rel->widget ) );
   }
   
   if( ugtk->update_entry->next )
   {
      ugtk->update_entry = ugtk->update_entry->next;
   }
   else
   {
      ugtk->update_entry = g_list_first( ugtk->property_list );
   }

   return TRUE;
}

static void update_lock_destroy_cb( gpointer _id )
{
   guint id = ( guint ) id;
   
   if( id )
   {
      g_source_remove( id );
   }
}

static void property_changed_cb( GtkWidget *widget, unicap_property_t *property, UnicapgtkPropertyDialog *ugtk )
{
   int i;
   guint timeout_id = 0;

   timeout_id = (guint)g_object_get_data( G_OBJECT( widget ), "update lock" );
   if( timeout_id )
   {
      g_source_remove( timeout_id );
   }
   
   // block the update of this property for the next 5 seconds to
   // avoid confusion when the property takes some time to reflect the
   // new setting ( eg. zoom )
   timeout_id = g_timeout_add( 5000, (GSourceFunc)remove_update_lock_timeout_cb, widget );
   g_object_set_data_full( G_OBJECT( widget ), "update lock", (gpointer)timeout_id, update_lock_destroy_cb );
   
   unicap_set_property( ugtk->unicap_handle, property );
 
   for( i = 0; i < property->relations_count; i++ )
   {
      GList *entry;

      entry = g_list_find_custom( ugtk->property_list, property->relations[i], (GCompareFunc)strcmp );
      if( entry )
      {
	 struct ppty_relation *rel = (struct ppty_relation *)entry->data;
	 unicapgtk_device_property_update( UNICAPGTK_DEVICE_PROPERTY( rel->widget ) );
      }
   }
}

static GtkWidget *create_category_vbox( UnicapgtkPropertyDialog *ugtk, unicap_handle_t unicap_handle, char *category )
{
   unicap_property_t ppty_spec, ppty;

   GtkWidget *vbox;

   GtkWidget *widget;
   GtkWidget *label;
   gint i;
   GtkSizeGroup *size_group;

   vbox = gtk_vbox_new( FALSE, 6 );
   gtk_container_set_border_width( GTK_CONTAINER( vbox ), 12 );

   unicap_void_property( &ppty_spec );
   strcpy( ppty_spec.category, category );
   
   
   size_group = gtk_size_group_new( GTK_SIZE_GROUP_HORIZONTAL );

   for( i = 0; SUCCESS( unicap_enumerate_properties( unicap_handle, 
						     &ppty_spec, 
						     &ppty, 
						     i ) ); i++ )
   {
      struct ppty_relation *rel;
	   
      widget = unicapgtk_device_property_new_by_handle( unicap_handle, &ppty );
      gtk_box_pack_start( GTK_BOX( vbox ), widget,FALSE, FALSE, 0 );
      g_signal_connect( G_OBJECT( widget ),
			"unicapgtk_device_property_changed",
			G_CALLBACK( property_changed_cb ),
			ugtk );
      label = unicapgtk_device_property_get_label( UNICAPGTK_DEVICE_PROPERTY( widget ) );
      if (label)
      {
	 gtk_size_group_add_widget( size_group, label );
      }
	   
      rel = g_malloc( sizeof( struct ppty_relation ) );
      unicap_copy_property( &rel->property, &ppty );
      rel->widget = widget;
      ugtk->property_list = g_list_append( ugtk->property_list, rel );
   }

   ugtk->size_groups = g_list_append( ugtk->size_groups, size_group );

   ugtk->update_entry = g_list_first( ugtk->property_list );

   return vbox;
}

static void append_pages( UnicapgtkPropertyDialog *ugtk, unicap_handle_t unicap_handle )
{
   unicap_property_t property;
   GList *categories = NULL;
   GList *entry;
   gchar *label;
   int i;
   
   for( i = 0; SUCCESS( unicap_enumerate_properties( unicap_handle, NULL, &property, i ) ); i++ )
   {
      if( !g_list_find_custom( categories, property.category, (GCompareFunc)strcmp ) )
      {
	 categories = g_list_append( categories, g_strdup(property.category) );
      }
   }
   
   for( entry = g_list_first( categories ); entry; entry=entry->next )
   {

      label = g_strdup( dgettext( GETTEXT_PACKAGE, entry->data ) );
      format_label( label, 128 );

      gtk_notebook_append_page( GTK_NOTEBOOK( ugtk->notebook ),
                                create_category_vbox( ugtk, unicap_handle, entry->data ),
				gtk_label_new( label ) );
      g_free( label );
   }  

   g_list_foreach( categories, (GFunc)g_free, NULL );
   g_list_free( categories );

   gtk_widget_show_all( ugtk->notebook );
}

static gboolean save_device_defaults( UnicapgtkPropertyDialog *ugtk, gboolean overwrite )
{
   gchar *config_path;
   gchar *ini_path;
   gchar *ini_filename;
   unicap_device_t device;
   GKeyFile *keyfile;
   gboolean result = FALSE;
   gchar *tmp;
   
   if( !ugtk->unicap_handle )
   {
      return FALSE;
   }

   if( !SUCCESS( unicap_get_device( ugtk->unicap_handle, &device ) ) )
   {
      return FALSE;
   }

   config_path = unicapgtk_get_user_config_path();
   if( !config_path )
   {
      return FALSE;
   }
   
   tmp = g_strconcat( device.identifier, ".ini", NULL );
   ini_filename = uri_escape_string( tmp );
   ini_path = g_build_path( G_DIR_SEPARATOR_S, config_path, ini_filename, NULL );
   g_free( tmp );
   g_free( ini_filename );
   g_free( config_path );
   
   if( !overwrite )
   {
      if( g_file_test( ini_path, G_FILE_TEST_EXISTS ) )
      {
	 g_free( ini_path );
	 return FALSE;
      }
   }
   
   keyfile = unicapgtk_save_device_state( ugtk->unicap_handle, UNICAPGTK_DEVICE_STATE_PROPERTIES );
   if( keyfile )
   {
      FILE *f;
      gchar *data;
      gsize size;
      
      data = g_key_file_to_data( keyfile, &size, NULL );

      f = fopen( ini_path, "w" );
      if( f )
      {
	 int ignore;
	 ignore = fwrite( data, size, 1, f );
	 fclose( f );

	 result = TRUE;
      }
      
      g_free( data );
      g_key_file_free( keyfile );
   }
   
   g_free( ini_path );

   return result;
}

static void load_device_defaults( UnicapgtkPropertyDialog *ugtk )
{
   gchar *config_path;
   gchar *ini_path;
   gchar *ini_filename;
   unicap_device_t device;
   GKeyFile *keyfile;
   gchar *tmp;
   
   if( !ugtk->unicap_handle )
   {
      return;
   }

   if( !SUCCESS( unicap_get_device( ugtk->unicap_handle, &device ) ) )
   {
      return;
   }

   config_path = unicapgtk_get_user_config_path();
   if( !config_path )
   {
      return;
   }
   
   tmp = g_strconcat( device.identifier, ".ini", NULL );
   ini_filename = uri_escape_string( tmp );
   ini_path = g_build_path( G_DIR_SEPARATOR_S, config_path, ini_filename, NULL );
   g_free( tmp );
   g_free( ini_filename );
   g_free( config_path );
   
   keyfile = g_key_file_new( );
   if( g_key_file_load_from_file( keyfile, 
				  ini_path, 
				  G_KEY_FILE_NONE, 
				  NULL ) )
   {
      unicapgtk_load_device_state( ugtk->unicap_handle, keyfile, UNICAPGTK_DEVICE_STATE_PROPERTIES );
   }

   g_free( ini_path );
   g_key_file_free( keyfile );

   // Update the display of the dialog
   unicapgtk_property_dialog_reset( ugtk );
}

static void unicapgtk_property_dialog_init( UnicapgtkPropertyDialog *ugtk )
{
   GtkWidget *button;

   ugtk->size_groups = NULL;

   gtk_dialog_set_has_separator( GTK_DIALOG( ugtk ), FALSE );

   gtk_dialog_add_button( GTK_DIALOG( ugtk ), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE );

   gtk_dialog_set_default_response( GTK_DIALOG( ugtk ), GTK_RESPONSE_CLOSE );

   g_signal_connect( ugtk,
		     "response",
		     G_CALLBACK( gtk_widget_hide ),
                     NULL );


   ugtk->notebook = gtk_notebook_new( );
   gtk_container_set_border_width( GTK_CONTAINER( ugtk->notebook ), 6 );
   gtk_box_pack_start( GTK_BOX( GTK_DIALOG( ugtk )->vbox ), ugtk->notebook, TRUE, TRUE, 0 );

   button = gtk_button_new_with_label( _("Update") );
   gtk_box_pack_start( GTK_BOX( GTK_DIALOG( ugtk )->action_area ), button, FALSE, TRUE, 0 );
   gtk_button_box_set_child_secondary (GTK_BUTTON_BOX ( GTK_DIALOG( ugtk )->action_area ),
                                       button, TRUE);
   g_signal_connect_swapped( G_OBJECT( button ), 
			     "clicked", 
			     G_CALLBACK( unicapgtk_property_dialog_reset ), 
			     ugtk );

   button = gtk_button_new_with_label( _("Defaults") );
   gtk_box_pack_start( GTK_BOX( GTK_DIALOG( ugtk )->action_area ), button, FALSE, TRUE, 0 );
   gtk_button_box_set_child_secondary (GTK_BUTTON_BOX ( GTK_DIALOG( ugtk )->action_area ),
                                       button, TRUE);
   g_signal_connect_swapped( G_OBJECT( button ), 
			     "clicked", 
			     G_CALLBACK( load_device_defaults ), 
			     ugtk );


   ugtk->unicap_handle = NULL;

   gtk_window_set_default_size ( GTK_WINDOW( ugtk ), 400, 300 );

   gtk_widget_show_all( GTK_WIDGET( ugtk ));
}


/**
 * unicapgtk_property_dialog_new:
 *
 * Creates a new #UnicapgtkPropertyDialog
 *
 * Returns: a new #UnicapgtkPropertyDialog
 */
GtkWidget *unicapgtk_property_dialog_new( void )
{
   UnicapgtkPropertyDialog *ugtk;

   ugtk = g_object_new( UNICAPGTK_TYPE_PROPERTY_DIALOG, NULL );
   ugtk->unicap_handle = 0;

   return GTK_WIDGET( ugtk );
}

/**
 * unicapgtk_property_dialog_new_by_handle:
 * @handle: a #unicap_handle_t
 *
 * Creates a new #UnicapgtkPropertyDialog with a given
 * #unicap_handle_t
 *
 * Returns: a new #UnicapgtkPropertyDialog
 */
GtkWidget* unicapgtk_property_dialog_new_by_handle( unicap_handle_t handle )
{
   UnicapgtkPropertyDialog *ugtk;
   unicap_device_t device;

   gchar title[512];

   ugtk = g_object_new( UNICAPGTK_TYPE_PROPERTY_DIALOG, NULL );

   ugtk->unicap_handle = unicap_clone_handle( handle );

   save_device_defaults( ugtk, FALSE );

   append_pages( ugtk, ugtk->unicap_handle );
   gtk_widget_show_all( ugtk->notebook );

   unicap_get_device( ugtk->unicap_handle, &device );
   sprintf( title, "%s - Properties", device.identifier );
   gtk_window_set_title( GTK_WINDOW( ugtk ), title );

   return GTK_WIDGET( ugtk );
}

/**
 * unicapgtk_property_dialog_set_device:
 * @ugtk: a #UnicapgtkPropertyDialog
 * @device_spec: the device to be controlled by this dialog
 *
 * Sets the device which should be controlled by this dialog
 */
void unicapgtk_property_dialog_set_device( UnicapgtkPropertyDialog *ugtk, unicap_device_t *device_spec )
{
   unicap_device_t device;
   gchar title[512];

   if( ugtk->unicap_handle )
   {
      unicap_close( ugtk->unicap_handle );
      ugtk->unicap_handle = 0;
   }

   if( SUCCESS( unicap_enumerate_devices( device_spec, &device, 0 ) ) )
   {
      unicap_open( &ugtk->unicap_handle, &device );
   }

   save_device_defaults( ugtk, FALSE );

   unicapgtk_property_dialog_reset( ugtk );

   sprintf( title, "%s - Properties", device.identifier );
   gtk_window_set_title( GTK_WINDOW( ugtk ), title );

}

/**
 * unicapgtk_property_dialog_set_handle:
 * @ugtk: a #UnicapgtkPropertyDialog
 * @handle: a handle of the device to be controlled by this dialog
 *
 * Sets the handle to the device to be controlled by this dialog. The
 * handle will be cloned by this call. 
 */
void unicapgtk_property_dialog_set_handle( UnicapgtkPropertyDialog *ugtk, unicap_handle_t handle )
{
   unicap_device_t device;
   gchar title[512];

   if( ugtk->unicap_handle )
   {
      unicap_close( ugtk->unicap_handle );
      ugtk->unicap_handle = 0;
   }

   if( handle )
   {
      ugtk->unicap_handle = unicap_clone_handle( handle );

      save_device_defaults( ugtk, FALSE );

      if( !SUCCESS( unicap_get_device( handle, &device ) ) )
      {
	 gtk_window_set_title( GTK_WINDOW( ugtk ), "Properties" );
	 return;
      }

      sprintf( title, "%s - Properties", device.identifier );
      gtk_window_set_title( GTK_WINDOW( ugtk ), title );
   }
   else
   {
      gtk_window_set_title( GTK_WINDOW( ugtk ), "No Device" );
   }

   unicapgtk_property_dialog_reset( ugtk );   
}

/**
 * unicapgtk_property_dialog_reset:
 * @ugtk: an #UnicapgtkPropertyDialog
 *
 * 
 */
void unicapgtk_property_dialog_reset( UnicapgtkPropertyDialog *ugtk )
{
   GList *entry;   
   gint cur_page;

   for( entry = ugtk->property_list; entry; entry = entry->next )
   {
      struct ppty_relation *rel;
      guint timeout_id = 0;

      rel = (struct ppty_relation *) entry->data;
      

      timeout_id = (guint)g_object_get_data( G_OBJECT( rel->widget ), "update lock" );
      if( timeout_id )
      {
	 g_source_remove( timeout_id );
      }
      g_free( entry->data );
   }
   
   cur_page = gtk_notebook_get_current_page( GTK_NOTEBOOK( ugtk->notebook ) );

   while( gtk_notebook_get_n_pages( GTK_NOTEBOOK( ugtk->notebook ) ) )
   {
      gtk_widget_destroy( gtk_notebook_get_nth_page( GTK_NOTEBOOK( ugtk->notebook ), 0 ) );
      gtk_notebook_remove_page( GTK_NOTEBOOK( ugtk->notebook ), 0 );
   }

   g_list_free( ugtk->property_list );
   ugtk->property_list = NULL;
   ugtk->update_entry = NULL;

   if( ugtk->unicap_handle )
   {
      unicap_reenumerate_properties( ugtk->unicap_handle, NULL );      
      
      append_pages( ugtk, ugtk->unicap_handle );
      
      gtk_widget_show_all( ugtk->notebook );
   }

   if( cur_page > 0 )
   {
      gtk_notebook_set_current_page( GTK_NOTEBOOK( ugtk->notebook ), cur_page );
   }

   apply_property_filter( ugtk );
}

static void apply_property_filter( UnicapgtkPropertyDialog *ugtk )
{
   GList *entry = NULL;

   if( !ugtk->property_filter_func )
   {
      return;
   }
   
   for( entry = ugtk->property_list; entry; entry = entry->next )
   {
      struct ppty_relation *rel;
      rel = (struct ppty_relation *) entry->data;
      
      if( ugtk->property_filter_func( &rel->property, ugtk->property_filter_user_data ) )
      {
	 gtk_widget_hide( rel->widget );
      }
      else
      {
	 gtk_widget_show( rel->widget );
      }
   }
}

void               unicapgtk_property_dialog_set_filter      ( UnicapgtkPropertyDialog *ugtk, 
							       unicap_property_filter_func_t func, 
							       gpointer user_data )
{
   ugtk->property_filter_func = func;
   ugtk->property_filter_user_data = user_data;
   
   apply_property_filter( ugtk );
}
