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

#ifndef __UNICAP_CPI_H__
#define __UNICAP_CPI_H__
#include "unicap.h"
#include "unicap_status.h"


int unicap_cache_add( char *key, void *value );
void* unicap_cache_get( char *key );
void unicap_cache_unref( char *key );



typedef struct _unicap_cpi * unicap_cpi_t;
		
typedef unicap_status_t (*cpi_register_t) ( unicap_cpi_t );

typedef unicap_status_t (*cpi_enumerate_devices_t) (unicap_device_t *device, int index);
typedef unicap_status_t (*cpi_open_t)              (void **, unicap_device_t *device );
typedef unicap_status_t (*cpi_close_t)             (void *);

typedef unicap_status_t (*cpi_reenumerate_formats_t) (void *, int *count );
typedef unicap_status_t (*cpi_enumerate_formats_t)   (void *, unicap_format_t *format, int index);
typedef unicap_status_t (*cpi_set_format_t)          (void *, unicap_format_t *format);
typedef unicap_status_t (*cpi_get_format_t)          (void *, unicap_format_t *format);

typedef unicap_status_t (*cpi_reenumerate_properties_t)(void *, int *count );
typedef unicap_status_t (*cpi_enumerate_properties_t)  (void *, unicap_property_t *property, int index);
typedef unicap_status_t (*cpi_set_property_t)          (void *, unicap_property_t *property);
typedef unicap_status_t (*cpi_get_property_t)          (void *, unicap_property_t *property);

typedef unicap_status_t (*cpi_capture_start_t) (void *);
typedef unicap_status_t (*cpi_capture_stop_t)  (void *);

typedef unicap_status_t (*cpi_queue_buffer_t)  (void *, unicap_data_buffer_t *buffer);
typedef unicap_status_t (*cpi_dequeue_buffer_t)(void *, unicap_data_buffer_t **buffer);
typedef unicap_status_t (*cpi_wait_buffer_t)   (void *, unicap_data_buffer_t **buffer);
typedef unicap_status_t (*cpi_poll_buffer_t)   (void *, int *count);

typedef void (*unicap_event_callback_t) (unicap_handle_t handle, unicap_event_t event, ... );

typedef unicap_status_t (*cpi_set_event_notify_t ) ( void *, unicap_event_callback_t func, unicap_handle_t handle );


#define UNICAP_CPI_SERIALIZED 0x1
#define UNICAP_CPI_REMOTE     0x2

#define CPI_CAP_ENUMERATE_DEVICES      ( 1<<0 )
#define CPI_CAP_OPEN                   ( 1<<1 )
#define CPI_CAP_CLOSE                  ( 1<<2 )
#define CPI_CAP_REENUMERATE_FORMATS    ( 1<<3 )
#define CPI_CAP_ENUMERATE_FORMATS      ( 1<<4 )
#define CPI_CAP_SET_FORMAT             ( 1<<5 )
#define CPI_CAP_GET_FORMAT             ( 1<<6 )
#define CPI_CAP_REENUMERATE_PROPERTIES ( 1<<7 )
#define CPI_CAP_ENUMERATE_PROPERTIES   ( 1<<8 )
#define CPI_CAP_SET_PROPERTY           ( 1<<9 )
#define CPI_CAP_GET_PROPERTY           ( 1<<10 )
#define CPI_CAP_CAPTURE_START          ( 1<<11 )
#define CPI_CAP_CAPTURE_STOP           ( 1<<12 )
#define CPI_CAP_QUEUE_BUFFER           ( 1<<13 )
#define CPI_CAP_DEQUEUE_BUFFER         ( 1<<14 )
#define CPI_CAP_WAIT_BUFFER            ( 1<<15 )
#define CPI_CAP_POLL_BUFFER            ( 1<<16 )
#define CPI_CAP_SET_EVENT_NOTIFY       ( 1<<17 )

struct _unicap_cpi
{
		unsigned int cpi_version;
		u_int64_t    cpi_capabilities;
		u_int64_t    cpi_flags;
		
		cpi_register_t cpi_register;

		cpi_enumerate_devices_t cpi_enumerate_devices;
		cpi_open_t              cpi_open;
		cpi_close_t             cpi_close;
		
		cpi_reenumerate_formats_t cpi_reenumerate_formats;
		cpi_enumerate_formats_t   cpi_enumerate_formats;
		cpi_set_format_t          cpi_set_format;
		cpi_get_format_t          cpi_get_format;          
		
		cpi_reenumerate_properties_t cpi_reenumerate_properties;
		cpi_enumerate_properties_t   cpi_enumerate_properties;
		cpi_set_property_t           cpi_set_property;
		cpi_get_property_t           cpi_get_property;          
		
		cpi_capture_start_t cpi_capture_start;
		cpi_capture_stop_t  cpi_capture_stop; 
		
		cpi_queue_buffer_t   cpi_queue_buffer;
		cpi_dequeue_buffer_t cpi_dequeue_buffer;
		cpi_wait_buffer_t    cpi_wait_buffer;
		cpi_poll_buffer_t    cpi_poll_buffer;   

		cpi_set_event_notify_t   cpi_set_event_notify;
		
		void *reserved[43];
};



#define CPI_VERSION_HI(x) (x>>16)
#endif//__UNICAP_CPI_H__
