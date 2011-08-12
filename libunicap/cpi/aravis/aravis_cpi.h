/*
  unicap aravis plugin

  Copyright (C) 2011  Arne Caspari

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

#ifndef   	ARAVIS_CPI_H_
# define   	ARAVIS_CPI_H_

#include <config.h>
#include <unicap.h>
#include <pthread.h>
#include <unicap_cpi.h>

#include <arv.h>

#include "aravis_properties.h"


#include "queue.h"
#include "debayer.h"

#if ARAVIS_DEBUG
#define DEBUG
#endif
#include "debug.h"

#define MAX_FORMATS 12


struct aravis_handle
{
	unsigned char type_flag;
   
	int removed;
	
	unicap_handle_t unicap_handle;
	unicap_event_callback_t event_callback;

	unicap_queue_t buffer_in_queue;
	unicap_queue_t buffer_out_queue;

	unicap_format_t current_format;
	unicap_format_t formats[MAX_FORMATS];
	int n_formats;

	struct aravis_property *properties;
	int n_properties;
	

	ArvCamera *camera;
	ArvStream *stream;
	
};


typedef struct aravis_handle *aravis_handle_t;



#endif 	    /* !ARAVIS_CPI_H_ */
