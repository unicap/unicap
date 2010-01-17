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

#ifndef __vid21394_h__
#define __vid21394_h__

#include <config.h>

#define VID21394_INVALID_HANDLE_VALUE 0

typedef struct vid21394_handle *vid21394handle_t;

enum vid21394_frequency
{
	VID21394_FREQ_50 = 0,
	VID21394_FREQ_60 = 1,
	VID21394_FREQ_AUTO = 3,
	VID21394_FREQ_UNKNOWN,
};

enum vid21394_video_mode
{	
	VID21394_Y_320x240     = 0x4,
	VID21394_Y41P_320x240  = 0x6,
	VID21394_UYVY_320x240  = 0x1,
	VID21394_YUY2_320x240  = 0x101,
	VID21394_Y_640x480     = 0x5,
	VID21394_Y41P_640x480  = 0x2,
	VID21394_UYVY_640x480  = 0x3,
	VID21394_YUY2_640x480  = 0x103,
	VID21394_Y_768x576     = 0x7,
	VID21394_Y41P_768x576  = 0x8,
	VID21394_UYVY_768x576  = 0x9,
	VID21394_YUY2_768x576  = 0x109
};

enum vid21394_input_channel
{
	VID21394_INPUT_COMPOSITE_1 = 0x4,
	VID21394_INPUT_COMPOSITE_2 = 0x5,

	VID21394_INPUT_COMPOSITE_3 = 0x1,
	VID21394_INPUT_COMPOSITE_4 = 0x3,

	VID21394_INPUT_SVIDEO      = 0x9, 
	VID21394_INPUT_AUTO        = 0xff,
};

/*
  Obtain a handle to a video 2 1394 device. 
  
  Input: sernum: Serial number of device to open

  Output: handle to the device or NULL if the device could not be opened

  errno: ENOSYS: This kernel lacks raw1394 support
         EAGAIN: 1394 interface not accessible
	 
 */
/* vid21394handle_t vid21394_open( unsigned long long sernum ); */
/* void vid21394_close( vid21394handle_t vid21394handle ); */


// converter operation
/* int vid21394_start_transmit( vid21394handle_t vid21394handle ); */
/* int vid21394_stop_transmit( vid21394handle_t vid21394handle ); */
/* int vid21394_set_input_channel( vid21394handle_t handle, enum vid21394_input_channel channel ); */
/* int vid21394_set_brightness( vid21394handle_t vid21394handle, unsigned int brightness ); */
/* int vid21394_set_contrast( vid21394handle_t vid21394handle, unsigned int contrast ); */
/* int vid21394_set_frequency( vid21394handle_t vid21394handle, enum vid21394_frequency freq ); */
/* int vid21394_get_firm_vers( vid21394handle_t handle ); */
/* int vid21394_set_video_mode( vid21394handle_t vid21394handle, enum vid21394_video_mode video_mode ); */
/* int vid21394_start_receive( vid21394handle_t vid21394handle ); */
/* int vid21394_stop_receive( vid21394handle_t vid21394handle ); */

/* void vid21394_queue_buffer( vid21394handle_t vid21394handle, void * buffer ); */
/* void *vid21394_dequeue_buffer( vid21394handle_t vid21394handle ); */
/* int vid21394_wait_buffer( vid21394handle_t vid21394handle, void **buffer ); */
/* int vid21394_poll_buffer( vid21394handle_t vid21394handle ); */


#endif //__vid21394_h__
