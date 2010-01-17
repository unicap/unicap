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

#include <config.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <pthread.h>

#include "unicap.h"
#include "unicap_status.h"
#include "unicap_cpi.h"

#include "unicap_cpi_std_struct.h"

#include <queue.h>
#include "vid21394_cpi.h"
#include "vid21394.h"
#include "vid21394_base.h"

#include "visca.h"

#if VID21394_DEBUG
#define DEBUG
#endif
#include "debug.h"
 
#include <1394util.h>

enum property_enum{
   PROP_BRIGHTNESS = 0,
   PROPERTY_COUNT
};


#define EXTRA_INPUTS

#define VIDEO_SOURCE_COMPOSITE_1 "Composite 1"
#define VIDEO_SOURCE_COMPOSITE_2 "Composite 2"
#define VIDEO_SOURCE_COMPOSITE_3 "Composite 3"
#define VIDEO_SOURCE_COMPOSITE_4 "Composite 4"
#define VIDEO_SOURCE_SVHS        "SVHS"
#define VIDEO_SOURCE_AUTO        "Auto"
#define VIDEO_SOURCE_NONE        "None"

#define video_source_default_item "Composite 1"

char *video_sources[] = 
{
   VIDEO_SOURCE_COMPOSITE_1,
   VIDEO_SOURCE_COMPOSITE_2,
#ifdef EXTRA_INPUTS
   VIDEO_SOURCE_COMPOSITE_3, 
   VIDEO_SOURCE_COMPOSITE_4, 
#endif
   VIDEO_SOURCE_SVHS,
   VIDEO_SOURCE_AUTO,
   VIDEO_SOURCE_NONE
};

#define video_norm_default_item "NTSC"

char *video_norm_menu_items[] = 
{
   "PAL", 
   "NTSC"
};


unicap_rect_t vid21394_pal_video_sizes[] =
{
   {0, 0, 320,240}, {0, 0, 640, 480}, {0, 0, 768, 576}
};

unicap_rect_t vid21394_ntsc_video_sizes[] =
{
   {0, 0, 320,240}, {0, 0, 640, 480}
};


unicap_format_t vid21394_formats[] =
{
   // UYVY
   { "UYVY", // name
     {0, 0, 768,576},{0, 0, 320,240},{0, 0, 768,576},
     -1,-1,
     vid21394_pal_video_sizes,
     3,
     16, // width, height, bpp
     FOURCC('U','Y','V','Y'), // fourcc
     0, // flags
     0,
     320*240*2}, // buffer size

   // YUY2
   { "YUY2", // name
     {0, 0, 768,576},{0, 0, 320,240},{0, 0, 768,576},
     -1,-1,
     vid21394_pal_video_sizes,
     3,
     16, // width, height, bpp
     FOURCC('Y','U','Y','2'), // fourcc
     0, // flags
     0,
     320*240*2}, // buffer size

   // Y411
   { "Y411",
     {0, 0, 768,576},{0, 0, 320,240},{0, 0, 768,576},
     -1,-1,
     vid21394_pal_video_sizes,
     3,
     12,
     FOURCC('Y','4','1','1'), 
     0, 
     0,
     320*240*2 + ( 320*240 >> 1 ) },

   // Y8
   { "Grey",
     {0, 0, 768,576},{0, 0, 320,240},{0, 0, 768,576},
     -1,-1,
     vid21394_pal_video_sizes,
     3,
     8,
     FOURCC('Y','8','0','0'), 
     0, 
     0,
     320*240 },

};

unicap_property_t vid21394_properties[] =
{
   { "brightness", S_PPTY_INFO_VIDEO, // name, class, unit
     {0.5}, // default value
     {{ 0.0, 1.0 }}, // range
     0.01, // stepping
     UNICAP_PROPERTY_TYPE_RANGE,
     UNICAP_FLAGS_MANUAL, 
     UNICAP_FLAGS_MANUAL,
     0, // property data
     0, // property data size
   },
   { "contrast", S_PPTY_INFO_VIDEO, // name, class, unit
     {0.5}, // default value
     {{ 0.0, 1.0 }}, // range
     0.01, // stepping
     UNICAP_PROPERTY_TYPE_RANGE,
     UNICAP_FLAGS_MANUAL, 
     UNICAP_FLAGS_MANUAL,
     0, // property data
     0, // property data size
   },
   { identifier: "force odd/even", 
     category: "device", 
     unit: "", 
     relations: NULL, 
     relations_count: 0, 
     { value: 0.0}, // default value
     { range: { min: 0.0, max: 1.0 }}, // range
     stepping: 1.0, // stepping
     type: UNICAP_PROPERTY_TYPE_RANGE,
     flags: UNICAP_FLAGS_MANUAL, 
     flags_mask: UNICAP_FLAGS_MANUAL,
     property_data: 0, // property data
     property_data_size: 0, // property data size
   },
   { identifier: "source", 
     category: "video", 
     unit: "", 
     relations: NULL, 
     relations_count: 0, 
     { menu_item: { video_source_default_item } }, // default value
     { menu: { menu_items: video_sources,
	       menu_item_count: sizeof( video_sources ) / sizeof( char * ) }}, // range
     0.01, // stepping
     UNICAP_PROPERTY_TYPE_MENU,
     UNICAP_FLAGS_MANUAL, 
     UNICAP_FLAGS_MANUAL,
     0, // property data
     0, // property data size
   },
   { identifier: "video norm",
     category: "video", 
     unit: "", 
     relations: NULL, 
     relations_count: 0, 
     { menu_item: { video_norm_default_item } }, 
     { menu: { menu_items: video_norm_menu_items, 
	       menu_item_count: sizeof( video_norm_menu_items ) / sizeof( char * ) }, 
     }, 
     stepping: 0, 
     type: UNICAP_PROPERTY_TYPE_MENU, 
     flags: UNICAP_FLAGS_MANUAL, 
     flags_mask: UNICAP_FLAGS_MANUAL, 
     property_data: 0, 
     property_data_size: 0,
   },
   { identifier: "rs232 io", 
     category: "device",
     unit: "", 
     relations: NULL, 
     relations_count: 0, 
     { value: 0 }, // default value
     { range: { min: 0, max: 0 } }, // range
     stepping: 0, // stepping
     type: UNICAP_PROPERTY_TYPE_DATA,
     flags: 0, 
     flags_mask: UNICAP_FLAGS_MANUAL,
     property_data: 0,
     property_data_size: 0,
   },
   { identifier: "rs232 baud rate", 
     category: "device", 
     unit: "", 
     relations: NULL, 
     relations_count: 0, 
     { value: 0 }, // default value
     { range: { min: 0, max: 4 } }, // range
     stepping: 1.0, // stepping
     type: UNICAP_PROPERTY_TYPE_RANGE,
     flags: 0, 
     flags_mask: UNICAP_FLAGS_MANUAL,
     property_data: 0, // property data
     property_data_size: 0, // property data size
   },
   { identifier: "link speed", 
     category: "device", 
     unit: "",
     relations: NULL, 
     relations_count: 0, 
     { value: 0.0},
     { range: { min: 0.0, max: 2.0}}, 
     stepping: 1.0, 
     type: UNICAP_PROPERTY_TYPE_RANGE, 
     flags: 0, 
     flags_mask: UNICAP_FLAGS_MANUAL,
     property_data: 0, 
     property_data_size: 0, 
   },
   { identifier: "firmware version",
     category: "device", 
     unit: "", 
     relations: NULL, 
     relations_count: 0, 
     { value: 0.0 }, // default value
     { range: { min: 0.0, max: 99999 }}, // range
     stepping: 1.0, // stepping
     type: UNICAP_PROPERTY_TYPE_RANGE,
     flags: 0, 
     flags_mask: UNICAP_FLAGS_MANUAL,
     property_data: 0, // property data
     property_data_size: 0, // property data size
   },
	
#if VID21394_BOOTLOAD
   { identifier: "enter bootload", 
     category: "device", 
     unit: "", 
     relations: NULL, 
     relations_count: 0, 
     { value: 0.0 }, // default value
     { range: { min: 0.0, max: 1.0 }}, // range
     stepping: 1.0, // stepping
     type: UNICAP_PROPERTY_TYPE_RANGE,
     flags: 0, 
     flags_mask: UNICAP_FLAGS_MANUAL,
     property_data: 0, // property data
     property_data_size: 0, // property data size
   },
#endif
};

struct _vid21394_data_struct
{
      int instance;
      int device;
      int format;
		
      int capture_running;
	
      unicap_property_t *current_properties;
      unicap_format_t current_formats[4];
		
		
      vid21394handle_t vid21394handle;
		
      unicap_queue_t *in_queue;
      int in_queue_lock;
      unicap_queue_t *out_queue;
      int out_queue_lock;
		
      char rs232_buffer[1024];
      int enable_visca_support;

};

typedef struct _vid21394_data_struct vid21394_data_t;

int g_instance_count = 0;


static 
unicap_status_t vid21394cpi_set_event_notify( void *cpi_data, unicap_event_callback_t func, unicap_handle_t unicap_handle );

static struct _unicap_cpi cpi_s =
{
   cpi_version: 1<<16,
   cpi_capabilities: 0x3ffff,

   cpi_enumerate_devices: cpi_enumerate_devices,
   cpi_open: cpi_open, 
   cpi_close: cpi_close,

   cpi_reenumerate_formats: cpi_reenumerate_formats, 
   cpi_enumerate_formats: cpi_enumerate_formats,   
   cpi_set_format: cpi_set_format,	   
   cpi_get_format: cpi_get_format,          

   cpi_reenumerate_properties: cpi_reenumerate_properties,
   cpi_enumerate_properties: cpi_enumerate_properties,
   cpi_set_property: cpi_set_property,
   cpi_get_property: cpi_get_property,

   cpi_capture_start: cpi_capture_start,
   cpi_capture_stop: cpi_capture_stop,
   
   cpi_queue_buffer: cpi_queue_buffer,
   cpi_dequeue_buffer: cpi_dequeue_buffer,
   cpi_wait_buffer: cpi_wait_buffer,
   cpi_poll_buffer: cpi_poll_buffer,
   
   cpi_set_event_notify: vid21394cpi_set_event_notify,
};

#if ENABLE_STATIC_CPI
void unicap_vid21394_register_static_cpi( struct _unicap_cpi **cpi )
{
   *cpi = &cpi_s;
}
#else
unicap_status_t cpi_register( struct _unicap_cpi *reg_data )
{
   memcpy( reg_data, &cpi_s, sizeof( struct _unicap_cpi ) );
/*    reg_data->cpi_version = 1<<16; */
/*    reg_data->cpi_capabilities = 0x3ffff; */

/*    reg_data->cpi_enumerate_devices = cpi_enumerate_devices; */
/*    reg_data->cpi_open = cpi_open;  */
/*    reg_data->cpi_close = cpi_close; */

/*    reg_data->cpi_reenumerate_formats = cpi_reenumerate_formats;  */
/*    reg_data->cpi_enumerate_formats = cpi_enumerate_formats;    */
/*    reg_data->cpi_set_format = cpi_set_format;	    */
/*    reg_data->cpi_get_format = cpi_get_format;           */

/*    reg_data->cpi_reenumerate_properties = cpi_reenumerate_properties; */
/*    reg_data->cpi_enumerate_properties = cpi_enumerate_properties; */
/*    reg_data->cpi_set_property = cpi_set_property; */
/*    reg_data->cpi_get_property = cpi_get_property; */

/*    reg_data->cpi_capture_start = cpi_capture_start; */
/*    reg_data->cpi_capture_stop = cpi_capture_stop; */
	
/*    reg_data->cpi_queue_buffer = cpi_queue_buffer; */
/*    reg_data->cpi_dequeue_buffer = cpi_dequeue_buffer; */
/*    reg_data->cpi_wait_buffer = cpi_wait_buffer; */
/*    reg_data->cpi_poll_buffer = cpi_poll_buffer; */
	
/*    reg_data->cpi_set_event_notify = vid21394cpi_set_event_notify; */

   return STATUS_SUCCESS;
}
#endif//ENABLE_STATIC_CPI
int cpi_open( void **cpi_data, unicap_device_t *device )
{
   vid21394_data_t *data;
   unsigned long long sernum = 0;
   raw1394handle_t raw1394handle;
   int i, found = 0;
   int port, num_ports = 0;

   *cpi_data = malloc( sizeof( struct _vid21394_data_struct ) );
   if( !*cpi_data )
   {
      return STATUS_NO_MEM;
   }


   data = (vid21394_data_t *)*cpi_data;

   memset( data, 0x0, sizeof( struct _vid21394_data_struct ) );

   data->current_properties = malloc( sizeof( vid21394_properties ) );
   if( !data->current_properties )
   {
      free( *cpi_data );
      return STATUS_NO_MEM;
   }
	
   for( i = 0; i < ( sizeof( vid21394_properties ) / sizeof( unicap_property_t ) ); i++ )
   {
      unicap_copy_property( &data->current_properties[i], &vid21394_properties[i] );
   }


   raw1394handle = raw1394_new_handle();
   if( !raw1394handle )
   {
      TRACE( "failed to get raw1394 handle\n" );
      return STATUS_NO_DEVICE;
   }
   num_ports = raw1394_get_port_info( raw1394handle, NULL, 0 );
	
   raw1394_destroy_handle( raw1394handle );

   for( port = 0; ( port < num_ports ) && !found; port++ )
   {
      raw1394handle = raw1394_new_handle_on_port( port );
		
      for( i = 0; i < raw1394_get_nodecount( raw1394handle ); i++ )
      {
	 if( ( get_unit_spec_ID( raw1394handle, i ) == 0x748 ) &&
	     ( ( get_unit_sw_version( raw1394handle, i ) == 0x526f6e ) ||
	       ( get_unit_sw_version( raw1394handle, i ) == 0x526f6f ) ) )
	 {
	    char identifier[128];
				
	    sprintf( identifier, "DFG/1394-1 %llx", get_guid( raw1394handle, i ) & 0xffffffff );
	    if( !strcmp( identifier, device->identifier ) )
	    {
	       sernum = get_guid( raw1394handle, i );
	       found = 1;
	       break;
	    }
	 }
      }	
      raw1394_destroy_handle( raw1394handle );
   }
	

   if( ( data->vid21394handle = vid21394_open( sernum ) ) == VID21394_INVALID_HANDLE_VALUE )
   {
      free( data );
      TRACE( "failed to open vid21294\n" );
		
      return STATUS_FAILURE;
   }
	
   g_instance_count++;

   data->instance = g_instance_count;
   data->device = 0;
   data->format = -1;
   data->capture_running = 0;

   data->in_queue = ucutil_queue_new();
   data->out_queue = ucutil_queue_new();

   cpi_reenumerate_formats( data, &i );

   if( VID21394_DETECT_21CF04 && ( data->vid21394handle->firmware_version >= 0x303 ) )
   {
      visca_camera_type_t cam_type;

      if( SUCCESS( visca_check_camera( data->vid21394handle, &cam_type ) ) )
      {
	 if( cam_type == VISCA_CAMERA_TYPE_FCB_IX47 )
	 {
	    TRACE( "FOUND FCB camera\n" );
	    data->enable_visca_support = 1;
	 }
      }
   }
			
#if VID21394_VISCA
   data->enable_visca_support = 1;
#endif

   return STATUS_SUCCESS;
}

int cpi_close( void *cpi_data )
{
   vid21394_data_t *data = cpi_data;

   vid21394_close( data->vid21394handle );


   ucutil_destroy_queue( data->in_queue );
   ucutil_destroy_queue( data->out_queue );	
	
   if( data->vid21394handle->unicap_handle )
   {
      free( data->vid21394handle->unicap_handle );
   }

   g_instance_count--;
   free( data );
	
   return STATUS_SUCCESS;
}



int cpi_enumerate_devices( unicap_device_t *device, int index )
{
   int node, port, num_ports = 0;
   int tmp_index = -1;
   int found_node = -1, found_port = -1;
	
   raw1394handle_t raw1394handle;

   unicap_status_t status = STATUS_SUCCESS;
	

   if( !device )
   {
      return STATUS_INVALID_PARAMETER;
   }

   raw1394handle = raw1394_new_handle();
   if( !raw1394handle )
   {
      TRACE( "failed to get raw1394 handle\n" );
      return STATUS_NO_DEVICE;
   }
   num_ports = raw1394_get_port_info( raw1394handle, NULL, 0 );

   raw1394_destroy_handle( raw1394handle );

   for( port = 0; ( port < num_ports ) && tmp_index != index; port++ )
   {
      int nodes;
      raw1394handle = raw1394_new_handle_on_port( port );

      nodes = raw1394_get_nodecount( raw1394handle );

      TRACE( "port: %d, nodes: %d, tmp_index: %d index: %d\n", port, nodes, tmp_index, index );
		
      for( node = 0; ( tmp_index != index ) && ( node < nodes ); node++ )
      {
	 if( ( get_unit_spec_ID( raw1394handle, node ) == 0x748 ) &&
	     ( ( get_unit_sw_version( raw1394handle, node ) == 0x526f6e ) ||
	       ( get_unit_sw_version( raw1394handle, node ) == 0x526f6f ) ) )
	 {
	    TRACE( "DFG1394 found on port: %d node: %d\n", port, node );
	    tmp_index++;
	    if( tmp_index == index )
	    {
	       found_node = node;
	       found_port = port;
	    }
	 }
      }	
      raw1394_destroy_handle( raw1394handle );
   }
	
   if( found_node != -1 )
   {
      raw1394handle = raw1394_new_handle_on_port( found_port );
      device->model_id = get_guid( raw1394handle, found_node );
      TRACE( "get guid port: %d node: %d\n", found_port, found_node );
      sprintf( device->identifier, "DFG/1394-1 %llx", device->model_id & 0xffffffff );
      strcpy( device->model_name, "DFG/1394-1" );
      strcpy( device->vendor_name, "unicap");
      device->vendor_id = 0xffff0000;
      device->flags = UNICAP_CPI_SERIALIZED;
		
      strcpy( device->device, "/dev/raw1394" );
      status = STATUS_SUCCESS;
      raw1394_destroy_handle( raw1394handle );		
   }
   else
   {
      status = STATUS_NO_DEVICE;
   }

	
   return status;
}

unicap_status_t cpi_reenumerate_formats( void *cpi_data, int *count )
{
   int nr_formats = sizeof( vid21394_formats ) / sizeof( unicap_format_t );
   vid21394_data_t *data = cpi_data;

   enum vid21394_frequency freq;
	
   vid21394_get_frequency( data->vid21394handle, &freq );

   if( freq == VID21394_FREQ_50 )
   {
      int i;
		
      for( i = 0; i < nr_formats; i++ )
      {
	 vid21394_formats[i].size.width = 768;
	 vid21394_formats[i].max_size.width = 768;
	 vid21394_formats[i].size.height = 576;
	 vid21394_formats[i].max_size.height = 576;
	 vid21394_formats[i].sizes = vid21394_pal_video_sizes;
	 vid21394_formats[i].size_count = sizeof( vid21394_pal_video_sizes ) / sizeof( unicap_rect_t );
      }
   }
   else
   {
      int i;
		
      for( i = 0; i < nr_formats; i++ )
      {
	 vid21394_formats[i].size.width = 640;
	 vid21394_formats[i].max_size.width = 640;
	 vid21394_formats[i].size.height = 480;
	 vid21394_formats[i].max_size.height = 480;
	 vid21394_formats[i].sizes = vid21394_ntsc_video_sizes;
	 vid21394_formats[i].size_count = sizeof( vid21394_ntsc_video_sizes ) / sizeof( unicap_rect_t );
      }
   }

   *count = nr_formats;
	
   memcpy( data->current_formats, vid21394_formats, sizeof( unicap_format_t ) * nr_formats );
	
   return STATUS_SUCCESS;
}

int cpi_enumerate_formats( void *cpi_data, unicap_format_t *format, int index )
{
   int nr_formats = sizeof( vid21394_formats ) / sizeof( unicap_format_t );
   vid21394_data_t *data = cpi_data;

   if( !data || !format )
   {
      return STATUS_INVALID_PARAMETER;
   }
	
   if( !data->current_formats )
   {
      int tmp;
      cpi_reenumerate_formats( cpi_data, &tmp );
   }

   if( ( index >= 0 ) && ( index < nr_formats ) )
   {
      memcpy( format, &data->current_formats[index], sizeof( unicap_format_t ) );
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
   int nr_formats = sizeof( vid21394_formats ) / sizeof( unicap_format_t );
   vid21394_data_t *data = cpi_data;
   vid21394handle_t vid21394handle = data->vid21394handle;
   enum vid21394_video_mode mode;
   unicap_status_t status;
	
   TRACE( "cpi_set_format\n" );

   if( !data->current_formats )
   {
      int tmp;
      cpi_reenumerate_formats( cpi_data, &tmp );
   }

   // search for a matching name in the formats list
   while( ( format_index < nr_formats ) && ( strcmp( format->identifier, vid21394_formats[format_index].identifier ) ) )
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


   if( vid21394handle->system_buffer )
   {
      free( vid21394handle->system_buffer );
      vid21394handle->system_buffer = NULL;
   }
	
   switch( format->fourcc )
   {
      case FOURCC( 'U', 'Y', 'V', 'Y' ):
	 if( format->size.width == 320 )
	 {
	    mode = VID21394_UYVY_320x240;
	    break;
	 }
	 else if( format->size.width == 640 )
	 {
	    mode = VID21394_UYVY_640x480;
	    break;
	 }
	 else if( format->size.width == 768 )
	 {
	    mode = VID21394_UYVY_768x576;
	    break;
	 } else {
	    TRACE( "invalid mode\n" );
	    return STATUS_FAILURE;
	 }
	 break;

      case FOURCC( 'Y', 'U', 'Y', '2' ):
	 if( format->size.width == 320 )
	 {
	    mode = VID21394_YUY2_320x240;
	    break;
	 }
	 else if( format->size.width == 640 )
	 {
	    mode = VID21394_YUY2_640x480;
	    break;
	 }
	 else if( format->size.width == 768 )
	 {
	    mode = VID21394_YUY2_768x576;
	    break;
	 } else {
	    TRACE( "invalid mode\n" );
	    return STATUS_FAILURE;
	 }
	 break;
		
      case FOURCC( 'Y', '4', '1', '1' ):
	 if( format->size.width == 320 )
	 {
	    mode = VID21394_Y41P_320x240;
	    break;
	 }
	 else if( format->size.width == 640 )
	 {
	    TRACE( "Y41P 640x480\n" );
	    mode = VID21394_Y41P_640x480;
	    break;
	 }
	 else if( format->size.width == 768 )
	 {
	    mode = VID21394_Y41P_768x576;
	    break;
	 } else {
	    TRACE( "invalid mode\n" );
	    return STATUS_FAILURE;
	 }
	 break;

      case FOURCC( 'Y', '8', '0', '0' ):
	 if( format->size.width == 320 )
	 {
	    mode = VID21394_Y_320x240;
	    break;
	 }
	 else if( format->size.width == 640 )
	 {
	    mode = VID21394_Y_640x480;
	    break;
	 }
	 else if( format->size.width == 768 )
	 {
	    mode = VID21394_Y_768x576;
	    break;
	 } else {
	    TRACE( "invalid mode\n" );
	    return STATUS_FAILURE;
	 }
	 break;

      default:
	 TRACE( "invalid mode\n" );
	 return STATUS_FAILURE;
	 break;
   }
	
   if( data->capture_running )
   {
      status = cpi_capture_stop( cpi_data );
      if( SUCCESS( status ) )
      {
	 status = vid21394_set_video_mode( data->vid21394handle, mode );
	 if( SUCCESS( status ) )
	 {
	    status = cpi_capture_start( cpi_data );
	 }
      }
   }
   else
   {
      status = vid21394_set_video_mode( data->vid21394handle, mode );
   }

   format->buffer_size = format->size.width * format->size.height * format->bpp / 8;

   data->current_formats[ format_index ].size.width = format->size.width;
   data->current_formats[ format_index ].size.height = format->size.height;

   unicap_copy_format( &vid21394handle->current_format, format );
   if( vid21394handle->system_buffer )
   {
      free( vid21394handle->system_buffer );
   }

   if( format->buffer_type == UNICAP_BUFFER_TYPE_SYSTEM )
   {
      vid21394handle->system_buffer = malloc( format->size.width * format->size.height * format->bpp / 8 );
      vid21394handle->system_buffer_entry.data = vid21394handle->system_buffer;
   }

   return STATUS_SUCCESS;
}

unicap_status_t cpi_get_format( void *cpi_data, unicap_format_t *format )
{
   vid21394_data_t *data = cpi_data;

   if( !data->current_formats )
   {
      int tmp;
      cpi_reenumerate_formats( cpi_data, &tmp );
   }

   if( data->format == -1 )
   {
      return STATUS_NO_FORMAT;
   }
	
   unicap_copy_format( format, &data->vid21394handle->current_format );
   format->buffer_size = ( format->size.width * format->size.height * format->bpp ) / 8;

   return STATUS_SUCCESS;
}

unicap_status_t cpi_reenumerate_properties( void *cpi_data, int *count )
{
   int nr_properties = sizeof( vid21394_properties ) / sizeof( unicap_property_t );
   vid21394_data_t *data = cpi_data;
   *count = nr_properties;
   if( data->enable_visca_support )
   {
      int visca_properties;
      if( SUCCESS( visca_reenumerate_properties( data->vid21394handle, &visca_properties ) ) )
      {
	 *count += visca_properties;
      }
   }
   return STATUS_SUCCESS;
}

unicap_status_t cpi_enumerate_properties( void *cpi_data, unicap_property_t *property, int index )
{
   vid21394_data_t *data = cpi_data;
   int nr_properties = sizeof( vid21394_properties ) / sizeof( unicap_property_t );
   unicap_status_t status = STATUS_SUCCESS;
	
   if( !data || !property )
   {
      return STATUS_INVALID_PARAMETER;
   }

   if( index < 0 )
   {
      return STATUS_NO_MATCH;
   }

   if( index >= nr_properties )
   {
      if( data->enable_visca_support )
      {
	 status = visca_enumerate_properties( property, index - nr_properties );
/* 			TRACE( "%s\n", property->identifier ); */
      }
      else
      {
	 status = STATUS_NO_MATCH;
      }
   }
   else
   {
      memcpy( property, &vid21394_properties[index], sizeof( unicap_property_t) );
   }

   return status;
}

unicap_status_t cpi_set_property( void *cpi_data, unicap_property_t *property )
{
   vid21394_data_t *data = cpi_data;
   int property_index = 0;
   int nr_properties = sizeof( vid21394_properties ) / sizeof( unicap_property_t );
   unicap_status_t status = STATUS_FAILURE;
	

   if( !data || !property )
   {
      TRACE( "invalid param\n" );
      return STATUS_INVALID_PARAMETER;
   }
	
   while( ( property_index < nr_properties ) && ( strcmp( property->identifier, vid21394_properties[property_index].identifier ) ) )
   {
      property_index++;
   }

   if( property_index == nr_properties )
   {
      if( data->enable_visca_support )
      {
	 status = visca_set_property( data->vid21394handle, property );
	 return status;
      }
      else
      {
	 TRACE( "no match, no VISCA\n" );
	 return STATUS_NO_MATCH;
      }
   }
	
   memcpy( &data->current_properties[property_index], property, sizeof( unicap_property_t ) );

   if( !strcmp( property->identifier, "brightness" ) )
   {
      status = vid21394_set_brightness( data->vid21394handle, property->value*255 );
   }
   else if( !strcmp( property->identifier, "contrast" ) )
   {
      status = vid21394_set_contrast( data->vid21394handle, property->value * 255 );
   }
   else if( !strcmp( property->identifier, "force odd/even" ) )
   {
      status = vid21394_set_force_odd_even( data->vid21394handle, property->value != 0 );
   }
   else if( !strcmp( property->identifier, "source" ) )
   {
      TRACE( "set source\n" );
      if( !strcmp( property->menu_item, VIDEO_SOURCE_COMPOSITE_1 ) )
      {
	 status = vid21394_set_input_channel( data->vid21394handle, 
					      VID21394_INPUT_COMPOSITE_1 );
      }
      else if( !strcmp( property->menu_item, VIDEO_SOURCE_COMPOSITE_2 ) )
      {
	 status = vid21394_set_input_channel( data->vid21394handle, 
					      VID21394_INPUT_COMPOSITE_2 );
      }
      else if( !strcmp( property->menu_item, VIDEO_SOURCE_COMPOSITE_3 ) )
      {
	 status = vid21394_set_input_channel( data->vid21394handle, 
					      VID21394_INPUT_COMPOSITE_3 );
      }
      else if( !strcmp( property->menu_item, VIDEO_SOURCE_COMPOSITE_4 ) )
      {
	 status = vid21394_set_input_channel( data->vid21394handle, 
					      VID21394_INPUT_COMPOSITE_4 );
      }
      else if( !strcmp( property->menu_item, VIDEO_SOURCE_SVHS ) )
      {
	 status = vid21394_set_input_channel( data->vid21394handle, 
					      VID21394_INPUT_SVIDEO );
      }
      else if( !strcmp( property->menu_item, VIDEO_SOURCE_AUTO ) )
      {
	 status = vid21394_set_input_channel( data->vid21394handle, 
					      VID21394_INPUT_AUTO );
      }
      else 
      {
	 status = STATUS_INVALID_PARAMETER;
      }
   }
   else if( !strcmp( property->identifier, "video norm" ) )
   {
      if( !strcmp( property->menu_item, video_norm_menu_items[0] ) )
      {
	 status = vid21394_set_frequency( data->vid21394handle, VID21394_FREQ_50 );
      }
      else if( !strcmp( property->menu_item, video_norm_menu_items[1] ) )
      {
	 status = vid21394_set_frequency( data->vid21394handle, VID21394_FREQ_60 );
      }
   }
	
#if VID21394_BOOTLOAD
   else if( !strcmp( property->identifier, "enter bootload" ) )
   {
      status = vid21394_enter_bootload( data->vid21394handle );
   }
#endif
#if EXPERIMENTAL_FW
   else if( !strcmp( property->identifier, "rs232 io" ) )
   {
      int size = property->value;

      TRACE( "send: out: %x, in: %x\n", property->property_data_size, size );
		
      status = vid21394_rs232_io( data->vid21394handle,
				  property->property_data,
				  property->property_data_size, 
				  (unsigned char*)data->rs232_buffer, 
				  size );
      property->property_data = data->rs232_buffer;
      property->property_data_size = size;
   }
   else if( !strcmp( property->identifier, "rs232 baud rate" ) ) 
   {
      int rate = property->value;

      TRACE( "RS 232 BAUD RATE\n" );
      status = vid21394_rs232_set_baudrate( data->vid21394handle, rate );
   }
   else if( !strcmp( property->identifier, "link speed" ) )
   {
      int speed = property->value;
      status = vid21394_set_link_speed( data->vid21394handle, speed );
   }
#endif
   else if( !strcmp( property->identifier, "firmware version" ) )
   {
      property->value = data->vid21394handle->firmware_version;
      status = STATUS_SUCCESS;
   }
   else
   {
      TRACE( "unknown property!\n" );
   }
	
   return status;
}

unicap_status_t cpi_get_property( void *cpi_data, unicap_property_t *property )
{
   vid21394_data_t *data = cpi_data;
   int property_index = 0;
   int nr_properties = sizeof( vid21394_properties ) / sizeof( unicap_property_t );
   unsigned int value;

   unicap_status_t status = STATUS_FAILURE;


   if( !data || !property )
   {
      return STATUS_INVALID_PARAMETER;
   }
	
   while( ( property_index < nr_properties ) && 
	  ( strcmp( property->identifier, vid21394_properties[property_index].identifier ) ) )
   {
      property_index++;
   }

   if( property_index == nr_properties )
   {
      if( data->enable_visca_support )
      {
	 status = visca_get_property( data->vid21394handle, property );
	 return status;
      }
      else
      {
	 return STATUS_NO_MATCH;
      }
   }
	
   memcpy( property, &data->current_properties[property_index], sizeof( unicap_property_t ) );

   if( !strcmp( property->identifier, "brightness" ) )
   {
      status = vid21394_get_brightness( data->vid21394handle, &value );
      property->value = (double)value / 255.0f;
   }
   else if( !strcmp( property->identifier, "contrast" ) )
   {
      status = vid21394_get_contrast( data->vid21394handle, &value );
      property->value = (double)value / 255.0f;
   }
   else if( !strcmp( property->identifier, "force odd/even" ) )
   {
      status = vid21394_get_force_odd_even( data->vid21394handle, &value );
      property->value = value;
   }
   else if( !strcmp( property->identifier, "source" ) )
   {
      enum vid21394_input_channel channel;
      status = vid21394_get_input_channel( data->vid21394handle, &channel );
      switch( channel )
      {
	 case VID21394_INPUT_COMPOSITE_1:
	    strcpy( property->menu_item, VIDEO_SOURCE_COMPOSITE_1 );
	    break;
	 case VID21394_INPUT_COMPOSITE_2:
	    strcpy( property->menu_item, VIDEO_SOURCE_COMPOSITE_2 );
	    break;
	 case VID21394_INPUT_COMPOSITE_3:
	    strcpy( property->menu_item, VIDEO_SOURCE_COMPOSITE_3 );
	    break;
	 case VID21394_INPUT_COMPOSITE_4:
	    strcpy( property->menu_item, VIDEO_SOURCE_COMPOSITE_4 );
	    break;
	 case VID21394_INPUT_SVIDEO:
	    strcpy( property->menu_item, VIDEO_SOURCE_SVHS );
	    break;
	 default:
	    strcpy( property->menu_item, VIDEO_SOURCE_NONE );
	    break;
      }
   }	
   else if( !strcmp( property->identifier, "video norm" ) )
   {
      enum vid21394_frequency freq;
      status = vid21394_get_frequency( data->vid21394handle, &freq );
      if( freq == VID21394_FREQ_50 )
      {
	 strcpy( property->menu_item, video_norm_menu_items[0] );
      }
      else if( freq == VID21394_FREQ_60 )
      {
	 strcpy( property->menu_item, video_norm_menu_items[1] );
      }
      else
      {
	 strcpy( property->menu_item, "unknown" );
      }
   }
	
#if EXPERIMENTAL_FW
   else if( !strcmp( property->identifier, "rs232 io" ) )
   {
      static char buffer[512];
      property->property_data_size = 512;
		
      property->property_data = buffer;
      status = vid21394_read_rs232( data->vid21394handle, 
				    (unsigned char*)&buffer[0], 
				    (int*)&property->property_data_size );
		
   }
#endif
   else if( !strcmp( property->identifier, "firmware version" ) )
   {
      property->value = data->vid21394handle->firmware_version;
      status = STATUS_SUCCESS;
   }
	

   return status;
}

unicap_status_t cpi_capture_start( void *cpi_data )
{
   vid21394_data_t *data = cpi_data;
   unicap_status_t status;
	

   status = vid21394_start_transmit( data->vid21394handle );
   if( status )
   {
      TRACE( "vid21394_start_transmit failed, err: %s\n", strerror( errno ) );
      return STATUS_FAILURE;
   }

   data->vid21394handle->stop_capture = 0;
   errno = 0;
   if( pthread_create( &data->vid21394handle->capture_thread, NULL, (void*(*)(void*))vid21394_capture_thread, data->vid21394handle )) 
   {
      perror( "create capture thread" );
      return STATUS_FAILURE;
   }

   TRACE( "Capture start\n" );
   data->capture_running = 1;

   return STATUS_SUCCESS;
}

unicap_status_t cpi_capture_stop( void *cpi_data )
{
   vid21394_data_t *data = cpi_data;
   unicap_status_t status;
   int was_running = data->capture_running;
	
   data->capture_running = 0;
   data->vid21394handle->stop_capture = 1;
   
   if( was_running )
   {
      pthread_join( data->vid21394handle->capture_thread, NULL );
   }
   status = vid21394_stop_transmit( data->vid21394handle );
	
   TRACE( "Capture stop\n" );

   return STATUS_SUCCESS;
}

unicap_status_t cpi_queue_buffer( void *cpi_data, unicap_data_buffer_t *buffer )
{
   vid21394_data_t *data = cpi_data;
   unicap_queue_t *queue = malloc( sizeof( struct _unicap_queue ) );
   queue->data = buffer;
   ucutil_insert_back_queue( data->in_queue, queue );

   vid21394_queue_buffer( data->vid21394handle, buffer->data );

   return STATUS_SUCCESS;
}

unicap_status_t cpi_dequeue_buffer( void *cpi_data, unicap_data_buffer_t **buffer )
{
   vid21394_data_t *data = cpi_data;
   unicap_queue_t *entry;

   if( data->capture_running )
   {
      return STATUS_IS_RECEIVING;
   }
	
   entry = ucutil_get_front_queue( data->in_queue );
	
   if( !entry )
   {
      return STATUS_NO_BUFFERS;
   }
	
   *buffer = entry->data;

   free( entry );
	
   return STATUS_SUCCESS;
}


unicap_status_t cpi_wait_buffer( void *cpi_data, unicap_data_buffer_t **buffer )
{
   vid21394_data_t *data = cpi_data;
   unicap_data_buffer_t *returned_buffer;
   unicap_queue_t *entry;
   unicap_status_t status;
   void *tmp_buffer;

   status = vid21394_wait_buffer( data->vid21394handle, &tmp_buffer );
   if( status )
   {
      TRACE( "vid21394_wait_buffer failed\n" );
      return STATUS_FAILURE;
   }

   entry = ucutil_get_front_queue( data->in_queue );
   if( !entry )
   {
      return STATUS_NO_BUFFERS;
   }

   returned_buffer = ( unicap_data_buffer_t * ) entry->data;
   *buffer = returned_buffer;
   returned_buffer->data = tmp_buffer;
   cpi_get_format( data, &returned_buffer->format );

   returned_buffer->buffer_size = returned_buffer->format.buffer_size;
	
   return STATUS_SUCCESS;
}

unicap_status_t cpi_poll_buffer( void *data, int *count )
{
   return STATUS_NOT_IMPLEMENTED;
}

unicap_status_t vid21394cpi_set_event_notify( void *cpi_data, unicap_event_callback_t func, unicap_handle_t unicap_handle )
{
   vid21394_data_t *data = cpi_data;

   data->vid21394handle->event_callback = func;
   data->vid21394handle->unicap_handle = unicap_handle;

   return STATUS_SUCCESS;
}

