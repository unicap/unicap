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

/**
 * SECTION: UnicapgtkDeviceProperty
 * @short_description: Widget to control a single device property
 * @see_also: #unicapgtk_property_dialog
 *
 * Device specific functionality is controlled via device properties. 
 *
 * The #UnicapgtkDeviceProperty widget provides an user interface to a
 * device property. It uses a GtkComboBox, a GtkScale or a custom
 * widget to represent the property value. If the property hast Auto
 * or OnePush functionality, buttons to access those functions are
 * also created. 
 */


#include <stdlib.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <string.h>
#include <math.h>

#include "config.h"
#include "unicapgtk_device_property.h"
#include "unicap.h"
#include "unicapgtk_priv.h"

#if UNICAPGTK_DEBUG
#define DEBUG
#endif
#include "debug.h"

double fmin(double,double);

enum {
   UNICAPGTK_DEVICE_PROPERTY_CHANGED_SIGNAL,
   LAST_SIGNAL
};

enum 
{
   PPTY_TYPE_MAPPED_FLOAT = UNICAP_PROPERTY_TYPE_UNKNOWN + 1,
};


static void unicapgtk_device_property_class_init (UnicapgtkDevicePropertyClass *klass);
static void unicapgtk_device_property_init       (UnicapgtkDeviceProperty      *ugtk);
static void unicapgtk_device_property_destroy    (GtkObject                    *object);

static guint unicapgtk_device_property_signals[LAST_SIGNAL] = { 0 };

static void unicapgtk_pack_device_property( UnicapgtkDeviceProperty *ugtk, const char *property_name );

static void flags_changed_cb( GtkToggleButton *button, UnicapgtkDeviceProperty *ugtk );
static void range_value_changed_cb( GtkRange *range, gpointer *user_data );
static void auto_changed_cb( GtkToggleButton *togglebutton, gpointer *user_data );
static void one_push_clicked_cb( GtkButton *button, gpointer *user_data );
static void value_changed_cb( GtkComboBox *combo_box, gpointer *user_data );
static void menu_changed_cb( GtkComboBox *combo_box, gpointer *user_data );
static void mapped_range_value_changed_cb( GtkRange *range, gpointer *user_data );
static char *double_to_string( double val );
static void create_mapping( double min, double max, struct mapping *mapping, int *nmappings );
static void free_mapping( struct mapping *mapping, int nmappings );
static gint get_mapping_idx( struct mapping *mappings, int nmappings, double val );

G_DEFINE_TYPE( UnicapgtkDeviceProperty, unicapgtk_device_property, GTK_TYPE_HBOX );


static gint get_mapping_idx( struct mapping *mappings, int nmappings, double val )
{
   int i;
   double min = 1000000.0;
   
   for( i = 0; (i < nmappings); i++ )
   {
      double d = fabs( val - mappings[i].val );
      if( d <= min )
      {
	 min = d;
      }
      else
      {
	 break;
      }
   }

   if( i > 0 )
   {
      i -= 1;
   }

   return i;
}

static char *double_to_string( double val )
{
   if( val <= (1/10000.0f) )
   {
      double divisor;

      divisor = 1 / val;
      divisor = ceil( divisor / 1000.0f );
      divisor *= 1000;
      return g_strdup_printf( "1/%d s", (int)divisor );
   }
   else if( val <= (1/1000.0f) )
   {
      double divisor;

      divisor = 1 / val;
      divisor = ceil( divisor / 100.0f );
      divisor *= 100;
      return g_strdup_printf( "1/%d s", (int)divisor );
   }
   else if( val <= (1/100.0f) )
   {
      double divisor;

      divisor = 1 / val;
      divisor = ceil( divisor / 10.0f );
      divisor *= 10;
      return g_strdup_printf( "1/%d s", (int)divisor );
   }
   else if( val <= (1/8.0f) )
   {
      double divisor;

      divisor = 1 / val;
      return g_strdup_printf( "1/%d s", (int)divisor );
   }
   else if( val > 60.0f )
   {
      if( (int) val % 60 )
      {
        return g_strdup_printf( "%d min %d s", (int)val / 60, (int)val % 60 );
      }
      else
      {
        return g_strdup_printf( "%d min", (int)val / 60 );
      }
   }

   return g_strdup_printf( "%g s", val );
}

static void create_mapping( double min, double max, struct mapping *mapping, int *nmappings )
{
   int steps=0;
   int s = 0;

   if( !mapping )
   {
      *nmappings = 0;
   }

   if( min <= 1/10000.0 )
   {
      int imin = (1/min)+1;
      int tsteps;
      int i;

      tsteps = ceil(( ( imin - 10000.0 ) ) / 1000.0);
      for( i = imin; i>10000; i-= 1000 )
      {
	 if( s < *nmappings )
	 {
	    mapping[s].val = 1/(double)i;
	    mapping[s].repr = double_to_string( mapping[s].val );
	 }
	 s++;
      }
      steps += tsteps;
   }
   if( max > 1/10000.0 )
   {
      int imax;
      int tsteps;
      int i;

      imax = 1/fmin(max,1/1000.0);
      tsteps = ( 10000 - imax ) / 1000 + 1;
      for( i = 10000; i>imax; i-= 1000 )
      {
	 if( s < *nmappings )
	 {
	    mapping[s].val = 1/(double)i;
	    mapping[s].repr = double_to_string( mapping[s].val );
	 }
	 s++;
      }
      steps += tsteps;
   }
   if( max > 1/1000.0 )
   {
      int imax;
      int tsteps;
      int i;

      imax = 1/fmin(max,1/200.0);
      tsteps = ( 1000 - imax ) / 100 - 1;
      for( i = 1000 - 100; i>imax; i-= 100 )
      {
	 if( s < *nmappings )
	 {
	    mapping[s].val = 1/(double)i;
	    mapping[s].repr = double_to_string( mapping[s].val );
	 }
	 s++;
      }
      steps += tsteps;
   }
   if( max > 1/200.0 )
   {
      int imax;
      int tsteps;
      int i;

      imax = 1/fmin(max,1/100.0);
      tsteps = ( 200 - imax ) / 10;      
      for( i = 200 - 10; i > imax; i-= 10 )
      {
	 if( s < *nmappings )
	 {
	    mapping[s].val = 1/(double)i;
	    mapping[s].repr = double_to_string( mapping[s].val );
	 }
	 s++;
      }
      steps += tsteps;
   }
   if( max > 1/100.0 )
   {
      int imax;
      int tsteps;
      int i;

      imax = 1/fmin(max,1/10.0);
      tsteps = ( 100 - imax ) / 5 + 1;      
      for( i = 100 - 5; i>=imax; i-= 5 )
      {
	 if( s < *nmappings )
	 {
	    mapping[s].val = 1/(double)i;
	    mapping[s].repr = double_to_string( mapping[s].val );
	 }
	 s++;
      }
      steps += tsteps;
   }
   if( max > 1/10.0 )
   {
      double dmax;
      double v;
      int tsteps;
      
      dmax = fmin(max, 0.5f );
      tsteps = ( dmax - 1/10.0f ) / 0.025f;
      for( v = 0.125; v < dmax; v+=0.025 )
      {
	 if( s < *nmappings )
	 {
	    mapping[s].val = v;
	    mapping[s].repr = double_to_string( mapping[s].val );
	 }
	 s++;
      }
      steps += tsteps;
   }
   if( max > 0.5 )
   {
      double dmax;
      double v;
      int tsteps;
      
      dmax = fmin(max, 2.0f );
      tsteps = ( dmax - 0.5f ) / 0.1f +1;
      for( v = 0.5; v < dmax; v+=0.1 )
      {
	 if( s < *nmappings )
	 {
	    mapping[s].val = v;
	    mapping[s].repr = double_to_string( mapping[s].val );
	 }
	 s++;
      }
      steps += tsteps;
   }
   if( max > 2.0 )
   {
      double dmax;
      double v;
      int tsteps;
      
      dmax = fmin(max, 10.0f );
      tsteps = ( dmax - 2.0f ) / 0.5f;
      for( v = 2.0; v < dmax; v+=0.5 )
      {
	 if( s < *nmappings )
	 {
	    mapping[s].val = v;
	    mapping[s].repr = double_to_string( mapping[s].val );
	 }
	 s++;
      }
      steps += tsteps;
   }
   if( max > 10.0 )
   {
      double dmax;
      double v;
      int tsteps;
      
      dmax = fmin(max, 60.0f );
      tsteps = ( dmax - 10.0f ) / 1.0f;
      for( v = 10.0; v < dmax; v+=1.0 )
      {
	 if( s < *nmappings )
	 {
	    mapping[s].val = v;
	    mapping[s].repr = double_to_string( mapping[s].val );
	 }
	 s++;
      }
      steps += tsteps;
   }
   if( max > 60.0 )
   {
      double v;
      int tsteps;
      
      tsteps = ( max - 60.0f ) / 60.0f;
      for( v = 60.0; v < max; v+=60.0 )
      {
	 if( s < *nmappings )
	 {
	    mapping[s].val = v;
	    mapping[s].repr = double_to_string( mapping[s].val );
	 }
	 s++;
      }
      steps += tsteps;
   }
   
   TRACE( "Created %d mappings for range %f..%f ( expected %d )\n", s, min, max, mapping ? steps : 0 );
/*    if( mapping ) */
/*    { */
/*       g_warning( "Created %d steps, expected %d", s, steps ); */
/*    } */
   
   *nmappings = s;
}

static void free_mapping( struct mapping *mapping, int nmappings )
{
   int i;
   
   for( i = 0; i < nmappings; i++ )
   {
      g_free( mapping[i].repr );
   }
   
   g_free( mapping );
}

static void unicapgtk_device_property_class_init( UnicapgtkDevicePropertyClass *klass )
{
   GTK_OBJECT_CLASS( klass )->destroy = unicapgtk_device_property_destroy;

   unicapgtk_initialize();

   unicapgtk_device_property_signals[UNICAPGTK_DEVICE_PROPERTY_CHANGED_SIGNAL] = 
      g_signal_new( "unicapgtk_device_property_changed",
		    G_TYPE_FROM_CLASS(klass),
		    G_SIGNAL_RUN_FIRST | G_SIGNAL_ACTION,
		    G_STRUCT_OFFSET(UnicapgtkDevicePropertyClass, unicapgtk_device_property),
		    NULL, 
		    NULL,                
		    g_cclosure_marshal_VOID__POINTER,
		    G_TYPE_NONE, 1, 
		    G_TYPE_POINTER );
}

static void unicapgtk_device_property_destroy( GtkObject *object )
{
   UnicapgtkDeviceProperty *ugtk = (UnicapgtkDeviceProperty *)object;

   if( ugtk->unicap_handle )
   {
      unicap_close( ugtk->unicap_handle );
      ugtk->unicap_handle = NULL;
   }

   if( ugtk->mapping )
   {
      free_mapping( ugtk->mapping, ugtk->nmappings );
      ugtk->mapping = NULL;
   }

   GTK_OBJECT_CLASS( unicapgtk_device_property_parent_class )->destroy( object );
}


static void unicapgtk_device_property_init( UnicapgtkDeviceProperty *ugtk )
{
   ugtk->hbox = GTK_WIDGET( ugtk );

   gtk_box_set_spacing( GTK_BOX( ugtk->hbox ), 12 );

   ugtk->unicap_handle = NULL;
}


static void new( UnicapgtkDeviceProperty *ugtk, unicap_property_t *property_spec )
{
   TRACE( "new handle=%p\n", ugtk->unicap_handle );
	
   ugtk->scale = NULL;
   ugtk->one_push_button = NULL;
   ugtk->auto_check_box = NULL;
   ugtk->value_combo_box = NULL;
   ugtk->menu_combo_box = NULL;

   if( ugtk->unicap_handle )
   {
      // Get the property that matches the 'property_spec'
      if( SUCCESS( unicap_enumerate_properties( ugtk->unicap_handle, 
						property_spec, 
						&ugtk->property_default, 
						0 ) ) )
      {
	 strcpy( ugtk->property.identifier, ugtk->property_default.identifier );
			
	 if( SUCCESS( unicap_get_property( ugtk->unicap_handle, 
					    &ugtk->property ) ) )
	 {
            TRACE( "property %s min: %f max: %f\n", ugtk->property.identifier, 
                   ugtk->property.range.min, 
                   ugtk->property.range.max );
            unicapgtk_pack_device_property( ugtk, ugtk->property.identifier );
	 }
         else
         {
	    TRACE( "unicap_get_property failed [%s]\n", ugtk->property.identifier );
         }
      }
   }
   else
   {
      memcpy( &ugtk->property, property_spec, sizeof( unicap_property_t ) );
      unicapgtk_pack_device_property( ugtk, ugtk->property.identifier );
   }
}


/**
 * unicapgtk_device_property_new_by_handle: 
 * @handle: unicap handle of the device
 * @property_spec: specification of the property which should be
 * controlled by this widget. This could be acquired by unicap_enumerate_properties.
 *
 * Creates a new #UnicapgtkDeviceProperty widget and sets the handle. 
 *
 * If a handle is set on the widget, the widget will control the
 * device directly, ie. if the user changes a value, the widget will
 * call unicap_set_property(...) to change the value on the device. 
 *
 * Returns: a new #UnicapgtkDeviceProperty
 */
GtkWidget* unicapgtk_device_property_new_by_handle( unicap_handle_t handle, 
						    unicap_property_t *property_spec )
{
   UnicapgtkDeviceProperty *ugtk;
	
   TRACE( "new; property id: %s\n", property_spec->identifier );

   ugtk = g_object_new( UNICAPGTK_TYPE_DEVICE_PROPERTY, NULL );

   ugtk->unicap_handle = unicap_clone_handle( handle );
	
   new( ugtk, property_spec );
	
   return GTK_WIDGET( ugtk );
}

/**
 * unicapgtk_device_property_new: 
 * @property_spec: specification of the property which should be
 * controlled by this widget. This could be acquired by unicap_enumerate_properties.
 *
 * Creates a new #UnicapgtkDeviceProperty widget.
 */
GtkWidget* unicapgtk_device_property_new( unicap_property_t *property_spec )
{
   UnicapgtkDeviceProperty *ugtk;
	
   TRACE( "new; property id: %s\n", property_spec->identifier );

   ugtk = g_object_new( UNICAPGTK_TYPE_DEVICE_PROPERTY, NULL );
	
   ugtk->unicap_handle = NULL;
	
   new( ugtk, property_spec );	
	
   return GTK_WIDGET( ugtk );
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

/**
 * unicapgtk_device_property_set:
 * @ugtk: a #UnicapgtkDeviceProperty
 * @property: the property to set. Must have the same identifier as
 * used on creation of the widget.
 *
 * Returns: TRUE if the change was successful
 */
gboolean unicapgtk_device_property_set( UnicapgtkDeviceProperty *ugtk, unicap_property_t *property )
{
   if( strcmp( property->identifier, ugtk->property.identifier ) )
   {
      return FALSE;
   }

   if( ugtk->unicap_handle )
   {
      if( SUCCESS( unicap_set_property( ugtk->unicap_handle, property ) ) )
      {
	 memcpy( &ugtk->property, property, sizeof( unicap_property_t ) );
	 unicapgtk_device_property_redraw( ugtk );
	 return TRUE;
      }else{
	 TRACE( "unicap_set_property failed!\n" );
      }
   }
	
   return FALSE;
}

/**
 * unicapgtk_device_property_get_label:
 * @ugtk: a #UnicapgtkDeviceProperty
 * 
 * Returns: the label widget
 */
GtkWidget *unicapgtk_device_property_get_label( UnicapgtkDeviceProperty *ugtk )
{
   return ugtk->label;
}

/**
 * unicapgtk_device_property_set_label:
 * @ugtk: a #UnicapgtkDeviceProperty
 * @label: the new label widget
 */
void unicapgtk_device_property_set_label( UnicapgtkDeviceProperty *ugtk, GtkWidget *label )
{
   if( ugtk->label )
   {
      gtk_widget_destroy( ugtk->label );
   }

   if( GTK_IS_LABEL( label ))
   {
      PangoAttrList  *attrs = pango_attr_list_new();
      PangoAttribute *attr = pango_attr_weight_new( PANGO_WEIGHT_BOLD );

      attr->start_index = 0;
      attr->end_index = -1;

      pango_attr_list_insert( attrs, attr );

      gtk_label_set_attributes( GTK_LABEL( label ), attrs );
      pango_attr_list_unref( attrs );

      gtk_misc_set_alignment( GTK_MISC( label ), 1.0, 0.5 );
   }

   gtk_box_pack_start( GTK_BOX( ugtk->hbox ), label, FALSE, FALSE, 0 );
   ugtk->label = label;
}

static gboolean is_mapped_float_type( unicap_property_t *property )
{
   if( ( !strcmp( property->unit, "s" ) ) && 
       ( property->range.min <= 1/1000.0 ) &&
       ( property->range.max >= 1.0 ) )
   {
      return TRUE;
   }
   
   return FALSE;
}

static void unicapgtk_pack_device_property( UnicapgtkDeviceProperty *ugtk, const char *property_name )
{
   GtkWidget *vbox = gtk_vbox_new( FALSE, 6 );
   GtkWidget *hbox = gtk_hbox_new( FALSE, 6 );
   GtkWidget *auto_check = 0;
   int ppty_type;

   if( !strcmp( property_name, "trigger_mode" ) )
   {
      if( 0 )
      {
      }
   }
   

   gtk_box_pack_end_defaults( GTK_BOX( ugtk->hbox ), vbox );
   gtk_box_pack_start_defaults( GTK_BOX( vbox ), hbox );

   ppty_type = ugtk->property.type;
   if( is_mapped_float_type( &ugtk->property ) )
   {
      ppty_type = PPTY_TYPE_MAPPED_FLOAT;
   }
   ugtk->type = ppty_type;

   if( property_name )
   {
      gchar *name = g_strdelimit( g_strdup( dgettext( GETTEXT_PACKAGE, property_name ) ), "_", ' ');
      format_label( name, strlen( name ) );
      
      unicapgtk_device_property_set_label( ugtk, gtk_label_new( name ) );

      g_free( name );
   }

   if( ugtk->property.flags_mask & UNICAP_FLAGS_AUTO )
   {
      auto_check = gtk_check_button_new_with_label( _("Auto") );

      gtk_box_pack_start( GTK_BOX( hbox ), auto_check, FALSE, FALSE, 0 );
      gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( auto_check ), 
				    ugtk->property.flags & UNICAP_FLAGS_AUTO ? TRUE : FALSE );
      ugtk->auto_check_box_toggled_hid = g_signal_connect( G_OBJECT( auto_check ),
							   "toggled", 
							   G_CALLBACK( auto_changed_cb ), 
							   (gpointer) ugtk );
      ugtk->auto_check_box = auto_check;
   }
   if( ugtk->property.flags_mask & UNICAP_FLAGS_ONE_PUSH )
   {
      GtkWidget *button = gtk_button_new_with_label( _("One Push") );
      gtk_box_pack_start( GTK_BOX( hbox ), button, FALSE, FALSE, 0 );
      ugtk->one_push_clicked_hid =  g_signal_connect( G_OBJECT( button ),
						      "clicked", 
						      G_CALLBACK( one_push_clicked_cb ), 
						      (gpointer) ugtk );
      ugtk->one_push_button = button;
      gtk_widget_set_sensitive( ugtk->one_push_button, ugtk->property.flags & UNICAP_FLAGS_MANUAL ? TRUE : FALSE );
   }
	
   switch( ppty_type )
   {
      case PPTY_TYPE_MAPPED_FLOAT:
      {
	 int nmappings = 0;
	 int idx;

	 create_mapping( ugtk->property.range.min, ugtk->property.range.max,
			 NULL, &nmappings );
	 
	 ugtk->mapping = g_malloc( sizeof( struct mapping ) * nmappings );
	 create_mapping( ugtk->property.range.min, ugtk->property.range.max,
			 ugtk->mapping, &nmappings );

	 ugtk->nmappings = nmappings;
	 ugtk->scale = gtk_hscale_new_with_range( 0.0, 
						  nmappings-1, 
						  1.0 );
	 idx = get_mapping_idx( ugtk->mapping, nmappings, ugtk->property.value );
	 gtk_range_set_value( GTK_RANGE( ugtk->scale ), (double)idx );
	 gtk_scale_set_draw_value( GTK_SCALE( ugtk->scale ), FALSE );
	 ugtk->scale_value_label = gtk_label_new( ugtk->mapping[idx].repr );
         gtk_misc_set_alignment( GTK_MISC( ugtk->scale_value_label ), 1.0, 0.5 );
	 gtk_label_set_width_chars( GTK_LABEL( ugtk->scale_value_label ), 8 );
	 gtk_box_pack_start( GTK_BOX( hbox ), ugtk->scale, TRUE, TRUE, 0 );
	 gtk_box_pack_start( GTK_BOX( hbox ), ugtk->scale_value_label, FALSE, FALSE, 0 );
	 gtk_widget_set_size_request( ugtk->scale, 100, -1 );	 
	 ugtk->scale_changed_hid = g_signal_connect( G_OBJECT( ugtk->scale ),
						     "value_changed", 
						     G_CALLBACK( mapped_range_value_changed_cb ),
						     (gpointer) ugtk );
	 
	 gtk_widget_set_sensitive( ugtk->scale, ugtk->property.flags & UNICAP_FLAGS_MANUAL ? TRUE : FALSE );	 
      }
      break;
      case UNICAP_PROPERTY_TYPE_RANGE:
      {
         gdouble step;

	 TRACE( "%s: Type: %d\nRange min: %f, max: %f, step: %f, value: %f\n", 
		ugtk->property.identifier,  
		ugtk->property.type,
		ugtk->property.range.min, 
		ugtk->property.range.max, 
		ugtk->property.stepping,
		ugtk->property.value );

	 if( ugtk->property.range.min >= ugtk->property.range.max )
	 {
	    TRACE( "Insane property\n" );
	    break;
	 }

         step = ugtk->property.stepping;
         if( step <= 0.0 ) {
            step = (ugtk->property.range.max - ugtk->property.range.min) / 100.0;
         }

	 ugtk->scale = gtk_hscale_new_with_range( ugtk->property.range.min, 
						  ugtk->property.range.max, 
						  step );

	 gtk_scale_set_value_pos( GTK_SCALE( ugtk->scale ), GTK_POS_TOP );
	 gtk_range_set_value( GTK_RANGE( ugtk->scale ), ugtk->property.value );

	 gtk_widget_show( ugtk->scale );
	 gtk_box_pack_start( GTK_BOX( hbox ), ugtk->scale, TRUE, TRUE, 0 );

	 gtk_widget_set_size_request( ugtk->scale, 100, -1 );

	 ugtk->scale_changed_hid = g_signal_connect( G_OBJECT( ugtk->scale ),
						     "value_changed", 
						     G_CALLBACK( range_value_changed_cb ),
						     (gpointer) ugtk );

	 gtk_widget_set_sensitive( ugtk->scale, ugtk->property.flags & UNICAP_FLAGS_MANUAL ? TRUE : FALSE );
      }
      break;

      case UNICAP_PROPERTY_TYPE_VALUE_LIST:
      {
	 GtkWidget *combo_box;
	 int i;
	 int sel = 0;
	 int prec = 0;

	 TRACE( "UNICAP_PROPERTY_TYPE_VALUE_LIST\n" );
	 // calculate required precision
	 for( i = 0; i < ugtk->property.value_list.value_count; i++ )
	 {
	    char buffer[128];
	    int j;
	    int max_prec = 6;

	    sprintf( buffer, "%0.*f", max_prec, ugtk->property.value_list.values[i] );

	    for( j = strlen( buffer ) - 1; ( buffer[j] == '0' ) && ( j > ( strlen( buffer ) - 6 ) ); j-- );

	    if( ( max_prec - ( ( strlen( buffer ) - 1 ) - j ) ) > prec )
	    {
	       prec = j;
	    }
	 }

	 combo_box = gtk_combo_box_new_text();

	 for( i = 0; i < ugtk->property.value_list.value_count; i++ )
	 {
	    char buffer[128];

	    sprintf( buffer, "%1.*lf", prec, ugtk->property.value_list.values[i] );
	    gtk_combo_box_append_text( GTK_COMBO_BOX( combo_box ), buffer );

	    if( ugtk->property.value == ugtk->property.value_list.values[i] )
	    {
	       sel = i;
	    }
	 }

	 ugtk->value_combo_box_changed_hid = g_signal_connect( G_OBJECT( combo_box ), 
							       "changed", 
							       (GCallback)value_changed_cb, ugtk );
	 gtk_widget_show( combo_box );
	 gtk_combo_box_set_active( GTK_COMBO_BOX( combo_box ), sel );
	 gtk_box_pack_start( GTK_BOX( vbox ), combo_box, FALSE, FALSE, 0 );
	 gtk_widget_set_sensitive( combo_box, ugtk->property.flags & UNICAP_FLAGS_MANUAL ? TRUE : FALSE );

	 ugtk->value_combo_box = combo_box;
      }
      break;
      case UNICAP_PROPERTY_TYPE_MENU:
      {
	 GtkWidget *combo_box;
	 int i;
	 int sel = 0;


	 combo_box = gtk_combo_box_new_text( );

	 for( i = 0; i < ugtk->property.menu.menu_item_count; i++ )
	 {
	    gtk_combo_box_append_text( GTK_COMBO_BOX( combo_box ), 
				       dgettext( GETTEXT_PACKAGE, ugtk->property.menu.menu_items[i] ) );

	    if( !strcmp( ugtk->property.menu.menu_items[i], ugtk->property.menu_item ) )
	    {
	       sel = i;
	    }
	 }

	 ugtk->menu_combo_box_changed_hid = g_signal_connect( G_OBJECT( combo_box ), 
							      "changed", 
							      (GCallback)menu_changed_cb, ugtk );
	 gtk_widget_show( combo_box );
	 gtk_combo_box_set_active( GTK_COMBO_BOX( combo_box ), sel );
	 gtk_box_pack_start( GTK_BOX( vbox ), combo_box, FALSE, FALSE, 0 );
	 gtk_widget_set_sensitive( combo_box, ugtk->property.flags & UNICAP_FLAGS_MANUAL ? TRUE : FALSE );

	 ugtk->menu_combo_box = combo_box;
      }

      case UNICAP_PROPERTY_TYPE_FLAGS:
      {
	 GtkWidget *check_button;
	 
	 if( ugtk->property.flags_mask & UNICAP_FLAGS_ON_OFF )
	 {
	    check_button = gtk_check_button_new_with_label( _("Enabled" ) );
	    gtk_box_pack_start( GTK_BOX( vbox ), check_button, FALSE, FALSE, 0 );
            gtk_widget_show( check_button );

            gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( check_button ), 
                                          ugtk->property.flags & UNICAP_FLAGS_ON_OFF ? TRUE : FALSE );

	    ugtk->flags_enable_changed_hid = g_signal_connect( G_OBJECT( check_button ), 
							       "toggled", 
							       ( GCallback)flags_changed_cb, ugtk );

	    ugtk->flags_enable_check_box = check_button;
	 }
      }
      break;
	 

      default:
        break;
   }
}

static void value_changed_cb( GtkComboBox *combo_box, gpointer *user_data )
{
   UnicapgtkDeviceProperty *ugtk = ( UnicapgtkDeviceProperty *)user_data;
   double value;
   unicap_property_t property;
   char *text;
	

   text = gtk_combo_box_get_active_text( combo_box );
   if( text )
   {
      sscanf( text, "%lf", &value );
      g_free( text );
	
      ugtk->property.value = value;
      
      unicap_copy_property( &property, &ugtk->property );
      
      g_signal_emit( G_OBJECT( ugtk ), 
		     unicapgtk_device_property_signals[ UNICAPGTK_DEVICE_PROPERTY_CHANGED_SIGNAL ], 
		     0, 
		     (gpointer)&property );
   }
}


static void flags_changed_cb( GtkToggleButton *button, UnicapgtkDeviceProperty *ugtk )
{
   gboolean active = gtk_toggle_button_get_active( button );
   unicap_property_t property;

   if( active )
   {
      ugtk->property.flags = UNICAP_FLAGS_ON_OFF;
   }
   else
   {
      ugtk->property.flags = 0;
   }
   
   unicap_copy_property( &property, &ugtk->property );
   
   g_signal_emit( G_OBJECT( ugtk ), 
		  unicapgtk_device_property_signals[ UNICAPGTK_DEVICE_PROPERTY_CHANGED_SIGNAL ], 
		  0, 
		  (gpointer)&property );
}

static void menu_changed_cb( GtkComboBox *combo_box, gpointer *user_data )
{
   UnicapgtkDeviceProperty *ugtk = ( UnicapgtkDeviceProperty *)user_data;
   unicap_property_t property;
   int sel;

   sel = gtk_combo_box_get_active( combo_box );

   if( sel >= 0 )
   {
      strcpy( ugtk->property.menu_item, ugtk->property.menu.menu_items[sel] );
   }
	
   unicap_copy_property( &property, &ugtk->property );

   g_signal_emit( G_OBJECT( ugtk ), 
		  unicapgtk_device_property_signals[ UNICAPGTK_DEVICE_PROPERTY_CHANGED_SIGNAL ], 
		  0, 
		  (gpointer)&property );
}


// ------------------------------------------------------
static void mapped_range_value_changed_cb( GtkRange *range, gpointer *user_data )
{
   UnicapgtkDeviceProperty *ugtk = ( UnicapgtkDeviceProperty *)user_data;
   unicap_property_t property;
   int idx;

   ugtk->property.flags = UNICAP_FLAGS_ON_OFF | UNICAP_FLAGS_MANUAL;
   idx = (int)gtk_range_get_value( range );

   gtk_label_set_text( GTK_LABEL( ugtk->scale_value_label ), ugtk->mapping[idx].repr );
   ugtk->property.value = ugtk->mapping[idx].val;

   unicap_copy_property( &property, &ugtk->property );
	
   g_signal_emit( G_OBJECT( ugtk ), 
		  unicapgtk_device_property_signals[ UNICAPGTK_DEVICE_PROPERTY_CHANGED_SIGNAL ], 
		  0, 
		  (gpointer)&property );
}

static void range_value_changed_cb( GtkRange *range, gpointer *user_data )
{
   UnicapgtkDeviceProperty *ugtk = ( UnicapgtkDeviceProperty *)user_data;
   unicap_property_t property;

   ugtk->property.flags = UNICAP_FLAGS_ON_OFF | UNICAP_FLAGS_MANUAL;
   ugtk->property.value = gtk_range_get_value( range );

   if( ugtk->property.stepping >= 1.0 ){
      double r;
      r = fmod( ugtk->property.value, ugtk->property.stepping );
      if( r != 0.0 ){
	 if(  r > ( ugtk->property.stepping / 2.0 ) ){
	    // round up
	    ugtk->property.value += ( ugtk->property.stepping - r );
	 } else {
	    ugtk->property.value -= r;
	 }
	 gtk_range_set_value( range, ugtk->property.value );
	 return;
      }
   }
	 
      

   unicap_copy_property( &property, &ugtk->property );
	
   g_signal_emit( G_OBJECT( ugtk ), 
		  unicapgtk_device_property_signals[ UNICAPGTK_DEVICE_PROPERTY_CHANGED_SIGNAL ], 
		  0, 
		  (gpointer)&property );
}

static void auto_changed_cb( GtkToggleButton *togglebutton, gpointer *user_data )
{
   UnicapgtkDeviceProperty *ugtk = ( UnicapgtkDeviceProperty *)user_data;
   unicap_property_t property;

   if( ugtk->unicap_handle )
   {
      unicap_get_property( ugtk->unicap_handle, &ugtk->property );
   }

   if( gtk_toggle_button_get_active( togglebutton ) )
   {
      ugtk->property.flags = UNICAP_FLAGS_ON_OFF | UNICAP_FLAGS_AUTO;
   }
   else
   {
      ugtk->property.flags = UNICAP_FLAGS_ON_OFF | UNICAP_FLAGS_MANUAL;
		
   }
	
		
   if( ugtk->property.flags_mask & UNICAP_FLAGS_ONE_PUSH )
   {
      gtk_widget_set_sensitive( ugtk->one_push_button, ugtk->property.flags & UNICAP_FLAGS_MANUAL ? TRUE : FALSE );
   }

   switch( ugtk->property.type )
   {
      case UNICAP_PROPERTY_TYPE_RANGE:
      {
	 gtk_widget_set_sensitive( ugtk->scale, ugtk->property.flags & UNICAP_FLAGS_MANUAL ? TRUE : FALSE );
	 g_signal_handlers_block_by_func( ugtk->scale, range_value_changed_cb, ugtk );
	 if( ugtk->type == PPTY_TYPE_MAPPED_FLOAT )
	 {
	    int idx = get_mapping_idx( ugtk->mapping, ugtk->nmappings, ugtk->property.value );
	    gtk_range_set_value( GTK_RANGE( ugtk->scale ), (double) idx );
	    gtk_label_set_text( GTK_LABEL( ugtk->scale_value_label ), ugtk->mapping[idx].repr );
	 }
	 else
	 {
	    gtk_range_set_value( GTK_RANGE( ugtk->scale ), ugtk->property.value );
	 }
	 g_signal_handlers_unblock_by_func( ugtk->scale, range_value_changed_cb, ugtk );

      }
      break;
		
      case UNICAP_PROPERTY_TYPE_VALUE_LIST:
      {
	 gtk_widget_set_sensitive( ugtk->value_combo_box, ugtk->property.flags & UNICAP_FLAGS_MANUAL ? TRUE : FALSE );
      }
      break;
		
      case UNICAP_PROPERTY_TYPE_MENU:
      {
	 gtk_widget_set_sensitive( ugtk->menu_combo_box, ugtk->property.flags & UNICAP_FLAGS_MANUAL ? TRUE : FALSE );
      }
      break;
		
      default:
	 break;
   }

   unicap_copy_property( &property, &ugtk->property );

   g_signal_emit( G_OBJECT( ugtk ), 
		  unicapgtk_device_property_signals[ UNICAPGTK_DEVICE_PROPERTY_CHANGED_SIGNAL ], 
		  0, 
		  (gpointer)&property );
}

static void one_push_clicked_cb( GtkButton *button, gpointer *user_data )
{
   UnicapgtkDeviceProperty *ugtk = ( UnicapgtkDeviceProperty *)user_data;
   unicap_property_t property;

   ugtk->property.flags = UNICAP_FLAGS_ON_OFF | UNICAP_FLAGS_ONE_PUSH;
	
   unicap_copy_property( &property, &ugtk->property );

   g_signal_emit( G_OBJECT( ugtk ), 
		  unicapgtk_device_property_signals[ UNICAPGTK_DEVICE_PROPERTY_CHANGED_SIGNAL ], 
		  0, 
		  (gpointer)&property );
}

/**
 * unicapgtk_device_property_redraw_sensitivity:
 * @ugtk: a #UnicapgtkDeviceProperty
 *
 * Updates the widgets sensitivity state according to the auto/manual flags
 *
 */
void unicapgtk_device_property_redraw_sensitivity( UnicapgtkDeviceProperty *ugtk )
{
   if( ugtk->scale )
   {
      gtk_widget_set_sensitive( ugtk->scale, ugtk->property.flags & UNICAP_FLAGS_MANUAL ? TRUE : FALSE );
   }

   if( ugtk->one_push_button )
   {
      gtk_widget_set_sensitive( ugtk->one_push_button, ugtk->property.flags & UNICAP_FLAGS_MANUAL ? TRUE : FALSE );
   }
	
   if( ugtk->auto_check_box )
   {
     gtk_widget_set_sensitive( ugtk->auto_check_box, ugtk->property.flags & UNICAP_FLAGS_AUTO ? TRUE : FALSE );
   }
}

/**
 * unicapgtk_device_property_redraw:
 * @ugtk: a #UnicapgtkDeviceProperty
 *
 * Updates the widgets to reflect the current property state
 *
 */
void unicapgtk_device_property_redraw( UnicapgtkDeviceProperty *ugtk )
{
   if( ugtk->scale )
   {
      g_signal_handler_block( ugtk->scale, ugtk->scale_changed_hid );
      gtk_widget_set_sensitive( ugtk->scale, ugtk->property.flags & UNICAP_FLAGS_MANUAL ? TRUE : FALSE );
      if( ugtk->type == PPTY_TYPE_MAPPED_FLOAT )
      {
	 int idx = get_mapping_idx( ugtk->mapping, ugtk->nmappings, ugtk->property.value );
	 gtk_range_set_value( GTK_RANGE( ugtk->scale ), (double) idx );
	 gtk_label_set_text( GTK_LABEL( ugtk->scale_value_label ), ugtk->mapping[idx].repr );
      }
      else
      {
	 gtk_range_set_value( GTK_RANGE( ugtk->scale ), ugtk->property.value );
      }
      gtk_range_set_range( GTK_RANGE( ugtk->scale ), ugtk->property.range.min, ugtk->property.range.max );
      g_signal_handler_unblock( ugtk->scale, ugtk->scale_changed_hid );
/* 		g_signal_stop_emission_by_name( ugtk->scale, "value_changed" ); */
   }

   if( ugtk->one_push_button )
   {
      g_signal_handler_block( ugtk->one_push_button, ugtk->one_push_clicked_hid );
      gtk_widget_set_sensitive( ugtk->one_push_button, ugtk->property.flags & UNICAP_FLAGS_MANUAL ? TRUE : FALSE );
      g_signal_handler_unblock( ugtk->one_push_button, ugtk->one_push_clicked_hid );
/* 		g_signal_stop_emission_by_name( ugtk->scale, "clicked" ); */
   }
	
   if( ugtk->auto_check_box )
   {
      g_signal_handler_block( ugtk->auto_check_box, ugtk->auto_check_box_toggled_hid );
      gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( ugtk->auto_check_box ), 
				    ugtk->property.flags & UNICAP_FLAGS_AUTO ? TRUE : FALSE );
      g_signal_handler_unblock( ugtk->auto_check_box, ugtk->auto_check_box_toggled_hid );
/* 		g_signal_stop_emission_by_name( ugtk->scale, "toggled" ); */
   }

   if( ugtk->flags_enable_check_box )
   {
      g_signal_handler_block( ugtk->flags_enable_check_box, ugtk->flags_enable_changed_hid );
      gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON( ugtk->flags_enable_check_box ), 
                                    ugtk->property.flags & UNICAP_FLAGS_ON_OFF ? TRUE : FALSE );
      g_signal_handler_unblock( ugtk->flags_enable_check_box, ugtk->flags_enable_changed_hid );
   }
}

/**
 * unicapgtk_device_property_update:
 * @ugtk: a #UnicapgtkDeviceProperty
 *
 * Reads the current property state from the device and updates the widget
 */
void unicapgtk_device_property_update( UnicapgtkDeviceProperty *ugtk )
{
   if( ugtk->unicap_handle )
   {
      unicap_get_property( ugtk->unicap_handle, &ugtk->property );
   }
	
   unicapgtk_device_property_redraw( ugtk );
}

/**
 * unicapgtk_device_property_update_sensitivity:
 * @ugtk: a #UnicapgtkDeviceProperty
 *
 * Reads the current property state from the device and updates the widgets sensitivity
 */
void unicapgtk_device_property_update_sensitivity( UnicapgtkDeviceProperty *ugtk )
{
   if( ugtk->unicap_handle )
   {
      unicap_get_property( ugtk->unicap_handle, &ugtk->property );
   }
	
   unicapgtk_device_property_redraw_sensitivity( ugtk );
}
