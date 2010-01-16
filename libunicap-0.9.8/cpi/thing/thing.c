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

/*
  thing.c: test plugin for the unicap llibrary
*/

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <stdio.h>

#include <unicap.h>
#include <unicap_status.h>
#include <unicap_cpi.h>

#if THING_DEBUG
#define DEBUG
#endif
#include <debug.h>

#include <sys/time.h>
#include <time.h>

#include "unicap_cpi_std_struct.h"

#include "queue.h"
#include "thing.h"

#define THING_SYSTEM_BUFFERS 8

unicap_rect_t null_format_sizes[] =
{
   { 0, 0, 320, 240 },
   { 0, 0, 384, 288 }, 
   { 0, 0, 640, 480 }
};
	

unicap_format_t null_formats[] =
{
   { identifier: "UYVY", // name
     size: { x: 0, y: 0, width: 320, height: 240}, 
     min_size: { x: 0, y: 0, width: 320, height: 240}, 
     max_size: { x: 0, y: 0, width: 640, height: 480},
     h_stepping: 0, 
     v_stepping: 0, 
     sizes: null_format_sizes, 
     size_count: 3,
     bpp: 16, // bpp
     fourcc: FOURCC('U','Y','V','Y'), // fourcc
     flags: 0, // flags
     buffer_types: UNICAP_FLAGS_BUFFER_TYPE_USER | UNICAP_FLAGS_BUFFER_TYPE_SYSTEM, 
     system_buffer_count: THING_SYSTEM_BUFFERS,
     buffer_size: 320*240*2}, // buffer size
   { identifier: "RGB32", // name
     size: { x: 0, y: 0, width: 320, height: 240}, 
     min_size: { x: 0, y: 0, width: 320, height: 240}, 
     max_size: { x: 0, y: 0, width: 640, height: 480},
     h_stepping: 0, 
     v_stepping: 0, 
     sizes: null_format_sizes, 
     size_count: 3,
     bpp: 32, // bpp
     fourcc: FOURCC('R','G','B','4'), // fourcc
     flags: 0, // flags
     buffer_types: UNICAP_FLAGS_BUFFER_TYPE_USER, 
     system_buffer_count: THING_SYSTEM_BUFFERS,
     buffer_size: 320*240*4}, // buffer size
};




char *thingv3_menu_items[] = 
{
   "MENU a", 
   "MENU b",
   "MENU c",
   "MENU d",
};

double value_list_items[] =
{
   0.0, 
   1.0, 
   1.5, 
   2.0, 
   3.75, 
   7.5, 
};		

char *long_name_relations[] = 
{
   "frame rate", 
   "thing value_list", 
};


unicap_property_t null_properties[] =
{
   { identifier: "frame rate",
     category: "video", 
     unit: "fps", 
     relations: 0, 
     relations_count: 0, 
     { value: 30 }, // default value
     { range: { min: 1.0, max: 30.0 } }, // range
     stepping: 1.0, // stepping
     type: UNICAP_PROPERTY_TYPE_RANGE,
     flags: UNICAP_FLAGS_MANUAL, 
     flags_mask: UNICAP_FLAGS_MANUAL,
     property_data: 0, // property data
     property_data_size: 0, // property data size
   },
   { identifier: "thing long name", 
     category: "video", 
     unit: "", 
     relations: long_name_relations, 
     relations_count: sizeof(long_name_relations) / sizeof(char*), 
     { value: 0.5 }, // default value
     { range: { min: 0.0, max: 1.0 } }, // range
     stepping: 0.01, // stepping
     type: UNICAP_PROPERTY_TYPE_RANGE,
     flags: UNICAP_FLAGS_MANUAL,
     flags_mask: UNICAP_FLAGS_AUTO | UNICAP_FLAGS_ONE_PUSH | UNICAP_FLAGS_MANUAL, 
     property_data: 0, // property data
     property_data_size: 0, // property data size
   },
   { identifier: "thing m1", 
     category: "video", 
     unit: "", 
     relations: 0, 
     relations_count: 0, 
     { value: 0}, // default value
     { range: { min: 0, max: 0 } }, // range
     stepping: 0.1, // stepping
     type: UNICAP_PROPERTY_TYPE_MENU,
     flags: UNICAP_FLAGS_MANUAL, 
     flags_mask: UNICAP_FLAGS_MANUAL,
     property_data: 0, // property data
     property_data_size: 0, // property data size
   },
   { identifier: "thing value_list", 
     category: "video", 
     unit: "", 
     relations: 0, 
     relations_count: 0, 
     { value: 0}, // default value
     { range: { min: 0, max: 0 } }, // range
     stepping: 0.1, // stepping
     type: UNICAP_PROPERTY_TYPE_VALUE_LIST,
     flags: UNICAP_FLAGS_MANUAL, 
     flags_mask: UNICAP_FLAGS_MANUAL,
     property_data: 0, // property data
     property_data_size: 0, // property data size
   },
   { identifier: "thing a1", 
     category: "video", 
     unit: "", 
     relations: 0, 
     relations_count: 0,	  
     { value: 0.5 }, // default value
     { range: { min: 0.0, max: 1.0 } }, // range
     stepping: 0.01, // stepping
     type: UNICAP_PROPERTY_TYPE_RANGE,
     flags: UNICAP_FLAGS_MANUAL, 
     flags_mask: UNICAP_FLAGS_MANUAL,
     property_data: 0, // property data
     property_data_size: 0, // property data size
   },
   { identifier: "thing t1", 
     category: "video", 
     unit: "", 
     relations: 0, 
     relations_count: 0,	  
     { value: 0.5 }, // default value
     { range: { min: 0.0, max: 1.0 } }, // range
     stepping: 0.01, // stepping
     type: UNICAP_PROPERTY_TYPE_RANGE,
     flags: 0, 
     flags_mask: UNICAP_FLAGS_MANUAL,
     property_data: 0, // property data
     property_data_size: 0, // property data size
   },
};

struct _null_data_struct
{
      int instance;
      int device;
      int format;
	
      int capture_running;
	
      unicap_property_t *current_properties;
	
      struct _unicap_queue *in_queue;
      int in_queue_lock;
      struct _unicap_queue *out_queue;
      int out_queue_lock;

      struct timeval last_capture_time;
      
      unicap_format_t current_format;

      unsigned char *system_buffers[THING_SYSTEM_BUFFERS];
      int current_system_buffer;

      int frame_rate;
};

typedef struct _null_data_struct null_data_t;

int g_instance_count = 0;

unicap_status_t cpi_register( struct _unicap_cpi *reg_data )
{
   TRACE( "Register thing.c\n" );
   reg_data->cpi_version = 1<<16;
   reg_data->cpi_capabilities = 0x3ffff;
   reg_data->cpi_enumerate_devices = cpi_enumerate_devices;
   reg_data->cpi_open = cpi_open; 
   reg_data->cpi_close = cpi_close;

   TRACE( "cpi_enumerate_devices: %p\n", cpi_enumerate_devices );
	
   reg_data->cpi_reenumerate_formats = cpi_reenumerate_formats; 
   reg_data->cpi_enumerate_formats = cpi_enumerate_formats;   
   reg_data->cpi_set_format = cpi_set_format;	   
   reg_data->cpi_get_format = cpi_get_format;          

   reg_data->cpi_reenumerate_properties = cpi_reenumerate_properties;
   reg_data->cpi_enumerate_properties = cpi_enumerate_properties;
   reg_data->cpi_set_property = cpi_set_property;
   reg_data->cpi_get_property = cpi_get_property;

   reg_data->cpi_capture_start = cpi_capture_start;
   reg_data->cpi_capture_stop = cpi_capture_stop;
	
   reg_data->cpi_queue_buffer = cpi_queue_buffer;
   reg_data->cpi_dequeue_buffer = cpi_dequeue_buffer;
   reg_data->cpi_wait_buffer = cpi_wait_buffer;
   reg_data->cpi_poll_buffer = cpi_poll_buffer;	

   return STATUS_SUCCESS;
}

int cpi_open( void **cpi_data, unicap_device_t *device )
{
   null_data_t *data;

   TRACE( "open thing.cpi, devcice: %p\n", device );
   *cpi_data = calloc( 1, sizeof( struct _null_data_struct ) );
   if( !*cpi_data )
   {
      TRACE( "->open STATUS_NO_MEM\n" );
      return STATUS_NO_MEM;
   }

   data = (null_data_t *)*cpi_data;

   data->current_properties = malloc( sizeof( null_properties ) );
   if( !data->current_properties )
   {
      free( *cpi_data );
      return STATUS_NO_MEM;
   }

   memcpy( data->current_properties, null_properties, sizeof( null_properties ) );


   TRACE( "->open prepare menu items\n" );

   // prepare menu items
   strcpy( data->current_properties[2].menu_item, "MENU c" );
   data->current_properties[2].menu.menu_items = thingv3_menu_items;
   data->current_properties[2].menu.menu_item_count = 4;

   // prepare value list
   data->current_properties[3].value = 1.0;
   data->current_properties[3].value_list.values = value_list_items;
   data->current_properties[3].value_list.value_count = ( sizeof( value_list_items ) / sizeof( double ) );
	
   TRACE( "->open g_instance_count\n" );

   g_instance_count++;

   data->instance = g_instance_count;
   data->device = 0;
   data->format = 0;
   memcpy( &data->current_format, &null_formats[0], sizeof( unicap_format_t ) );
   data->current_format.size.width = 640;
   data->current_format.size.height = 480;
   data->capture_running = 0;

   TRACE( "->open queues\n" );

   data->in_queue = malloc( sizeof( struct _unicap_queue ) );
   if( !data->in_queue )
   {
      TRACE( "->open NO_MEM\n" );
      return STATUS_NO_MEM;
   }
   _init_queue( data->in_queue );
   TRACE( "->open out_queue\n" );
   data->out_queue = malloc( sizeof( struct _unicap_queue ) );
   if( !data->out_queue )
   {
      TRACE( "->open NO_MEM\n" );
      return STATUS_NO_MEM;
   }
   _init_queue( data->out_queue );

   data->frame_rate = 30;

   TRACE( "->open\n" );

   return STATUS_SUCCESS;
}

int cpi_close( void *cpi_data )
{
   null_data_t *data = cpi_data;

   TRACE( "close thing\n" );
   g_instance_count--;
   free( data );
	
   return STATUS_SUCCESS;
}



int cpi_enumerate_devices( unicap_device_t *device, int index )
{
   TRACE( "thing.cpi: enumerate devices\n" );
   if( !device )
   {
      return STATUS_INVALID_PARAMETER;
   }

   TRACE( "index: %d\n", index );
	
   if( index >= 0 && index < 3 )
   {
      sprintf( device->identifier, "Thing capture %d", index + 1 );
      strcpy( device->model_name, "thing" );
      strcpy( device->vendor_name, "Genuine unicap");
   }
   else
   {
      return STATUS_NO_DEVICE;
   }
	
   device->model_id = ( index + 1 ) + ( 0x80 << 16 );
   device->vendor_id = 0xffff0000;
   device->flags = UNICAP_CPI_SERIALIZED;
	
   strcpy( device->device, "" ) ; // no controlling device

   return STATUS_SUCCESS;
}

unicap_status_t cpi_reenumerate_formats( void *cpi_data, int *count )
{
   int nr_formats = sizeof( null_formats ) / sizeof( unicap_format_t );

   TRACE( "thing.cpi: reenumerate formats\n" );
   *count = nr_formats;
   return STATUS_SUCCESS;
}

int cpi_enumerate_formats( void *cpi_data, unicap_format_t *format, int index )
{
   int nr_formats = sizeof( null_formats ) / sizeof( unicap_format_t );
   null_data_t *data = cpi_data;

   TRACE( "thing.cpi: enumerate formats\n" );
	
   if( !data || !format )
   {
      return STATUS_INVALID_PARAMETER;
   }
	
   if( ( index >= 0 ) && ( index < nr_formats ) )
   {
      memcpy( format, &null_formats[index], sizeof( unicap_format_t ) );
   }
   else
   {
      return STATUS_NO_MATCH;
   }
	
   return STATUS_SUCCESS;
}

int cpi_set_format( void *cpi_data, unicap_format_t *format )
{
   int format_index = 0;
   int nr_formats = sizeof( null_formats ) / sizeof( unicap_format_t );
   null_data_t *data = cpi_data;
   int i;

   TRACE( "thing.cpi: set_format\n " );
   // search for a matching name in the formats list
   while( ( format_index < nr_formats ) && ( strcmp( format->identifier, null_formats[format_index].identifier ) ) )
   {
      format_index++;
   }
	
   // check if actually found a match or the end of the list was reached
   if( format_index == nr_formats )
   {
      return STATUS_NO_MATCH;
   }
	
   // remember the format
   data->format = format_index;
   format->buffer_size = format->size.width * format->size.height * ( format->bpp / 8 );
   memcpy( &data->current_format, &null_formats[format_index], sizeof( unicap_format_t ) );

   data->current_format.size.width = format->size.width;
   data->current_format.size.height = format->size.height;

   for( i = 0; i < THING_SYSTEM_BUFFERS; i++ )
   {
      if( data->system_buffers[i] )
      {
	 free( data->system_buffers[i] );
      }
      data->system_buffers[i] = malloc( format->buffer_size );
   }
	
   return STATUS_SUCCESS;
}

unicap_status_t cpi_get_format( void *cpi_data, unicap_format_t *format )
{
   null_data_t *data = cpi_data;

   TRACE( "thing.cpi: get_format\n" );

   if( data->format == -1 )
   {
      return STATUS_NO_FORMAT;
   }
	
/* 	memcpy( format, &null_formats[data->format], sizeof( struct _unicap_format ) ); */
   memcpy( format, &data->current_format, sizeof( unicap_format_t ) );
	
   return STATUS_SUCCESS;
}

unicap_status_t cpi_reenumerate_properties( void *data, int *count )
{
   int nr_properties = sizeof( null_properties ) / sizeof( struct _unicap_property );
	
   TRACE( "reenumerate properties\n" );
	
   *count = nr_properties;
   return STATUS_SUCCESS;
}
unicap_status_t cpi_enumerate_properties( void *cpi_data, unicap_property_t *property, int index )
{
   int nr_properties = sizeof( null_properties ) / sizeof( struct _unicap_property );
   null_data_t *data = cpi_data;
	
   TRACE( "thing.cpi: enumerate properties %d\n", index );

   if( !data || !property )
   {
      return STATUS_INVALID_PARAMETER;
   }

   if( ( index < 0 ) || ( index >= nr_properties ) )
   {
      return STATUS_NO_MATCH;
   }
	
   memcpy( property, &data->current_properties[index], sizeof( unicap_property_t) );

   return STATUS_SUCCESS;
}

unicap_status_t cpi_set_property( void *cpi_data, unicap_property_t *property )
{
   null_data_t *data = cpi_data;
   int property_index = 0;
   int nr_properties = sizeof( null_properties ) / sizeof( unicap_property_t );
	
   TRACE( "thing.cpi: set_property\n" );
	
   if( !data || !property )
   {
      return STATUS_INVALID_PARAMETER;
   }
	
   while( ( property_index < nr_properties ) && ( strcmp( property->identifier, null_properties[property_index].identifier ) ) )
   {
      property_index++;
   }

   if( property_index == nr_properties )
   {
      return STATUS_NO_MATCH;
   }
	
   if( !strcmp( property->identifier, "frame rate" ) )
   {
      data->frame_rate = property->value;
   }
   else
   {
      int relations_count = data->current_properties[property_index].relations_count;
      char **relations = data->current_properties[property_index].relations;
      memcpy( &data->current_properties[property_index], property, sizeof( unicap_property_t ) );
      data->current_properties[property_index].relations_count = relations_count;
      data->current_properties[property_index].relations = relations;
   }

   return STATUS_SUCCESS;
}

unicap_status_t cpi_get_property( void *cpi_data, unicap_property_t *property )
{
   null_data_t *data = cpi_data;
   int property_index = 0;
   int nr_properties = sizeof( null_properties ) / sizeof( unicap_property_t );
	
   if( !data || !property )
   {
      return STATUS_INVALID_PARAMETER;
   }
	
   while( ( property_index < nr_properties ) && ( strcmp( property->identifier, null_properties[property_index].identifier ) ) )
   {
      property_index++;
   }

   if( property_index == nr_properties )
   {
      return STATUS_NO_MATCH;
   }
	
   memcpy( property, &data->current_properties[property_index], sizeof( unicap_property_t ) );

   return STATUS_SUCCESS;
}

unicap_status_t cpi_capture_start( void *cpi_data )
{
   null_data_t *data = cpi_data;

   int old_state = data->capture_running;
	
   data->capture_running = 1;

   gettimeofday( &data->last_capture_time, 0 );
	
   return old_state ? STATUS_CAPTURE_ALREADY_STARTED : STATUS_SUCCESS;
}

unicap_status_t cpi_capture_stop( void *cpi_data )
{
   null_data_t *data = cpi_data;

   int old_state = data->capture_running;
	
   data->capture_running = 0;
	
   return old_state ? STATUS_CAPTURE_ALREADY_STOPPED : STATUS_SUCCESS;
}

unicap_status_t cpi_queue_buffer( void *cpi_data, unicap_data_buffer_t *buffer )
{
   null_data_t *data = cpi_data;
   struct _unicap_queue *queue = malloc( sizeof( struct _unicap_queue ) );
   TRACE( "Queue buffer\n" );
	
   if( !( data->current_format.buffer_types & (1<<buffer->type) ) )
   {
      TRACE( "Current format does not support this buffer type\n" );
      return STATUS_INVALID_PARAMETER;
   }

   if( !( buffer->type == UNICAP_BUFFER_TYPE_SYSTEM ) &&
       buffer->buffer_size < ( data->current_format.size.width * 
			       data->current_format.size.height * 
			       data->current_format.bpp / 8 ) )
   {
      TRACE( "Wrong parameters for buffer: %dx%dx%d => %d\n", 
	     data->current_format.size.width, 
	     data->current_format.size.height, 
	     data->current_format.bpp, 
	     buffer->buffer_size );
      return STATUS_BUFFER_TOO_SMALL;
   }

   if( buffer->type == UNICAP_BUFFER_TYPE_SYSTEM )
   {
      buffer->data = data->system_buffers[data->current_system_buffer];
      data->current_system_buffer = ( data->current_system_buffer + 1 ) % THING_SYSTEM_BUFFERS;
   }
   queue->data = buffer;
   TRACE( "buffer = %p, size = %d \n", buffer, buffer->buffer_size );
	
   _insert_back_queue( data->in_queue, queue );

   return STATUS_SUCCESS;
}

unicap_status_t cpi_dequeue_buffer( void *cpi_data, unicap_data_buffer_t **buffer )
{
   null_data_t *data = cpi_data;
   struct _unicap_queue *entry;

   TRACE( "Dequeue buffer\n" );

   if( data->capture_running )
   {
      return STATUS_IS_RECEIVING;
   }
	
   entry = _get_front_queue( data->in_queue );
	
   if( !entry )
   {
      return STATUS_NO_BUFFERS;
   }
	
   *buffer = entry->data;
	
   free( entry );
	
   return STATUS_SUCCESS;
}


static void fill_buffer( char *buffer, int x, int y, int bpp )
{
   memset( buffer, 0xff, (bpp/8)*x*y );
   switch( bpp )
   {
      case 8:
      {
	 static char fill = 0x0;
	 fill++;
	 memset( buffer, fill, x*y );
      }
      break;
      case 16:
      {
	 static short fill = 0x0000;
	 int i,j;
	 unsigned short *sbuffer = ( unsigned short* )buffer;
/* 			TRACE( "fill: %04x\n", fill ); */

	 fill = ( fill + 0xf ) % 0xff;
	 for( i = (x/3); i < ((x*2)/3); i++ )
	 {
	    for( j = (y/3); j < ((y*2)/3); j++ )
	    {
	       sbuffer[ (j*x)+i ] = fill;
	    }
	 }
      }
      break;

      case 32:
      {
	 static long fill = 0x00000000;
	 int i, j;

	 unsigned long *lbuffer = ( unsigned long* )buffer;
			
	 fill = ( fill + 0x0a0a0aff );
	 TRACE( "fill = %08x\n", fill );
	 for( i = (x/3 ); i < ( ( x * 2 ) / 3 ); i++ )
	 {
	    for( j = ( y/3 ); j < ( ( y*2 ) / 3 ); j++ )
	    {
	       lbuffer[ (j*x)+i ] = fill;
	    }
	 }
      }
      break;
		
      default: 
      {
      }
      break;
   }
}


unicap_status_t cpi_wait_buffer( void *cpi_data, unicap_data_buffer_t **buffer )
{
   null_data_t *data = cpi_data;
   static char fillbyte = 0;
	

   if( data->in_queue->next )
   {
      struct _unicap_queue *entry;
      int offset = 0;
      int x; 
      int y;
      int width = null_formats[data->format].size.width;
      struct timeval ctime;
      long usec;
		
      gettimeofday( &ctime, 0 );
      ctime.tv_sec -= data->last_capture_time.tv_sec;
      if( ctime.tv_usec < data->last_capture_time.tv_usec )
      {
	 ctime.tv_sec--;
	 ctime.tv_usec += 1000000;
      }
      ctime.tv_usec -= data->last_capture_time.tv_usec;
      usec = ctime.tv_usec + ( ctime.tv_sec * 1000000 );
      gettimeofday( &data->last_capture_time, 0 );
      if( usec < (1000000/data->frame_rate) )
      {
	 usleep( usec );
      }

      _move_to_queue( data->in_queue, data->out_queue );
      entry = _get_front_queue( data->out_queue );
      *buffer = entry->data;
      memcpy( &(*buffer)->format, &data->current_format, sizeof( unicap_format_t ) );			
		
      fill_buffer( (*buffer)->data,
		   (*buffer)->format.size.width,
		   (*buffer)->format.size.height,
		   (*buffer)->format.bpp );
		
      free( entry );

      return STATUS_SUCCESS;
   }
	
   return STATUS_FAILURE;
}

unicap_status_t cpi_poll_buffer( void *cpi_data, int *count )
{
   null_data_t *data = cpi_data;	

   *count = _queue_get_size( data->in_queue );
	
   return STATUS_SUCCESS;
}
