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
#include <unicap.h>
#include <unicap_status.h>
#include <unicap_cpi.h>
#include "unicap_helpers.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include <errno.h>

#ifdef RAW1394_1_1_API
#include <libraw1394/raw1394.h>
#include <libraw1394/csr.h>
#else
#include <raw1394.h>
#include <csr.h>
#endif

#include <queue.h>

#include "dcam.h"
#include "dcam_offsets.h"
#include "dcam_functions.h"
#include "dcam_v_modes.h"
#include "dcam_property.h"
#include "dcam_capture.h"
#include "dcam_busreset.h"
#include "unicap_cpi.h"

#include "1394util.h"

#if DCAM_DEBUG
#define DEBUG
#endif
#include "debug.h"

#include <sys/time.h>
#include <time.h>

static unicap_status_t cpi_enumerate_devices( unicap_device_t *device, int index );
static unicap_status_t cpi_open( void **cpi_data, unicap_device_t *device );
static unicap_status_t cpi_close( void *cpi_data );
static unicap_status_t cpi_reenumerate_formats( void *cpi_data, int *count );
static unicap_status_t cpi_enumerate_formats( void *cpi_data, unicap_format_t *format, int index );
static unicap_status_t cpi_set_format( void *cpi_data, unicap_format_t *format );
static unicap_status_t cpi_get_format( void *cpi_data, unicap_format_t *format );
static unicap_status_t cpi_reenumerate_properties( void *cpi_data, int *count );
static unicap_status_t cpi_enumerate_properties( void *cpi_data, unicap_property_t *property, int index );
static unicap_status_t cpi_set_property( void *cpi_data, unicap_property_t *property );
static unicap_status_t cpi_get_property( void *cpi_data, unicap_property_t *property );
static unicap_status_t cpi_queue_buffer( void *cpi_data, unicap_data_buffer_t *buffer );
static unicap_status_t cpi_dequeue_buffer( void *cpi_data, unicap_data_buffer_t **buffer );
static unicap_status_t cpi_wait_buffer( void *cpi_data, unicap_data_buffer_t **buffer );
static unicap_status_t cpi_poll_buffer( void *cpi_data, int *count );
static unicap_status_t dcam_set_event_notify( void *cpi_data, unicap_event_callback_t func, unicap_handle_t unicap_handle );

extern unicap_format_t _dcam_unicap_formats[];
extern struct _dcam_isoch_mode dcam_isoch_table[];

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

   cpi_capture_start: dcam_capture_start,
   cpi_capture_stop: dcam_capture_stop,
   
   cpi_queue_buffer: cpi_queue_buffer,
   cpi_dequeue_buffer: cpi_dequeue_buffer,
   cpi_wait_buffer: cpi_wait_buffer,
   cpi_poll_buffer: cpi_poll_buffer,

   cpi_set_event_notify: dcam_set_event_notify,
};

#if ENABLE_STATIC_CPI
void unicap_dcam_register_static_cpi( struct _unicap_cpi **cpi )
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

/*    reg_data->cpi_capture_start = dcam_capture_start; */
/*    reg_data->cpi_capture_stop = dcam_capture_stop; */
	
/*    reg_data->cpi_queue_buffer = cpi_queue_buffer; */
/*    reg_data->cpi_dequeue_buffer = cpi_dequeue_buffer; */
/*    reg_data->cpi_wait_buffer = cpi_wait_buffer; */
/*    reg_data->cpi_poll_buffer = cpi_poll_buffer; */

/*    reg_data->cpi_set_event_notify = dcam_set_event_notify; */

   return STATUS_SUCCESS;
}
#endif//ENABLE_STATIC_CPI

int cpi_enumerate_devices( unicap_device_t *device, int index )
{	
   raw1394handle_t raw1394handle;
   int numcards;
   struct raw1394_portinfo portinfo[16];
   int current_index = 0;
   int card = 0;

/* 	TRACE( "dcam_enumerate_devices ( i = %d ) \n", index ); */

   raw1394handle = raw1394_new_handle();
   if( !raw1394handle )
   {
      if( !errno )
      {
	 TRACE( "dcam: no kernel support\n" );
	 return STATUS_NO_DEVICE;
      }
      else
      {
	 TRACE( "dcam: can' t get handle\n" );
	 return STATUS_NO_DEVICE;
      }
   }

   numcards = raw1394_get_port_info( raw1394handle, portinfo, 16 );
   if( !numcards )
   {
      TRACE( "dcam: no 1394 cards!\n" );

      raw1394_destroy_handle( raw1394handle );
      return STATUS_NO_DEVICE;
   }
   else if( numcards < 0 )
   {
      raw1394_destroy_handle( raw1394handle );
      return STATUS_NO_DEVICE;
   }
	
   raw1394_destroy_handle( raw1394handle );
	
   // go through all present cards, search for cameras
   for( card = 0; card < numcards; card++ )
   {
      int nodecount;
      int node = 0;
      if( ( raw1394handle = raw1394_new_handle_on_port( card ) ) == 0 )
      {
	 return STATUS_NO_DEVICE;
      }
		
      raw1394_set_userdata( raw1394handle, 0 );

      TRACE( "dcam: probing card %d\n", card );

      nodecount = raw1394_get_nodecount( raw1394handle );
      for( node = 0; node < nodecount; node++ )
      {
	 int unit_directory_count;
	 int directory;

	 TRACE( "dcam: probing node %d\n", node );

	 // shortcut since most devices only have 1 unit directory
	 if( _dcam_is_compatible( raw1394handle, node, 0 ) )
	 {
	    if( index == current_index )
	    {
	       unicap_status_t status;
	       status = _dcam_get_device_info( raw1394handle, node, 0, device );
	       if( status == STATUS_SUCCESS )
	       {
		  TRACE( "found dcam\n" );
		  // got the device with the index we want
		  raw1394_destroy_handle( raw1394handle );
		  return status;
	       }
	       else
	       {
		  TRACE( "can not get device info!\n" );
	       }
	    }
	    current_index++;
	    continue;
	 }

	 unit_directory_count = _dcam_get_directory_count( raw1394handle, node );
	 if( unit_directory_count <= 1 )
	 {
	    TRACE( "directory count <= 1 for node: %d\n", node );
	    continue; // try next device
	 }

	 // scan through all directories of this device 
	 for( directory = 1; directory < unit_directory_count; directory++ )
	 {
	    if( _dcam_is_compatible( raw1394handle, node, directory ) )
	    {
	       if( index == current_index )
	       {
		  unicap_status_t status;
		  status = _dcam_get_device_info( raw1394handle, node, directory, device );
		  if( status == STATUS_SUCCESS )
		  {
		     // got the device with the index we want
		     raw1394_destroy_handle( raw1394handle );
		     return status;
		  }
	       }
	       current_index++;
	    }
	 }// for( directory..
      }// for( node..
      raw1394_destroy_handle( raw1394handle );
   }// for( card..
	
   return STATUS_NO_DEVICE;
}

void *wakeup_routine( void *data )
{
   dcam_handle_t dcamhandle = ( dcam_handle_t ) data;
   while( dcamhandle->raw1394handle )
   {
      raw1394_wake_up( dcamhandle->raw1394handle );
      sleep( 1 );
   }	

   return 0;
}


int cpi_open( void **cpi_data, unicap_device_t *device )
{
   dcam_handle_t dcamhandle;
   unicap_status_t status;

   int port, node, directory;

   int i_tmp;
   char *env;
	
   int is_initializing;
   struct timeval init_timeout;

   *cpi_data = malloc( sizeof( struct _dcam_handle ) );
   if( !*cpi_data )
   {
      return STATUS_NO_MEM;
   }

   dcamhandle = (dcam_handle_t) *cpi_data;
   memset( dcamhandle, 0x0, sizeof( struct _dcam_handle ) );


   TIME("dcam_find_device",
   {
      status = _dcam_find_device( device, &port, &node, &directory );
      if( !SUCCESS( status ) )
      {
	 TRACE( "cpi_open: Could not find device\n" );
	 free( *cpi_data );
	 return status;
      }
   }
      )


      dcamhandle->allocate_bandwidth = 0;
   if( ( env = getenv( "UNICAP_DCAM_BW_CONTROL" ) ) )
   {
      if( !strncasecmp( "enable", env, strlen( "enable" ) ) )
      {
	 dcamhandle->allocate_bandwidth = 0;
      }
   }
	
   dcamhandle->device_present = 1;
	
   dcamhandle->raw1394handle = raw1394_new_handle_on_port( port );
   dcamhandle->port = port;
   dcamhandle->node = node;
   dcamhandle->directory = directory;
   dcamhandle->current_frame_rate = -1;
	
   dcamhandle->use_dma = 1;
   dcamhandle->timeout_seconds = 1; // 1 Second timeout default

   raw1394_set_userdata( dcamhandle->raw1394handle, dcamhandle );

   dcamhandle->command_regs_base = _dcam_get_command_regs_base( dcamhandle->raw1394handle, 
								node, 
								_dcam_get_unit_directory_address( 
								   dcamhandle->raw1394handle, 
								   node, 
								   directory ) );

   TIME("dcam_prepare_property_table", 
   {
      _dcam_prepare_property_table( dcamhandle, &dcamhandle->dcam_property_table );
   }
      );

#if UNICAP_THREADS
   if( pthread_create( &dcamhandle->timeout_thread, NULL, wakeup_routine, dcamhandle ) < 0 )
   {
      TRACE( "FAILED to create timeout_thread!\n" );
      dcamhandle->timeout_thread = 0;
   }
#else
   dcamhandle->timeout_thread = 0;
#endif

	

   memcpy( &dcamhandle->unicap_device, device, sizeof( unicap_device_t ) );

   raw1394_set_bus_reset_handler( dcamhandle->raw1394handle, dcam_busreset_handler );

   // power cycle
   TIME("power_cycle",
   {
		
      _dcam_write_register( dcamhandle->raw1394handle, 
			    dcamhandle->node, 
			    dcamhandle->command_regs_base + 0x610, 
			    0<<31 );	
		
      _dcam_write_register( dcamhandle->raw1394handle, 
			    dcamhandle->node, 
			    dcamhandle->command_regs_base + 0x610, 
			    1<<31 );	
   }
      );
		
   TIME("initialize",
   {
      // Camera initialize
		
      _dcam_write_register( dcamhandle->raw1394handle, 
			    dcamhandle->node, 
			    dcamhandle->command_regs_base + 0x0, 
			    1<<31 );	
		
      gettimeofday( &init_timeout, NULL );
      init_timeout.tv_sec++;
      is_initializing = 1;
      while( is_initializing )
      {
	 quadlet_t quad;
	 struct timeval ctime;
			
	 usleep( 100000 );
			
	 _dcam_read_register( dcamhandle->raw1394handle, 
			      dcamhandle->node, 
			      dcamhandle->command_regs_base + 0x0, 
			      &quad );
	 gettimeofday( &ctime, NULL );
			
	 // wait until either init flag is cleared or timout expired
	 is_initializing = timercmp( &init_timeout, &ctime, > ) && ( quad >> 31 );
      }
   }
      );
	

   cpi_reenumerate_formats( dcamhandle, &i_tmp );
   cpi_reenumerate_properties( dcamhandle, &i_tmp );
   
   ucutil_init_queue( &dcamhandle->input_queue );
   ucutil_init_queue( &dcamhandle->output_queue );

   return STATUS_SUCCESS;
}

int cpi_close( void *cpi_data )
{
   dcam_handle_t dcamhandle = (dcam_handle_t)cpi_data;
   raw1394handle_t raw1394handle = dcamhandle->raw1394handle;
	
   if( !cpi_data )
   {
      return STATUS_SUCCESS;
   }
	
   dcam_capture_stop( cpi_data );
	
   dcamhandle->raw1394handle = 0;
#if UNICAP_THREADS
   if( dcamhandle->timeout_thread )
   {
      pthread_join( dcamhandle->timeout_thread, NULL );
   }
#endif
   raw1394_destroy_handle( raw1394handle );
   
   if( dcamhandle->unicap_handle )
   {
      free( dcamhandle->unicap_handle );
   }
   

   free( dcamhandle );

   return STATUS_SUCCESS;
}

unicap_status_t cpi_reenumerate_formats( void *cpi_data, int *count )
{
   dcam_handle_t dcamhandle = (dcam_handle_t) cpi_data;
   unicap_status_t status;

   dcamhandle->v_format_count = sizeof( dcamhandle->v_format_array ) / sizeof( unicap_format_t );
	
   status = _dcam_prepare_format_array( dcamhandle, dcamhandle->node, dcamhandle->directory, 
					dcamhandle->v_format_array, &dcamhandle->v_format_count );
	
   if( count )
   {	
      *count = dcamhandle->v_format_count;
   }
	
   return status;
}

int cpi_enumerate_formats( void *cpi_data, unicap_format_t *format, int index )
{
   dcam_handle_t dcamhandle = (dcam_handle_t)cpi_data;

   if( index < 0 )
   {
      TRACE( "index < 0 !!!\n" );
      return STATUS_INVALID_PARAMETER;
   }
   if( index > ( dcamhandle->v_format_count - 1 ) )
   {
      TRACE( "index (%d) > format count\n", index, dcamhandle->v_format_count-1 );
      return STATUS_NO_MATCH;
   }
	
   memcpy( format, &dcamhandle->v_format_array[index], sizeof( unicap_format_t ) );
	
   TRACE( "dcam enumerate formats, index: %d  id: %s\n", index, format->identifier );
   TRACE( "format: %dx%dx%d\n", format->size.width, format->size.height, format->bpp );
   return STATUS_SUCCESS;
}

/*
  set format and mode on the camera, according to "format"

  tries to keep current_frame_rate to the actual value. lowers current_frame_rate if it is not available for the current mode or 
  if there is not enough bandwidth for the current frame rate. 
  if current_frame_rate == -1: set to highest

  returns STATUS_NO_MATCH if "format" is not supported by the camera
  STATUS_FAILURE  if write register fails
  STATUS_FRAME_RATE_NOT_AVAILABLE if no frame rate is 
*/
int cpi_set_format( void *cpi_data, unicap_format_t *format )
{
   dcam_handle_t dcamhandle = (dcam_handle_t)cpi_data;
   int index = 0;
   int frame_rate;
/* 	frame_rate_inq_t rate_inq; */
   quadlet_t quad;

   unicap_status_t status;

   while( ( index < dcamhandle->v_format_count ) &&
	  ( strcmp( format->identifier, dcamhandle->v_format_array[index].identifier ) ) )
   {
      index++;
   }
	
   if( index == dcamhandle->v_format_count )
   {
      TRACE( "no match, index = %d, count = %d\n", index, dcamhandle->v_format_count );
      return STATUS_NO_MATCH;
   }
	
   dcamhandle->current_format_index = index;
   TRACE( "current_format_index: %d, in identifier: %s out identifier: %s\n", 
	  dcamhandle->current_format_index, 
	  format->identifier, 
	  dcamhandle->v_format_array[index].identifier);

   for( index = 0; strcmp( _dcam_unicap_formats[index].identifier, format->identifier ); index++ )
   {
   }
	
   // check the available bandwidth and set the highest available frame rate
   if( dcamhandle->current_frame_rate == -1 )
   {
      dcamhandle->current_frame_rate = 5;
   }

   // read the frame rate inquiry register for this mode
   if( !SUCCESS( status = _dcam_read_register( dcamhandle->raw1394handle, 
					       dcamhandle->node, 
					       dcamhandle->command_regs_base + 0x200 + (index * 4 ), 
					       &quad ) ) )
   {
      return status;
   }	

   for( frame_rate = dcamhandle->current_frame_rate; frame_rate >= 0; frame_rate-- )
   {
      if( _dcam_check_frame_rate_available( quad, frame_rate ) )
      {
/* 			int bw_available; */
/* 			bw_available = _1394util_get_available_bandwidth( dcamhandle->raw1394handle ); */
/* 			if( bw_available >= dcam_isoch_table[ (index * 6 + frame_rate) ].bytes_per_packet ) */
/* 			{ */
	 quad = frame_rate << 29;
				
	 if( SUCCESS( _dcam_write_register( dcamhandle->raw1394handle, 
					    dcamhandle->node, 
					    dcamhandle->command_regs_base + 0x600, 
					    quad ) ) )
	 {			   
	    // frame rate available && enough bandwidth
	    dcamhandle->current_frame_rate = frame_rate;
	    break;
	 } /* else { */
/* 					TRACE( "failed to set frame rate\n" ); */
/* 					return STATUS_FAILURE; */
/* 				} */
/* 			} */
      }
   }

   if( dcamhandle->current_frame_rate < 0 )
   {
      return STATUS_FRAME_RATE_NOT_AVAILABLE;
   }

   _dcam_set_mode_and_format( dcamhandle, index );

   dcamhandle->current_iso_index = index * 6 + dcamhandle->current_frame_rate;

/* 	TRACE( "<-------- set format\n" ); */
   return STATUS_SUCCESS;
}

/*
  get format currently set on the camera
*/
unicap_status_t cpi_get_format( void *cpi_data, unicap_format_t *format )
{
   dcam_handle_t dcamhandle = ( dcam_handle_t ) cpi_data;
   int mode_index;
   int format_index;
   int index;
   unicap_status_t status;

/* 	int format_count = sizeof( _dcam_unicap_formats ) / sizeof( unicap_format_t ); */
   int format_count = 3*8;
	
   status = _dcam_get_current_v_mode( dcamhandle, &mode_index );
   if( !SUCCESS( status ) )
   {
      return status;
   }
	
   status = _dcam_get_current_v_format( dcamhandle, &format_index );
   if( !SUCCESS( status ) )
   {
      return status;
   }
	
   index = format_index * 8 + mode_index;

   TRACE( "get format: mode_index: %d format_index: %d\n", mode_index, format_index );

   if( index < format_count )
   {
      memcpy( format, &_dcam_unicap_formats[index], sizeof( unicap_format_t ) );
      TRACE( "format->buffer_size: %d\n", format->buffer_size );
   } else {
      return STATUS_FAILURE;
   }
	
	
   return STATUS_SUCCESS;
}

unicap_status_t cpi_reenumerate_properties( void *cpi_data, int *count )
{
   dcam_handle_t dcamhandle = (dcam_handle_t) cpi_data;
   unicap_status_t status = STATUS_SUCCESS;
   int i;

   quadlet_t feature_hi, feature_lo;

   *count = 0;

   if( _dcam_read_register( dcamhandle->raw1394handle, 
			    dcamhandle->node, 
			    dcamhandle->command_regs_base + 0x404, 
			    &feature_hi ) < 0 )
   {
      return STATUS_FAILURE;
   }
	
   if( _dcam_read_register( dcamhandle->raw1394handle, 
			    dcamhandle->node, 
			    dcamhandle->command_regs_base + 0x408, 
			    &feature_lo ) )
   {
      return STATUS_FAILURE;
   }
	

   dcamhandle->feature_hi = feature_hi;
   dcamhandle->feature_lo = feature_lo;

   for( i = 0; dcamhandle->dcam_property_table[i].id != DCAM_PPTY_END; i++ )
   {
      if( ( dcamhandle->dcam_property_table[i].feature_hi_mask & feature_hi ) ||
	  ( dcamhandle->dcam_property_table[i].feature_lo_mask & feature_lo ) )
      {
	 if( SUCCESS( dcamhandle->dcam_property_table[i].init_function( 
			 dcamhandle, 
			 0,
			 &dcamhandle->dcam_property_table[i] ) ) )
	 {
	    (*count)++;
	 }
      }
   }
	
	
   return status;	
}

unicap_status_t cpi_enumerate_properties( void *cpi_data, unicap_property_t *property, int index )
{
   dcam_handle_t dcamhandle = (dcam_handle_t) cpi_data;
   int i;
   int current = 0;

   unicap_status_t status = STATUS_NO_MATCH;

   if( index < 0 )
   {
      return STATUS_INVALID_PARAMETER;
   }
	
   for( i = 0; dcamhandle->dcam_property_table[i].id != DCAM_PPTY_END; i++ )
   {
      if( ( dcamhandle->dcam_property_table[i].feature_hi_mask & dcamhandle->feature_hi ) ||
	  ( dcamhandle->dcam_property_table[i].feature_lo_mask & dcamhandle->feature_lo ) )
      {
	 if( current == index )
	 {
	    unicap_copy_property( property, &dcamhandle->dcam_property_table[i].unicap_property );
	    status = STATUS_SUCCESS;
	    break;
	 }
	 current++;
      }
   }
	
   return status;
}

unicap_status_t cpi_set_property( void *cpi_data, unicap_property_t *property )
{
   dcam_handle_t dcamhandle = (dcam_handle_t) cpi_data;
   unicap_status_t status = STATUS_NO_MATCH;
   int i;
	
   for( i = 0; dcamhandle->dcam_property_table[ i ].id != DCAM_PPTY_END; i++ )
   {
      if( !strcmp( property->identifier, dcamhandle->dcam_property_table[ i ].unicap_property.identifier ) )
      {
	 dcam_property_t *dcam_property = &dcamhandle->dcam_property_table[ i ];
			
	 status = dcamhandle->dcam_property_table[ i ].set_function( dcamhandle, 
								     property, 
								     dcam_property );
	 break;
      }
   }
	
   return status;
}

unicap_status_t cpi_get_property( void *cpi_data, unicap_property_t *property )
{
   dcam_handle_t dcamhandle = (dcam_handle_t) cpi_data;
   unicap_status_t status = STATUS_NO_MATCH;
   int i;
	
   for( i = 0; dcamhandle->dcam_property_table[ i ].id != DCAM_PPTY_END; i++ )
   {
      if( !strcmp( property->identifier, dcamhandle->dcam_property_table[ i ].unicap_property.identifier ) )
      {
	 dcam_property_t *dcam_property = &dcamhandle->dcam_property_table[ i ];

	 unicap_copy_property_nodata( property, &dcam_property->unicap_property );

	 status = dcamhandle->dcam_property_table[ i ].get_function( dcamhandle, 
								     property, 
								     dcam_property );
	 break;
      }
   }
	
   return status;
}

unicap_status_t dcam_capture_start( void *cpi_data )
{
   dcam_handle_t dcamhandle = (dcam_handle_t) cpi_data;

   int channel;	

   quadlet_t quad;
   int retries = 1;
	
  allocate:
   if( ( channel = _1394util_find_free_channel( dcamhandle->raw1394handle ) ) < 0 )
   {
      TRACE( "dcam.cpi: failed to allocate channel\n");
      return STATUS_INSUFFICIENT_RESOURCES;
   }
	
   if( dcamhandle->allocate_bandwidth )
   {
      if( STATUS_SUCCESS != (_1394util_allocate_bandwidth(
				dcamhandle->raw1394handle,
				dcam_isoch_table[dcamhandle->current_iso_index].bytes_per_packet ) ) )
      {
			
	 TRACE( "dcam.cpi: failed to allocate bandwidth\n" );
	 _1394util_free_channel( dcamhandle->raw1394handle, channel );
	 if( retries-- )
	 {
	    TRACE( "trying a busreset\n" );
	    raw1394_reset_bus_new( dcamhandle->raw1394handle, RAW1394_SHORT_RESET );
	    sleep( 1 ); // waiting one second to allow devices to reallocate BW
	    goto allocate;
	 }
			
	 return STATUS_INSUFFICIENT_BANDWIDTH;
      }
      else
      {
	 TRACE( "BW_allocate: success\n" );
      }
		
   }
	
   TRACE( "BW allocated\n" );

   if( dcam_isoch_table[dcamhandle->current_iso_index].min_speed <= S400 )
   {
      quad = ( channel << 28 ) | ( S400 << 24 );
   }
   else
   {
      quad = ( channel << 28 ) | 
	 ( dcam_isoch_table[dcamhandle->current_iso_index].min_speed << 24 );
   }   
	
   if( _dcam_write_register( dcamhandle->raw1394handle, 
			     dcamhandle->node, 
			     dcamhandle->command_regs_base + 0x60c, 
			     quad ) < 0 )
   {
      _1394util_free_channel( dcamhandle->raw1394handle, 
			      channel );
      _1394util_free_bandwidth( 
	 dcamhandle->raw1394handle, 
	 dcam_isoch_table[dcamhandle->current_iso_index].bytes_per_packet );
      TRACE( "failed to write iso_channel register\n" );
      return STATUS_FAILURE;
   }
	
   if( dcamhandle->allocate_bandwidth )
   {
      dcamhandle->bandwidth_allocated =
	 dcam_isoch_table[dcamhandle->current_iso_index].bytes_per_packet;
   }
		
   dcamhandle->channel_allocated = channel;

	
   // enable iso transmission
   if( _dcam_write_register( dcamhandle->raw1394handle, 
			     dcamhandle->node, 
			     dcamhandle->command_regs_base + O_ISO_CTRL, 
			     1 << 31 ) < 0 )
   {
      TRACE( "failed to write iso enable register\n" );
      return STATUS_FAILURE;
   }
   TRACE( "bytes per pkt: %d idx: %d, channel: %d, frame rate: %d\n",
	  dcam_isoch_table[dcamhandle->current_iso_index].bytes_per_packet,
	  dcamhandle->current_iso_index,
	  channel,
	  dcamhandle->current_frame_rate);


   if( dcamhandle->use_dma )
   {
      // use video1394 for isoch reception

      dcamhandle->buffer_size = 
	 dcam_isoch_table[ dcamhandle->current_iso_index ].bytes_per_frame;
      TRACE( "dcamhandle->buffer_size: %d\n", dcamhandle->buffer_size );
      TRACE( "dcamhandle->current_iso_index: %d\n", dcamhandle->current_iso_index );
		
      if( !SUCCESS( _dcam_dma_setup( dcamhandle ) ) )
      {
	 _1394util_free_channel( dcamhandle->raw1394handle, 
				 channel );
	 _1394util_free_bandwidth( dcamhandle->raw1394handle, 
				   dcam_isoch_table[dcamhandle->current_iso_index].bytes_per_packet );

	 _dcam_write_register( dcamhandle->raw1394handle, 
			       dcamhandle->node, 
			       dcamhandle->command_regs_base + O_ISO_CTRL, 
			       0 );
	 TRACE( "failed to setup DMA\n" );
	 return STATUS_FAILURE;
      }
      dcamhandle->dma_capture_thread_quit = 0;

#if UNICAP_THREADS		
      pthread_create( &dcamhandle->dma_capture_thread, NULL, dcam_dma_capture_thread, dcamhandle );
#endif
   }
   else
   {
      // use raw1394 packet_per_buffer for isoch reception

      if( raw1394_iso_recv_init( dcamhandle->raw1394handle, 
				 dcam_iso_handler, 
				 1000, 
				 dcam_isoch_table[dcamhandle->current_iso_index].bytes_per_packet, 
				 channel, 
#ifdef RAW1394_1_1_API
				 RAW1394_DMA_PACKET_PER_BUFFER,
#endif
				 10 ) < 0 )
      {
	 _1394util_free_channel( dcamhandle->raw1394handle, 
				 channel );
	 _1394util_free_bandwidth( dcamhandle->raw1394handle, 
				   dcam_isoch_table[dcamhandle->current_iso_index].bytes_per_packet );
	 // disable iso transmission
	 _dcam_write_register( dcamhandle->raw1394handle, 
			       dcamhandle->node, 
			       dcamhandle->command_regs_base + O_ISO_CTRL, 
			       0 );
	 TRACE( "raw1394_iso_recv_init failed\n" );
	 return STATUS_FAILURE;
      }

	
      if( raw1394_iso_recv_start( dcamhandle->raw1394handle, -1, -1, -1 ) < 0 )
      {
	 _1394util_free_channel( dcamhandle->raw1394handle, 
				 channel );
	 _1394util_free_bandwidth( dcamhandle->raw1394handle, 
				   dcam_isoch_table[dcamhandle->current_iso_index].bytes_per_packet );
	 _dcam_write_register( dcamhandle->raw1394handle, 
			       dcamhandle->node, 
			       dcamhandle->command_regs_base + O_ISO_CTRL, 
			       0 );
	 return STATUS_FAILURE;
      }			

      dcamhandle->wait_for_sy = 1;
      dcamhandle->buffer_size = dcam_isoch_table[dcamhandle->current_iso_index].bytes_per_frame;
      dcamhandle->current_buffer_offset = 0;

   }

   dcamhandle->capture_running = 1;

   return STATUS_SUCCESS;
}

unicap_status_t dcam_capture_stop( void *cpi_data )
{
   dcam_handle_t dcamhandle = (dcam_handle_t) cpi_data;

   unicap_status_t status = STATUS_SUCCESS;

   if( dcamhandle->capture_running )
   {
      if( !dcamhandle->use_dma )
      {
	 raw1394_iso_stop( dcamhandle->raw1394handle );
      }
      else
      {
	 dcamhandle->dma_capture_thread_quit = 1;
			
#if UNICAP_THREADS
	 // send a signal to interrupt the ioctl
	 pthread_kill( dcamhandle->dma_capture_thread, SIGUSR1 );
	 pthread_join( dcamhandle->dma_capture_thread, NULL );
#endif

	 _dcam_dma_unlisten( dcamhandle );
	 _dcam_dma_free( dcamhandle );
      }

      if( dcamhandle->device_present )
      {
	 _1394util_free_channel( dcamhandle->raw1394handle,
				 dcamhandle->channel_allocated );
	 _1394util_free_bandwidth( dcamhandle->raw1394handle,
				   dcamhandle->bandwidth_allocated );
      }
   }
   else
   {
      TRACE( "capture not running!\n" );
      status = STATUS_CAPTURE_ALREADY_STOPPED;
   }


   // Disable isochronous transmission
   _dcam_write_register( dcamhandle->raw1394handle,
			 dcamhandle->node,
			 dcamhandle->command_regs_base + 0x614,
			 0 );

   dcamhandle->capture_running = 0;

   // if a buffer is in use, return it to the input queue
   if( dcamhandle->current_buffer )
   {
      ucutil_insert_front_queue( &dcamhandle->input_queue, dcamhandle->current_buffer );
      dcamhandle->current_buffer = 0;
   }

   return status;
}

unicap_status_t cpi_queue_buffer( void *cpi_data, unicap_data_buffer_t *buffer )
{
   dcam_handle_t dcamhandle = (dcam_handle_t) cpi_data;
   unicap_queue_t *entry;

   entry = malloc( sizeof( unicap_queue_t ) );
   if( !entry )
   {
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   entry->data = buffer;

   ucutil_insert_back_queue( &dcamhandle->input_queue, entry );

   return STATUS_SUCCESS;
}

unicap_status_t cpi_dequeue_buffer( void *cpi_data, unicap_data_buffer_t **buffer )
{
   dcam_handle_t dcamhandle = (dcam_handle_t)cpi_data;
   unicap_queue_t *entry;
	
   if( dcamhandle->capture_running )
   {
      return STATUS_IS_RECEIVING;
   }
	
   entry = ucutil_get_front_queue( &dcamhandle->input_queue );

   if( !entry )
   {
      entry = ucutil_get_front_queue( &dcamhandle->output_queue );
   }

   if( !entry )
   {
      return STATUS_NO_BUFFERS;
   }

   (*buffer) = entry->data;
   free( entry );

   return STATUS_SUCCESS;
}

unicap_status_t cpi_wait_buffer( void *cpi_data, unicap_data_buffer_t **buffer )
{
   dcam_handle_t dcamhandle = (dcam_handle_t) cpi_data;
   unicap_status_t status = STATUS_SUCCESS;

   if( !dcamhandle->output_queue.next && !dcamhandle->capture_running )
   {
      TRACE( "capture stopped\n" );
      return STATUS_IS_STOPPED;
   }

#if !UNICAP_THREADS
   dcam_dma_wait_buffer( dcamhandle );
#endif
	
   if( dcamhandle->output_queue.next )
   {
      unicap_queue_t *first_entry;
      unicap_data_buffer_t *returned_buffer;

      first_entry = ucutil_get_front_queue( &dcamhandle->output_queue );
      (*buffer) = first_entry->data;
      returned_buffer = *buffer;

      unicap_copy_format( &returned_buffer->format, 
			  &dcamhandle->v_format_array[dcamhandle->current_format_index] );
      free( first_entry );
   }
   else
   {
      if( dcamhandle->input_queue.next || dcamhandle->current_buffer )
      {
	 unicap_queue_t *first_entry;
	 struct timeval timeout_time, cur_time;
	 unicap_data_buffer_t *returned_buffer;
			
	 if( gettimeofday( &timeout_time, NULL ) < 0 )
	 {
	    return STATUS_FAILURE;
	 }
			
	 timeout_time.tv_sec += dcamhandle->timeout_seconds;
	 while( !dcamhandle->output_queue.next )
	 {
	    // raw1394_loop_iterate is waked regularly by a seperate thread ( every 500ms )
	    if( gettimeofday( &cur_time, NULL ) < 0 )
	    {
	       return STATUS_FAILURE;
	    }
				
	    if( timercmp( &cur_time, &timeout_time, > ) )
	    {	
	       // restart DMA on timeout
	       // There seems to be a bug in the video1394 driver that causes the 
	       // DMA to lock up without reason. We try to restart the DMA and return 
	       // STATUS_TIMEOUT to notify the application
	       // TODO: find out if this is fixed in recent kernels
	       dcam_capture_stop( dcamhandle );
	       dcam_capture_start( dcamhandle );
	       return STATUS_TIMEOUT;
	    }

	    // TODO: Implement semaphore based waiting
	    {
	       // raw1394_loop_iterate is called in a seperate thread
	       usleep( 1000 );
	    }				
	 }
	 first_entry = ucutil_get_front_queue( &dcamhandle->output_queue );
	 *buffer = first_entry->data;
	 returned_buffer = *buffer;
			
	 unicap_copy_format( &returned_buffer->format, 
			     &dcamhandle->v_format_array[dcamhandle->current_format_index] );

			
			
	 free( first_entry );
      }
      else
      {
	 return STATUS_NO_BUFFERS;
      }
   }

   return status;
}

unicap_status_t cpi_poll_buffer( void *cpi_data, int *count )
{
   dcam_handle_t dcamhandle = ( dcam_handle_t ) cpi_data;
   int buffers = 0;

   buffers = ucutil_queue_get_size( &dcamhandle->output_queue );

   *count = buffers;

   return STATUS_SUCCESS;
}

unicap_status_t dcam_set_event_notify( void *cpi_data, unicap_event_callback_t func, unicap_handle_t unicap_handle )
{
   dcam_handle_t dcamhandle = ( dcam_handle_t ) cpi_data;

   dcamhandle->event_callback = func;
   dcamhandle->unicap_handle = unicap_handle;
   
   return STATUS_SUCCESS;
}
