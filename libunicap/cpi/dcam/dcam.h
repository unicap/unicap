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

#ifndef __DCAM_H__
#define __DCAM_H__

#include <config.h>

#include <unicap.h>
#include <unicap_status.h>
#include <unicap_cpi.h>
#ifdef RAW1394_1_1_API
#include <libraw1394/raw1394.h>
#include <libraw1394/csr.h>
#endif
#ifdef RAW1394_1_0_API
#include <raw1394.h>
#include <csr.h>
#endif
#include <pthread.h>

#include "queue.h"

#define S100 0
#define S200 1
#define S400 2
#define S800 3
#define S1600 4
#define S3200 5

#define DCAM_NUM_DMA_BUFFERS 8

enum dcam_property_enum
{
	DCAM_PPTY_BRIGHTNESS = 0, 
	DCAM_PPTY_ABS_FOCUS,
	DCAM_PPTY_ABS_ZOOM,
	DCAM_PPTY_INIT_LENS,
	DCAM_PPTY_AUTO_EXPOSURE, 
	DCAM_PPTY_SHARPNESS, 
	DCAM_PPTY_WHITE_BALANCE_MODE,
	DCAM_PPTY_WHITE_BALANCE_U, 
	DCAM_PPTY_WHITE_BALANCE_V, 
	DCAM_PPTY_HUE, 
	DCAM_PPTY_SATURATION, 
	DCAM_PPTY_GAMMA, 
	DCAM_PPTY_SHUTTER, 
	DCAM_PPTY_GAIN, 
	DCAM_PPTY_IRIS, 
	DCAM_PPTY_FOCUS, 
	DCAM_PPTY_TEMPERATURE, 
	DCAM_PPTY_SOFTWARE_TRIGGER,
	DCAM_PPTY_TRIGGER_MODE, 
	DCAM_PPTY_TRIGGER_POLARITY, 
	DCAM_PPTY_ZOOM, 
	DCAM_PPTY_PAN, 
	DCAM_PPTY_TILT, 
	DCAM_PPTY_OPTICAL_FILTER, 
	DCAM_PPTY_CAPTURE_QUALITY, 
	DCAM_PPTY_FRAME_RATE, 
	DCAM_PPTY_RW_REGISTER, 
	DCAM_PPTY_TIMEOUT,

	DCAM_PPTY_GPIO, 
	DCAM_PPTY_STROBE, 
	DCAM_PPTY_STROBE_MODE,
	DCAM_PPTY_STROBE_DURATION,
	DCAM_PPTY_STROBE_DELAY,
	DCAM_PPTY_STROBE_POLARITY,

	DCAM_PPTY_END
};

typedef enum dcam_property_enum dcam_property_enum_t;

typedef struct _dcam_handle *dcam_handle_t;
typedef struct _dcam_property dcam_property_t;

typedef unicap_status_t (* dcam_property_func_t) ( dcam_handle_t dcamhandle, 
						   unicap_property_t *property, 
						   dcam_property_t *dcam_property );

struct _dcam_property
{
		dcam_property_enum_t id;
		unicap_property_t unicap_property;
		u_int32_t register_offset; 
		u_int32_t absolute_offset;
		quadlet_t register_inq;     // content of inquiry register
		quadlet_t register_default; // initial content of cntl register 
		quadlet_t register_value;   // cached content of cntl register
		u_int32_t type;             // PPTY_TYPE
		quadlet_t feature_hi_mask;
		quadlet_t feature_lo_mask;
		dcam_property_func_t set_function;
		dcam_property_func_t get_function;
		dcam_property_func_t init_function;
};


struct _dcam_handle
{
		raw1394handle_t raw1394handle;
		int port;
		int node;
		int directory;

		unicap_device_t unicap_device;
		
		int allocate_bandwidth;

		nodeaddr_t command_regs_base; // base address of the command registers
		
		int v_format_count; 
		unicap_format_t v_format_array[3*8];

		int dma_fd; // 
		unsigned char *dma_buffer; // the ring buffer for video1394 dma capture
		size_t dma_buffer_size; //size of one dma_buffer
		int current_dma_capture_buffer; // 
		int use_dma; // flags: 1 == use video1394  0 == use raw1394
		int dma_vmmap_frame_size;
		int timeout_seconds;
		
		int current_format_index; // index of this format in the v_format_array
		int current_iso_index;    // index of this format in the isoch table
		int current_frame_rate;   // 
		int channel_allocated;    // current channel
		int bandwidth_allocated;  // current bandwidth

		unicap_buffer_type_t buffer_type; // user or system pointers
		
		dcam_property_t *dcam_property_table;
		quadlet_t feature_hi;
		quadlet_t feature_lo;

		char* trigger_modes[5]; // list of trigger modes supported by the camera
		int trigger_mode_count;
		unsigned int trigger_parameter; // 
      char *trigger_polarities[2];
      

		volatile int device_present; // set to 1 on device open and set to 0 by the bus reset handler if the device is removed

		volatile int capture_running; // set to 1 on capture start, reset to 0 on stop or error conditions

		pthread_t capture_thread; // handle of the thread to handle raw1394 capturing
		pthread_t timeout_thread; // thread to wake up raw1394_loop_iterate regularly for timeouts
		pthread_t dma_capture_thread;
		int dma_capture_thread_quit;
		int wait_for_sy; // used by raw1394 iso handler
		unsigned int current_buffer_offset; // used by raw1394 iso handler

		unsigned int buffer_size; // size of one frame
		struct _unicap_queue *current_buffer; // currently filled buffer
		
		struct _unicap_queue input_queue; 
		struct _unicap_queue output_queue;

		struct timeval last_register_access;
   struct timeval current_fill_end_time;
   struct timeval current_Fill_start_time;

		unicap_event_callback_t event_callback;
		unicap_handle_t unicap_handle;
};


struct _dcam_isoch_mode
{
		unsigned int bytes_per_frame;
		unsigned int bytes_per_packet;
		unsigned int min_speed;
};

/*
  definitions for property types
 */
enum _dcam_ppty_type_enum
{
	PPTY_TYPE_BRIGHTNESS=1,
	PPTY_TYPE_WHITEBAL_U,
	PPTY_TYPE_WHITEBAL_V,
	PPTY_TYPE_TEMPERATURE,
	PPTY_TYPE_TRIGGER,
	PPTY_TYPE_TRIGGER_POLARITY,
	PPTY_TYPE_FRAMERATE,
	PPTY_TYPE_REGISTER
};

extern dcam_property_t _dcam_properties[];

union _abs_value
{
		quadlet_t quad;
		float f;
};

typedef union _abs_value abs_value_t;


/**********************************************************************************************************/

/*
  Function prototypes
*/

unicap_status_t dcam_capture_stop( void *cpi_data );
unicap_status_t dcam_capture_start( void *cpi_data );


#ifdef PERTIMING
struct timeval starttime;
struct timeval endtime;
#define TIME(name,x) {gettimeofday( &starttime, NULL );}{x}{\
	gettimeofday( &endtime, NULL );\
	if( ( endtime.tv_usec - starttime.tv_usec ) < 0 )\
	{\
		endtime.tv_usec += 1000000;\
		endtime.tv_sec--;\
	}\
	endtime.tv_usec -= starttime.tv_usec;\
	endtime.tv_sec -= starttime.tv_sec;\
	printf( "timed '%s': %ds:%dms\n", name,endtime.tv_sec, endtime.tv_usec/1000 );\
}
#else
#define TIME(name,x) x
#endif

#endif//__DCAM_H__
