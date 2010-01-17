/* 
 *
 * Copyright (C) 2006 JPK Instruments AG;  used with permission
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/**
 * SECTION: UnicapgtkDeviceSelection
 * @short_description: Widget to list and select video capture devices
 */

#include "config.h"

#include <string.h>

#include <gtk/gtk.h>

#include <unicap.h>
#include <unicapgtk.h>

#if UNICAPGTK_DEBUG
#define DEBUG
#endif
#include <debug.h>

#include "unicapgtk_device_selection.h"


enum
{
   UNICAPGTK_DEVICE_SELECTION_CHANGED_SIGNAL, 
   LAST_SIGNAL
};


enum
{
   NOT_SPECIAL,
   SPECIAL_NONE,
   SPECIAL_SEPARATOR,
   SPECIAL_RESCAN
};

enum
{
   LABEL       = UNICAPGTK_DEVICE_LAST_PUBLIC_COLUMN + 1,
   SENSITIVE   = UNICAPGTK_DEVICE_LAST_PUBLIC_COLUMN + 2,
   SPECIAL     = UNICAPGTK_DEVICE_LAST_PUBLIC_COLUMN + 3,
   NUM_COLUMNS = UNICAPGTK_DEVICE_LAST_PUBLIC_COLUMN + 4
};

enum
{
   PROP_0,
   PROP_INCLUDE_RESCAN_ENTRY, 
};


static GObject * device_combo_box_constructor     (GType                  type,
                                                   guint                  n_params,
                                                   GObjectConstructParam *params);
static void      device_combo_box_finalize         (GObject      *object);
static void      device_combo_box_set_property     (GObject      *object,
                                                    guint         property_id,
                                                    const GValue *value,
                                                    GParamSpec   *pspec);
static void      device_combo_box_get_property     (GObject      *object,
                                                    guint         property_id,
                                                    GValue       *value,
                                                    GParamSpec   *pspec);
static void      device_combo_box_realize          (GtkWidget    *widget);
static void      device_combo_box_unrealize        (GtkWidget    *widget);

static gboolean  unicapgtk_device_selection_set_by_id (UnicapgtkDeviceSelection *combo,
						       const gchar          *device_id);
static void      device_combo_box_store_id         (UnicapgtkDeviceSelection *combo,
                                                    const gchar          *device_id);
static gboolean  device_combo_box_row_separator    (GtkTreeModel *model,
                                                    GtkTreeIter  *iter,
                                                    gpointer      data);
static void      device_combo_box_changed          (GtkComboBox  *combo);
static void      device_combo_box_add_rescan_entry (GtkListStore *store);
static void      device_combo_box_add_none_entry   (GtkListStore *store);


G_DEFINE_TYPE (UnicapgtkDeviceSelection,
               unicapgtk_device_selection, GTK_TYPE_COMBO_BOX);

#define parent_class unicapgtk_device_selection_parent_class
static guint unicapgtk_device_selection_signals[LAST_SIGNAL] = { 0 };

static void
unicapgtk_device_selection_class_init (UnicapgtkDeviceSelectionClass *klass)
{
   GObjectClass   *object_class = G_OBJECT_CLASS (klass);
   GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

   object_class->constructor  = device_combo_box_constructor;
   object_class->set_property = device_combo_box_set_property;
   object_class->get_property = device_combo_box_get_property;
   object_class->finalize     = device_combo_box_finalize;

   widget_class->realize      = device_combo_box_realize;
   widget_class->unrealize    = device_combo_box_unrealize;

   g_object_class_install_property (object_class,
				    PROP_INCLUDE_RESCAN_ENTRY,
				    g_param_spec_boolean ("include-rescan-entry",
							  NULL, NULL,
							  FALSE,
							  G_PARAM_CONSTRUCT_ONLY |
							  G_PARAM_READWRITE));

   unicapgtk_device_selection_signals[UNICAPGTK_DEVICE_SELECTION_CHANGED_SIGNAL] = 
      g_signal_new( "unicapgtk_device_selection_changed",
		    G_TYPE_FROM_CLASS(klass),
		    G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		    G_STRUCT_OFFSET(UnicapgtkDeviceSelectionClass, parent_instance),
		    NULL, 
		    NULL,                
		    g_cclosure_marshal_VOID__STRING,
		    G_TYPE_NONE, 1, 
		    G_TYPE_STRING );
}

static void
unicapgtk_device_selection_init (UnicapgtkDeviceSelection *combo)
{
   GtkListStore    *store;
   GtkCellRenderer *cell;

   store = gtk_list_store_new (NUM_COLUMNS,
			       G_TYPE_STRING,  /* UNICAPGTK_DEVICE_ID     */
			       G_TYPE_STRING,  /* UNICAPGTK_DEVICE_VENDOR */
			       G_TYPE_STRING,  /* UNICAPGTK_DEVICE_MODEL  */
			       G_TYPE_STRING,  /* LABEL                */
			       G_TYPE_BOOLEAN, /* SENSITIVE            */
			       G_TYPE_INT      /* SPECIAL              */);

   gtk_combo_box_set_model (GTK_COMBO_BOX (combo), GTK_TREE_MODEL (store));
   g_object_unref (store);

   gtk_combo_box_set_row_separator_func (GTK_COMBO_BOX (combo),
					 device_combo_box_row_separator,
					 NULL, NULL);

   cell = g_object_new (GTK_TYPE_CELL_RENDERER_TEXT,
			"xalign", 0.0,
			NULL);
   gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (combo), cell, TRUE);
   gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (combo), cell,
				   "text",      LABEL,
				   "sensitive", SENSITIVE,
				   NULL);

   device_combo_box_add_none_entry (store);

   combo->label_fmt_string = g_strdup( "%v %m" );
}

static GObject *
device_combo_box_constructor (GType                  type,
                              guint                  n_params,
                              GObjectConstructParam *params)
{
   UnicapgtkDeviceSelection *combo;
   GObject              *object;

   object = G_OBJECT_CLASS (parent_class)->constructor (type, n_params, params);

   combo = UNICAPGTK_DEVICE_SELECTION (object);

   if (combo->include_rescan_entry)
   {
      GtkTreeModel *model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

      device_combo_box_add_rescan_entry (GTK_LIST_STORE (model));
    }

      g_signal_connect (combo, "changed",
                        G_CALLBACK (device_combo_box_changed),
                        NULL);

   return object;
}

static void
device_combo_box_finalize (GObject *object)
{
   device_combo_box_store_id (UNICAPGTK_DEVICE_SELECTION (object), NULL);

   G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
device_combo_box_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
   UnicapgtkDeviceSelection *combo = UNICAPGTK_DEVICE_SELECTION (object);

   switch (property_id)
   {
      case PROP_INCLUDE_RESCAN_ENTRY:
	 combo->include_rescan_entry = g_value_get_boolean (value);
	 break;
      default:
	 G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	 break;
   }
}

static void
device_combo_box_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
   UnicapgtkDeviceSelection *combo = UNICAPGTK_DEVICE_SELECTION (object);

   switch (property_id)
   {
      case PROP_INCLUDE_RESCAN_ENTRY:
	 g_value_set_boolean (value, combo->include_rescan_entry);
	 break;
      default:
	 G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	 break;
   }
}

static void
device_combo_box_realize (GtkWidget *widget)
{
   UnicapgtkDeviceSelection *combo = UNICAPGTK_DEVICE_SELECTION (widget);

   g_return_if_fail (combo->busy_cursor == NULL);

   combo->busy_cursor =
      gdk_cursor_new_for_display (gtk_widget_get_display (widget), GDK_WATCH);

   GTK_WIDGET_CLASS (parent_class)->realize (widget);
}

static void
device_combo_box_unrealize (GtkWidget *widget)
{
   UnicapgtkDeviceSelection *combo = UNICAPGTK_DEVICE_SELECTION (widget);

   if (combo->busy_cursor)
   {
      gdk_cursor_unref (combo->busy_cursor);
      combo->busy_cursor = NULL;
   }

   GTK_WIDGET_CLASS (parent_class)->unrealize (widget);
}

static void
device_combo_box_add_rescan_entry (GtkListStore *store)
{
   GtkTreeIter iter;

   gtk_list_store_append (store, &iter);
   gtk_list_store_set (store, &iter,
		       SENSITIVE, FALSE,
		       SPECIAL, SPECIAL_SEPARATOR,
		       -1);

   gtk_list_store_append (store, &iter);
   gtk_list_store_set (store, &iter,
		       LABEL,     "Rescan for capture devices",
		       SENSITIVE, TRUE,
		       SPECIAL, SPECIAL_RESCAN,
		       -1);
}

static void
device_combo_box_add_none_entry (GtkListStore *store)
{
   GtkTreeIter iter;

   gtk_list_store_append (store, &iter);
   gtk_list_store_set (store, &iter,
		       LABEL,     "(None)",
		       SENSITIVE, FALSE,
		       SPECIAL,   SPECIAL_NONE,
		       -1);
}

static gboolean
device_combo_box_row_separator (GtkTreeModel *model,
                                GtkTreeIter  *iter,
                                gpointer      data)
{
   gint special;

   gtk_tree_model_get (model, iter,
		       SPECIAL, &special,
		       -1);

   return (special == SPECIAL_SEPARATOR);
}

static void
device_combo_box_store_id (UnicapgtkDeviceSelection *combo,
                           const gchar          *device_id)
{
   if (combo->active_device)
      g_free (combo->active_device);

   combo->active_device = g_strdup (device_id);
}

static void
device_combo_box_changed( GtkComboBox *combo )
{
   UnicapgtkDeviceSelection *device_combo = UNICAPGTK_DEVICE_SELECTION( combo );
   GtkTreeIter           iter;

   if( gtk_combo_box_get_active_iter (combo, &iter) )
   {
      GtkTreeModel *model  = gtk_combo_box_get_model( combo );
      gchar        *device_id;
      gint          special;

      gtk_tree_model_get( model, &iter,
                          UNICAPGTK_DEVICE_ID, &device_id,
                          SPECIAL,          &special,
                          -1 );

      if( device_id )
      {
	 device_combo_box_store_id( device_combo, device_id );
	 g_free( device_id );
	 g_signal_emit( G_OBJECT( combo ), 
			unicapgtk_device_selection_signals[UNICAPGTK_DEVICE_SELECTION_CHANGED_SIGNAL], 
			0, 
			device_combo->active_device );
	 return;
      }

      g_signal_stop_emission_by_name( combo, "changed" );

      switch( special )
      {
	 case NOT_SPECIAL:
	    break;

	 case SPECIAL_RESCAN:
	    unicapgtk_device_selection_rescan (device_combo);
	    break;

	 default:
	    device_combo_box_store_id (device_combo, NULL);
	    break;
      }
   }
}

static gboolean
unicapgtk_device_selection_set_by_id (UnicapgtkDeviceSelection *combo,
				      const gchar          *device_id)
{
   GtkTreeModel *model;
   GtkTreeIter   iter;
   gboolean      valid;

   g_return_val_if_fail (UNICAPGTK_IS_DEVICE_SELECTION (combo), FALSE);
   g_return_val_if_fail (device_id != NULL, FALSE);

   model = gtk_combo_box_get_model (GTK_COMBO_BOX (combo));

   for( valid = gtk_tree_model_get_iter_first( model, &iter );
	valid;
	valid = gtk_tree_model_iter_next( model, &iter ) )
   {
      gchar *value;

      gtk_tree_model_get( model, &iter,
                          UNICAPGTK_DEVICE_ID, &value,
                          -1 );

      if (value && strcmp (device_id, value) == 0)
      {
	 g_free (value);

	 gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &iter);

	 return TRUE;
      }

      g_free (value);
   }

   return FALSE;
}

/**
 * unicapgtk_device_selection_new:
 * @include_rescan_entry: if TRUE, the an entry will get added to the
 * combo box allowing the user to manually trigger a device rescan
 *
 * Creates a new #UnicapgtkDeviceSelection
 *
 * Returns: a new #UnicapgtkDeviceSelection
 */
GtkWidget *
unicapgtk_device_selection_new (gboolean include_rescan_entry)
{
   return g_object_new (UNICAPGTK_TYPE_DEVICE_SELECTION,
			"include-rescan-entry", include_rescan_entry,
			NULL);
}

static 
gchar *replace_seq( const gchar * input, gchar *seq, gchar *string )
{
   gchar *output;
   gchar *tmp;
      
   char *s = (char *)input;

   output = g_malloc0( 1 );
   
   while( strstr( s, seq ) )
   {
      gchar *con = g_strndup( s, strstr( s, seq ) - s );
      tmp = g_strconcat( output, con, NULL );
      g_free( output );
      g_free( con );
      output = g_strconcat( tmp, string, NULL );
      g_free( tmp );
      s = strstr( s, seq ) + 2;
   }

   tmp = g_strconcat( output, s, NULL );
   g_free( output );
   output = tmp;

   return output;
   
}

static 
gchar *format_device_label( const gchar *format, unicap_device_t *device )
{
   char *tmp, *tmp2;
   char modelidx[128];
   char modelidX[128];
   char vendoridx[128];
   char vendoridX[128];

   sprintf( modelidx, "%llx", device->model_id );
   sprintf( modelidX, "%llX", device->model_id );
   sprintf( vendoridx, "%x", device->vendor_id );
   sprintf( vendoridX, "%X", device->vendor_id );

   tmp = replace_seq( format, "%i", device->identifier );
   tmp2 = replace_seq( tmp, "%v", device->vendor_name );
   g_free( tmp );
   tmp = replace_seq( tmp2, "%m", device->model_name );
   g_free( tmp2 );
   tmp2 = replace_seq( tmp, "%s", modelidx );
   g_free( tmp );
   tmp = replace_seq( tmp2, "%S", modelidX );
   g_free( tmp2 );
   tmp2 = replace_seq( tmp, "%p", vendoridx );
   g_free( tmp );
   tmp = replace_seq( tmp2, "%P", vendoridX );
   g_free( tmp2 );
   
   return tmp;
}


/**
 * unicapgtk_device_selection_rescan: 
 * @combo: an #UnicapgtkDeviceSelection
 * 
 * Tiggers rescan of available video capture devices and updates the
 * combo box.
 *
 * Returns: number of available devices
 */
gint
unicapgtk_device_selection_rescan( UnicapgtkDeviceSelection *combo )
{
   unicap_device_t  device;
   GtkListStore    *store;
   GtkTreeIter      iter;
   gint             i;

   g_return_val_if_fail( UNICAPGTK_IS_DEVICE_SELECTION( combo ), 0 );

   store = GTK_LIST_STORE( gtk_combo_box_get_model( GTK_COMBO_BOX( combo ) ) );
   g_return_val_if_fail( GTK_IS_LIST_STORE( store ), 0 );

   if( GTK_WIDGET_REALIZED( combo ) )
   {
      gdk_window_set_cursor( GTK_WIDGET( combo )->window, combo->busy_cursor );
      gdk_display_sync( gtk_widget_get_display( GTK_WIDGET( combo ) ) );
   }

   gtk_list_store_clear( store );

   gtk_combo_box_set_active( GTK_COMBO_BOX( combo ), -1 );

   unicap_reenumerate_devices( NULL );

   for( i = 0; SUCCESS( unicap_enumerate_devices( NULL, &device, i ) ); i++ )
   {
      gchar    *label;
      int lock;
      gboolean  available = TRUE;
      
      lock = unicap_is_stream_locked( &device );

      if( ( lock & UNICAP_FLAGS_LOCKED ) && !( lock & UNICAP_FLAGS_LOCK_CURRENT_PROCESS ) ){
	 available = FALSE;
      }

      label = format_device_label( combo->label_fmt_string, &device );
	  
      gtk_list_store_append( store, &iter);
      gtk_list_store_set( store, &iter,
                          UNICAPGTK_DEVICE_ID,     device.identifier,
                          UNICAPGTK_DEVICE_MODEL,  device.model_name,
                          UNICAPGTK_DEVICE_VENDOR, device.vendor_name,
                          LABEL,                label,
                          SENSITIVE,            available,
                          -1);

      g_free( label );
   }

   if( i == 0 )
   {
      device_combo_box_add_none_entry( store );
   }

   if( combo->include_rescan_entry )
   {
      device_combo_box_add_rescan_entry( store );
   }

   if( combo->active_device )
   {
      unicapgtk_device_selection_set_by_id( combo, combo->active_device );
   }

   if( GTK_WIDGET_REALIZED( combo ) )
   {
      gdk_window_set_cursor( GTK_WIDGET( combo )->window, NULL );
   }   

   return i;
}

/**
 * unicapgtk_device_selection_set_device:
 * @combo: a #UnicapgtkDeviceSelection
 * @device: a #unicap_device_t
 * 
 * Sets the combo box to the given device.
 *
 * Returns: TRUE on success
 */
gboolean
unicapgtk_device_selection_set_device( UnicapgtkDeviceSelection *combo,
				       unicap_device_t      *device)
{
   g_return_val_if_fail( UNICAPGTK_IS_DEVICE_SELECTION( combo ), FALSE);

   if( device )
   {
      return unicapgtk_device_selection_set_by_id( combo, device->identifier );
   }

   gtk_combo_box_set_active( GTK_COMBO_BOX( combo ), 0 );

   return FALSE;
}

/**
 * unicapgtk_device_selection_set_label_fmt: 
 * @combo: a #UnicapgtkDeviceSelection
 * @fmt: a format string
 *
 * Sets the format string for the device labels. The following special
 * sequences are currently supported: 
 *
 * %%i: Identifier
 *
 * %%v: Vendor Name
 * 
 * %%m: Model Name
 * 
 * %%s: Model Id, hexadecimal with lowercase letters
 *
 * %%S: Model Id, hexadecimal with uppercase letters
 *
 * %%p: Vendor Id, hexadecimal with lowercase letters
 *
 * %%P: Vendor Id, hexadecimal with uppercase letters
 *
 */
void
unicapgtk_device_selection_set_label_fmt( UnicapgtkDeviceSelection *combo,
					  const gchar *fmt )
{
   g_free( combo->label_fmt_string );
   combo->label_fmt_string = g_strdup( fmt );
}
