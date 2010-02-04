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
#include <sys/time.h>
#include <time.h>

#include <SDL.h>

#define BUFFERS 4

#define UYVY 0x59565955 /* UYVY (packed, 16 bits) */
#define FOURCC(a,b,c,d) (unsigned int)((((unsigned int)d)<<24)+(((unsigned int)c)<<16)+(((unsigned int)b)<<8)+a)


static void print_fps( )
{
   static struct timeval next,current;
   static int count = 0;
   suseconds_t usec;

   if( count == 0 )
   {
      gettimeofday(&next, NULL);
      next.tv_sec += 1;
   }

   count++;
	
   gettimeofday(&current, NULL);

   if( timercmp( &next, &current, <= ) )
   {
      printf( "FPS: %d\n", count );
      count = 0;
   }
}	

int main( int argc, char **argv )
{
   unicap_handle_t handle;
   unicap_device_t device;
   unicap_format_t format_spec;
   unicap_format_t format;
   unicap_data_buffer_t buffers[BUFFERS];
   unicap_data_buffer_t *returned_buffer;
   int quit=0;	
   int i;

   SDL_Surface *screen;
   SDL_Overlay *overlay;

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
   else if( i < 0 )
   {
      printf( "No video device found!\n" );
      return 0;
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
   for( i = 0; SUCCESS( unicap_enumerate_formats( handle, NULL/*&format_spec*/, &format, i ) ); i++ )
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
     When a video format has more than one size, ask for which size to use
   */
   if( format.size_count )
   {
      printf( "Select video format size!\n" );
      for( i = 0; i < format.size_count; i++ )
      {
	 printf( "%d: %dx%d\n", i, format.sizes[i].width, format.sizes[i].height );
      }
      do
      {
	 printf( "Selection: " );
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
     Initialize the image buffer
   */
   for( i = 0; i < BUFFERS; i++ )
   {
      memset( &buffers[i], 0x0, sizeof( unicap_data_buffer_t ) );
   }

   /*
     Pass the pointer to the overlay to the unicap data buffer. 
   */
   for( i = 0; i < BUFFERS; i++ )
   {
      buffers[i].data = overlay->pixels[0];	
      buffers[i].buffer_size = format.size.width * format.size.height * format.bpp / 8;
   }
	
   /*
     Start the capture process on the device
   */
   if( !SUCCESS( unicap_start_capture( handle ) ) )
   {
      fprintf( stderr, "Failed to start capture on device: %s\n", device.identifier );
      exit( 1 );
   }
	

   for( i = 0; i < BUFFERS; i++ )
   {
      unicap_queue_buffer( handle, &buffers[i] );
   }

   while( !quit )
   {
      SDL_Rect rect = { x: 0, y: 0, w: format.size.width, h: format.size.height };
      SDL_Event event;
      int buffers_ready;
      unicap_data_buffer_t *ready_buffer;
      unicap_status_t status;

      //
      // 
      //
      if( !SUCCESS( unicap_poll_buffer( handle, &buffers_ready ) ) )
      {
	 buffers_ready = 1;
      }
		
      for( i = 0; i < buffers_ready - 1; i++ )
      {
	 /*
	   Wait until the image buffer is ready
	 */
	 if( !SUCCESS( unicap_wait_buffer( handle, &returned_buffer ) ) )
	 {
	    fprintf( stderr, "Failed to wait for buffer on device: %s\n", device.identifier );
	 }

	 print_fps();

	 unicap_queue_buffer( handle, returned_buffer );
      }



      status = unicap_wait_buffer( handle, &returned_buffer );
      if( !SUCCESS( status  ) )
      {
	 fprintf( stderr, "Failed to wait for buffer on device: %s\n", device.identifier );
      }
      print_fps();

      /*
	Display the video data
      */
      SDL_UnlockYUVOverlay( overlay );
      SDL_DisplayYUVOverlay( overlay, &rect );
      SDL_LockYUVOverlay(overlay);

      if( SUCCESS( status ) )
      {
	 unicap_queue_buffer( handle, returned_buffer );
      }

      while( SDL_PollEvent( &event ) )
      {
	 if( event.type == SDL_QUIT )
	 {
	    printf( "Quit\n" );
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
