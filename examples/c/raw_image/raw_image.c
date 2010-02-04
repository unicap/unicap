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
   raw_image.c

   Sample program demonstrating the basic usage of the unicap library


   This example opens the first device found by the unicap library, selects the first 
   video format supported by the device and captures 1 frame of image data. 

   The image data is then writen as raw data to stdout.
**/

#include <stdlib.h>
#include <stdio.h>
#include <unicap.h>
#include <unicap_status.h>

int main( int argc, char **argv )
{
	unicap_handle_t handle;
	unicap_device_t device;
	unicap_format_t format;
	unicap_data_buffer_t buffer;
	unicap_data_buffer_t *returned_buffer;

	unsigned char *image_buffer;
	
	/*
	  Get the first device found by the unicap library
	 */
	if( !SUCCESS( unicap_enumerate_devices( NULL, &device, 0 ) ) )
	{
		fprintf( stderr, "No device found!\n" );
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
	
	/*
	  Get the first video format supported by the device
	 */
	if( !SUCCESS( unicap_enumerate_formats( handle, NULL, &format, 0 ) ) )
	{
		fprintf( stderr, "Failed to get video format\n" );
		exit( 1 );
	}
	
	/* 
	   If a format provides more than one size, set the first one
	*/
	if( format.size_count )
	{
		format.size.width = format.sizes[0].width;
		format.size.height = format.sizes[0].height;
	}

	/*
	  Set this video format
	 */
	if( !SUCCESS( unicap_set_format( handle, &format ) ) )
	{
		fprintf( stderr, "Failed to set video format\n" );
		exit( 1 );
	}

	/*
	  Initialize the image buffer
	 */
	memset( &buffer, 0x0, sizeof( unicap_data_buffer_t ) );
	
	/*
	  Allocate memory for the image buffer
	 */
	if( !( image_buffer = (unsigned char *)malloc( format.buffer_size ) ) )
	{
		fprintf( stderr, "Failed to allocate %d bytes\n" );
		exit( 1 );
	}
	buffer.data = image_buffer;
	buffer.buffer_size = format.buffer_size;


	/*
	  Start the capture process on the device
	 */
	if( !SUCCESS( unicap_start_capture( handle ) ) )
	{
		fprintf( stderr, "Failed to start capture on device: %s\n", device.identifier );
		exit( 1 );
	}
	
	/*
	  Queue the buffer
	  
	  The buffer now gets filled with image data by the capture device
	*/
	if( !SUCCESS( unicap_queue_buffer( handle, &buffer ) ) )
	{
		fprintf( stderr, "Failed to queue a buffer on device: %s\n", device.identifier );
		exit( 1 );
	}
	
	/*
	  Wait until the image buffer is ready
	*/
	if( !SUCCESS( unicap_wait_buffer( handle, &returned_buffer ) ) )
	{
		fprintf( stderr, "Failed to wait for buffer on device: %s\n", device.identifier );
		exit( 1 );
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

	fprintf( stderr, "Output raw image data: \nwidth: %d\nheight: %d\nbpp: %d\nFOURCC: %c%c%c%c\nBufferSize: %d\n", 
			 format.size.width, 
			 format.size.height,
			 format.bpp, 
			 format.fourcc & 0xff, 
			 ( format.fourcc >> 8 ) & 0xff, 
			 ( format.fourcc >> 16 ) & 0xff, 
			 ( format.fourcc >> 24 ) & 0xff , 
			 returned_buffer->buffer_size );

	if( !returned_buffer->buffer_size )
	{
		fprintf( stderr, "FATAL: Returned a buffer size of 0!\n" );
		exit( -1 );
	}

	if( !returned_buffer->data )
	{
		fprintf( stderr, "FATAL: Returned buffer is NULL!\n" );
		exit( -1 );
	}

	// If the buffer has a depth of 8 or 24 bits per pixel, print a valid pnm header
	// In this case you can pipe the output to eg. 'image.pnm' and should be able 
	// to open it directly with 'gimp'
	switch( format.bpp )
	{
		case 8:
		{
			// Assume Greyscale data
			printf( "P5\n%d %d 255\n", format.size.width, format.size.height );
		}
		break;
		
		case 24: 
		{
			// Assume RGB24 data
			printf( "P6\n%d %d 255\n", format.size.width, format.size.height );
		}
		break;
		
		default:
			break;
	}	

	printf( "retbuffer->data: %p\n", returned_buffer->data );

	if( fwrite( returned_buffer->data, returned_buffer->buffer_size, 1, stdout ) < 0 )
	{
		perror( "Write failed" );
	}
	
	

	free( image_buffer );

	return 0;
}
