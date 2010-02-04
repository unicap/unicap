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
   sdl_display.c

   This example demonstrates how to display a live video stream using libSDL

 **/


#include <stdlib.h>
#include <stdio.h>
#include <unicap.h>
#include <unicap_status.h>
#include "colorspace.h"

#include <SDL.h>
#include <time.h>

#include <jpeglib.h>

#define UYVY 0x59565955 /* UYVY (packed, 16 bits) */
#define FOURCC(a,b,c,d) (unsigned int)((((unsigned int)d)<<24)+(((unsigned int)c)<<16)+(((unsigned int)b)<<8)+a)

#define MIN(x,y) (x < y ? x : y )


int main( int argc, char **argv )
{
	unicap_handle_t handle;
	unicap_device_t device;
	unicap_format_t format_spec;
	unicap_format_t format;
	unicap_data_buffer_t buffer;
	unicap_data_buffer_t *returned_buffer;
	int width, height;
	
	int i;

	SDL_Surface *screen;
	SDL_Overlay *overlay;

	int quit=0;
	int imgcnt = 0;

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
	format_spec.fourcc = FOURCC('U','Y','V','Y');
	
	/*
	  Get the list of video formats of the colorformat UYVY
	 */
	for( i = 0; SUCCESS( unicap_enumerate_formats( handle, &format_spec, &format, i ) ); i++ )
	{
		printf( "%d: %s [%dx%d]\n", 
				i,
				format.identifier, 
				format.size.width, 
				format.size.height );
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
	
   /*
     If a video format has more than one size, ask for which size to use
   */
   if( format.size_count )
   {
      for( i = 0; i < format.size_count; i++ )
      {
	 printf( "%d: %dx%d\n", i, format.sizes[i].width, format.sizes[i].height );
      }
      do
      {
	 printf( "Select video format size: " );
	 scanf( "%d", &i );
      }while( ( i < 0 ) && ( i > format.size_count ) );
      format.size.width = format.sizes[i].width;
      format.size.height = format.sizes[i].height;
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

	/**
	   Init SDL & SDL_Overlay
	 **/
	if ( SDL_Init(SDL_INIT_VIDEO) < 0 )
	{
	   fprintf(stderr, "Failed to initialize SDL:  %s\n", SDL_GetError());
	   exit(1);
	}
	
	atexit(SDL_Quit);

	/*
	  Make sure the video window does not get too big. 
	 */
	width = MIN( format.size.width, 800 );
	height = MIN( format.size.height, 600 );

	screen = SDL_SetVideoMode( width, height, 32, SDL_HWSURFACE);
	if ( screen == NULL ) {
	   fprintf(stderr, "Unable to set video mode: %s\n", SDL_GetError());
	   exit(1);
	}
	
	overlay = SDL_CreateYUVOverlay( format.size.width, 
									format.size.height, SDL_UYVY_OVERLAY, screen );
	if( overlay == NULL )
	{
	   fprintf( stderr, "Unable to create overlay: %s\n", SDL_GetError() );
	   exit( 1 );
	}

	/*
	  Pass the pointer to the overlay to the unicap data buffer. 
	 */
	buffer.data = overlay->pixels[0];	
   buffer.buffer_size = format.size.width * format.size.height * format.bpp / 8;
	
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
		SDL_Rect rect;
		SDL_Event event;

		rect.x = 0;
		rect.y = 0;
		rect.w = width;
		rect.h = height;
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
		}

		/*
		  Display the video data
		 */
		SDL_UnlockYUVOverlay( overlay );
		SDL_DisplayYUVOverlay( overlay, &rect );

		while( SDL_PollEvent( &event ) )
		{
			switch( event.type )
			{
				case SDL_QUIT:
					quit = 1;
					break;
					
				case SDL_MOUSEBUTTONDOWN:
				{
					unsigned char *pixels;
					struct jpeg_compress_struct cinfo;
					struct jpeg_error_mgr jerr;
					FILE *outfile;
					JSAMPROW row_pointer[1];
					int row_stride;
					char filename[128];

					struct timeval t1, t2;
					unsigned long long usecs;
					
					sprintf( filename, "%04d.jpg", imgcnt++ );
					
					cinfo.err = jpeg_std_error(&jerr);
					/* Now we can initialize the JPEG compression object. */
					jpeg_create_compress(&cinfo);
					if ((outfile = fopen( filename, "wb" ) ) == NULL ) 
					{
						fprintf(stderr, "can't open %s\n", "file");
						exit(1);
					}
					jpeg_stdio_dest(&cinfo, outfile);
					
					cinfo.image_width = format.size.width; 	/* image width and height, in pixels */
					cinfo.image_height = format.size.height;
					cinfo.input_components = 3;		/* # of color components per pixel */
					cinfo.in_color_space = JCS_RGB; 	/* colorspace of input image */
					
					jpeg_set_defaults(&cinfo);
					pixels = malloc( format.size.width * format.size.height * 3 );
					uyvy2rgb24( pixels, returned_buffer->data,
						    format.size.width * format.size.height * 3,
						    format.size.width * format.size.height * 2 );

					gettimeofday( &t1, NULL );
					jpeg_start_compress(&cinfo, TRUE);
					while( cinfo.next_scanline < cinfo.image_height )
					{
						row_pointer[0] = &pixels[cinfo.next_scanline * format.size.width * 3 ];
						(void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
					}
					jpeg_finish_compress(&cinfo);
					gettimeofday( &t2, NULL );

					usecs = t2.tv_sec * 1000000LL + t2.tv_usec;
					usecs -= ( t1.tv_sec * 1000000LL + t1.tv_usec );
					
					printf( "Compression took: %lld usec\n", usecs );

					/* After finish_compress, we can close the output file. */
					fclose(outfile);
					jpeg_destroy_compress(&cinfo);
					
					free( pixels );
				}
				
				break;
				
				
				default: 
					break;
			}
		}
		SDL_LockYUVOverlay(overlay);
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

	SDL_Quit();
	
	return 0;
}
