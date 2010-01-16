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

#include <config.h>

#ifdef RAW1394_1_1_API
#include <libraw1394/raw1394.h>
#include <libraw1394/csr.h>
#else
#include <raw1394.h>
#include <csr.h>
#endif

#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include <errno.h>

#include <unicap_status.h>
#include "video1394.h"

#include "dcam.h"
#include "dcam_capture.h"

#if DCAM_DEBUG
#define DEBUG
#endif
#include "debug.h"

static void new_frame_event( dcam_handle_t dcamhandle, unicap_data_buffer_t *buffer )
{
   if( dcamhandle->event_callback )
   {
      dcamhandle->event_callback( dcamhandle->unicap_handle, UNICAP_EVENT_NEW_FRAME, buffer );
   }
}

static void drop_frame_event( dcam_handle_t dcamhandle )
{
	if( dcamhandle->event_callback )
	{
		dcamhandle->event_callback( dcamhandle->unicap_handle, UNICAP_EVENT_NEW_FRAME );
	}
}	

static void cleanup_handler( void *arg )
{
	TRACE( "cleanup_handler\n" );
}

/*
  loop in the background while capturing to call the iso handle regularly
 */
void *dcam_capture_thread( void *data )
{
	dcam_handle_t dcamhandle = (dcam_handle_t)data;

	pthread_cleanup_push( cleanup_handler, data );

	while( dcamhandle->capture_running )
	{
		raw1394_loop_iterate( dcamhandle->raw1394handle );
	}

	pthread_cleanup_pop( 0 );
	
	return 0;
}

/*
  handler for raw1394 isoch reception

  sequence: 

  - get buffer from input queue
  - start on sy == 1 to fill buffer
  - fill buffer until dcamhandle->buffer_size is reached
  - put buffer into output queue
 */
enum raw1394_iso_disposition dcam_iso_handler( raw1394handle_t raw1394handle, 
					       unsigned char * data, 
					       unsigned int    len, 
					       unsigned char   channel,
					       unsigned char   tag, 
					       unsigned char   sy, 
					       unsigned int    cycle, 
					       unsigned int    dropped )
{
	dcam_handle_t dcamhandle = raw1394_get_userdata( raw1394handle );
	if( !len )
	{
		return 0;
	}
	
/* 	TRACE( "len: %d, sy: %d, cycle: %d, dropped: %d,current_offset: %d, buffer_size: %d\n", */
/* 	       len, */
/* 	       sy, */
/* 	       cycle, */
/* 	       dropped, */
/* 	       dcamhandle->current_buffer_offset, */
/* 	       dcamhandle->buffer_size); */
	

	if( dcamhandle->wait_for_sy )
	{
		if( !sy )
		{
			return 0;
		}
		
		dcamhandle->current_buffer_offset = 0;
		dcamhandle->current_buffer = ucutil_get_front_queue( &dcamhandle->input_queue );
		if( !dcamhandle->current_buffer )
		{
			return 0;
		}
		dcamhandle->wait_for_sy = 0;
/* 		gettimeofday( &dcamhandle->current_buffer->fill_start_time, NULL ); */
	}
	
	if( (dcamhandle->current_buffer_offset + len) > dcamhandle->buffer_size )
	{
		TRACE( "dcam.cpi: (isoch handler) got more data than allowed\n" );
		return 0;
	}
	
	memcpy( dcamhandle->current_buffer->data + dcamhandle->current_buffer_offset, data, len );
	
	dcamhandle->current_buffer_offset += len;
	if( dcamhandle->current_buffer_offset == dcamhandle->buffer_size )
	{
/* 		gettimeofday( &dcamhandle->current_buffer->fill_end_time, NULL ); */
		ucutil_insert_back_queue( &dcamhandle->output_queue, dcamhandle->current_buffer );
/* 		new_frame_event( dcamhandle, dcamhandle->current_buffer ); */
		dcamhandle->current_buffer = 0;
		dcamhandle->wait_for_sy = 1;
	}
	
	return 0;
}

/*
  setup video1394 module for dma capture

  - opens /dev/video1394/%port%
 */
unicap_status_t _dcam_dma_setup( dcam_handle_t dcamhandle )
{
	char dev_file[512];

	struct video1394_wait vwait;
	struct video1394_mmap vmmap;
	
	int i;
	
	sprintf( dev_file, "/dev/video1394/%d", dcamhandle->port );
	dcamhandle->dma_fd = open( dev_file, O_RDONLY );
	if( dcamhandle->dma_fd < 0 )
	{
		struct stat statbuf;

		TRACE( "failed to open video1394 device %s\n", dev_file );

		sprintf( dev_file, "/dev/video1394-%d", dcamhandle->port );
		dcamhandle->dma_fd = open( dev_file, O_RDONLY );
		if( dcamhandle->dma_fd < 0 )
		{
			sprintf( dev_file, "/dev/video1394" );
			if( !stat (dev_file, &statbuf) && !S_ISDIR(statbuf.st_mode) )
				dcamhandle->dma_fd = open( dev_file, O_RDONLY );
			
			if( dcamhandle->dma_fd < 0 )
			{
				TRACE( "failed to open video1394 device %s\n", dev_file );
				return STATUS_FAILURE;
			}
		}
	}

	dcamhandle->current_dma_capture_buffer = -1;

	TRACE( "dma setup buffer size: %d\n", dcamhandle->buffer_size );
	
	vmmap.sync_tag = 1;
	vmmap.nb_buffers = DCAM_NUM_DMA_BUFFERS;
	vmmap.flags = VIDEO1394_SYNC_FRAMES;
	vmmap.buf_size = dcamhandle->buffer_size;
	vmmap.channel = dcamhandle->channel_allocated;
	
	if( ioctl( dcamhandle->dma_fd, VIDEO1394_IOC_LISTEN_CHANNEL, &vmmap ) < 0 )
	{
		TRACE( "VIDEO1394_LISTEN_CHANNEL ioctl failed\n" );
		TRACE( "error: %s\n", strerror( errno ) );
		TRACE( "buffer_size: %d, channel: %d\n", 
		       vmmap.buf_size, 
		       vmmap.channel );
		return STATUS_FAILURE;
	}
	
	dcamhandle->dma_vmmap_frame_size = vmmap.buf_size;
	dcamhandle->dma_buffer_size = DCAM_NUM_DMA_BUFFERS * dcamhandle->dma_vmmap_frame_size;

	dcamhandle->dma_buffer = mmap( 0, 
				       dcamhandle->dma_buffer_size,
				       PROT_READ, 
				       MAP_SHARED, 
				       dcamhandle->dma_fd, 
				       0 );
	if( dcamhandle->dma_buffer == (unsigned char *)(-1) )
	{
		TRACE( "mmap failed\n" );
		ioctl( dcamhandle->dma_fd, VIDEO1394_IOC_UNLISTEN_CHANNEL, &vmmap.channel );
		return STATUS_FAILURE;
	}

	for( i = 0; i < DCAM_NUM_DMA_BUFFERS; i++ )
	{
		vwait.channel = dcamhandle->channel_allocated;
		vwait.buffer = i;
		TRACE( "q: %d\n", vwait.buffer );
		if( ioctl( dcamhandle->dma_fd, VIDEO1394_IOC_LISTEN_QUEUE_BUFFER, &vwait ) < 0 )
		{
			TRACE( "VIDEO1394_LISTEN_QUEUE_BUFFER ioctl failed\n" );
			return STATUS_FAILURE;
		}
	}
	
	return STATUS_SUCCESS;
}

/*
  free resources acquired by dma_init
 */
unicap_status_t _dcam_dma_free( dcam_handle_t dcamhandle )
{

	if( dcamhandle->dma_buffer )
	{
		munmap( dcamhandle->dma_buffer, dcamhandle->dma_buffer_size );
	}
	
	close( dcamhandle->dma_fd );
	
	return STATUS_SUCCESS;
}

/*
  dma unlisten
 */
unicap_status_t _dcam_dma_unlisten( dcam_handle_t dcamhandle )
{
	int channel = dcamhandle->channel_allocated;
	
	if( ioctl( dcamhandle->dma_fd, 
		   VIDEO1394_IOC_UNLISTEN_CHANNEL, 
		   &channel ) < 0 )
	{
		return STATUS_FAILURE;
	}
	
	return STATUS_SUCCESS;
}

static void sighandler(int sig )
{
	return;
}

void *dcam_dma_capture_thread( void *arg )
{
	dcam_handle_t dcamhandle = ( dcam_handle_t ) arg;
	struct video1394_wait vwait;
	unicap_queue_t *entry;

	int i = 0;
	int ready_buffer;
	
	signal( SIGUSR1, sighandler );
	while( dcamhandle->dma_capture_thread_quit == 0 )
	{		
		vwait.channel = dcamhandle->channel_allocated;
		i = vwait.buffer = ( dcamhandle->current_dma_capture_buffer + 1 ) % DCAM_NUM_DMA_BUFFERS;
	
		if( ioctl( dcamhandle->dma_fd, VIDEO1394_IOC_LISTEN_WAIT_BUFFER, &vwait ) != 0 )
		{
			TRACE( "VIDEO1394_LISTEN_WAIT_BUFFER ioctl failed\n" );
			TRACE( "channel: %d, buffer: %d\n", vwait.channel, vwait.buffer );
			TRACE( "error: %s\n", strerror( errno ) );
			// increase the buffer counter to wait for next buffer on the next try
			dcamhandle->current_dma_capture_buffer = ( dcamhandle->current_dma_capture_buffer + 1 ) % DCAM_NUM_DMA_BUFFERS;
			usleep( 25000 );
			continue;
		}
		
		ready_buffer = ( vwait.buffer + i ) % DCAM_NUM_DMA_BUFFERS;
		
		entry = ucutil_get_front_queue( &dcamhandle->input_queue );
		if( entry )
		{
			unicap_data_buffer_t *data_buffer;
			data_buffer = entry->data;
			memcpy( &data_buffer->fill_time, &vwait.filltime, sizeof( struct timeval ) );
			
			if( data_buffer->type == UNICAP_BUFFER_TYPE_SYSTEM )
			{
				data_buffer->data = dcamhandle->dma_buffer + i * dcamhandle->buffer_size;
			}
			else
			{
				memcpy( data_buffer->data, 
					dcamhandle->dma_buffer + i * dcamhandle->dma_vmmap_frame_size, 
					dcamhandle->buffer_size );
			}
			
			data_buffer->buffer_size = dcamhandle->buffer_size;
			ucutil_insert_back_queue( &dcamhandle->output_queue, entry );			
			data_buffer = 0;
		}

		{
		   unicap_data_buffer_t tmpbuffer;
		   
		   tmpbuffer.data = dcamhandle->dma_buffer + i * dcamhandle->buffer_size;
		   tmpbuffer.buffer_size = dcamhandle->buffer_size;
		   unicap_copy_format( &tmpbuffer.format, 
				       &dcamhandle->v_format_array[dcamhandle->current_format_index] );
		   memcpy( &tmpbuffer.fill_time, &vwait.filltime, sizeof( struct timeval ) );
		   new_frame_event( dcamhandle, &tmpbuffer );
		}   
		
		for( ; i != ready_buffer; i = ( ( i + 1 ) % DCAM_NUM_DMA_BUFFERS ) )
		{
			entry = ucutil_get_front_queue( &dcamhandle->input_queue );
			if( entry )
			{
				unicap_data_buffer_t *data_buffer;

				data_buffer = entry->data;
				memcpy( &data_buffer->fill_time, &vwait.filltime, sizeof( struct timeval ) );
				if( data_buffer->type == UNICAP_BUFFER_TYPE_SYSTEM )
				{
					data_buffer->data = dcamhandle->dma_buffer + i * dcamhandle->buffer_size;
				}
				else
				{
					memcpy( data_buffer->data, 
							dcamhandle->dma_buffer + i * dcamhandle->dma_vmmap_frame_size, 
							dcamhandle->buffer_size );
				}
				data_buffer->buffer_size = dcamhandle->buffer_size;

				ucutil_insert_back_queue( &dcamhandle->output_queue, entry );

				data_buffer = 0;
			}

			{
			   unicap_data_buffer_t tmpbuffer;
			   
			   tmpbuffer.data = dcamhandle->dma_buffer + i * dcamhandle->buffer_size;
			   tmpbuffer.buffer_size = dcamhandle->buffer_size;
			   unicap_copy_format( &tmpbuffer.format, 
					       &dcamhandle->v_format_array[dcamhandle->current_format_index] );
			   new_frame_event( dcamhandle, &tmpbuffer );
			}   


			vwait.buffer = i;

			if( ioctl( dcamhandle->dma_fd, VIDEO1394_IOC_LISTEN_QUEUE_BUFFER, &vwait ) < 0 )
			{
				TRACE( "VIDEO1394_LISTEN_QUEUE_BUFFER ioctl failed\n" );
				continue;
			}
		}
		
		vwait.buffer = ready_buffer;
		
		if( ioctl( dcamhandle->dma_fd, VIDEO1394_IOC_LISTEN_QUEUE_BUFFER, &vwait ) < 0 )
		{
			TRACE( "VIDEO1394_LISTEN_QUEUE_BUFFER ioctl failed\n" );
			continue;
		}

		dcamhandle->current_dma_capture_buffer = ready_buffer;
	}

	return NULL;
}


unicap_status_t dcam_dma_wait_buffer( dcam_handle_t dcamhandle )
{
   struct video1394_wait vwait;
   unicap_queue_t *entry;

   int i = 0;
   int ready_buffer;
   
   vwait.channel = dcamhandle->channel_allocated;
   i = vwait.buffer = ( dcamhandle->current_dma_capture_buffer + 1 ) % DCAM_NUM_DMA_BUFFERS;
	
   if( ioctl( dcamhandle->dma_fd, VIDEO1394_IOC_LISTEN_WAIT_BUFFER, &vwait ) != 0 )
      {
	 TRACE( "VIDEO1394_LISTEN_WAIT_BUFFER ioctl failed\n" );
	 TRACE( "channel: %d, buffer: %d\n", vwait.channel, vwait.buffer );
	 TRACE( "error: %s\n", strerror( errno ) );
	 // increase the buffer counter to wait for next buffer on the next try
	 dcamhandle->current_dma_capture_buffer = ( dcamhandle->current_dma_capture_buffer + 1 ) % DCAM_NUM_DMA_BUFFERS;
	 return STATUS_FAILURE;
      }
		
   ready_buffer = ( vwait.buffer + i ) % DCAM_NUM_DMA_BUFFERS;
		
   entry = ucutil_get_front_queue( &dcamhandle->input_queue );
   if( entry )
   {
      unicap_data_buffer_t *data_buffer;
      data_buffer = entry->data;
      memcpy( &data_buffer->fill_time, &vwait.filltime, sizeof( struct timeval ) );
			
      if( data_buffer->type == UNICAP_BUFFER_TYPE_SYSTEM )
      {
	 data_buffer->data = dcamhandle->dma_buffer + i * dcamhandle->buffer_size;
      }
      else
      {
	 memcpy( data_buffer->data, 
		 dcamhandle->dma_buffer + i * dcamhandle->dma_vmmap_frame_size, 
		 dcamhandle->buffer_size );
      }
			
      data_buffer->buffer_size = dcamhandle->buffer_size;
      ucutil_insert_back_queue( &dcamhandle->output_queue, entry );			
      data_buffer = 0;
   }

   {
      unicap_data_buffer_t tmpbuffer;
		   
      tmpbuffer.data = dcamhandle->dma_buffer + i * dcamhandle->buffer_size;
      tmpbuffer.buffer_size = dcamhandle->buffer_size;
      unicap_copy_format( &tmpbuffer.format, 
			  &dcamhandle->v_format_array[dcamhandle->current_format_index] );
      memcpy( &tmpbuffer.fill_time, &vwait.filltime, sizeof( struct timeval ) );
      new_frame_event( dcamhandle, &tmpbuffer );
   }   
		
   for( ; i != ready_buffer; i = ( ( i + 1 ) % DCAM_NUM_DMA_BUFFERS ) )
   {
      entry = ucutil_get_front_queue( &dcamhandle->input_queue );
      if( entry )
      {
	 unicap_data_buffer_t *data_buffer;

	 data_buffer = entry->data;
	 memcpy( &data_buffer->fill_time, &vwait.filltime, sizeof( struct timeval ) );
	 if( data_buffer->type == UNICAP_BUFFER_TYPE_SYSTEM )
	 {
	    data_buffer->data = dcamhandle->dma_buffer + i * dcamhandle->buffer_size;
	 }
	 else
	 {
	    memcpy( data_buffer->data, 
		    dcamhandle->dma_buffer + i * dcamhandle->dma_vmmap_frame_size, 
		    dcamhandle->buffer_size );
	 }
	 data_buffer->buffer_size = dcamhandle->buffer_size;

	 ucutil_insert_back_queue( &dcamhandle->output_queue, entry );

	 data_buffer = 0;
      }

      {
	 unicap_data_buffer_t tmpbuffer;
			   
	 tmpbuffer.data = dcamhandle->dma_buffer + i * dcamhandle->buffer_size;
	 tmpbuffer.buffer_size = dcamhandle->buffer_size;
	 unicap_copy_format( &tmpbuffer.format, 
			     &dcamhandle->v_format_array[dcamhandle->current_format_index] );
	 new_frame_event( dcamhandle, &tmpbuffer );
      }   


      vwait.buffer = i;

      if( ioctl( dcamhandle->dma_fd, VIDEO1394_IOC_LISTEN_QUEUE_BUFFER, &vwait ) < 0 )
      {
	 TRACE( "VIDEO1394_LISTEN_QUEUE_BUFFER ioctl failed\n" );
	 return STATUS_FAILURE;
      }
   }
		
   vwait.buffer = ready_buffer;
		
   if( ioctl( dcamhandle->dma_fd, VIDEO1394_IOC_LISTEN_QUEUE_BUFFER, &vwait ) < 0 )
   {
      TRACE( "VIDEO1394_LISTEN_QUEUE_BUFFER ioctl failed\n" );
      return STATUS_FAILURE;
   }

   dcamhandle->current_dma_capture_buffer = ready_buffer;
   
   return STATUS_SUCCESS;
}
