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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "config.h"

#include <unicap.h>
#include <unicap_status.h>

#if UNICAP_DEBUG
#define DEBUG
#endif
#include "debug.h"

unicap_status_t unicap_void_device( unicap_device_t *device )
{
	if( !device )
	{
		return STATUS_INVALID_PARAMETER;
	}

	memset( device, 0, sizeof( unicap_device_t ) );
	
	strcpy( device->identifier, "" );
	strcpy( device->model_name, "" );
	strcpy( device->vendor_name, "" );
	device->model_id = -1;
	device->vendor_id = -1;
	strcpy( device->cpi_layer, "" );
	strcpy( device->device, "" );

	return STATUS_SUCCESS;
}

unicap_status_t unicap_void_format( unicap_format_t *format )
{
	if( !format )
	{
		return STATUS_INVALID_PARAMETER;
	}

	memset( format, 0x0, sizeof( unicap_format_t ) );

	strcpy( format->identifier, "" );
	format->size.width = format->size.height = format->bpp = -1;
	format->fourcc = 0;
	format->min_size.width = format->max_size.width = format->min_size.height = format->max_size.height = -1;
	format->h_stepping = format->v_stepping = -1;
	format->buffer_size = -1;
	format->buffer_type = UNICAP_BUFFER_TYPE_USER;
	
	return STATUS_SUCCESS;
}

unicap_status_t unicap_void_property( unicap_property_t *property )
{
	if( !property )
	{
		return STATUS_INVALID_PARAMETER;
	}

	memset( property, 0x0, sizeof( unicap_property_t ) );
	
	strcpy( property->identifier, "" );
	strcpy( property->unit, "" );
	strcpy( property->category, "" );
	property->value = 0.0f;
	property->range.min = 0.0f;
	property->range.max = 0.0f;
	property->stepping = 0.0f;
	property->flags = 0;
	property->flags_mask = 0;
	
	return STATUS_SUCCESS;
}

unicap_status_t unicap_describe_device( unicap_device_t *device, char *buffer, size_t *buffer_size )
{
	char tmp_buffer[ 512 ];
	
	sprintf( tmp_buffer, 
		 "Device id: %s\n"\
		 "Model name: %s\n"\
		 "Vendor name: %s\n"\
		 "Model id: %llu\n"\
		 "Vendor id: %u ( 0x%x )\n"\
		 "cpi: %s\n"\
		 "device: %s\n", 
		 device->identifier ? device->identifier : "(nil)",
		 device->model_name ? device->model_name : "(nil)", 
		 device->vendor_name ? device->vendor_name : "(nil)", 
		 device->model_id,
		 device->vendor_id, device->vendor_id, 
		 device->cpi_layer ? device->cpi_layer : "(nil)", 
		 device->device ? device->device : "(nil)" );
	
	strncpy( buffer, tmp_buffer, (*buffer_size)-1 );
	
	return STATUS_SUCCESS;
}

unicap_status_t unicap_describe_format( unicap_format_t *format, char *buffer, size_t *buffer_size )
{
	char tmp_buffer[512];
	
	sprintf( tmp_buffer, 
		 "Format name: %s\n"\
		 "width: %d\n"\
		 "height: %d\n"\
		 "bpp: %d\n"\
		 "fourcc: %c%c%c%c\n"\
		 "min_width: %d\n"\
		 "min_height: %d\n"\
		 "max_width: %d\n"\
		 "max_height: %d\n"\
		 "h_stepping: %d\n"\
		 "v_stepping: %d\n", 
		 format->identifier, 
		 format->size.width, 
		 format->size.height,
		 format->bpp,
		 format->fourcc,
		 format->fourcc>>8,
		 format->fourcc>>16,
		 format->fourcc>>24,
		 format->min_size.width,
		 format->min_size.height,
		 format->max_size.width, 
		 format->max_size.height, 
		 format->h_stepping, 
		 format->v_stepping );
	
	strncpy( buffer, tmp_buffer, *buffer_size );
	
	return STATUS_SUCCESS;
}

unicap_format_t *unicap_copy_format( unicap_format_t *dest, const unicap_format_t *src )
{
	strcpy( dest->identifier, src->identifier );

	memcpy( &dest->size, &src->size, sizeof( unicap_rect_t ) );
	memcpy( &dest->min_size, &src->min_size, sizeof( unicap_rect_t ) );
	memcpy( &dest->max_size, &src->max_size, sizeof( unicap_rect_t ) );

	dest->h_stepping = src->h_stepping;
	dest->v_stepping = src->v_stepping;
	dest->sizes = src->sizes;
	dest->size_count = src->size_count;
	
	dest->bpp = src->bpp;
	dest->fourcc = src->fourcc;
	dest->flags = src->flags;

	dest->buffer_types = src->buffer_types;
	
	dest->buffer_size = src->buffer_size;
	dest->buffer_type = src->buffer_type;
	return dest;
}

unicap_property_t *unicap_copy_property( unicap_property_t *dest, const unicap_property_t *src )
{
	strcpy( dest->identifier, src->identifier );
	strcpy( dest->category, src->category );
	strcpy( dest->unit, src->unit );
	dest->relations = src->relations;
	dest->relations_count = src->relations_count;
	dest->type = src->type;

	switch( src->type )
	{
		case UNICAP_PROPERTY_TYPE_MENU:
		{
			strcpy( dest->menu_item, src->menu_item );
			dest->menu.menu_items = src->menu.menu_items;
			dest->menu.menu_item_count = src->menu.menu_item_count;
		}
		break;
		
		case UNICAP_PROPERTY_TYPE_VALUE_LIST:
		{
			dest->value = src->value;
			dest->value_list.values = src->value_list.values;
			dest->value_list.value_count = src->value_list.value_count;
		}
		break;
		
		case UNICAP_PROPERTY_TYPE_RANGE:
		{
			dest->value = src->value;
			dest->range.min = src->range.min;
			dest->range.max = src->range.max;
		}
		break;

		default: 
			break;
	}
	
	dest->stepping = src->stepping;
	dest->flags = src->flags;
	dest->flags_mask = src->flags_mask;
	dest->property_data = src->property_data;
	dest->property_data_size = src->property_data_size;

	return dest;
}

unicap_property_t *unicap_copy_property_nodata( unicap_property_t *dest, unicap_property_t *src )
{
	strcpy( dest->identifier, src->identifier );
	strcpy( dest->category, src->category );
	strcpy( dest->unit, src->unit );
	dest->relations = src->relations;
	dest->relations_count = src->relations_count;
	dest->type = src->type;

	switch( src->type )
	{
		case UNICAP_PROPERTY_TYPE_MENU:
		{
			strcpy( dest->menu_item, src->menu_item );
			dest->menu.menu_items = src->menu.menu_items;
			dest->menu.menu_item_count = src->menu.menu_item_count;
		}
		break;
		
		case UNICAP_PROPERTY_TYPE_VALUE_LIST:
		{
			dest->value = src->value;
			dest->value_list.values = src->value_list.values;
			dest->value_list.value_count = src->value_list.value_count;
		}
		break;
		
		case UNICAP_PROPERTY_TYPE_RANGE:
		{
			dest->value = src->value;
			dest->range.min = src->range.min;
			dest->range.max = src->range.max;
		}
		break;

		default: 
			break;
	}
	
	dest->stepping = src->stepping;
	dest->flags = src->flags;
	dest->flags_mask = src->flags_mask;

	return dest;
}

unicap_status_t unicap_describe_property( unicap_property_t *property, char *buffer, size_t *buffer_size )
{
	char tmp_buffer[512];
	
	sprintf( tmp_buffer, 
		 "Property name: %s\n"\
		 "class: %s\n"\
		 "unit: %s\n"\
		 "value: %g\n"\
		 "range: min: %g\n"\
		 "       max: %g\n"\
		 "stepping: %g\n"\
		 "property data size: %d\n",
		 property->identifier,
		 property->category,
		 property->unit,
		 property->value,
		 property->range.min,
		 property->range.max,
		 property->stepping,
		 property->property_data_size );
	
	strncpy( buffer, tmp_buffer, *buffer_size );
	
	return STATUS_SUCCESS;
}
