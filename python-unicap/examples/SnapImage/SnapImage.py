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
# Video image capture sample for pyunicap
#



import sys
import os
import getopt

import Image

import unicap


def save_buffer( name, filetype, imgnum, maximgnum, buf ):
    if maximgnum > 1:
	fmt = "%%0%dd" % (len(str(maximgnum)))
	if name.find( '.' ) != -1:
	    basename = name[:name.rfind('.')]
	    suffix = name[name.rfind('.')+1:]
	    filename = basename + fmt % (imgnum) + '.' + suffix
	else:
	    filename = name + fmt % (imgnum) 
    else:
	filename = name

    rgbbuf = buf.convert( 'RGB3' )
    img = Image.fromstring( 'RGB', imgbuf.format['size'], rgbbuf.tostring() )
    img.save( filename, filetype )

def print_usage():
    print """
    USAGE:
    python %s [-d device_id] [-n num-frames] [-k num-frames] [-t type] [-s WIDTHxHEIGHT] [-c FOURCC] [-m] filename
    python %s -l

    OPTIONS:

    -d or --device=identifier
        Specifies the video capture device to use. 

    -n or --numframes=n
        Specifies the number of frames to be captured. The filename will get extended by the image number
	( ie.: 'Image.jpg' will become 'Image01.jpg', 'Image02.jpg' and so on )

    -k or --skip=n
        Specifies the number of frames to be skipped before the first image gets saved. Usefull for
	webcams that need some time to adapt to the brightness conditions.

    -t or --type=filetype
        Specifies the file type of the image file. Must be a type supported by the Python Imaging Library
	( PIL ). Most common types are 'png' and 'jpeg'. If this parameter is not given, the file type is
	determined by the filename suffix.

    -s or --size=size
        Specifies the size of the video format in the form WIDTHxHEIGHT

    -c or --fourcc=fourcc
        Specifies the colour format to be requested by the device.

    -m or --memory
        Causes the images to be stored in the system memory before getting written to disk. This avoids
	frame loss due to disk activity.

    -l or --list:
        Prints a list of available video capture devices
    """ % ( sys.argv[0],sys.argv[0] )
    
    sys.exit(2)
    

try:
    opts, args = getopt.getopt( sys.argv[1:], "d:n:t:s:c:k:ml", ["device=", "numframes=", "type=", "size=", "fourcc=", "skip=", "memory", "list"] )
except getopt.error, msg:
    print_usage()

try:
    filename = args[0]
except IndexError:
    filename = None
    
device_id = None
numframes = 1
try:
    filetype = filename[filename.rfind('.')+1:]
except:
    filetype = 'jpg'
vidformat = {}
memorycapture = False
skip = 0

for o,a in opts:
    if ( o == "--device" ) or ( o == "-d" ):
	device_id = a
    elif ( o == "--numframes" ) or ( o == "-n" ):
	numframes = int( a )
    elif ( o == "--type" ) or ( o == "-t" ):
	filetype = a
    elif ( o == "--size" ) or ( o == "-s" ):
	width, height = [int(x) for x in a.split('x')]
	vidformat['size'] = (width,height)
    elif ( o == "--fourcc" ) or ( o == '-c' ):
	vidformat['fourcc'] = a
    elif ( o == "--skip" ) or ( o == "-k" ):
	skip = int(a)
    elif ( o == "--memory" ) or ( o == "-m" ):
	memorycapture = True
    elif ( o == "--list" ) or ( o == "-l" ):
	print "Available Devices:"
	for dev in unicap.enumerate_devices():
	    print '"'+dev['identifier']+'"'
	sys.exit(0)
	
if not filename:
    print_usage()    

# If no device_id was specified on the command line, use the first device
if not device_id:
    device_id = unicap.enumerate_devices()[0]['identifier']

device = unicap.Device( device_id )

if vidformat:
    device.set_format( vidformat )

# PIL expects 'jpeg' as an image format specifier
if filetype == 'jpg':
    filetype = 'jpeg'

device.start_capture()

for i in range( 0, skip ):
    device.wait_buffer()

images = []
image_count = 0
for i in range( 0, numframes ):
    imgbuf = device.wait_buffer()

    if memorycapture:
	images.append( imgbuf )
    else:
	image_count += 1
	save_buffer( filename, filetype, image_count, numframes, imgbuf )

for imgbuf in images:
    image_count += 1
    save_buffer( filename, filetype, image_count, numframes, imgbuf )
