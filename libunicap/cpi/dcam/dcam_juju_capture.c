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
#include <stdlib.h>
#include <poll.h>
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
#include <linux/firewire-cdev.h>
#include <limits.h>

#include "dcam.h"
#include "dcam_juju.h"

#if DCAM_DEBUG
#define DEBUG
#endif
#include "debug.h"

#define __HIDDEN__ __attribute__((visibility("hidden")))

#define ptr_to_u64(p) ((__u64)(unsigned long)(p))
#define u64_to_ptr(p) ((void *)(unsigned long)(p))

extern struct _dcam_isoch_mode dcam_isoch_table[];

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

int dcam_juju_probe (dcam_handle_t dcamhandle)
{
	int fd;
	char filename[PATH_MAX];
	int ret = 0;
	struct fw_cdev_get_info info;

	sprintf (filename, "/dev/fw%d", dcamhandle->port);
	fd = open (filename, O_RDWR);
	if (fd < 0){
		TRACE ("Probe for JUJU: device file inaccessible\n");
		return 0;
	}

	info.version = FW_CDEV_VERSION;
	info.rom = 0;
	info.rom_length = 0;
	info.bus_reset = 0;
	if (ioctl (fd, FW_CDEV_IOC_GET_INFO, &info) < 0){
		TRACE ("Probe for JUJU: failed to retrieve info\n");
		close (fd);
		return 0;
	}
	if (info.version < 2){
		TRACE ("Probe for JUJU: ABI version 1 unsupported\n");
		return 0;
	}
	close (fd);
	return 1;
}

unicap_status_t __HIDDEN__
dcam_juju_setup (dcam_handle_t dcamhandle)
{
	struct fw_cdev_create_iso_context create;
	char filename[PATH_MAX];
	sprintf (filename, "/dev/fw%d", dcamhandle->port);

	dcamhandle->dma_fd = open (filename, O_RDWR);
	if (dcamhandle->dma_fd < 0){
		TRACE ("JUJU setup: device file inaccessible\n");
		return STATUS_FAILURE;
	}

	create.type = FW_CDEV_ISO_CONTEXT_RECEIVE;
	create.header_size = 8;
	create.channel = dcamhandle->channel_allocated;
	create.speed = S400;
	if (ioctl (dcamhandle->dma_fd, FW_CDEV_IOC_CREATE_ISO_CONTEXT, &create)
	    < 0){
		TRACE ("Failed to create iso context\n");
		return STATUS_FAILURE;
	}
	dcamhandle->juju_iso_handle = create.handle;

	dcamhandle->dma_buffer_size = DCAM_NUM_DMA_BUFFERS *
		dcamhandle->buffer_size;
	dcamhandle->dma_buffer = mmap (NULL, dcamhandle->dma_buffer_size,
				       PROT_READ | PROT_WRITE,
				       MAP_SHARED,
				       dcamhandle->dma_fd, 0);
	if (dcamhandle->dma_buffer == MAP_FAILED){
		TRACE ("Failed to map dma buffer\n");
		return STATUS_FAILURE;
	}


	return STATUS_SUCCESS;
}

unicap_status_t
dcam_juju_prepare_iso (dcam_handle_t dcamhandle)
{
	int i;
	size_t packet_size = dcam_isoch_table[dcamhandle->current_iso_index]
		.bytes_per_packet;
	size_t frame_size = dcam_isoch_table[dcamhandle->current_iso_index]
		.bytes_per_frame;
	int iso_packets_per_frame = frame_size / packet_size;
	int cdev_iso_packet_count = 8;
	int cdev_packets_per_frame = ((iso_packets_per_frame +
				       cdev_iso_packet_count - 1 ) /
				      cdev_iso_packet_count);
	size_t size = cdev_packets_per_frame * sizeof (struct fw_cdev_iso_packet);

	for (i=0; i < DCAM_NUM_DMA_BUFFERS; i++){
		int j;
		int packets_left = iso_packets_per_frame;
		int packet_count = cdev_iso_packet_count;
		struct fw_cdev_iso_packet *packets;
		dcamhandle->juju_buffers[i].packets = calloc (size, 1);
		dcamhandle->juju_buffers[i].size = size;
		if (!dcamhandle->juju_buffers[i].packets){
			TRACE ("Failed to allocate packets");
			for (j = 0; j < i-1; j++)
				free (dcamhandle->juju_buffers[i].packets);
			return STATUS_FAILURE;
		}
		packets = dcamhandle->juju_buffers[i]
			.packets;
		for (j = 0; j < cdev_packets_per_frame; j++){
			if (packets_left < packet_count)
				packet_count = packets_left;
			packets[j].control =
				FW_CDEV_ISO_HEADER_LENGTH (8 * packet_count) |
				FW_CDEV_ISO_PAYLOAD_LENGTH (packet_size *
							    packet_count);
			packets_left -= packet_count;
		}
		packets[0].control |= FW_CDEV_ISO_SKIP;
		packets[cdev_packets_per_frame - 1].control
			|= FW_CDEV_ISO_INTERRUPT;
	}

	for (i=0; i < DCAM_NUM_DMA_BUFFERS; i++)
		dcam_juju_queue_buffer (dcamhandle, i);

	return STATUS_SUCCESS;
}

unicap_status_t
dcam_juju_start_iso (dcam_handle_t dcamhandle)
{
	struct fw_cdev_start_iso start_iso;
	int ret;

	start_iso.cycle = -1;
	start_iso.tags = FW_CDEV_ISO_CONTEXT_MATCH_ALL_TAGS;
	start_iso.sync = 1;
	start_iso.handle = dcamhandle->juju_iso_handle;

	ret = ioctl (dcamhandle->dma_fd, FW_CDEV_IOC_START_ISO, &start_iso);
	if (ret < 0 ){
		TRACE ("Failed to start iso\n");
		return STATUS_FAILURE;
	}
	return STATUS_SUCCESS;
}

unicap_status_t
dcam_juju_shutdown (dcam_handle_t dcamhandle)
{
	unicap_status_t status = STATUS_SUCCESS;
	struct fw_cdev_stop_iso stop_iso;
	if (dcamhandle->dma_fd > 0){
		int i;
		if (ioctl(dcamhandle->dma_fd, FW_CDEV_IOC_STOP_ISO, &stop_iso) < 0){
			TRACE ("Failed to stop iso\n");
			status = STATUS_FAILURE;
		}
		
		munmap(dcamhandle->dma_buffer, dcamhandle->dma_buffer_size);
		
		for (i=0; i < DCAM_NUM_DMA_BUFFERS; i++){
			free (dcamhandle->juju_buffers[i].packets);
		}
		
		close (dcamhandle->dma_fd);
		dcamhandle->dma_fd = -1;
	}

	return status;
}

unicap_status_t
dcam_juju_queue_buffer (dcam_handle_t dcamhandle, int index)
{
	struct fw_cdev_queue_iso queue;
	struct dcam_juju_buffer *buffer = dcamhandle->juju_buffers + index;

	queue.size = buffer->size;
	queue.data = ptr_to_u64 (dcamhandle->dma_buffer +
				 (index *
				  dcamhandle->buffer_size));
	queue.packets = ptr_to_u64 (buffer->packets);
	queue.handle = dcamhandle->juju_iso_handle;

	if (ioctl (dcamhandle->dma_fd, FW_CDEV_IOC_QUEUE_ISO, &queue) < 0){
		TRACE ("Failed to queue iso buffer\n");
		return STATUS_FAILURE;
	}

	return STATUS_SUCCESS;
}

unicap_status_t
dcam_juju_dequeue_buffer (dcam_handle_t dcamhandle,
			  int *index,
			  uint32_t *cycle_time,
			  int *packet_discont)
{
	struct pollfd fds[1];
	size_t packet_size = dcam_isoch_table[dcamhandle->current_iso_index]
		.bytes_per_packet;
	size_t frame_size = dcam_isoch_table[dcamhandle->current_iso_index]
		.bytes_per_frame;
	int iso_packets_per_frame = frame_size / packet_size;
	struct
	{
		struct fw_cdev_event_iso_interrupt irq;
		uint32_t headers[iso_packets_per_frame*2 + 16];
	} iso;

	fds[0].fd = dcamhandle->dma_fd;
	fds[0].events = POLLIN;

	while ( dcamhandle->dma_capture_thread_quit == 0 ){
		int err, len;
		err = poll (fds, 1, -1);
		if (err <= 0){
			if (errno == EINTR)
				continue;
			return STATUS_FAILURE;
		}

		len = read (dcamhandle->dma_fd, &iso, sizeof (iso));
		if (len < 0){
			TRACE ("Failed to read juju response\n");
			return STATUS_FAILURE;
		}

		if (iso.irq.type == FW_CDEV_EVENT_ISO_INTERRUPT)
			break;
	}

	if (dcamhandle->dma_capture_thread_quit != 0){
		return STATUS_FAILURE;
	}

	int i;
	uint8_t *bytes = (uint8_t *)(iso.headers+1);
	uint32_t cycle = (bytes[2] << 8) | bytes[3];
	*packet_discont = 0;

	for (i = 1; i < iso_packets_per_frame; i++){
		uint8_t *bytes = (uint8_t *)(iso.headers+(i*2+1));
		uint32_t new_cycle = (bytes[2] << 8) | bytes[3];
		// The Imaging Source cameras may have a 1 packet "gap"
		if ((abs(new_cycle-(cycle+1))%65535) > 2){
			*packet_discont = 1;
			break;
		}
		cycle = new_cycle;
	}

	*index = dcamhandle->current_dma_capture_buffer;
	dcamhandle->current_dma_capture_buffer =
		(dcamhandle->current_dma_capture_buffer + 1) %
		DCAM_NUM_DMA_BUFFERS;

	return STATUS_SUCCESS;
}



static void sighandler(int sig )
{
	return;
}

static uint32_t
calc_usec (uint32_t cycles)
{
    uint32_t sec = (cycles & 0xe000000) >> 13;
    uint32_t cyc = (cycles & 0x1fff000);
    uint32_t usec = sec * 1000000 + cycles * 125;

    return usec;
}

void *dcam_juju_capture_thread( void *arg )
{
	dcam_handle_t dcamhandle = ( dcam_handle_t ) arg;
	unicap_queue_t *entry;

	int ready_buffer;
	signal( SIGUSR1, sighandler );
	while( dcamhandle->dma_capture_thread_quit == 0 )
	{
		int index;
		unicap_queue_t *entry;
		int packet_discont = 0;
		struct fw_cdev_get_cycle_timer tm;
		uint32_t cycle_time;
		struct timeval tv;

		if (!SUCCESS( dcam_juju_dequeue_buffer (dcamhandle, &index, 
							&cycle_time, 
							&packet_discont))){
			TRACE ("Dequeue failed!\n");
			continue;
		}
		if (packet_discont && dcamhandle->enable_frame_drop){
			TRACE ("Drop buffer\n");
			// packet discontinuity -> drop buffer
			if (!SUCCESS (dcam_juju_queue_buffer (dcamhandle,
							      index))){
				TRACE ("Failed to queue buffer\n");
			}
		}

		/* if (ioctl (dcamhandle->dma_fd,  */
		/* 	   FW_CDEV_IOC_GET_CYCLE_TIMER,  */
		/* 	   &tm) == 0){ */
		/* 	uint32_t usec; */
		/* 	calc_usec (cycle_time); */
		/* } */
		gettimeofday (&tv, NULL);

		entry = ucutil_get_front_queue( &dcamhandle->input_queue );
		if (entry){
			unicap_data_buffer_t *data_buffer;
			data_buffer = entry->data;
			if( data_buffer->type == UNICAP_BUFFER_TYPE_SYSTEM ){
				data_buffer->data =
					dcamhandle->dma_buffer +
					index * dcamhandle->buffer_size;
			} else {
				memcpy( data_buffer->data,
					( dcamhandle->dma_buffer +
					  index * dcamhandle->buffer_size),
					dcamhandle->buffer_size );
			}

			data_buffer->buffer_size = dcamhandle->buffer_size;
			memcpy (&data_buffer->fill_time,
				&tv,
				sizeof (struct timeval));

			ucutil_insert_back_queue( &dcamhandle->output_queue,
						  entry );
			data_buffer = 0;
		}

		{
		   unicap_data_buffer_t tmpbuffer;
		   tmpbuffer.data = dcamhandle->dma_buffer + index *
			   dcamhandle->buffer_size;
		   tmpbuffer.buffer_size = dcamhandle->buffer_size;
		   unicap_copy_format( &tmpbuffer.format,
				       &dcamhandle->
				       v_format_array[dcamhandle->
						      current_format_index] );
		   memcpy (&tmpbuffer.fill_time,
			   &tv,
			   sizeof (struct timeval));
		   new_frame_event( dcamhandle, &tmpbuffer );
		}

		if (!SUCCESS (dcam_juju_queue_buffer (dcamhandle,
						      index))){
			TRACE ("Failed to queue buffer\n");
		}
	}

	return NULL;
}
