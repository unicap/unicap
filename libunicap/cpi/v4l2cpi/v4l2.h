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

#ifndef __V4L2_H__
#define __V4L2_H__

#include <pthread.h>
#include <semaphore.h>
#include <unicap.h>
#include <unicap_cpi.h>

#include "buffermanager.h"


#define FOURCC(a,b,c,d) (unsigned int)((((unsigned int)d)<<24)+(((unsigned int)c)<<16)+(((unsigned int)b)<<8)+a)

#define MAX_V4L2_FORMATS      128
#define V4L2_MAX_VIDEO_INPUTS 32
#define V4L2_MAX_VIDEO_NORMS  32
#define V4L2_MAX_FRAME_RATES  32
#define V4L2_NUM_BUFFERS      8

struct v4l_cache_entry
{
      char devpath[512];
      unicap_device_t device;
      int is_v4l2;
      int fd;
};    

enum _cpi_v4l2_io_method_enum
{
   CPI_V4L2_IO_METHOD_UNDECIDED = 0,
   CPI_V4L2_IO_METHOD_USERPOINTER, 
   CPI_V4L2_IO_METHOD_MMAP, 
   CPI_V4L2_IO_METHOD_READWRITE,
};

typedef enum _cpi_v4l2_io_method_enum cpi_v4l2_io_method_enum_t;

typedef struct _v4l2_handle *v4l2_handle_t;

struct _cpi_v4l2_buffer
{
      void *start;
      size_t length;
};

struct v4l2_uc_compat
{
      char *driver;
      int (*probe_func)(v4l2_handle_t handle,const char*path);
      int (*count_ext_property_func)(v4l2_handle_t handle);
      unicap_status_t (*enumerate_property_func) ( v4l2_handle_t handle, int index, unicap_property_t *property );
      unicap_status_t (*override_property_func) ( v4l2_handle_t handle, struct v4l2_queryctrl *ctrl, unicap_property_t *property );
      unicap_status_t (*set_property_func) ( v4l2_handle_t handle, unicap_property_t *property );
      unicap_status_t (*get_property_func) ( v4l2_handle_t handle, unicap_property_t *property );
      unicap_status_t (*fmt_get_func) ( struct v4l2_fmtdesc *v4l2fmt, struct v4l2_cropcap *cropcap, 
					char **identifier, unsigned int *fourcc, int *bpp );
      unicap_status_t (*override_framesize_func)( v4l2_handle_t handle, struct v4l2_frmsizeenum *frms );
      unicap_status_t (*tov4l2format_func)( v4l2_handle_t handle, unicap_format_t *format );
};


struct _v4l2_handle
{
      char device[512];
      int fd;
      char card_name[512];      

      unicap_format_t *unicap_formats;
      int format_count;
      int supported_formats;
      unicap_format_t current_format;
      int format_mask[MAX_V4L2_FORMATS];
		
      unicap_property_t *unicap_properties;
      __u32 *control_ids;
      int property_count;
      int video_in_count;
      int frame_rate_count;
      char *video_inputs[V4L2_MAX_VIDEO_INPUTS];
      char *video_norms[V4L2_MAX_VIDEO_NORMS];
      double frame_rates[V4L2_MAX_FRAME_RATES];
      int sizes_allocated;
   
   buffer_mgr_t buffer_mgr;

      cpi_v4l2_io_method_enum_t io_method;
      int buffer_count;
      struct _cpi_v4l2_buffer *buffers; // for method mmap
      int *free_buffers;
	
      struct _unicap_queue *in_queue;
      int in_queue_lock;
      struct _unicap_queue *out_queue;
      int out_queue_lock;

      int capture_running;
      volatile int quit_capture_thread;
      volatile int dqindex;
      volatile int qindex;
      pthread_t capture_thread;
      
      sem_t sema;

      unicap_event_callback_t event_callback;
      unicap_handle_t unicap_handle;

      int drop_count;
      
      double frame_rate;

      struct v4l2_uc_compat *compat;

      int removed;

      unsigned short pid;
};


#endif//__V4L2_H__
