#include <stdlib.h>
#include <gtk/gtk.h>
#include <unicapgtk.h>

#include <unicap.h>
#include <unicap_status.h>

typedef enum _CropType
{
	CROP_TYPE_X = 0, 
	CROP_TYPE_Y, 
	CROP_TYPE_WIDTH, 
	CROP_TYPE_HEIGHT, 
} CropType;



void crop_slider_changed_cb( GtkRange *range, UnicapgtkVideoDisplay *ugtk )
{
	CropType type;
	UnicapgtkVideoDisplayCropping crop;
	
	type = (CropType)g_object_get_data( G_OBJECT( range ), "crop_type" );

	unicapgtk_video_display_get_crop( ugtk, &crop );

	switch( type )
	{
		case CROP_TYPE_X:
			crop.crop_x = gtk_range_get_value( range );
			break;
			
		case CROP_TYPE_Y:
			crop.crop_y = gtk_range_get_value( range );
			break;
			
		case CROP_TYPE_WIDTH:
			crop.crop_width = gtk_range_get_value( range );
			break;
			
		case CROP_TYPE_HEIGHT:
			crop.crop_height = gtk_range_get_value( range );
			break;
			
		default: 
			g_assert( FALSE );
			break;
	}
	
	unicapgtk_video_display_set_crop( ugtk, &crop );
}

int main( int   argc,
          char *argv[] )
{
  GtkWidget *window;
  GtkWidget *ugtk;
  unicap_device_t device;

  GtkWidget *crop_window;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *widget;
  
  UnicapgtkVideoDisplayCropping crop;

  gtk_init (&argc, &argv);

  // Create main window
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK( gtk_main_quit ), NULL);
  
  // Create a display for the first capture device found by unicap
  ugtk = unicapgtk_video_display_new( );  
  gtk_container_add (GTK_CONTAINER (window), ugtk);

  gtk_widget_show_all (window);

  if( !SUCCESS( unicap_enumerate_devices( NULL, &device, 0 ) ) )
  {
	  GtkWidget *dialog = gtk_message_dialog_new( NULL, 
												  GTK_DIALOG_MODAL, 
												  GTK_MESSAGE_ERROR, 
												  GTK_BUTTONS_CLOSE, 
												  "No device connected!" );
	  gtk_dialog_run( GTK_DIALOG( dialog ) );
	  exit( -1 );
  }

  unicapgtk_video_display_set_device( UNICAPGTK_VIDEO_DISPLAY( ugtk ), &device );

  if( unicapgtk_video_display_start( UNICAPGTK_VIDEO_DISPLAY( ugtk ) ) == 0 )
  {
	  GtkWidget *dialog = gtk_message_dialog_new( NULL, 
												  GTK_DIALOG_MODAL, 
												  GTK_MESSAGE_ERROR, 
												  GTK_BUTTONS_CLOSE, 
												  "Failed to start live display!\n" \
												  "No device connected?" );
	  gtk_dialog_run( GTK_DIALOG( dialog ) );
	  exit(-1);
  }

  unicapgtk_video_display_get_crop( UNICAPGTK_VIDEO_DISPLAY( ugtk ), &crop );


  crop_window = gtk_window_new( GTK_WINDOW_TOPLEVEL );
  gtk_window_set_type_hint( GTK_WINDOW( crop_window ), GDK_WINDOW_TYPE_HINT_DIALOG );

  gtk_window_set_title( GTK_WINDOW( crop_window ), "crop" );
  vbox = gtk_vbox_new( 0, 0 );
  gtk_container_add( GTK_CONTAINER( crop_window ), vbox );

  hbox = gtk_hbox_new( 5, 5 );
  gtk_box_pack_start_defaults( GTK_BOX( vbox ), hbox );
  widget = gtk_label_new( "X pos" );
  gtk_box_pack_start( GTK_BOX( hbox ), widget, FALSE, FALSE, 0 );
  widget = gtk_hscale_new_with_range( 0.0f, crop.crop_width, 1.0f );
  gtk_box_pack_start_defaults( GTK_BOX( hbox ), widget );  
  g_object_set_data( G_OBJECT( widget ), "crop_type", (gpointer) CROP_TYPE_X );
  g_signal_connect( G_OBJECT( widget ), "value-changed", (GCallback) crop_slider_changed_cb, (gpointer)ugtk );
  gtk_widget_set_size_request( widget, 200, -1 );
  gtk_range_set_value( GTK_RANGE( widget ), crop.crop_x );
  
  hbox = gtk_hbox_new( 5, 5 );
  gtk_box_pack_start_defaults( GTK_BOX( vbox ), hbox );
  widget = gtk_label_new( "Y pos" );
  gtk_box_pack_start( GTK_BOX( hbox ), widget, FALSE, FALSE, 0 );
  widget = gtk_hscale_new_with_range( 0.0f, crop.crop_height, 1.0f );
  gtk_box_pack_start_defaults( GTK_BOX( hbox ), widget );
  g_object_set_data( G_OBJECT( widget ), "crop_type", (gpointer) CROP_TYPE_Y );
  g_signal_connect( G_OBJECT( widget ), "value-changed", (GCallback) crop_slider_changed_cb, (gpointer)ugtk );
  gtk_widget_set_size_request( widget, 200, -1 );
  gtk_range_set_value( GTK_RANGE( widget ), crop.crop_y );

  hbox = gtk_hbox_new( 5, 5 );
  gtk_box_pack_start_defaults( GTK_BOX( vbox ), hbox );
  widget = gtk_label_new( "width" );
  gtk_box_pack_start( GTK_BOX( hbox ), widget, FALSE, FALSE, 0 );
  widget = gtk_hscale_new_with_range( 0.0f, crop.crop_width, 1.0f );
  gtk_box_pack_start_defaults( GTK_BOX( hbox ), widget );
  g_object_set_data( G_OBJECT( widget ), "crop_type", (gpointer) CROP_TYPE_WIDTH );
  g_signal_connect( G_OBJECT( widget ), "value-changed", (GCallback) crop_slider_changed_cb, (gpointer)ugtk );
  gtk_widget_set_size_request( widget, 200, -1 );
  gtk_range_set_value( GTK_RANGE( widget ), crop.crop_width );
  
  hbox = gtk_hbox_new( 5, 5 );
  gtk_box_pack_start_defaults( GTK_BOX( vbox ), hbox );
  widget = gtk_label_new( "height" );
  gtk_box_pack_start( GTK_BOX( hbox ), widget, FALSE, FALSE, 0 );
  widget = gtk_hscale_new_with_range( 0.0f, crop.crop_height, 1.0f );
  gtk_box_pack_start_defaults( GTK_BOX( hbox ), widget );
  g_object_set_data( G_OBJECT( widget ), "crop_type", (gpointer) CROP_TYPE_HEIGHT );
  g_signal_connect( G_OBJECT( widget ), "value-changed", (GCallback) crop_slider_changed_cb, (gpointer)ugtk );
  gtk_widget_set_size_request( widget, 200, -1 );
  gtk_range_set_value( GTK_RANGE( widget ), crop.crop_height );
  
  gtk_widget_show_all( crop_window );
  
  gtk_main ();
  
  return 0;
}
