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

#ifndef __V4L_H__
#define __V4L_H__

#include <pthread.h>
#include <semaphore.h>


struct _v4l_handle
{
      char device[512];
      int fd;
      
      struct video_capability v4lcap;
      struct video_picture v4lpict;
      struct video_picture v4lpict_default;
      struct video_mbuf v4lmbuf;
      struct video_window v4lwindow;
		

      unsigned int palette[32];

      unicap_format_t current_format;

      void *map;
      int queued_buffer;
      int ready_buffer;
      struct _unicap_queue *in_queue;
      unsigned int in_queue_lock;
      struct _unicap_queue *out_queue;
      unsigned int out_queue_lock;

      int capture_running;
      
      pthread_t capture_thread;
      int quit_capture_thread;
      sem_t sema;
      sem_t out_sema;
      
      unicap_event_callback_t event_callback;
      unicap_handle_t unicap_handle;      
};

typedef struct _v4l_handle *v4l_handle_t;

#define FOURCC(a,b,c,d) (unsigned int)((((unsigned int)a))+(((unsigned int)b)<<8)+(((unsigned int)c)<<16)+(((unsigned int)d)<<24))

#endif//__V4L_H__
