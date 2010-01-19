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

#include <unicap.h>
#include <unicap_status.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#include "euvccam_cpi.h"
#include "euvccam_usb.h"

#include "unicap_cpi.h"
#include "queue.h"

#include "euvccam_device.h"
#include "euvccam_devspec.h"
#include "euvccam_capture.h"





static unicap_status_t euvccam_enumerate_devices( unicap_device_t *device, int index );
static unicap_status_t euvccam_open( void **cpi_data, unicap_device_t *device );
static unicap_status_t euvccam_close( euvccam_handle_t handle );
static unicap_status_t euvccam_reenumerate_formats( euvccam_handle_t handle, int *_pcount );
static unicap_status_t euvccam_enumerate_formats( euvccam_handle_t handle, unicap_format_t *format, int index );
static unicap_status_t euvccam_set_format( euvccam_handle_t handle, unicap_format_t *format );
static unicap_status_t euvccam_get_format( euvccam_handle_t handle, unicap_format_t *format );
static unicap_status_t euvccam_reenumerate_properties( euvccam_handle_t handle, int *_pcount );
static unicap_status_t euvccam_enumerate_properties( euvccam_handle_t handle,
						     unicap_property_t *property, int index );
static unicap_status_t euvccam_set_property( euvccam_handle_t handle, unicap_property_t *property );
static unicap_status_t euvccam_get_property( euvccam_handle_t handle, unicap_property_t *property );
static unicap_status_t euvccam_capture_start( euvccam_handle_t handle );
static unicap_status_t euvccam_capture_stop( euvccam_handle_t handle );
static unicap_status_t euvccam_queue_buffer( euvccam_handle_t handle, unicap_data_buffer_t *buffer );
static unicap_status_t euvccam_dequeue_buffer( euvccam_handle_t handle, unicap_data_buffer_t **buffer );
static unicap_status_t euvccam_wait_buffer( euvccam_handle_t handle, unicap_data_buffer_t **buffer );
static unicap_status_t euvccam_poll_buffer( euvccam_handle_t handle, int *count );
static unicap_status_t euvccam_set_event_notify( euvccam_handle_t handle, unicap_event_callback_t func, unicap_handle_t unicap_handle );


static struct _unicap_cpi cpi_s = 
{
   cpi_version: 1<<16,
   cpi_capabilities: 0x3ffff,
   cpi_flags: UNICAP_CPI_SERIALIZED,

   cpi_enumerate_devices: euvccam_enumerate_devices,
   cpi_open: (cpi_open_t)euvccam_open, 
   cpi_close: (cpi_close_t)euvccam_close,

   cpi_reenumerate_formats: (cpi_reenumerate_formats_t)euvccam_reenumerate_formats, 
   cpi_enumerate_formats: (cpi_enumerate_formats_t)euvccam_enumerate_formats,   
   cpi_set_format: (cpi_set_format_t)euvccam_set_format,	   
   cpi_get_format: (cpi_get_format_t)euvccam_get_format,          

   cpi_reenumerate_properties: (cpi_reenumerate_properties_t)euvccam_reenumerate_properties,
   cpi_enumerate_properties: (cpi_enumerate_properties_t)euvccam_enumerate_properties,
   cpi_set_property: (cpi_set_property_t)euvccam_set_property,
   cpi_get_property: (cpi_get_property_t)euvccam_get_property,

   cpi_capture_start: (cpi_capture_start_t)euvccam_capture_start,
   cpi_capture_stop: (cpi_capture_stop_t)euvccam_capture_stop,
   
   cpi_queue_buffer: (cpi_queue_buffer_t)euvccam_queue_buffer,
   cpi_dequeue_buffer: (cpi_dequeue_buffer_t)euvccam_dequeue_buffer,
   cpi_wait_buffer: (cpi_wait_buffer_t)euvccam_wait_buffer,
   cpi_poll_buffer: (cpi_poll_buffer_t)euvccam_poll_buffer,

   cpi_set_event_notify: (cpi_set_event_notify_t)euvccam_set_event_notify,
};

unicap_status_t cpi_register( struct _unicap_cpi *reg_data )
{
   memcpy( reg_data, &cpi_s, sizeof( struct _unicap_cpi ) );

   log_init();

   euvccam_usb_init();

   return STATUS_SUCCESS;
}

static unicap_status_t euvccam_enumerate_devices( unicap_device_t *device, int index )
{
   unicap_status_t status = STATUS_NO_MATCH;   
   euvccam_usb_device_t *dev;

   unicap_void_device( device );

   dev = euvccam_usb_find_device( index );
   if( dev ){
      strcpy( device->identifier, dev->identifier );
      strcpy( device->model_name, dev->strProduct );
      strcpy( device->vendor_name, dev->strVendor );
      device->vendor_id = dev->idVendor;
      device->model_id = dev->serial;
      strcpy( device->cpi_layer, "euvccam_cpi" );
      status = STATUS_SUCCESS;
   }
   
   return status;
}

static unicap_status_t euvccam_open( void **cpi_data, unicap_device_t *device )
{
   unicap_status_t status = STATUS_FAILURE;
   euvccam_handle_t handle;
   int i;
   
   handle = malloc( sizeof( struct euvccam_handle ) );
   if( !handle )
      return STATUS_FAILURE;

   memset( handle, 0x0, sizeof( struct euvccam_handle ) );

   *cpi_data = handle;
   

   status = euvccam_usb_open_device( device, &handle->dev );
   if( !SUCCESS( status ) )
      goto err;
   
   status = euvccam_read_vendor_register( handle->dev.fd, 0x1a, &handle->type_flag );
   if( !SUCCESS( status ) )
      goto err;
   
   
   for( i = 0; euvccam_devspec[i].pid != 0; i++ )
   {
      if( ( euvccam_devspec[i].pid == handle->dev.idProduct ) &&
	  ( euvccam_devspec[i].type_flag == handle->type_flag ) )
      {
	 handle->devspec_index = i;
	 break;
      }
   }
   
   euvccam_device_get_format( handle, &handle->current_format );
   if( !handle->current_format )
      euvccam_set_format( handle, &euvccam_devspec[handle->devspec_index].format_list[0].format );


   _init_queue( &handle->buffer_in_queue );
   _init_queue( &handle->buffer_out_queue );

   handle->debayer_data.rgain = 4096;
   handle->debayer_data.bgain = 4096;
   handle->debayer_data.use_rbgain = 1;
   
   return status;

  err:
   if( handle->dev.fd >= 0 )
      euvccam_usb_close_device( &handle->dev );
   free( handle );
   return status;
}

static unicap_status_t euvccam_close( euvccam_handle_t handle )
{
   if( handle->dev.fd >= 0 )
      euvccam_usb_close_device( &handle->dev );
   free( handle );   
   return STATUS_SUCCESS;
}

static unicap_status_t euvccam_reenumerate_formats( euvccam_handle_t handle, int *_pcount )
{
   if( _pcount )
      *_pcount = euvccam_devspec[handle->devspec_index].format_count;
   
   return STATUS_SUCCESS;
}

static unicap_status_t euvccam_enumerate_formats( euvccam_handle_t handle, unicap_format_t *format, int index )
{
   unicap_status_t status = STATUS_NO_MATCH;
   
   if( (index >=0) && (index < euvccam_devspec[handle->devspec_index].format_count) )
   {
      unicap_copy_format( format, &euvccam_devspec[handle->devspec_index].format_list[index].format );
      status = STATUS_SUCCESS;
   }

   return status;
}

static unicap_status_t euvccam_set_format( euvccam_handle_t handle, unicap_format_t *format )
{
   return euvccam_device_set_format( handle, format );
}

static unicap_status_t euvccam_get_format( euvccam_handle_t handle, unicap_format_t *format )
{
   unicap_status_t status = STATUS_FAILURE;

   if( handle->current_format ){
      unicap_copy_format( format, &handle->current_format->format );
      status = STATUS_SUCCESS;
   }

   return status;
}

static unicap_status_t euvccam_reenumerate_properties( euvccam_handle_t handle, int *_pcount )
{
   int count = euvccam_devspec[ handle->devspec_index ].property_count;
   
   if( _pcount ){
      int i;
      for( i = 0; i < count; i++ ){
	 if( euvccam_devspec[ handle->devspec_index ].property_list[ i ].enumerate_func ){
	    unicap_property_t property;
	    unicap_void_property( &property );
	    
	    if( !SUCCESS( euvccam_devspec[ handle->devspec_index ].property_list[ i ].enumerate_func( handle, &property ) ) ){
	       count--;
	    }
	 }
      }
      *_pcount = count;
   }

   return STATUS_SUCCESS;
}

static unicap_status_t euvccam_enumerate_properties( euvccam_handle_t handle,
						     unicap_property_t *property, int index )
{
   unicap_status_t status = STATUS_NO_MATCH;
   if( index < euvccam_devspec[ handle->devspec_index ].property_count ){
      unicap_copy_property( property, &euvccam_devspec[ handle->devspec_index ].property_list[index].property );
      status = STATUS_SUCCESS;
   }
      
   return status;
}

static unicap_status_t euvccam_set_property( euvccam_handle_t handle, unicap_property_t *property )
{
   int i;
   unicap_status_t status = STATUS_NO_MATCH;
   
   for( i = 0; i < euvccam_devspec[ handle->devspec_index ].property_count; i++ ){
      if( !strncmp( property->identifier, euvccam_devspec[ handle->devspec_index ].property_list[i].property.identifier, sizeof( property->identifier ) ) ){
	 status = euvccam_devspec[ handle->devspec_index ].property_list[ i ].set_func( handle, property );
      }
   }
	 
   return status;
}

static unicap_status_t euvccam_get_property( euvccam_handle_t handle, unicap_property_t *property )
{
   int i;
   unicap_status_t status = STATUS_NO_MATCH;
   
   for( i = 0; i < euvccam_devspec[ handle->devspec_index ].property_count; i++ ){
      if( !strncmp( property->identifier, euvccam_devspec[ handle->devspec_index ].property_list[i].property.identifier, sizeof( property->identifier ) ) ){
	 void *property_data = property->property_data;
	 int property_data_size = property->property_data_size;
	 unicap_copy_property( property, &euvccam_devspec[ handle->devspec_index ].property_list[ i ].property );
	 property->property_data = property_data;
	 property->property_data_size = property_data_size;
	 status = euvccam_devspec[ handle->devspec_index ].property_list[ i ].get_func( handle, property );
      }
   }
	 
   return status;
}

static unicap_status_t euvccam_capture_start( euvccam_handle_t handle )
{
   return euvccam_capture_start_capture( handle );
}

static unicap_status_t euvccam_capture_stop( euvccam_handle_t handle )
{
   return euvccam_capture_stop_capture( handle );
}

static unicap_status_t euvccam_queue_buffer( euvccam_handle_t handle, unicap_data_buffer_t *buffer )
{
   return STATUS_NOT_IMPLEMENTED;
}

static unicap_status_t euvccam_dequeue_buffer( euvccam_handle_t handle, unicap_data_buffer_t **buffer )
{
   return STATUS_NOT_IMPLEMENTED;
}

static unicap_status_t euvccam_wait_buffer( euvccam_handle_t handle, unicap_data_buffer_t **buffer )
{
   return STATUS_NOT_IMPLEMENTED;
}

static unicap_status_t euvccam_poll_buffer( euvccam_handle_t handle, int *count )
{
   return STATUS_NOT_IMPLEMENTED;
}

static unicap_status_t euvccam_set_event_notify( euvccam_handle_t handle, unicap_event_callback_t func, unicap_handle_t unicap_handle )
{
   handle->event_callback = func;
   handle->unicap_handle = unicap_handle;
   
   return STATUS_SUCCESS;
}


