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
#include <status.h>

#include <linux/types.h>

#include <SDL.h>

#define UYVY 0x59565955 /* UYVY (packed, 16 bits) */
#define FOURCC(a,b,c,d) (unsigned int)((((unsigned int)d)<<24)+(((unsigned int)c)<<16)+(((unsigned int)b)<<8)+a)


size_t y4112rgb24( __u8 *dest, __u8 *source, size_t dest_size, size_t source_size )
{
	int i;
	int dest_offset = 0;
	
/* 	if( dest_size < ( source_size *2 ) ) */
/* 	{ */
/* 		return 0; */
/* 	} */

	for( i = 0; i < source_size; i+=6 )
	{
		__u8 *r, *b, *g;
		__u8 *y1, *y2, *y3, *y4, *u, *v;
		
		float fr, fg, fb;
		float fy1, fy2, fy3, fy4, fu, fv;

		r = dest + dest_offset;
		g = r + 1;
		b = g + 1;
		
		u = source + i;
		y1 = u + 1;
		y2 = y1 + 1;
		v = y2 + 1;
		y3 = v + 1;
		y4 = y3 + 1;

		fu = *u;
		fv = *v;
		fy1= *y1;
		fy2= *y2;
		fy3= *y3;
		fy4= *y4;

		fr = fy1 - 0.0009267 * ( fu - 128 ) + 1.4016868 * ( fv - 128 );
		fg = fy1 - 0.3436954 * ( fu - 128 ) - 0.7141690 * ( fv - 128 );
		fb = fy1 + 1.7721604 * ( fu - 128 ) + 0.0009902 * ( fv - 128 );

		*r = (__u8) ( fr > 255 ? 255 : ( fr < 0 ? 0 : fr ) );
		*g = (__u8) ( fg > 255 ? 255 : ( fg < 0 ? 0 : fg ) );
		*b = (__u8) ( fb > 255 ? 255 : ( fb < 0 ? 0 : fb ) );
		
		
		dest_offset += 3;
		
		r = dest + dest_offset;
		g = r + 1;
		b = g + 1;
		
		fr = 255;//fy2 - 0.0009267 * ( fu - 128 ) + 1.4016868 * ( fv - 128 );
		fg = 255;//fy2 - 0.3436954 * ( fu - 128 ) - 0.7141690 * ( fv - 128 );
		fb = fy2 + 1.7721604 * ( fu - 128 ) + 0.0009902 * ( fv - 128 );

		*r = (__u8) ( fr > 255 ? 255 : ( fr < 0 ? 0 : fr ) );
		*g = (__u8) ( fg > 255 ? 255 : ( fg < 0 ? 0 : fg ) );
		*b = (__u8) ( fb > 255 ? 255 : ( fb < 0 ? 0 : fb ) );

		dest_offset += 3;

		r = dest + dest_offset;
		g = r + 1;
		b = g + 1;
		
		fr = fy3 - 0.0009267 * ( fu - 128 ) + 1.4016868 * ( fv - 128 );
		fg = fy3 - 0.3436954 * ( fu - 128 ) - 0.7141690 * ( fv - 128 );
		fb = fy3 + 1.7721604 * ( fu - 128 ) + 0.0009902 * ( fv - 128 );

		*r = (__u8) ( fr > 255 ? 255 : ( fr < 0 ? 0 : fr ) );
		*g = (__u8) ( fg > 255 ? 255 : ( fg < 0 ? 0 : fg ) );
		*b = (__u8) ( fb > 255 ? 255 : ( fb < 0 ? 0 : fb ) );

		dest_offset += 3;

		r = dest + dest_offset;
		g = r + 1;
		b = g + 1;
		
		fr = fy4 - 0.0009267 * ( fu - 128 ) + 1.4016868 * ( fv - 128 );
		fg = fy4 - 0.3436954 * ( fu - 128 ) - 0.7141690 * ( fv - 128 );
		fb = fy4 + 1.7721604 * ( fu - 128 ) + 0.0009902 * ( fv - 128 );

		*r = (__u8) ( fr > 255 ? 255 : ( fr < 0 ? 0 : fr ) );
		*g = (__u8) ( fg > 255 ? 255 : ( fg < 0 ? 0 : fg ) );
		*b = (__u8) ( fb > 255 ? 255 : ( fb < 0 ? 0 : fb ) );

		dest_offset += 3;

		
	}

// From SciLab : This is the good one.
	//r = 1 * y -  0.0009267*(u-128)  + 1.4016868*(v-128);^M
	//g = 1 * y -  0.3436954*(u-128)  - 0.7141690*(v-128);^M
	//b = 1 * y +  1.7721604*(u-128)  + 0.0009902*(v-128);^M

	// YUV->RGB
	// r = 1.164 * (y-16) + 1.596*(v-128);
	// g = 1.164 * (y-16) + 0.813*(v-128) - 0.391*(u-128);
	// b = 1.164 * (y-16) + 2.018*(u-128);
	return source_size * 2;
}


int main( int argc, char **argv )
{
	unicap_handle_t handle;
	unicap_device_t device;
	unicap_format_t format_spec;
	unicap_format_t format;
	unicap_data_buffer_t buffer;
	unicap_data_buffer_t *returned_buffer;
	
	int i;

	SDL_Surface *screen;
	SDL_Surface *rgb_surface;
	Uint32 rmask, gmask, bmask, amask;

	int quit=0;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
	rmask = 0xff000000;
	gmask = 0x00ff0000;
	bmask = 0x0000ff00;
	amask = 0x000000ff;
#else
	rmask = 0x000000ff;
	gmask = 0x0000ff00;
	bmask = 0x00ff0000;
	amask = 0xff000000;
#endif

	printf( "select video device\n" );
	for( i = 0; SUCCESS( unicap_enumerate_devices( NULL, &device, i ) ); i++ )
	{
		printf( "%i: %s\n", i, device.identifier );
	}
	if( --i > 0 )
	{
		printf( "Select video capture device: %d" );
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
	format_spec.fourcc = FOURCC('Y','4','1','1');
	
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
	
	format.size.width = 640;
	format.size.height = 480;

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

	screen = SDL_SetVideoMode( format.size.width, format.size.height, 32, SDL_HWSURFACE);
	if ( screen == NULL ) {
	   fprintf(stderr, "Unable to set video mode: %s\n", SDL_GetError());
	   exit(1);
	}
	
	rgb_surface = SDL_CreateRGBSurface( SDL_SWSURFACE, 
										format.size.width, 
										format.size.height, 
										24, 
										0, 
										0, 
										0, 
										0 );

	/*
	  Pass the pointer to the overlay to the unicap data buffer. 
	 */
	buffer.data = malloc( 640 * 480 * 2 );
	
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

		rect.x = rect.y = 0;
		rect.w = format.size.width;
		rect.h = format.size.height;

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

		SDL_LockSurface( rgb_surface );
		
		y4112rgb24( rgb_surface->pixels,
					buffer.data,
					buffer.format.size.width * buffer.format.size.height * 3,
					buffer.buffer_size );
		

		SDL_UnlockSurface( rgb_surface );

		SDL_BlitSurface( rgb_surface, NULL, screen, NULL );
		SDL_UpdateRect( screen, 0, 0, 0, 0 );

		/*
		  Display the video data
		 */

		while( SDL_PollEvent( &event ) )
		{
			if( event.type == SDL_QUIT )
			{
				quit=1;
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

	SDL_Quit();
	
	return 0;
}
