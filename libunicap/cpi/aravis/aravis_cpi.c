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

#include <unicap.h>
#include <unicap_status.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arv.h>
#include <glib.h>


#include "aravis_cpi.h"
#include "aravis_tools.h"

#include "unicap_cpi.h"
#include "queue.h"
#include "logging.h"


static unicap_status_t aravis_enumerate_devices( unicap_device_t *device, int index );
static unicap_status_t aravis_open( void **cpi_data, unicap_device_t *device );
static unicap_status_t aravis_close( aravis_handle_t handle );
static unicap_status_t aravis_reenumerate_formats( aravis_handle_t handle, int *_pcount );
static unicap_status_t aravis_enumerate_formats( aravis_handle_t handle, unicap_format_t *format, int index );
static unicap_status_t aravis_set_format( aravis_handle_t handle, unicap_format_t *format );
static unicap_status_t aravis_get_format( aravis_handle_t handle, unicap_format_t *format );
static unicap_status_t aravis_reenumerate_properties( aravis_handle_t handle, int *_pcount );
static unicap_status_t aravis_enumerate_properties( aravis_handle_t handle,
						     unicap_property_t *property, int index );
static unicap_status_t aravis_set_property( aravis_handle_t handle, unicap_property_t *property );
static unicap_status_t aravis_get_property( aravis_handle_t handle, unicap_property_t *property );
static unicap_status_t aravis_capture_start( aravis_handle_t handle );
static unicap_status_t aravis_capture_stop( aravis_handle_t handle );
static unicap_status_t aravis_queue_buffer( aravis_handle_t handle, unicap_data_buffer_t *buffer );
static unicap_status_t aravis_dequeue_buffer( aravis_handle_t handle, unicap_data_buffer_t **buffer );
static unicap_status_t aravis_wait_buffer( aravis_handle_t handle, unicap_data_buffer_t **buffer );
static unicap_status_t aravis_poll_buffer( aravis_handle_t handle, int *count );
static unicap_status_t aravis_set_event_notify( aravis_handle_t handle, unicap_event_callback_t func, unicap_handle_t unicap_handle );

static void aravis_stream_callback( aravis_handle_t handle, ArvStreamCallbackType type, ArvBuffer *buffer);

static struct _unicap_cpi cpi_s = 
{
  cpi_version: 1<<16,
  cpi_capabilities: 0x3ffff,
  cpi_flags: UNICAP_CPI_SERIALIZED,

  cpi_enumerate_devices: aravis_enumerate_devices,
  cpi_open: (cpi_open_t)aravis_open, 
  cpi_close: (cpi_close_t)aravis_close,

  cpi_reenumerate_formats: (cpi_reenumerate_formats_t)aravis_reenumerate_formats, 
  cpi_enumerate_formats: (cpi_enumerate_formats_t)aravis_enumerate_formats,   
  cpi_set_format: (cpi_set_format_t)aravis_set_format,	   
  cpi_get_format: (cpi_get_format_t)aravis_get_format,          

  cpi_reenumerate_properties: (cpi_reenumerate_properties_t)aravis_reenumerate_properties,
  cpi_enumerate_properties: (cpi_enumerate_properties_t)aravis_enumerate_properties,
  cpi_set_property: (cpi_set_property_t)aravis_set_property,
  cpi_get_property: (cpi_get_property_t)aravis_get_property,

  cpi_capture_start: (cpi_capture_start_t)aravis_capture_start,
  cpi_capture_stop: (cpi_capture_stop_t)aravis_capture_stop,
   
  cpi_queue_buffer: (cpi_queue_buffer_t)aravis_queue_buffer,
  cpi_dequeue_buffer: (cpi_dequeue_buffer_t)aravis_dequeue_buffer,
  cpi_wait_buffer: (cpi_wait_buffer_t)aravis_wait_buffer,
  cpi_poll_buffer: (cpi_poll_buffer_t)aravis_poll_buffer,

  cpi_set_event_notify: (cpi_set_event_notify_t)aravis_set_event_notify,
};

static int is_initialized = 0;

unicap_status_t cpi_register (struct _unicap_cpi *reg_data) 
{
	memcpy (reg_data, &cpi_s, sizeof( struct _unicap_cpi ));

	if (!is_initialized){
		is_initialized = 1;
		
		log_init();
		
		g_thread_init (NULL);
		g_type_init ();
		arv_debug_enable ("device,misc");
	}
	

	return STATUS_SUCCESS;
}

static unicap_status_t aravis_enumerate_devices (unicap_device_t *device, int index)
{
	unicap_status_t status = STATUS_NO_MATCH;   

	if (index == 0)
		arv_update_device_list ();

	unicap_void_device (device);

	if (index < arv_get_n_devices ()){
		ArvCamera *camera;
		strncpy (device->identifier, arv_get_device_id (index), sizeof (device->identifier)-1);
		camera = arv_camera_new (device->identifier);
		if (arv_camera_get_model_name (camera))
			strncpy (device->model_name, arv_camera_get_model_name (camera), sizeof (device->model_name)-1);
		if (arv_camera_get_vendor_name (camera))
			strncpy (device->vendor_name, arv_camera_get_vendor_name (camera), sizeof (device->vendor_name)-1);
		strcpy (device->cpi_layer, "aravis_cpi");
		status = STATUS_SUCCESS;

		g_object_unref (camera);
	}

	return status;
}

static unicap_status_t aravis_open( void **cpi_data, unicap_device_t *device )
{
	unicap_status_t status = STATUS_SUCCESS;
	aravis_handle_t handle;
	int i;
   
	handle = malloc (sizeof (struct aravis_handle));
	if( !handle )
		return STATUS_FAILURE;

	memset( handle, 0x0, sizeof( struct aravis_handle ) );

	*cpi_data = handle;

	handle->camera = arv_camera_new (device->identifier);
	if (!handle->camera)
		goto err;
   
	aravis_reenumerate_formats (handle, NULL);
	aravis_get_format( handle, &handle->current_format );

	aravis_properties_create_table (handle->camera, &handle->properties, &handle->n_properties);

	/* _init_queue( &handle->buffer_in_queue ); */
	/* _init_queue( &handle->buffer_out_queue ); */

	return status;

  err:
	free( handle );
	return status;
}

static unicap_status_t aravis_close( aravis_handle_t handle )
{
	free( handle );   
	return STATUS_SUCCESS;
}

static unicap_status_t aravis_reenumerate_formats( aravis_handle_t handle, int *_pcount )
{
	int idx = 0;
	int i;
	guint n_pixel_formats;
	gint64 *pixel_formats;
	int min_width, max_width;
	int min_height, max_height;
	
	pixel_formats = arv_camera_get_available_pixel_formats (handle->camera, 
								&n_pixel_formats);

	arv_camera_get_width_bounds (handle->camera, &min_width, &max_width);
	arv_camera_get_height_bounds (handle->camera, &min_height, &max_height);

	for (i = 0; i < n_pixel_formats; i++){
		unsigned int fourcc;
		
		fourcc = aravis_tools_get_fourcc (pixel_formats[i]);
		if (fourcc){
			unicap_void_format (&handle->formats[idx]);
			handle->formats[idx].fourcc = fourcc;
			strcpy (handle->formats[idx].identifier, aravis_tools_get_pixel_format_string (pixel_formats[i]));
			handle->formats[idx].bpp = aravis_tools_get_bpp (pixel_formats[i]);
			handle->formats[idx].min_size.width = min_width;
			handle->formats[idx].min_size.height = min_height;
			handle->formats[idx].max_size.width = max_width;
			handle->formats[idx].max_size.height = max_height;
			handle->formats[idx].size.width = max_width;
			handle->formats[idx].size.height = max_height;
			handle->formats[idx].buffer_size = max_width * max_height * handle->formats[idx].bpp / 8;
			handle->formats[idx].buffer_type = UNICAP_BUFFER_TYPE_SYSTEM;
			idx++;
		}	
	}

	g_free (pixel_formats);

	handle->n_formats = idx;

	if( _pcount )
		*_pcount = handle->n_formats;
   
	return STATUS_SUCCESS;
}

static unicap_status_t aravis_enumerate_formats( aravis_handle_t handle, unicap_format_t *format, int index )
{
	unicap_status_t status = STATUS_NO_MATCH;
	
	if ((index >= 0) && (index < handle->n_formats)){
		unicap_copy_format (format, &handle->formats[index]);
		status = STATUS_SUCCESS;
	}
   
	return status;
}

static unicap_status_t aravis_set_format( aravis_handle_t handle, unicap_format_t *format )
{
	ArvPixelFormat fmt = aravis_tools_get_pixel_format (format);
	
	if (fmt == 0)
		return STATUS_INVALID_PARAMETER;
	
	arv_camera_set_pixel_format (handle->camera, fmt);
	arv_camera_set_region (handle->camera, format->size.x, format->size.y, format->size.width, format->size.height);
	unicap_copy_format (&handle->current_format, format);

	return STATUS_SUCCESS;
}

static unicap_status_t aravis_get_format( aravis_handle_t handle, unicap_format_t *format )
{
	ArvPixelFormat pixel_fmt = arv_camera_get_pixel_format (handle->camera);

	if (!pixel_fmt)
		return STATUS_FAILURE;
	
	unicap_void_format (format);

	strcpy (format->identifier, aravis_tools_get_pixel_format_string (pixel_fmt));
	format->fourcc = aravis_tools_get_fourcc (pixel_fmt);
	format->bpp = aravis_tools_get_bpp (pixel_fmt);
	arv_camera_get_region (handle->camera, 
			       &format->size.x, 
			       &format->size.y, 
			       &format->size.width, 
			       &format->size.height);
	arv_camera_get_width_bounds (handle->camera, 
				     &format->min_size.width, 
				     &format->max_size.width);
	arv_camera_get_height_bounds (handle->camera, 
				      &format->min_size.height,
				      &format->max_size.height);	
	format->buffer_size = format->bpp * format->size.width * format->size.height / 8;
	format->buffer_type = UNICAP_BUFFER_TYPE_SYSTEM;

	return STATUS_SUCCESS;
}

static unicap_status_t aravis_reenumerate_properties( aravis_handle_t handle, int *_pcount )
{
	int count = 0;
	
	if( _pcount ){
		*_pcount = handle->n_properties;
	}

	return STATUS_SUCCESS;
}

static unicap_status_t aravis_enumerate_properties( aravis_handle_t handle,
						     unicap_property_t *property, int index )
{
	unicap_status_t status = STATUS_NO_MATCH;
	
	if ((index) >= 0 && (index < handle->n_properties)){
		unicap_copy_property (property, &handle->properties[index].property);
		status = STATUS_SUCCESS;
	}
      
	return status;
}

static unicap_status_t aravis_set_property( aravis_handle_t handle, unicap_property_t *property )
{
	unicap_status_t status = STATUS_NO_MATCH;   

	int i;
	
	for (i=0; i < handle->n_properties; i++){
		if (!strncmp (handle->properties[i].property.identifier, property->identifier, sizeof (property->identifier))){
			status = handle->properties[i].set (handle->camera, property);
		}
	}
	 
	return status;
}

static unicap_status_t aravis_get_property( aravis_handle_t handle, unicap_property_t *property )
{
	unicap_status_t status = STATUS_NO_MATCH;
	 
	int i;
	
	for (i=0; i < handle->n_properties; i++){
		if (!strncmp (handle->properties[i].property.identifier, property->identifier, sizeof (property->identifier))){
			unicap_copy_property (property, &handle->properties[i].property);
			status = handle->properties[i].get (handle->camera, property);
		}
	}

	return status;
}

static unicap_status_t aravis_capture_start( aravis_handle_t handle )
{
	guint payload;
	int i;
	
	handle->stream = arv_camera_create_stream( handle->camera, aravis_stream_callback, handle);
	arv_camera_set_acquisition_mode (handle->camera, ARV_ACQUISITION_MODE_CONTINUOUS);
	arv_camera_start_acquisition (handle->camera);

	payload = arv_camera_get_payload (handle->camera);
	for (i=0; i < 8; i++)
		arv_stream_push_buffer (handle->stream, arv_buffer_new (payload, NULL));
	
	return handle->stream ? STATUS_SUCCESS : STATUS_FAILURE;
}

static unicap_status_t aravis_capture_stop( aravis_handle_t handle )
{
	arv_camera_stop_acquisition (handle->camera);
	if (handle->stream){
		g_object_unref (handle->stream);
		handle->stream = NULL;
	}
	return STATUS_SUCCESS;
}

static unicap_status_t aravis_queue_buffer( aravis_handle_t handle, unicap_data_buffer_t *buffer )
{
	return STATUS_NOT_IMPLEMENTED;
}

static unicap_status_t aravis_dequeue_buffer( aravis_handle_t handle, unicap_data_buffer_t **buffer )
{
	return STATUS_NOT_IMPLEMENTED;
}

static unicap_status_t aravis_wait_buffer( aravis_handle_t handle, unicap_data_buffer_t **buffer )
{
	return STATUS_NOT_IMPLEMENTED;
}

static unicap_status_t aravis_poll_buffer( aravis_handle_t handle, int *count )
{
	return STATUS_NOT_IMPLEMENTED;
}

static unicap_status_t aravis_set_event_notify( aravis_handle_t handle, unicap_event_callback_t func, unicap_handle_t unicap_handle )
{
	handle->event_callback = func;
	handle->unicap_handle = unicap_handle;
   
	return STATUS_SUCCESS;
}

static void aravis_stream_callback( aravis_handle_t handle, ArvStreamCallbackType type, ArvBuffer *buffer)
{
	if (type == ARV_STREAM_CALLBACK_TYPE_BUFFER_DONE){
		unicap_data_buffer_t data_buffer;
		unicap_copy_format (&data_buffer.format, &handle->current_format);
		data_buffer.buffer_size = buffer->size;
		data_buffer.data = buffer->data;
		data_buffer.type = UNICAP_BUFFER_TYPE_SYSTEM;
		data_buffer.fill_time.tv_sec =  buffer->timestamp_ns / 1000000000ULL;
		data_buffer.fill_time.tv_usec = (buffer->timestamp_ns % 1000000000ULL) / 1000ULL;
		handle->event_callback (handle->unicap_handle, UNICAP_EVENT_NEW_FRAME, &data_buffer);
		arv_stream_push_buffer (handle->stream, buffer);
	}
}
