/*
  unicap euvccam plugin

  Copyright (C) 2009  Arne Caspari

  Compilation, installation or use of this program requires a written license. 

  By compilling, installing or using this software you agree to accept the terms 
  of the license as specified in the COPYING file in the root folder of this 
  software package.
 */

#ifndef   	EUVCCAM_CPI_H_
# define   	EUVCCAM_CPI_H_

#include <unicap.h>
#include <pthread.h>
#include <unicap_cpi.h>

#include "queue.h"
#include "debayer.h"

#include "euvccam_usb.h"

#include "debug.h"

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
