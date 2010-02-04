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

   TODO: 
     Since we are using only one buffer, it is possible that one can not achieve full frame rate 
	 with the DFG/1394!

 **/


#include <stdlib.h>
#include <stdio.h>
#include <unicap.h>
#include <unicap_status.h>

#include <SDL.h>

#define UYVY 0x59565955 /* UYVY (packed, 16 bits) */
#define FOURCC(a,b,c,d) (unsigned int)((((unsigned int)d)<<24)+(((unsigned int)c)<<16)+(((unsigned int)b)<<8)+a)


int main( int argc, char **argv )
{
	unicap_handle_t handle;
	unicap_device_t device;
	unicap_format_t format_spec;
	unicap_format_t format;

	unicap_property_t property;
	
	unicap_data_buffer_t buffer;
	unicap_data_buffer_t *returned_buffer;
	
	int i;

	SDL_Surface *screen;
	SDL_Overlay *overlay;

	int quit=0;


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
	  Let the user select a video norm
	 */
	unicap_void_property( &property ); // initialize property structure
	strcpy( property.identifier, "video norm" ); 
	unicap_get_property( handle, &property );
	for( i = 0; i < property.menu.menu_item_count; i++ )
	{
		printf( "%d: %s\n", i, property.menu.menu_items[i] );
	}

	// repeat until user selected a valid norm
	do
	{
		printf( "Select video norm: " );
		scanf( "%d", &i );
	}while( ( i < 0 ) || ( i > property.menu.menu_item_count ) ); 

	strcpy( property.menu_item, property.menu.menu_items[i] );
	
	if( !SUCCESS( unicap_set_property( handle, &property ) ) )
	{
		fprintf( stderr, "Failed to set property!\n" );
	}


	/*
	  Let the user select a video input
	 */
	unicap_void_property( &property ); // initialize property structure
	strcpy( property.identifier, "source" ); 
	unicap_get_property( handle, &property );
	for( i = 0; i < property.menu.menu_item_count; i++ )
	{
		printf( "%d: %s\n", i, property.menu.menu_items[i] );
	}

	// repeat until user selected a valid source
	do
	{
		printf( "Select video source: " );
		scanf( "%d", &i );
	}while( ( i < 0 ) || ( i > property.menu.menu_item_count ) ); 

	strcpy( property.menu_item, property.menu.menu_items[i] );
	
	if( !SUCCESS( unicap_set_property( handle, &property ) ) )
	{
		fprintf( stderr, "Failed to set property!\n" );
	}

	unicap_void_format( &format_spec );
	
	/*
	  Get the list of video formats
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
	  The DFG/1394 offers more than one size per video format. 
	  List them all and let the user select
	 */
	printf( "Available video format sizes: \n" );
	for( i = 0; i < format.size_count; i++ )
	{
		printf( "%d: %d x %d\n", i, format.sizes[i].width, format.sizes[i].height );
	}
	
	do
	{
		printf( "Select format size: " );
		scanf( "%d", &i );
	}while( ( i < 0 ) || ( i > format.size_count ) );
	format.size.width = format.sizes[i].width;
	format.size.height = format.sizes[i].height;

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
	
	overlay = SDL_CreateYUVOverlay( format.size.width, 
					format.size.height, 
					format.fourcc, 
					screen );
	if( overlay == NULL )
	{
	   fprintf( stderr, "Unable to create overlay: %s\n", SDL_GetError() );
	   exit( 1 );
	}

	/*
	  Pass the pointer to the overlay to the unicap data buffer. 
	 */
	buffer.data = overlay->pixels[0];	
	

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

		/*
		  Display the video data
		 */
		SDL_UnlockYUVOverlay( overlay );
		SDL_DisplayYUVOverlay( overlay, &rect );
		SDL_LockYUVOverlay(overlay);

		while( SDL_PollEvent( &event ) )
		{
			if( event.type == SDL_QUIT )
			{
				printf( "quit\n" );
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
