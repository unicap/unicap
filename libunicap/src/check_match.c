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

#include "config.h"
#include "check_match.h"

#if UNICAP_DEBUG
#define DEBUG
#endif
#include "debug.h"


/*
  Returns 1 if 'device' maches on 'specifier' or 0 if it does not.

  Input:
    specifier: 
    device:
    
  Return values: 1
                 0
 */
int _check_device_match( unicap_device_t *specifier, unicap_device_t *device )
{

	if( !specifier ) 
	{
		return 1;
	}
	
	if( strlen( specifier->identifier ) )
	{
		if( strncmp( specifier->identifier, device->identifier, 256 ) )
		{
/* 			TRACE( "identifier mismatch: %s <> %s\n", specifier->identifier, device->identifier ); */
			return 0;
		}
		else
		{
			return 1;
		}
	}

	if( strlen( specifier->model_name ) )
	{
		if( strncmp( specifier->model_name, device->model_name, 256 ) )
		{
/* 			TRACE( "model name mismatch %s <> %s\n", specifier->model_name, device->model_name ); */
			return 0;
		}
	}
	
	if( strlen( specifier->vendor_name ) )
	{
		if( strncmp( specifier->vendor_name, device->vendor_name, 256 ) )
		{
/* 			TRACE( "vendor name mismatch\n" ); */
			return 0;
		}
	}
	
	if( specifier->model_id != -1 )
	{
		if( specifier->model_id != device->model_id )
		{
/* 			TRACE( "model id mismatch\n" ); */
			return 0;
		}
	}
	
	if( specifier->vendor_id != -1 )
	{
		if( specifier->vendor_id != device->vendor_id )
		{
/* 			TRACE( "vendor_id mismatch\n" ); */
			return 0;
		}
	}
	
	if( strlen( specifier->cpi_layer ) )
	{
		if( strncmp( specifier->cpi_layer, device->cpi_layer, 256 ) )
		{
/* 			TRACE( "cpi_layer mismatch\n" ); */
			return 0;
		}
	}
	
	if( strlen( specifier->device ) )
	{
		if( strncmp( specifier->device, device->device, 256 ) )
		{
/* 			TRACE( "device mismatch\n" ); */
			return 0;
		}
	}
	
	return 1;
}

int _check_format_match( unicap_format_t *specifier, unicap_format_t *format )
{
	int i;

/* 	char b[512]; */
/* 	size_t bs = sizeof( b ); */

	if( !specifier )
	{
		return 1;
	}

/* 	unicap_describe_format( specifier, b, &bs ); */
/* 	printf( "spec: %s", b ); */
/* 	bs = sizeof( b ); */
/* 	unicap_describe_format( format, b, &bs ); */
/* 	printf( "format: %s", b ); */
	
	if( strlen( specifier->identifier ) && 
		strncmp( specifier->identifier, format->identifier, 256 ) )
	{
/* 		TRACE( "identifier missmatch\n" ); */
		return 0;
	}
	
	if( ( specifier->size.width != -1 ) &&
		( specifier->size.width != format->size.width ) )
	{
		int match = 0;
		if( format->size_count )
		{
		   for( i = 0; i < format->size_count; i++ )
		   {
		      if( format->sizes[i].width == specifier->size.width )
		      {
			 match = 1;
			 
		      }
		   }
		}
		else
		{
		   if( ( format->min_size.width <= specifier->size.width ) && 
		       ( format->max_size.width >= specifier->size.width ) )
		   {
		      match = 1;
		   }
		}
		
		if( !match )
		{
/* 			TRACE( "width mismatch\n" ); */
			return 0;
		}
	}
	
	if( ( specifier->size.height != -1 ) &&
		( specifier->size.height != format->size.height ) )
	{
		int match = 0;
		for( i = 0; i < format->size_count; i++ )
		{
			if( format->sizes[i].height == specifier->size.height )
			{
				match = 1;
			}
		}
		
		if( !match )
		{
			if( ( format->min_size.height != -1 ) &&
				( specifier->size.height >= format->min_size.height ) &&
				( specifier->size.height <= format->max_size.height ) )
			{
				//check the cropping size
/* 				match = !( ( specifier->size.height - format->min_size.height ) %  */
/* 						   format->v_stepping ); */

			   match = 1;
			}
		}

		if( !match )
		{
/* 			TRACE( "height mismatch\n" ); */
			return 0;
		}
	}
	
	if( ( specifier->bpp != -1 ) &&
		( specifier->bpp != format->bpp ) )
	{
/* 		TRACE( "bpp missmatch\n" ); */
		return 0;
	}
	
	if( ( specifier->fourcc != 0 ) &&
		( specifier->fourcc != format->fourcc ) )
	{
/* 		TRACE( "fourcc missmatch\n" ); */
		return 0;
	}
	
	if( ( specifier->min_size.width != -1 ) && 
		( specifier->min_size.width < format->min_size.width ) )
	{
/* 		TRACE( "min_size.w missmatch\n" ); */
		return 0;
	}
	
	if( ( specifier->max_size.width != -1 ) &&
		( specifier->max_size.width > format->max_size.width ) )
	{
/* 		TRACE( "max_size.w missmatch\n" ); */
		return 0;
	}

	if( ( specifier->min_size.height != -1 ) &&
		( specifier->min_size.height < format->min_size.height ) )
	{
/* 		TRACE( "min_size.h missmatch\n" ); */
		return 0;
	}
	
	if( ( specifier->max_size.height != -1 ) &&
		( specifier->max_size.height > format->max_size.height ) )
	{
/* 		TRACE( "max_size.h missmatch\n" ); */
		return 0;
	}
	
	if( ( specifier->h_stepping != -1 ) &&
		( specifier->h_stepping != format->h_stepping ) )
	{
/* 		TRACE( "h_step missmatch\n" ); */
		return 0;
	}
	
	if( ( specifier->v_stepping != -1 ) &&
		( specifier->v_stepping != format->v_stepping ) )
	{
/* 		TRACE( "v_step missmatch\n" ); */
		return 0;
	}
	
	if( ( specifier->buffer_size != -1 ) &&
		( specifier->buffer_size != format->buffer_size ) )
	{
/* 		TRACE( "buffer_size missmatch\n" ); */
		return 0;
	}
	
	return 1;
}

int _check_property_match( unicap_property_t *specifier, unicap_property_t *property )
{
	if( !specifier )
	{
		return 1;
	}

	if( strlen( specifier->identifier ) )
	{
		if( strcmp( specifier->identifier, property->identifier ) )
		{
/* 			TRACE( "match failed\n" ); */
			return 0;
		}
	}
	
	if( strlen( specifier->unit ) )
	{
		if( strcmp( specifier->unit, property->unit ) )
		{
/* 			TRACE( "match failed\n" ); */
			return 0;
		}
	}

	if( strlen( specifier->category ) )
	{
		if( strcmp( specifier->category, property->category ) )
		{
/* 			TRACE( "match failed\n" ); */
			return 0;
		}
	}
	
	
	if( specifier->value != 0 )
	{
		if( specifier->value != property->value )
		{
/* 			TRACE( "match failed\n" ); */
			return 0;
		}
	}
	
	if( specifier->range.min != 0 )
	{
		if( specifier->range.min < property->range.min )
		{
/* 			TRACE( "match failed\n" ); */
			return 0;
		}
	}
	
	if( specifier->range.max != 0 )
	{
		if( specifier->range.max > property->range.max )
		{
/* 			TRACE( "match failed\n" ); */
			return 0;
		}
	}
	
	if( specifier->stepping != 0 )
	{
		if( specifier->stepping != property->stepping )
		{
/* 			TRACE( "match failed\n" ); */
			return 0;
		}
	}
	
	return 1;
}
