/*
    unicap
    Copyright (C) 2004  Arne Caspari

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
   xv_display.c

   This example demonstrates how to display a live video stream using XVideo extensions

 **/


#include <stdlib.h>
#include <stdio.h>
#include <unicap.h>
#include <unicap_status.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xvlib.h>
#include <X11/keysym.h>

#include "xv.h"

#define UYVY 0x59565955 /* UYVY (packed, 16 bits) */
#define YUYV 0x56595559
#define YUY2 0x32595559

int main( int argc, char **argv )
{
	unicap_handle_t handle;
	unicap_device_t device;
	unicap_format_t format_spec;
	unicap_format_t format;
	unicap_data_buffer_t buffer;
	unicap_data_buffer_t *returned_buffer;
	
	int i;

	int quit=0;

	XEvent xevent;
	XGCValues xgcvalues;
	Display *display=NULL;
	Window window=(Window)NULL;
	GC gc;
	int connection=-1;	
	xv_handle_t xvhandle;



	printf( "select video device\n" );
	for( i = 0; SUCCESS( unicap_enumerate_devices( NULL, &device, i ) ); i++ )
	{
		printf( "%i: %s\n", i, device.identifier );
	}
	if( --i > 0 )
	{
		printf( "Select video capture device: " );
		scanf( "%d", &i );
	}

	if( !SUCCESS( unicap_enumerate_devices( NULL, &device, i ) ) )
	{
		fprintf( stderr, "Failed to get info for device '%s'\n", device.identifier );
		exit( 1 );
	}

	/*
	  Acquire a handle to this device
	 */
	if( !SUCCESS( unicap_open( &handle, &device ) ) )
	{
		fprintf( stderr, "Failed to open device: %s\n", device.identifier );
		exit( 1 );
	}

	printf( "Opened video capture device: %s\n", device.identifier );

	/*
	  Create a format specification to limit the list of formats returned by 
	  unicap_enumerate_formats to the ones with the color format 'UYVY'
	 */
	unicap_void_format( &format_spec );
	format_spec.fourcc = YUYV;
	
	/*
	  Get the list of video formats of the colorformat UYVY
	 */
	for( i = 0; SUCCESS( unicap_enumerate_formats( handle, &format_spec, &format, i ) ); i++ )
	{
	   printf( "fcc: %08x  ( %08x )\n", format.fourcc, format_spec.fourcc );
		if( format.fourcc == YUYV )
		{
			printf( "%d: %s [%dx%d]\n", 
					i,
					format.identifier, 
					format.size.width, 
					format.size.height );
		}
	}
	if( --i > 0 )
	{
		printf( "Select video format: " );
		scanf( "%d", &i );
	}
	if( !SUCCESS( unicap_enumerate_formats( handle, &format_spec, &format, i ) ) )
	{
		fprintf( stderr, "Failed to get video format\n" );
		exit( 1 );
	}
	
/* 	format.size.width = 640; */
/* 	format.size.height = 480; */
	

	/*
	  Set this video format
	 */
	if( !SUCCESS( unicap_set_format( handle, &format ) ) )
	{
		fprintf( stderr, "Failed to set video format\n" );
		exit( 1 );
	}
	if( !SUCCESS( unicap_get_format( handle, &format ) ) )
	{
		fprintf( stderr, "Failed to get video format\n" );
		exit( 1 );
	}

	/*
	  Initialize the image buffer
	 */
	memset( &buffer, 0x0, sizeof( unicap_data_buffer_t ) );

	/*
	  Initialize Xv and create a X window
	 */
	xvhandle = malloc( sizeof( struct _xv_handle ) );
	if( !xvhandle )
	{
		fprintf( stderr, "Failed to allocate memory for xvhandle!\n" );
		exit( 1 );
	}
	
	display = XOpenDisplay( getenv("DISPLAY") );
	if( !display )
	{
		fprintf( stderr, "Failed to open X display\n" );
		exit( 1 );
	}
	
	if( xv_init( xvhandle,
		     display,
		     YUY2,
		     format.size.width,
		     format.size.height,
		     format.bpp ) < 0 )
	{
		fprintf( stderr, "Failed to initialize Xv\n" );
		exit( 1 );
	}

	window = XCreateSimpleWindow( display, DefaultRootWindow( display ), 0, 0,
								  format.size.width, format.size.height,
								  0, WhitePixel( display, DefaultScreen( display ) ), 0x010203 );
	
	XSelectInput(display,window,StructureNotifyMask|KeyPressMask);
	XMapWindow(display,window);
	connection=ConnectionNumber(display);
	gc=XCreateGC(display,window,0,&xgcvalues);
	printf( "gc = %p\n", gc );
	
	xvhandle->drawable = window;
	xvhandle->gc = gc;
	
	/*
	 */
	buffer.data = xvhandle->image->data;
	printf( "buffer_size: %d\n" , format.buffer_size );
	
	if( !buffer.data )
	{
		fprintf( stderr, "Failed to allocate memory for the image buffer\n" );
		exit( 1 );
	}
	
	/*
	  Start the capture process on the device
	 */
	if( !SUCCESS( unicap_start_capture( handle ) ) )
	{
		fprintf( stderr, "Failed to start capture on device: %s\n", device.identifier );
		exit( 1 );
	}
	

	while( !quit )
	{
		/*
		  Queue the buffer
		  
		  The buffer now gets filled with image data by the capture device
		*/
		if( !SUCCESS( unicap_queue_buffer( handle, &buffer ) ) )
		{
			fprintf( stderr, "Failed to queue a buffer on device: %s\n", device.identifier );
/* 			exit( 1 ); */
		}
		
		/*
		  Wait until the image buffer is ready
		*/
		if( !SUCCESS( unicap_wait_buffer( handle, &returned_buffer ) ) )
		{
			fprintf( stderr, "Failed to wait for buffer on device: %s\n", device.identifier );
/* 			exit( 1 ); */
		}

		printf( "foo!\n" );

		/*
		  Display the video data
		 */
		xv_update_image( xvhandle,
				 returned_buffer->data,
				 returned_buffer->buffer_size,
				 returned_buffer->format.size.width,
				 returned_buffer->format.size.height );
/* 		XFlush(display); */
		
		while(XPending(display)>0)
		{
			XNextEvent(display,&xevent);
			switch(xevent.type)
			{
				case XK_q:
				case XK_Q:
					quit = 1;
					break;
					
				default:
					break;
			}
		}
		
	}
	
	/*
	  Stop the device
	 */
	if( !SUCCESS( unicap_stop_capture( handle ) ) )
	{
		fprintf( stderr, "Failed to stop capture on device: %s\n", device.identifier );
	}

	/*
	  Close the device 

	  This invalidates the handle
	 */
	if( !SUCCESS( unicap_close( handle ) ) )
	{
		fprintf( stderr, "Failed to close the device: %s\n", device.identifier );
	}

	return 0;
}
