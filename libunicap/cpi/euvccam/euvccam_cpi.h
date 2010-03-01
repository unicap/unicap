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

#ifndef   	EUVCCAM_CPI_H_
# define   	EUVCCAM_CPI_H_

#include <config.h>
#include <unicap.h>
#include <pthread.h>
#include <unicap_cpi.h>

#include "queue.h"
#include "debayer.h"

#if EUVCCAM_DEBUG
#define DEBUG
#endif
#include "debug.h"

#include "euvccam_usb.h"


struct euvccam_handle
{
      euvccam_usb_device_t dev;
   unsigned char type_flag;
   
   int removed;

   unicap_handle_t unicap_handle;
   unicap_event_callback_t event_callback;
   
   int devspec_index;
   struct euvccam_video_format_description *current_format;

   unicap_queue_t buffer_in_queue;
   unicap_queue_t buffer_out_queue;

   pthread_t capture_thread;
   int capture_thread_quit;
   int capture_running;

   int streaming_endpoint;
   
   char current_ae_mode;
   unsigned int current_wb_r;
   unsigned int current_wb_g;

   debayer_data_t debayer_data;
};


typedef struct euvccam_handle *euvccam_handle_t;



#endif 	    /* !EUVCCAM_CPI_H_ */
