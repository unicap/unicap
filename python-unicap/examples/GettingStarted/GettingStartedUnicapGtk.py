#
#  Copyright (C) 2009  Arne Caspari <arne@unicap-imaging.org>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
#

#
# Getting started sample program for pyunicap
#
# This example demonstrates how to
#
# - Create a VideoDisplay widget to display live video
# - Drawing circles and lines on the image displayed by the VideoDisplay
# - Create a DeviceSelection widget and let the user switch the video capture device
#

import gtk

import unicap
import unicapgtk
import gobject

class AppWindow( gtk.Window ):
    def __init__(self):
	gtk.Window.__init__(self)
	self.connect( 'delete-event', gtk.main_quit )

	self.do_save = False

	vbox = gtk.VBox()
	self.add( vbox )

	hbox = gtk.HBox()
	vbox.pack_start( hbox, False, True )

	capture_button = gtk.ToggleButton( label = "Capture" )
	hbox.pack_start( capture_button, False, False )
	capture_button.connect( 'toggled', self.__on_capture_toggled )

	button = gtk.Button( stock = gtk.STOCK_SAVE )
	hbox.pack_start( button, False, False )
	button.connect( 'clicked', self.__on_save_clicked )

	prop_button = gtk.Button( label = "Properties" )
	hbox.pack_start( prop_button, False, False )

	self.fmt_sel = unicapgtk.VideoFormatSelection()
	hbox.pack_end( self.fmt_sel, False, False )

	self.dev_sel = unicapgtk.DeviceSelection()
	self.dev_sel.rescan()
	self.dev_sel.connect( 'unicapgtk_device_selection_changed', self.__on_device_changed )
	hbox.pack_end( self.dev_sel, False, False )

	self.display = unicapgtk.VideoDisplay()
	self.display.connect( 'unicapgtk_video_display_predisplay', self.__on_new_frame )
	self.display.set_property( "scale-to-fit", True )
	vbox.pack_start( self.display, True, True )

	self.property_dialog = unicapgtk.PropertyDialog()
	self.property_dialog.connect( 'delete-event', self.property_dialog.hide_on_delete )
	prop_button.connect( 'clicked', lambda(x): self.property_dialog.present() )
	self.property_dialog.hide()

	dev = unicap.enumerate_devices()[0]
	self.dev_sel.set_device( dev )
	self.fmt_sel.set_device( dev )

	vbox.show_all()

	self.set_default_size( 640, 480 )

	capture_button.set_active( True )		

    def __on_device_changed(self, dev_sel, identifier ):
	self.display.set_device( identifier )
	self.property_dialog.set_device( identifier )
	self.fmt_sel.set_device( identifier )

    def __on_capture_toggled(self, button):
	if button.get_active():
	    self.display.start()
	else:
	    self.display.stop()

    def __on_save_clicked(self, button):
	pixbuf = self.display.get_still_image()
        dlg = gtk.FileChooserDialog( action=gtk.FILE_CHOOSER_ACTION_SAVE,
                                     buttons=(gtk.STOCK_CANCEL,gtk.RESPONSE_REJECT,
                                              gtk.STOCK_OK,gtk.RESPONSE_ACCEPT) )
        dlg.set_default_response( gtk.RESPONSE_ACCEPT )
        response = dlg.run()
        if response == gtk.RESPONSE_ACCEPT:
	    filename = dlg.get_filename()
	    if filename.endswith( '.jpg' ):
		filetype = 'jpeg'
	    elif filename.endswith( '.png' ):
		filetype = 'png'
	    else:
		filename += '.jpg'
		filetype = 'jpeg'
	    pixbuf.save( filename, filetype )
	dlg.destroy()

    def __on_new_frame( self, display, pointer ):
	# Note: The imgbuf created with wrap_gpointer is only valid during this callback
	imgbuf = unicap.ImageBuffer.wrap_gpointer( pointer )


	# Draw a circle and a cross on the video image
	width = imgbuf.format['size'][0]
	height = imgbuf.format['size'][1]				   
	
	imgbuf.draw_line( (width/2 - 50, height/2 - 50), (width/2 + 50, height/2 + 50), (255,0,0), 0 )
	imgbuf.draw_line( (width/2 + 50, height/2 - 50), (width/2 - 50, height/2 + 50), (255,0,0), 0 )
	imgbuf.draw_circle( (width/2, height/2), 50, (255,255,255) )
	

if __name__ == '__main__':
    window = AppWindow()
    window.show()

    gtk.main()
