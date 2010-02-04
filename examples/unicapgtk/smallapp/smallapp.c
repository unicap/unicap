#include <stdlib.h>
#include <gtk/gtk.h>
#include <unicap.h>
#include <ucil.h>
#include <unicapgtk.h>

#include <stdio.h>
#include <string.h>

static void save_image( UnicapgtkVideoDisplay *ugtk );
static void show_property_dialog( UnicapgtkVideoDisplay *ugtk );

static GtkItemFactoryEntry menu_entries[] = 
{
   { "/_File", NULL, NULL, 0, "<Branch>" }, 
   { "/_File/_Save Image", "<CTRL>S", save_image, 1, "<Item>" },
   { "/_File/_Quit", "<CTRL>Q", gtk_main_quit, 0 , "<StockItem>",GTK_STOCK_QUIT },
   { "/_Device", NULL, NULL, 0, "<Branch>" },
   { "/_Device/_Parameters", "<CTRL>P", show_property_dialog, 1, "<Item>"},
};

static gint nmenu_entries = G_N_ELEMENTS( menu_entries );


/*
  format_change_cb: 
  Callback called when the user selected a new video format
*/
static 
void format_change_cb( GtkWidget *ugtk, unicap_format_t *format, GtkWidget *ugtk_display )
{
   GtkWidget *property_dialog;

   property_dialog = g_object_get_data( G_OBJECT( ugtk_display ), "property_dialog" );
   g_assert( property_dialog );

   unicapgtk_video_display_stop( UNICAPGTK_VIDEO_DISPLAY( ugtk_display ) );
   unicapgtk_video_display_set_format( UNICAPGTK_VIDEO_DISPLAY( ugtk_display ), format );
   unicapgtk_video_display_start( UNICAPGTK_VIDEO_DISPLAY( ugtk_display ) );

   // reset the property dialog since some properties ( eg. frame rate ) might change when the format changes
   unicapgtk_property_dialog_reset( UNICAPGTK_PROPERTY_DIALOG( property_dialog ) );	
}

/*
  pause_toggle_cb: 
  Callback called when the user pressed the pause button
*/
static 
void pause_toggle_cb( GtkWidget *toggle_button, GtkWidget *ugtk_display )
{
   unicapgtk_video_display_set_pause( UNICAPGTK_VIDEO_DISPLAY( ugtk_display ), 
				      gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON( toggle_button ) ) );
}

/*
  device_change_cb: 
  callback called when the user selected a video capture device
*/
static 
void device_change_cb( UnicapgtkDeviceSelection *selection, gchar *device_id, GtkWidget *ugtk_display )
{
   unicap_device_t device;
   unicap_handle_t handle;
   GtkWidget *property_dialog;
   GtkWidget *format_selection;
   GtkWidget *window;

   property_dialog = g_object_get_data( G_OBJECT( ugtk_display ), "property_dialog" );
   g_assert( property_dialog );

   format_selection = g_object_get_data( G_OBJECT( ugtk_display ), "format_selection" );
   g_assert( format_selection );

   window = g_object_get_data( G_OBJECT( ugtk_display ), "app-window" );
   g_assert( window );

   unicap_void_device( &device );
   strcpy( device.identifier, device_id );
   
   if( !SUCCESS( unicap_enumerate_devices( &device, &device, 0 ) ) ||
       !SUCCESS( unicap_open( &handle, &device ) ) )
   {
      // device is not available anymore
      g_printerr( "*** TODO: device rescan*** device not available!\n" );
      return;
   }
   
   if( unicap_is_stream_locked( &device ) )
   {
      // could not acquire lock
      unicap_close( handle );
      g_printerr( "*** TODO: device rescan*** device is locked\n" );
      return;
   }
		
   unicapgtk_video_display_set_handle( UNICAPGTK_VIDEO_DISPLAY( ugtk_display ), handle );
   unicapgtk_property_dialog_set_handle( UNICAPGTK_PROPERTY_DIALOG( property_dialog ), handle );
   unicapgtk_video_format_selection_set_handle( UNICAPGTK_VIDEO_FORMAT_SELECTION( format_selection ), handle );
   unicap_close( handle );
   gtk_window_set_title( GTK_WINDOW( window ), device.identifier );
}

static void show_property_dialog( UnicapgtkVideoDisplay *ugtk )
{
   GtkWidget *dialog;
	
   dialog = g_object_get_data( G_OBJECT( ugtk ), "property_dialog" );
   g_assert( dialog );
	
   gtk_window_present( GTK_WINDOW( dialog ) );
}

static void save_image( UnicapgtkVideoDisplay *ugtk )
{
   GtkWidget *fsdialog;
   GtkWidget *app_window;
   gint response;

   app_window = g_object_get_data( G_OBJECT( ugtk ), "app-window" );
   g_assert( app_window );

   fsdialog = gtk_file_chooser_dialog_new( "Save Still Image", 
					   GTK_WINDOW( app_window ), 
					   GTK_FILE_CHOOSER_ACTION_SAVE, 
					   GTK_STOCK_SAVE, GTK_RESPONSE_OK, 
					   GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL );

   gtk_file_chooser_set_current_name( GTK_FILE_CHOOSER( fsdialog ), "Image.png" );
   response = gtk_dialog_run( GTK_DIALOG( fsdialog ) );
	
   if( response == GTK_RESPONSE_OK )
   {
      GdkPixbuf *pixbuf;
      gchar *filename;

      pixbuf = unicapgtk_video_display_get_still_image( ugtk );
      if( pixbuf )
      {
	 filename = gtk_file_chooser_get_filename( GTK_FILE_CHOOSER( fsdialog ) );
	 gdk_pixbuf_save( pixbuf, filename, "png", NULL, NULL );
	 g_object_unref( pixbuf );
      }
      else
      {
	 g_warning( "Failed to acquire image\n" );
      }
   }
   
   gtk_widget_destroy( fsdialog );
}


/*
  create_application_window - Creates the main application window

  The window consists of a button bar ( containing a video format selector, 
  a pause button, a save still image button and the preferences button ). 
  Below the toolbar, place the video display widget
*/
GtkWidget *create_application_window( )
{
   GtkWidget *window;
   GtkWidget *menu_bar;
   GtkWidget *ugtk_display;
   GtkWidget *ugtk_format_selection;
   GtkWidget *widget;
   GtkWidget *vbox;
   GtkWidget *hbox;
   GtkWidget *button_box;
   GtkWidget *device_selection;
   GtkWidget *scrolled_window;

   GtkItemFactory *factory;
   GtkAccelGroup *accel_group;
	
   window = gtk_window_new( GTK_WINDOW_TOPLEVEL );
   g_signal_connect( G_OBJECT( window ), "destroy", G_CALLBACK( gtk_main_quit ), NULL);	
   gtk_window_set_default_size( GTK_WINDOW( window ), 680, 560 );

   vbox = gtk_vbox_new( 0,0 );
   gtk_container_add( GTK_CONTAINER( window ), vbox );

   ugtk_display = unicapgtk_video_display_new( );
/*    g_object_set( G_OBJECT( ugtk_display ), "scale-to-fit", TRUE, "backend", "xv", "backend_fourcc", UCIL_FOURCC( 'U', 'Y', 'V', 'Y' ), NULL ); */
   g_object_set( G_OBJECT( ugtk_display ), "backend", "gtk", NULL );
   g_object_set_data( G_OBJECT( window ), "ugtk_display", ugtk_display );
   g_object_set_data( G_OBJECT( ugtk_display ), "app-window", window );

   accel_group = gtk_accel_group_new();
   factory = gtk_item_factory_new( GTK_TYPE_MENU_BAR, "<UnicapgtkSmallappMain>", accel_group );
   gtk_item_factory_create_items( factory, nmenu_entries, menu_entries, ugtk_display );
   gtk_window_add_accel_group( GTK_WINDOW( window ), accel_group );
   menu_bar = gtk_item_factory_get_widget( factory, "<UnicapgtkSmallappMain>" );
   gtk_box_pack_start( GTK_BOX( vbox ), menu_bar, FALSE, TRUE, 0 );

   hbox = gtk_hbox_new( 0, 0 );
   gtk_box_pack_start( GTK_BOX( vbox ), hbox, FALSE, FALSE, 2 );

   device_selection = unicapgtk_device_selection_new(TRUE);
   g_signal_connect( G_OBJECT( device_selection ), "unicapgtk_device_selection_changed", 
		     (GCallback)device_change_cb, ugtk_display );
   gtk_box_pack_start_defaults( GTK_BOX( hbox ), device_selection );

   ugtk_format_selection = unicapgtk_video_format_selection_new( );
   gtk_box_pack_start_defaults( GTK_BOX( hbox ), ugtk_format_selection );
   g_signal_connect( G_OBJECT( ugtk_format_selection ), "unicapgtk_video_format_changed",
		     G_CALLBACK( format_change_cb ), ugtk_display );

   button_box = gtk_hbutton_box_new();
   gtk_box_pack_start( GTK_BOX( hbox ), button_box, 0, 0, 0 );

   widget = gtk_toggle_button_new_with_label( GTK_STOCK_MEDIA_PAUSE );
   gtk_button_set_use_stock( GTK_BUTTON( widget ), TRUE );
   gtk_container_add( GTK_CONTAINER( button_box ), widget );
   g_signal_connect( G_OBJECT( widget ), "toggled", G_CALLBACK( pause_toggle_cb ), ugtk_display );

   scrolled_window = gtk_scrolled_window_new( NULL, NULL );
   gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW( scrolled_window ), 
				   GTK_POLICY_AUTOMATIC, 
				   GTK_POLICY_AUTOMATIC );

   gtk_box_pack_start( GTK_BOX( vbox ), scrolled_window, TRUE, TRUE, 2 );

   gtk_scrolled_window_add_with_viewport( GTK_SCROLLED_WINDOW( scrolled_window ), ugtk_display );
   gtk_widget_show_all( window );

   g_object_set_data( G_OBJECT( ugtk_display ), "format_selection", ugtk_format_selection );
   g_object_set_data( G_OBJECT( ugtk_display ), "device_selection", device_selection );


   return window;
}


int main( int argc, char *argv[] )
{
   GtkWidget *display_window;
   GtkWidget *video_display;
   GtkWidget *device_selection;

   GtkWidget *widget;

   gtk_init (&argc, &argv);

   //
   // Create the main application window
   // 
   display_window  = create_application_window( );

   video_display = g_object_get_data( G_OBJECT( display_window ), "ugtk_display" );
   g_assert( video_display );
   device_selection = g_object_get_data( G_OBJECT( video_display ), "device_selection" );
   g_assert( device_selection );

   //
   // Create a window containing the properties for the 
   // video capture device and a device selection menu
   //
   widget = unicapgtk_property_dialog_new( );
   gtk_widget_show_all( widget );
   g_object_set_data( G_OBJECT( video_display ), "property_dialog", widget );

   //
   // Rescan devices and select first device
   //
   if( unicapgtk_device_selection_rescan( UNICAPGTK_DEVICE_SELECTION( device_selection ) ) > 0 )
   {
      gtk_combo_box_set_active (GTK_COMBO_BOX( device_selection ), 0);
   }

   gtk_main ();
	
   return 0;
}
