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

#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#include <config.h>

#include "unicap.h"
#include "unicap_cpi.h"
#include "unicap_status.h"

#ifdef RAW1394_1_1_API
#include <libraw1394/raw1394.h>
#endif
#ifdef RAW1394_1_0_API
#include <raw1394.h>
#endif

#include "vid21394.h"

#include <pthread.h>

#include <semaphore.h>

#include "queue.h"

#define I2C_SAA7112_ID 0x420000
#define I2C_BRIGHTNESS 0xA00
#define I2C_CONTRAST   0xB00
#define I2C_SYNCCONTROL 0x800 
#define I2C_BYTE_ORDER  0x8500

#define MAX_VID21394_HANDLES 16
#define FCP_TIMEOUT 5



#define EXPERIMENTAL_FW 1

union fcp_command
{
      unsigned int int32value[2];
      unsigned long long int64value;
};

struct buffer_entry
{
      unsigned char *data;
      struct buffer_entry *next;
};

struct timeout_data
{
      raw1394handle_t raw1394handle;
      volatile int capture_running;
      volatile int quit;
};

typedef struct timeout_data timeout_data_t;

struct parse_state
{
      unicap_data_buffer_t *buffer;
      struct _unicap_queue *in_queue;
      struct _unicap_queue *out_queue;
};

struct vid21394_handle
{
      struct vid21394_handle *vid21394handle;
      raw1394handle_t         raw1394handle;
      unicap_event_callback_t event_callback;
      unicap_handle_t         unicap_handle;		
      int                     node;
      int                     port;
      unsigned long long serial_number;
      int                device_present;
      pthread_t          timeout_thread;
      timeout_data_t     timeout_data;
		
      int                channel;   // -1 when not transmitting
      int                bandwidth; //  0 when not transmitting
      void *             userdata;
	
/* 		volatile unsigned long fcp_sync_bits; */
      sem_t fcp_sync_sem[32];
      int fcp_status[32];		

      unsigned int fcp_response[256];
      int fcp_response_length;
      volatile unsigned int fcp_data;
      volatile unsigned int fcp_ext_data;
      volatile unsigned int firmware_version;
      enum vid21394_video_mode video_mode;
      unsigned long video_freq;
	
      struct _unicap_queue queued_buffers;
      struct _unicap_queue ready_buffers;

      struct _unicap_queue *current_data_buffer;

      struct parse_state parse_state;
      size_t raw_buffer_size; // size of a field buffer with isoch headers
      size_t dma_buffer_size;
      size_t dma_vmmap_frame_size;
      int dma_fd;
      int dma_capture_thread_quit;
      pthread_t dma_capture_thread;
      int current_offset;
      int current_field;
      int current_line_offset;
      int current_line_length;
      int current_buffer_size;
		
      int current_line_to_copy;
      int current_bytes_copied;

      int last_field_type;

      int copy_done;
      int start_copy;
      int copied_field_0;
      int copied_field_1;

      int num_buffers;

      int nr_pkts;
		
      int is_receiving; // set to 1 if isoch receive is active

      struct timeval filltime;

      int stop_capture;
      pthread_t capture_thread;

      unsigned char *system_buffer;
      struct _unicap_queue system_buffer_entry;
      unicap_format_t current_format;
};




vid21394handle_t vid21394_open ( unsigned long long sernum );
void             vid21394_close( vid21394handle_t vid21394handle );

int vid21394_start_transmit( vid21394handle_t vid21394handle );
int vid21394_stop_transmit( vid21394handle_t vid21394handle );

int vid21394_start_receive( vid21394handle_t vid21394handle );
int vid21394_stop_receive( vid21394handle_t vid21394handle );

void _vid21394_add_to_queue( struct _unicap_queue *queue, struct _unicap_queue *entry );

unicap_status_t vid21394_set_input_channel( vid21394handle_t vid21394handle, 
					    enum vid21394_input_channel channel );

unicap_status_t vid21394_get_input_channel( vid21394handle_t vid21394handle, 
					    enum vid21394_input_channel *channel );

unicap_status_t vid21394_set_brightness   ( vid21394handle_t vid21394handle, 
					    unsigned int brightness );

unicap_status_t vid21394_get_brightness   ( vid21394handle_t vid21394handle, 
					    unsigned int *brightness );

unicap_status_t vid21394_set_frequency    ( vid21394handle_t vid21394handle, 
					    enum vid21394_frequency freq );

unicap_status_t vid21394_get_frequency    ( vid21394handle_t vid21394handle, 
					    enum vid21394_frequency *freq );

unicap_status_t vid21394_set_contrast     ( vid21394handle_t vid21394handle, 
					    unsigned int contrast );

unicap_status_t vid21394_get_contrast     ( vid21394handle_t vid21394handle, 
					    unsigned int *contrast );

unicap_status_t vid21394_set_force_odd_even( vid21394handle_t vid21394handle, 
					     unsigned int force );

unicap_status_t vid21394_get_force_odd_even( vid21394handle_t vid21394handle, 
					     unsigned int *force );

#if VID21394_BOOTLOAD
unicap_status_t vid21394_enter_bootload( vid21394handle_t vid21394handle );
#endif

unicap_status_t vid21394_write_rs232( vid21394handle_t vid21394handle, 
				      unsigned char *data, 
				      int datalen );

unicap_status_t vid21394_read_rs232 ( vid21394handle_t vid21394handle, 
				      unsigned char *data, 
				      int *datalen );

unicap_status_t vid21394_rs232_set_baudrate( vid21394handle_t vid21394handle, 
					     int rate );

unicap_status_t vid21394_rs232_io          ( vid21394handle_t vid21394handle, 
					     unsigned char *out_data, 
					     int out_data_length,
					     unsigned char *in_data, 
					     int in_data_length );

unicap_status_t vid21394_set_link_speed    ( vid21394handle_t vid21394handle, 
					     int speed );

unicap_status_t vid21394_set_video_mode    ( vid21394handle_t vid21394handle, 
					     enum vid21394_video_mode video_mode );

void * vid21394_capture_thread( vid21394handle_t vid21394handle );

void vid21394_queue_buffer( vid21394handle_t vid21394handle,
			    void * buffer );

int vid21394_wait_buffer  ( vid21394handle_t vid21394handle,
			    void **buffer );

#endif //__COMMANDS_H__

