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
# - Create a device object
# - Get and set video formats
# - Get and set device properties
# - Capture a video image
# - Convert an image buffer to a different color space
#

import unicap
import time


# Create a device object with the first video capture device found
dev = unicap.Device( unicap.enumerate_devices()[1] )

# Get a list of supported video formats
fmts = dev.enumerate_formats()

# Set the first video format in the list at maximum resolution
#fmt = fmts[0]
#fmt['size'] = fmt['max_size']
#dev.set_format( fmt )

# Get a list of supported device properties
# The properties in this list are set to their default values
props = dev.enumerate_properties()


prop = dev.get_property( 'Frame Rate')
prop['value'] = 20.0
dev.set_property( prop )

# Get the current state of the first property
prop = dev.get_property( 'Shutter' )
prop['value'] = 1.0
dev.set_property( prop )



# Start capturing video
dev.start_capture()

dev.set_property( prop )
imgbuf = dev.wait_buffer( 10 )

rates = [ 0.1, 0.25, 0.5, 0.75, 1.0, 1.5, 2.0, 2.5, 3.0, 3.5, 4.0 ]


for r in rates:
    prop['value'] = r
    dev.set_property( prop )
    

    for i in range( 0, 10 ):	
	t1 = time.time()
	imgbuf = dev.wait_buffer( 10 )
	dt = time.time() - t1
	print 'dt: %f  ===> %f' % ( dt, 1.0/dt )
    print 'Captured an image. Colorspace: ' + str( imgbuf.format['fourcc'] ) + ', Size: ' + str( imgbuf.format['size'] )
	


# Convert the image to RGB3
rgbbuf = imgbuf.convert( 'RGB3' )


# Stop capturing video
dev.stop_capture()

