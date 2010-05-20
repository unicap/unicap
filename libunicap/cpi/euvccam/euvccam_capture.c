/*
  unicap euvccam plugin

  Copyright (C) 2009  Arne Caspari

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


#include <config.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <errno.h>
#include <linux/usbdevice_fs.h>
#include <semaphore.h>
#include <pthread.h>
#include <unicap.h>
#include <unicap_cpi.h>
#include <stdlib.h>
#include <time.h>


#include "euvccam_cpi.h"
#include "euvccam_devspec.h"
#include "euvccam_colorproc.h"
#include "queue.h"



#define MAX_READ_WRITE	   (16 * 1024)
#define NUM_URBS           32
#define NUM_SYSTEM_BUFFERS 4

static void sighandler(int sig )
{
	return;
}


struct buffer_done_context
{
   sem_t sema;
   unicap_data_buffer_t *buffer;
   unicap_data_buffer_t *conv_buffer;
   euvccam_convert_func_t conv_func;
   euvccam_handle_t euvccam_handle;
   volatile int quit;
   unicap_handle_t unicap_handle;
   unicap_event_callback_t event_callback;
};

struct timeout_thread_context
{
   volatile euvccam_handle_t handle;
   struct timeval timeout;
   volatile int quit;
};

static void *timeout_thread( struct timeout_thread_context *context )
{
   while (!context->quit){
      struct timeval tv;
      
      gettimeofday (&tv, NULL);
      if ((context->timeout.tv_sec + 2) < tv.tv_sec){
	 TRACE ("capture timeout\n");
	 pthread_kill (context->handle->capture_thread, SIGUSR1);
      }

      sleep( 1 );
   }
   
   return NULL;
}


static void *buffer_done_thread( struct buffer_done_context *context )
{
   
   while( !context->quit ){
      struct timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts);
      ts.tv_sec += 1;
      if( sem_timedwait( &context->sema, &ts) != 0 )
	 continue;
      
      if( context->quit )
	 break;

      if( context->event_callback ){
	 if( context->conv_buffer && context->conv_func ){
	    if( context->euvccam_handle->debayer_data.wb_auto_mode != WB_MODE_MANUAL ){
	       euvccam_colorproc_auto_wb( context->euvccam_handle, context->buffer );
	       if( context->euvccam_handle->debayer_data.wb_auto_mode == WB_MODE_ONE_PUSH )
		  context->euvccam_handle->debayer_data.wb_auto_mode = WB_MODE_MANUAL;
	    }
	    context->conv_func( context->euvccam_handle, context->conv_buffer, context->buffer );
	    context->event_callback( context->unicap_handle, UNICAP_EVENT_NEW_FRAME, context->conv_buffer );
	 }else{
	    context->event_callback( context->unicap_handle, UNICAP_EVENT_NEW_FRAME, context->buffer );
	 }
      }
   }   

   return NULL;
}


static void buffer_done( euvccam_handle_t handle, struct buffer_done_context *context )
{
   gettimeofday( &context->buffer->fill_time, NULL );
   sem_post( &context->sema );
}

static int __inline__ submit_urb( int fd, struct usbdevfs_urb *urb )
{
   return ioctl( fd, USBDEVFS_SUBMITURB, urb );
}

static int __inline__ reap_urb( int fd, struct usbdevfs_urb **urb )
{
   return ioctl( fd, USBDEVFS_REAPURB, urb );
}


static void *capture_thread( euvccam_handle_t handle )
{
   unsigned char *usb_buffer[NUM_URBS];
   int i;

   int bytes_done = 0;
   int current_buffer = 0;
   int wait_transfer_done = 0;
   int usb_buffer_size = 0;

   pthread_t buffer_done_thread_id = 0;
   struct buffer_done_context context;
   
   unicap_data_buffer_t system_buffers[ NUM_SYSTEM_BUFFERS ];

   struct usbdevfs_urb *urbs[NUM_URBS];

   pthread_t capture_timeout_thread_id = 0;
   struct timeout_thread_context timeout_context;
   
   
   

   if( handle->current_format->usb_buffer_size == 0 ){
      TRACE( "Invalid video format!\n" );
      abort();
      return NULL;
   }

   usb_buffer_size = handle->current_format->format.size.width * handle->current_format->format.size.height;

   signal( SIGUSR1, sighandler );

   sem_init( &context.sema, 0, 0 );
   context.buffer = NULL;
   context.conv_buffer = NULL;
   context.conv_func = NULL;
   context.quit = 0;
   context.euvccam_handle = handle;
   context.unicap_handle = handle->unicap_handle;
   context.event_callback = handle->event_callback;

   if( handle->current_format->convert_func ){
      context.conv_buffer = calloc( 1, sizeof( unicap_data_buffer_t ) );
      unicap_copy_format( &context.conv_buffer->format, &handle->current_format->format );
      context.conv_buffer->buffer_size = context.conv_buffer->format.buffer_size;
      context.conv_buffer->data = malloc( context.conv_buffer->format.buffer_size );
      context.conv_buffer->type = UNICAP_FLAGS_BUFFER_TYPE_SYSTEM;
      context.conv_buffer->flags = 0;
      context.conv_buffer->frame_number = 0;
      context.conv_func = handle->current_format->convert_func;
   }
   
   if( pthread_create( &buffer_done_thread_id, NULL, (void*(*)(void*))buffer_done_thread, &context ) != 0 ){
      sem_destroy( &context.sema );
      return NULL;
   }

   gettimeofday( &timeout_context.timeout, NULL );
   timeout_context.handle = handle;
   timeout_context.quit = 0;
   if( pthread_create ( &capture_timeout_thread_id, NULL, (void*(*)(void*))timeout_thread, &timeout_context) != 0 ){
      return NULL;
   }

   for( i = 0; i < NUM_URBS; i++ ){  
      struct usbdevfs_urb *urb;
      int ret;

      usb_buffer[i] = (unsigned char *)malloc( MAX_READ_WRITE );
      if( !usb_buffer[i] ){
	 int j;
	 for( j = 0; j < i; j++ ){
	    free( usb_buffer[j] );
	 }
	 return NULL;
      }

      urb = malloc( sizeof( struct usbdevfs_urb ) );
      if( !urb )
	 return NULL;
      // TODO: free URBs and buffers on failure
      memset( urb, 0x0, sizeof( struct usbdevfs_urb ) );

      urb->type = USBDEVFS_URB_TYPE_BULK;
      urb->endpoint = handle->streaming_endpoint;
      urb->buffer = usb_buffer[i];
      urb->buffer_length = MAX_READ_WRITE;

      ret = submit_urb( handle->dev.fd, urb );
      if( ret < 0 ){
	 TRACE( "Failed to submit urb!\n" );
	 perror( "ioctl" );
	 return NULL;
      }

      urbs[i] = urb;
   }

   for( i = 0; i < NUM_SYSTEM_BUFFERS; i++ ){
      unicap_copy_format( &system_buffers[i].format, &handle->current_format->format );
      system_buffers[i].buffer_size = /* handle->current_format-> */usb_buffer_size;
      system_buffers[i].data = malloc( system_buffers[i].buffer_size );
      system_buffers[i].type = UNICAP_FLAGS_BUFFER_TYPE_SYSTEM;
      system_buffers[i].flags = 0;
      system_buffers[i].frame_number = 0;
      memset( &system_buffers[i].fill_time, 0x0, sizeof( struct timeval ) );
      memset( &system_buffers[i].duration, 0x0, sizeof( struct timeval ) );
      memset( &system_buffers[i].capture_start_time, 0x0, sizeof( struct timeval ) );
   }

   while( !handle->capture_thread_quit && !handle->removed){
      struct usbdevfs_urb *urb;
      int ret;
      int corrupt_frame = 0;

      ret = reap_urb(handle->dev.fd, &urb);
      gettimeofday (&timeout_context.timeout, NULL);
      if( ret < 0 ){
	 switch( errno ){
	 case ENODEV:
	    TRACE( "NODEV\n" );
	    if( !handle->removed){
	       handle->removed = 1;
	    }
	    break;
	    
	 default:
	 case EAGAIN:
	    /* perror( "reap: " ); */
	    usleep(1000);
	    continue;
	    break;
	 
	    //fail
	    break;
	 }
	 
	 // on error - break out of the loop
	 break;
      } else {
	 int transfer_done = 0;
	 if( urb->actual_length == 0 ){
	    submit_urb( handle->dev.fd, urb );
	    usleep(1000);
	    continue;
	 }
	 
	 
	 if( urb->actual_length < urb->buffer_length ){
	    transfer_done = 1;
	    wait_transfer_done = 0;
	 }else if( wait_transfer_done ){
	    submit_urb( handle->dev.fd, urb );
	    continue;
	 }
	    
	 if( bytes_done == 0 ){
	    // first packet in stream: skip header
	    memcpy( system_buffers[ current_buffer ].data, urb->buffer + 2, urb->actual_length - 2 );
	    bytes_done += urb->actual_length - 2;
	    
	 }else{
	    if( ( bytes_done + urb->actual_length ) <= /* handle->current_format-> */usb_buffer_size ){
	       memcpy( system_buffers[ current_buffer ].data + bytes_done, urb->buffer, urb->actual_length );
	       bytes_done += urb->actual_length;
	    }else{
	       corrupt_frame = 1;

	       transfer_done = 1;
	       if( !transfer_done )
		  wait_transfer_done = 1;
	       TRACE( "corrupt_frame: bytes = %d / %d \n", bytes_done + urb->actual_length, /* handle->current_format-> */usb_buffer_size );
	       bytes_done = 0;
	    }  
	 }

	 if( transfer_done ){
	    if( !corrupt_frame && ( bytes_done == /* handle->current_format-> */usb_buffer_size ) ){
	       context.buffer = &system_buffers[ current_buffer ];
	       buffer_done( handle, &context );
	       current_buffer = ( current_buffer + 1 ) % NUM_SYSTEM_BUFFERS;
	    }else{
#ifdef DEBUG
	       context.buffer = &system_buffers[ current_buffer ];
	       buffer_done( handle, &context );
	       current_buffer = ( current_buffer + 1 ) % NUM_SYSTEM_BUFFERS;
	       TRACE( "corrupt_frame: bytes = %d / %d \n", bytes_done, /* handle->current_format-> */usb_buffer_size );
#endif
	    }
	       
	    bytes_done = 0;
	 }

	 submit_urb( handle->dev.fd, urb );
      }
   }

   TRACE( "Capture thread exit\n" );

   context.quit = 1;
   timeout_context.quit = 1;
   sem_post( &context.sema );
   pthread_join( buffer_done_thread_id, NULL );
   sem_destroy( &context.sema );
   pthread_join( capture_timeout_thread_id, NULL );

   if( context.conv_buffer ){
      free( context.conv_buffer->data );
      free( context.conv_buffer );
      context.conv_buffer = NULL;
   }


   // Discard all URBs
   for( i = 0; i < NUM_URBS; i++ ){
      struct usbdevfs_urb *urb = urbs[i];
      int ret;
      
      ret = ioctl( handle->dev.fd, USBDEVFS_DISCARDURB, urb );
   }   

   // Reap all URBs and free buffers
   for( i = 0; i < NUM_URBS; i++ ){
      struct usbdevfs_urb *urb;
      if( reap_urb( handle->dev.fd, &urb ) == 0 ){
	 free( urb->buffer );
	 free( urb );
      }
   }

   for( i = 0; i < NUM_SYSTEM_BUFFERS; i++ ){
      free( system_buffers[i].data );
   }   

   if( handle->removed  && handle->event_callback ){
      handle->event_callback( handle->unicap_handle, UNICAP_EVENT_DEVICE_REMOVED );
   }
      
   return NULL;
}


unicap_status_t euvccam_capture_start_capture( euvccam_handle_t handle )
{
   struct sched_param param;
   
   if( handle->capture_running )
      return STATUS_SUCCESS;
   
   handle->capture_thread_quit = 0;
   handle->streaming_endpoint = 0x82;
   
   if( pthread_create( &handle->capture_thread, NULL, (void*(*)(void*))capture_thread, handle ) != 0 )
      return STATUS_FAILURE;

   param.sched_priority = 5;

/*    if( pthread_setschedparam( handle->capture_thread, SCHED_FIFO, &param ) == 0 ){ */
/*       TRACE( "Successfully moved to RT scheduler\n" ); */
/*    }else{ */
/*       perror( "pthread_setschedparam: " ); */
/*    } */

   handle->capture_running = 1;

   return STATUS_SUCCESS;
}

unicap_status_t euvccam_capture_stop_capture( euvccam_handle_t handle )
{
   if( handle->capture_running ){
      TRACE( "Stop capture\n" );
      /* // send signal to interrupt blocking ioctls */
      pthread_kill( handle->capture_thread, SIGUSR1 );
      handle->capture_thread_quit = 1;
      pthread_join( handle->capture_thread, NULL );
   }
   handle->capture_running = 0;
   
   return STATUS_SUCCESS;
}
