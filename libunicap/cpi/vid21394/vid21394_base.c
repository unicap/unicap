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


#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <fcntl.h>
#include <netinet/in.h> // for ntohl
#include <unistd.h>

//for nanosleep
#include <time.h>

// Old .pc descriptions had .../libraw1394 in their path. 
#if RAW1394_1_1_API
#include <libraw1394/raw1394.h>
#include <libraw1394/csr.h>
#else
#include <raw1394.h>
#include <csr.h>
#endif

/* #include "kernel-video1394.h" */
#include "video1394.h"
#include "vid21394_base.h"
#include "1394util.h"
#include "Fcp.h"
#include <queue.h>
#include "unicap_status.h"

#if VID21394_DEBUG
#define DEBUG
#endif
#include <debug.h>

#define DMA 1

int vid21394_video_mode_sizes[]=
{
   0x0,
   320*240*2,       // QVGA 4:2:2
   640*480+640*240, // VGA  4:1:1
   640*480*2,       // VGA  4:2:2
   320*240,         // QVGA 8:0:0
   640*480,         // VGA  8:0:0
   320*240+320*120, // QVGA 4:1:1
   768*576,         // PAL  8:0:0
   768*576+768*288, // PAL  4:1:1
   768*576*2        // PAL  4:2:2
};

int vid21394_video_mode_line_lengths[]=
{
   0x0,
   320*2,           // QVGA 4:2:2
   640+320,         // VGA  4:1:1
   640*2,           // VGA  4:2:2
   320,             // QVGA 8:0:0
   640,             // VGA  8:0:0
   320+160,         // QVGA 4:1:1
   768,             // PAL  8:0:0
   768+384,         // PAL  4:1:1
   768*2            // PAL  4:2:2
};

static void *_vid21394_timeout_thread( void *arg );
static int _vid21394_busreset_handler( raw1394handle_t raw1394handle, unsigned int generation );

static int _vid21394_fcp_handler( raw1394handle_t handle, nodeid_t nodeid, int response,
				  size_t length, unsigned char *data );


static int _vid21394_send_fcp_command( vid21394handle_t vid21394handle, 
				       unsigned long long fcp_command, 
				       int sync_bit, 
				       unsigned long *response );

static unicap_status_t _vid21394_send_fcp_command_ext( vid21394handle_t vid21394handle, 
						       unsigned long long fcp_command,
						       unsigned long extra_quad, 
						       int sync_bit, 
						       unsigned long *response );

static unicap_status_t _vid21394_send_fcp_command_new( vid21394handle_t vid21394handle, 
						       unsigned long long fcp_command,
						       unsigned long sync_bit,
						       void *data, 
						       size_t data_length, 
						       void *response, 
						       size_t *response_length );

static int _vid21394_find_device( unsigned long long sernum, int *port, int *node );

enum vid21394_byte_order
{
   VID21394_BYTE_ORDER_UYVY = 0x0, 
   VID21394_BYTE_ORDER_YUYV = 0x1, 
   VID21394_BYTE_ORDER_VYUY = 0x2, 
   VID21394_BYTE_ORDER_YVYU = 0x3
};

typedef enum vid21394_byte_order vid21394_byte_order_t;

static unsigned long long prepare_fcp_command( unsigned int x )
{
   union fcp_command command;
	
   command.int32value[0] = ntohl( FCP_COMMAND );
   command.int32value[1] = ntohl( x );
	
   return command.int64value;
	
}

#ifdef DEBUG

/*
  Dumps the data contained in the FCP response to stdout
*/
static void _vid21394_dump_fcp_data( int response, int length, unsigned char * data )
{
   
/*    TRACE( "got fcp %s of %d bytes:", */
/* 	  (response ? "response" : "command"), length); */


/*    while( length ) */
/*    { */
/*       printf(" %02x", *data); */
/*       data++; */
/*       length--; */
/*    } */
/*    printf("\n"); */
}

#endif //DEBUG

/*
  Handler for Fcp responses ( called from libraw1394 )

  See raw1394.h
*/
static int _vid21394_fcp_handler(raw1394handle_t handle, nodeid_t nodeid, int response,
				 size_t length, unsigned char *data)
{

   unsigned long fcp_lo;
   unsigned char fcp_command_id;
   unsigned char fcp_status;

   int command_bit;
	
   vid21394handle_t vid21394handle = raw1394_get_userdata( handle );

#ifdef DEBUG
   _vid21394_dump_fcp_data( response, length, data );
#endif

   if( length < 8 )
   {
      TRACE( "FCP response does not contain enough data!\n" );
      return -1;
   }

   fcp_lo = *( ( unsigned long * ) data );
   if( htonl( fcp_lo ) != FCP_COMMAND )
   {
      TRACE( "FCP response does not match! quadlet #1's value is: %08lx, should be: %08x\n", 
	     fcp_lo, FCP_COMMAND );
      return -1;
   }

   data += 4; // skip the first 4 bytes which are FCP_COMMAND
	
   fcp_command_id = *data;
   data++;
   fcp_status = *data;
   data++;

   if( fcp_command_id == ENTER_BOOTLOAD_MSG )
   {
      command_bit = 31;
   }
   else
   {
      command_bit = fcp_command_id - 0x10;
   }
   if( sem_post( &vid21394handle->fcp_sync_sem[command_bit] ) < 0 )
   {
#ifdef DEBUG
      perror( "cpi/vid21394" );
#endif
      return STATUS_FAILURE;
   }
	
   vid21394handle->fcp_status[command_bit] = fcp_status;

   switch( fcp_command_id )
   {
      case READ_I2C_BYTE_MSG:
      {
/* 	 TRACE( "Got response for READ_I2C_BYTE_MSG\n" ); */
	 vid21394handle->fcp_data = *(data+1);
	 break;
      }
		
      case WRITE_I2C_BYTE_MSG:
      {
/* 	 TRACE( "Got response for WRITE_I2C_BYTE_MSG\n" ); */
	 break;
      }
		
      case SET_VIDEO_MODE_MSG:
      {
/* 	 TRACE( "Got response for SET_VIDEO_MODE_MSG\n" ); */
	 break;
      }
		
      case GET_FIRM_VERS_MSG:
      {
/* 	 TRACE( "Got response for GET_FIRM_VERS_MSG\n" ); */
	 vid21394handle->firmware_version = *data ;
	 vid21394handle->firmware_version = vid21394handle->firmware_version << 8;
	 vid21394handle->firmware_version += *(data+1) ;
/* 	 TRACE( "data: %x\ndata+1: %x\n", *data, *(data+1) ); */
/* 	 TRACE( "pUserdata->firmware_version: %x\n", vid21394handle->firmware_version ); */
	 break;
      }
		
      case INPUT_SELECT_MSG:
      {
/* 	 TRACE( "Got response for INPUT_SELECT_MSG\n" ); */
	 break;
      }
		
      case CHK_VIDEO_LOCK_MSG:
      {
/* 	 TRACE( "Got response for CHK_VIDEO_LOCK_MSG\n" ); */
	 vid21394handle->fcp_data =( *(data+1) + 
				     ( *(data) << 8 ) );
	 break;
      }

      case ENA_ISOCH_TX_MSG:
      {
/* 	 TRACE( "Got response for ENA_ISOCH_TX_MSG\n" ); */
	 break;
      }
		
      case AVSYNC_EVERY_HLINE_MSG:
      {
/* 	 TRACE( "Got response for AVSYNC_EVERY_HLINE_MSG\n" ); */
	 break;
      }
		
      case READ_LINK_REG_MSG:
      {
/* 	 TRACE( "Got response for READ_LINK_REG_MSG\n" ); */
	 vid21394handle->fcp_data =( *(data+5) + 
				     ( *(data+4 ) << 8 ) + 
				     ( *(data+3 ) << 16 ) + 
				     ( *(data+2 ) << 24 ) );
	 break;
      }
		
      case WRITE_LINK_REG_MSG:
      {
/* 	 TRACE( "Got response for WRITE_LINK_REG_MSG\n" ); */
	 break;
      }
		
      case SERIAL_NUMBER_MSG:
      {
/* 	 TRACE( "Got response for SERIAL_NUMBER_MSG\n" ); */
	 break;
      }
		
      case SET_VIDEO_FREQUENCY_MSG:
      {
/* 	 TRACE( "Got response for SET_VIDEO_FREQUENCY_MSG\n" ); */
	 break;
      }

      case RS232_IO_MSG:
      {
/* 	 TRACE( "Got response for RS232_IO_MSG\n" ); */
	 if( length < 12 )
	 {
/* 	    TRACE( "FCP response does not contain enough data!\n" ); */
	    break;
	 }
	 if( ( *data+1 ) ) // if this was a read request
	 {
/* 	    TRACE( "%d bytes of data\n", *(data+1)); */
	    memcpy( &vid21394handle->fcp_response[0], data+2, *data+1 );
	    vid21394handle->fcp_response_length = *(data+1 );
	    vid21394handle->fcp_status[command_bit] = fcp_status;
	 }
	 break;
      }
		
      case LINK_SPEED_MSG:
      {
/* 	 TRACE( "Got response for LINK_SPEED_MSG\n" ); */
	 break;
      }		

      case ENTER_BOOTLOAD_MSG:
      {
/* 	 TRACE( "Got response for ENTER_BOOT_MODE_MSG\n" ); */
	 break;
      }
   }//switch
	
   return fcp_status;
}

/*
  Handles bus reset
  
  - All resources get freed upon a bus reset
  - Reallocate channel and bandwidth

  TODO:
  if device is detached and reattached, restart transmission if it was started before
*/
static int _vid21394_busreset_handler( raw1394handle_t raw1394handle, unsigned int generation )
{
   vid21394handle_t vid21394handle = raw1394_get_userdata( raw1394handle );
   
   int new_node, new_port;

   raw1394_update_generation( raw1394handle, generation );
   
   // find new port of the device
   // allows detaching of the device and re-attaching on another port
   if( !SUCCESS( _vid21394_find_device( vid21394handle->serial_number, &new_node, &new_port ) ) )
   {
      vid21394handle->device_present = 0;
	   
      return 0;
   }
   
   // find new node of the device
   if( new_port != vid21394handle->port )
   {
      vid21394handle->device_present = 0;
      return 0;
   }
   
/* #ifdef DEBUG */
/*    fprintf( stderr, __LIBNAME ": new generation: %d, new node: %d, new port: %d", generation, new_node, new_port ); */
/* #endif */
   
   vid21394handle->port = new_port;
   vid21394handle->node = new_node;

   // reallocate channel and bandwidth
   _1394util_allocate_channel( raw1394handle, vid21394handle->channel );
/*    _1394util_allocate_bandwidth( raw1394handle, vid21394handle->bandwidth ); */

   return 0;
}


/*
  Send a Fcp command to the device

  Input: vid21394handle
  fcp_command: quadlet to send as fcp command
  sync_bit: bit used in fcp_sync_bits
	 
  Output: response: response code returned by the device to this command

  Return: STATUS_SUCCESS
  STATUS_TIMEOUT: command took longer than 1 sec
  STATUS_FAILURE
*/
static unicap_status_t _vid21394_send_fcp_command( vid21394handle_t vid21394handle, 
						   unsigned long long fcp_command, 
						   int sync_bit, 
						   unsigned long *response )
{
   nodeid_t nodeid;
   raw1394handle_t raw1394handle; 

   unsigned long long prepared_fcp_command;
	
   struct timeval cur_time;
   struct timeval timeout_time;

   if( !vid21394handle->device_present )
   {
      TRACE( "No device!\n" );
      return STATUS_NO_DEVICE;
   }
	
   raw1394handle = vid21394handle->raw1394handle;
   nodeid = 0xffc0 | vid21394handle->node;

   sem_init( &vid21394handle->fcp_sync_sem[sync_bit], 0, 0 );

   prepared_fcp_command = prepare_fcp_command( fcp_command );

/*    TRACE( "prepared fcp command: %016llx\n", prepared_fcp_command ); */
	
   if( 0 > raw1394_write( raw1394handle, 
			  nodeid, 
			  CSR_REGISTER_BASE + CSR_FCP_COMMAND, 
			  sizeof( prepared_fcp_command ), 
			  (quadlet_t *)&prepared_fcp_command ) )
   {
      TRACE( "raw1394 write failed\n" );
      return STATUS_FAILURE;
   }
	

   if( gettimeofday( &timeout_time, NULL ) < 0 )
   {
      TRACE( "gettimeofday failed\n" );
      return STATUS_FAILURE;
   }
	
   timeout_time.tv_sec += FCP_TIMEOUT; // 1 second timeout
	
   while( sem_trywait( &vid21394handle->fcp_sync_sem[ sync_bit ] ) != 0 )
   {
      // raw1394_loop_iterate is waked regularly by a seperate thread ( every 500ms )
      if( gettimeofday( &cur_time, NULL ) < 0 )
      {
	 TRACE( "gettimeofday failed\n" );
	 return STATUS_FAILURE;
      }
		
      if( ( cur_time.tv_sec > timeout_time.tv_sec ) || 
	  ( ( cur_time.tv_sec == timeout_time.tv_sec ) && 
	    ( cur_time.tv_usec > timeout_time.tv_usec ) ) )
      {
	 TRACE( "timeout\n" );
	 return STATUS_TIMEOUT;
      }
		
      raw1394_loop_iterate( raw1394handle );
   }

   if( vid21394handle->fcp_status[ sync_bit ] != FCP_ACK_MSG )
   {
      TRACE( "no ack msg\n" );
      return STATUS_FAILURE;
   }

   if( response )
   {
      *response = vid21394handle->fcp_data;
   }
	
	
   return STATUS_SUCCESS;
}

static unicap_status_t _vid21394_send_fcp_command_new( vid21394handle_t vid21394handle, 
						       unsigned long long fcp_command,
						       unsigned long sync_bit,
						       void *data, 
						       size_t data_length, 
						       void *response, 
						       size_t *response_length )
{
   unsigned long prepared_fcp_command[70];
	
   struct timeval cur_time;
   struct timeval timeout_time;

   int i;

   raw1394handle_t raw1394handle = vid21394handle->raw1394handle;

   nodeid_t nodeid = 	nodeid = 0xffc0 | vid21394handle->node;
	
   unicap_status_t status = STATUS_SUCCESS;

   if( data_length > 268 )
   {
      TRACE( "incorrect data length, too much data\n" );
      return STATUS_FAILURE;
   }

   sem_init( &vid21394handle->fcp_sync_sem[sync_bit], 0, 0 );

   prepared_fcp_command[0] = ntohl( FCP_COMMAND );
   prepared_fcp_command[1] = ntohl( fcp_command );
   for( i = 0; i < data_length; i+=4 )
   {
      prepared_fcp_command[2 + ( i/4 )] = ntohl( *(unsigned long *)(data + i ) );
      if( data_length > 4 )
      {
/* 	 TRACE( "prepare fcpcommand: %d = %08x len: %d\n", 2+(i/4),
		* (unsigned int )prepared_fcp_command[2 + ( i/4 )], data_length ); */
      }  
   }

   if( 0 > raw1394_write( raw1394handle, 
			  nodeid, 
			  CSR_REGISTER_BASE + CSR_FCP_COMMAND,
			  (2 * ( sizeof( unsigned long ) ) ) + data_length, 
			  (quadlet_t *)prepared_fcp_command ) )
   {
      TRACE( "raw1394 write failed" );
      return STATUS_FAILURE;
   }
	
   if( gettimeofday( &timeout_time, NULL ) < 0 )
   {
		
      return STATUS_FAILURE;
   }
	
   timeout_time.tv_sec += FCP_TIMEOUT; // 10 second timeout
	
   while( sem_trywait( &vid21394handle->fcp_sync_sem[sync_bit] ) != 0 )
   {
      // raw1394_loop_iterate is waked regularly by a seperate thread ( every 500ms )
      if( gettimeofday( &cur_time, NULL ) < 0 )
      {
	 TRACE( "gettimeofday failed\n" );
	 return STATUS_FAILURE;
      }
		
      if( ( cur_time.tv_sec > timeout_time.tv_sec ) || 
	  ( ( cur_time.tv_sec == timeout_time.tv_sec ) && 
	    ( cur_time.tv_usec > timeout_time.tv_usec ) ) )
      {
	 TRACE( "timeout\n" );
	 return STATUS_TIMEOUT;
      }
		
      raw1394_loop_iterate( raw1394handle );
   }

   if( vid21394handle->fcp_status[sync_bit] != 0xaa )
   {
      TRACE( "no ack msg\n" );
      return STATUS_FAILURE;
   }	
	
   if( *response_length <= vid21394handle->fcp_response_length )
   {
      memcpy( response, &vid21394handle->fcp_response[0], vid21394handle->fcp_response_length );
      *response_length = vid21394handle->fcp_response_length;
   }
   else
   {
      status = STATUS_BUFFER_TOO_SMALL;
   }
	
	
   return status;
}


/*
  Send a Fcp command to the device

  Input: vid21394handle
  fcp_command: quadlet to send as fcp command
  sync_bit: bit used in fcp_sync_bits
	 
  Output: response: response code returned by the device to this command

  Return: STATUS_SUCCESS
  STATUS_TIMEOUT: command took longer than 1 sec
  STATUS_FAILURE
*/
static unicap_status_t _vid21394_send_fcp_command_ext( vid21394handle_t vid21394handle, 
						       unsigned long long fcp_command,
						       unsigned long extra_quad, 
						       int sync_bit, 
						       unsigned long *response )
{
   nodeid_t nodeid;
   raw1394handle_t raw1394handle; 

   unsigned long prepared_fcp_command[3];
	
   struct timeval cur_time;
   struct timeval timeout_time;

   if( !vid21394handle->device_present )
   {
      return STATUS_NO_DEVICE;
   }
	
   raw1394handle = vid21394handle->raw1394handle;
   nodeid = 0xffc0 | vid21394handle->node;
   vid21394handle->fcp_data = 0;
   vid21394handle->fcp_ext_data = 0;

   sem_init( &vid21394handle->fcp_sync_sem[sync_bit], 0, 0 );

   prepared_fcp_command[0] = ntohl( FCP_COMMAND );
   prepared_fcp_command[1] = ntohl( fcp_command );
   prepared_fcp_command[2] = ntohl( extra_quad );

   if( 0 > raw1394_write( raw1394handle, 
			  nodeid, 
			  CSR_REGISTER_BASE + CSR_FCP_COMMAND, 
			  3 * ( sizeof( unsigned long ) ), 
			  (quadlet_t *)prepared_fcp_command ) )
   {
      TRACE( "raw1394 write failed\n" );
      return STATUS_FAILURE;
   }
	

   if( gettimeofday( &timeout_time, NULL ) < 0 )
   {
		
      return STATUS_FAILURE;
   }
	
   timeout_time.tv_sec += 1; // 1 second timeout
	
   while( sem_trywait( &vid21394handle->fcp_sync_sem[sync_bit] ) != 0 )
   {
      // raw1394_loop_iterate is waked regularly by a seperate thread ( every 500ms )
      if( gettimeofday( &cur_time, NULL ) < 0 )
      {
	 TRACE( "gettimeofday failed\n" );
	 return STATUS_FAILURE;
      }
		
      if( ( cur_time.tv_sec > timeout_time.tv_sec ) || 
	  ( ( cur_time.tv_sec == timeout_time.tv_sec ) && 
	    ( cur_time.tv_usec > timeout_time.tv_usec ) ) )
      {
	 TRACE( "timeout command: %08x\n", (unsigned int) fcp_command );
	 return STATUS_TIMEOUT;
      }
		
      raw1394_loop_iterate( raw1394handle );
   }

   if( vid21394handle->fcp_status[ sync_bit ] != FCP_ACK_MSG )
   {
      TRACE( "no ack msg\n" );
      return STATUS_FAILURE;
   }

   if( response )
   {
      *response = vid21394handle->fcp_data;
   }
	
	
   return STATUS_SUCCESS;
}


/*
  Find the port the device with the serial number sernum is connected to.

  Input: raw1394handle
  sernum

  Returns: port or -1 if device is unfound
*/
static int _vid21394_find_device( unsigned long long sernum, int *port, int *node )
{
   int numcards = 0;
   struct raw1394_portinfo portinfo[16];
   int probeport;
   int probenode = 0;

   raw1394handle_t raw1394handle;

   unicap_status_t status = STATUS_FAILURE;

   *node = -1;
   *port = -1;

   raw1394handle = raw1394_new_handle();
   numcards = raw1394_get_port_info( raw1394handle, portinfo, 16 );
   if( numcards < 0 )
   {
      TRACE( "Error querying 1394 card info\n" );
      return -1;
   } else if( numcards == 0 ) {
      TRACE( "No 1394 cards found!\n" );
      return -1;
   }
   raw1394_destroy_handle( raw1394handle );

   for( probeport = 0; ( probeport < numcards ) && ( *node == -1 ); probeport++ )
   {
      if( ( raw1394handle = raw1394_new_handle_on_port( probeport ) ) < 0 )
      {
	 TRACE( "Can't set port [%d]\n", probeport );
	 return -1;
      }
		
		
      for( probenode = 0; probenode < raw1394_get_nodecount( raw1394handle ); probenode++ )
      {
	 unsigned long long guid;
	 guid = get_guid( raw1394handle, probenode );
	 if( guid == sernum )
	 {
	    status = STATUS_SUCCESS;
	    *node = probenode;
	    *port = probeport;
	    TRACE( "device found @ port: %d\n", probeport );
	    break;
	 } else {
	 }
      }
      raw1394_destroy_handle( raw1394handle );
   }
   return status;
}

/*
  Watchdog thread which wakes raw1394_loop_iterate every 500ms to prevent blocking
*/
static void *_vid21394_timeout_thread( void *arg )
{
   timeout_data_t *timeout_data = ( timeout_data_t *) arg;
   raw1394handle_t raw1394handle = timeout_data->raw1394handle;
   int i = 0;
   while( timeout_data->quit == 0 )
   {
      usleep( 5000 );
      i++;
      if( i > 100 )
      {
	 raw1394_wake_up( raw1394handle );
	 i = 0;
      }
   }
   return 0;
}



/*
  Returns the firmware version of the converter

  Input: vid21394handle: handle to the device

  Output: firmware version or -1 on failure
*/
int vid21394_get_firm_vers( vid21394handle_t vid21394handle )
{
   unsigned long long fcp_get_firm_vers = GET_FIRM_VERS_MSG << 24;	
   int bit = GET_FIRM_VERS_MSG - 0x10;
   int result;

   result = _vid21394_send_fcp_command( vid21394handle, fcp_get_firm_vers, bit, NULL );
   if( SUCCESS( result ) )
   {
      return vid21394handle->firmware_version;
   } else {
      return -1;
   }
	
}

/*
  input: serial number of the device to open, 0 for first free device
  
  output: new handle to device, VID21394_INVALID_HANDLE on error

  error values: EBUSY: requested device already in use by this lib
  ENODEV: no device with this sernum or no free device
  ENOMEM: out of memory
  EAGAIN: raw1394 error, could not get resource
  ENOSYS: system lacks raw1394 support
  ENXIO:  The requested device was not found on the bus
*/
vid21394handle_t vid21394_open( unsigned long long sernum )/* raw1394handle_t handle, int node ) */
{
   raw1394handle_t raw1394handle = NULL;
   vid21394handle_t vid21394handle = (vid21394handle_t) malloc( sizeof( struct vid21394_handle ) );


   if( !vid21394handle )
   {
      return VID21394_INVALID_HANDLE_VALUE;
   }

   bzero( vid21394handle, sizeof( struct vid21394_handle ) );

   if( sernum )
   {
      int port;
      int node;
      if( !SUCCESS(_vid21394_find_device( sernum, &port, &node ) ) )
      {
	 raw1394_destroy_handle( raw1394handle );
	 free( vid21394handle );
	 return VID21394_INVALID_HANDLE_VALUE;
      }

      TRACE( "port: %d node: %d\n", port, node );

      raw1394handle = raw1394_new_handle_on_port( port );
      if( !raw1394handle )
      {
	 if( !errno )
	 {
	    TRACE( "This kernel lacks raw1394 support!\n" );
	    free( vid21394handle );
	    return VID21394_INVALID_HANDLE_VALUE;
	 } else {
	    TRACE( "Could not get raw1394 handle!\n" );
	    free( vid21394handle );
	    return VID21394_INVALID_HANDLE_VALUE;
	 }
      }

      TRACE( "Open device on port: %d, node: %d\n", port, node );

      vid21394handle->port = port;
      vid21394handle->node = node;
   }
   else // !sernum
   {
      free( vid21394handle );
      return VID21394_INVALID_HANDLE_VALUE;
   }

   raw1394_set_userdata( raw1394handle, vid21394handle );

   raw1394_set_bus_reset_handler( raw1394handle, _vid21394_busreset_handler );
	
   raw1394_set_fcp_handler( raw1394handle, _vid21394_fcp_handler );
   raw1394_start_fcp_listen( raw1394handle );

   ucutil_init_queue( &vid21394handle->queued_buffers );
   ucutil_init_queue( &vid21394handle->ready_buffers );
   vid21394handle->current_data_buffer = 0;

   vid21394handle->raw1394handle = raw1394handle;
   vid21394handle->serial_number = sernum;

   vid21394handle->device_present = 1;

   vid21394handle->vid21394handle = vid21394handle;
   vid21394handle->num_buffers = 2;
   vid21394handle->current_line_offset = 0;
   vid21394handle->current_line_length = 0; // indicates no video mode set

   vid21394handle->timeout_data.quit = 0;
   vid21394handle->timeout_data.raw1394handle = raw1394handle;
	
   if( pthread_create( &vid21394handle->timeout_thread, 
		       NULL, 
		       _vid21394_timeout_thread, 
		       &vid21394handle->timeout_data ) )
   {
      TRACE( "Failed to create timeout thread\n" );
   }
	
   vid21394_get_firm_vers( vid21394handle );

   return vid21394handle;
}

/*
  Close a device opened by vid21394_open

  frees bandwidth and channel
  
  Input: vid21394handle: handle to the device
*/
void vid21394_close( vid21394handle_t vid21394handle )
{
/* 	int    retval; */

   if( !vid21394handle )
   {
      return;
   }

   if( vid21394handle->timeout_thread )
   {
      vid21394handle->timeout_data.quit = 1;
      pthread_join( vid21394handle->timeout_thread, NULL );
      vid21394handle->timeout_thread = 0;
   }

   if( vid21394handle->bandwidth )
   {
      _1394util_free_bandwidth( vid21394handle->raw1394handle, vid21394handle->bandwidth );
      vid21394handle->bandwidth = 0;
   }
	
   if( vid21394handle->channel != -1 )
   {
      _1394util_free_channel( vid21394handle->raw1394handle, vid21394handle->channel );
      vid21394handle->channel = -1;
   }	

   if( vid21394handle->raw1394handle )
   {
      raw1394_destroy_handle( vid21394handle->raw1394handle );
      vid21394handle->raw1394handle = 0;
   }

   free( vid21394handle );
}



/*
  Start transmission of video data from the converter

  Input: vid21394handle: handle to the device
  
  Output: STATUS_SUCCESS: video mode set successfully
  STATUS_NO_VIDEO_MODE: no video mode was set prior to this call
  STATUS_NO_CHANNEL: no isochronous channel free on the 1394 bus
  STATUS_INSUFFICIENT_BANDWIDTH: not enough free bandwidth on the 1394 bus
  STATUS_FAILURE: other error
*/
int vid21394_start_transmit( vid21394handle_t vid21394handle )
{
   const static unsigned long ENABLE_TRANSMITTER = 0x100;
   unsigned long long fcp_ena_isoch_tx_command = ENA_ISOCH_TX_MSG << 24;

   int bit = ENA_ISOCH_TX_MSG - 0x10;
	
   int channel, bandwidth;

   if( !vid21394handle->current_line_length )
   {
      // no video mode set
      TRACE( "No video mode set!\n" );
      return STATUS_NO_VIDEO_MODE;
   }

   if( ( channel = _1394util_find_free_channel( vid21394handle->raw1394handle ) ) < 0 )
   {
      TRACE( "Failed to find a free channel!\n" );
      return STATUS_NO_CHANNEL;
   }
	
   // calculate bandwidth
   // bandwidth = 2 * linelength + CIP headers
   bandwidth = ( vid21394handle->current_line_length * 2 ) + 8;
/* 	if( ( status = _1394util_allocate_bandwidth( raw1394handle, bandwidth ) ) < 0 ) */
/* 	{ */
/* 		_1394util_free_channel( raw1394handle, channel ); */
/* 		return status; */
/* 	} */

   vid21394handle->channel = channel;
/* 	vid21394handle->bandwidth = bandwidth; */

   fcp_ena_isoch_tx_command = fcp_ena_isoch_tx_command | ENABLE_TRANSMITTER ;
   fcp_ena_isoch_tx_command = fcp_ena_isoch_tx_command | ( channel << 16 );	

/* 	fcp_ena_isoch_tx_command = prepare_fcp_command( fcp_ena_isoch_tx_command ); */

   TRACE( "vid21394: transmitting on channel: %d %016llx\n", channel, fcp_ena_isoch_tx_command );

   return( _vid21394_send_fcp_command( vid21394handle, fcp_ena_isoch_tx_command, bit, NULL ) );
}


/*
  Stop transmission of video data from the converter
  
  Input: vid21394handle: handle to the device

  Return: status
*/
int vid21394_stop_transmit( vid21394handle_t vid21394handle )
{
   const static unsigned long DISABLE_TRANSMITTER = 0x000;
   unsigned long long fcp_ena_isoch_tx_command = ENA_ISOCH_TX_MSG << 24;

   int bit = ENA_ISOCH_TX_MSG - 0x10;

   unsigned long channel = 0;	

   TRACE( "verify me: stop channel\n" );

   fcp_ena_isoch_tx_command = fcp_ena_isoch_tx_command | DISABLE_TRANSMITTER ;
   fcp_ena_isoch_tx_command = fcp_ena_isoch_tx_command | ( channel << 16 );	

   return( _vid21394_send_fcp_command( vid21394handle, fcp_ena_isoch_tx_command, bit, NULL ) );
}


/*
  Set input channel
  
  Input: vid21394handle: handle to the device
  channel: new input channel

  Returns: STATUS_SUCCESS
  STATUS_TIMEOUT
  STATUS_NO_DEVICE
  STATUS_FAILURE
*/
int vid21394_set_input_channel( vid21394handle_t vid21394handle, enum vid21394_input_channel channel )
{
   unsigned long long fcp_input_select = INPUT_SELECT_MSG << 24;	
   int bit = INPUT_SELECT_MSG - 0x10;

   fcp_input_select = fcp_input_select | ( channel << 16 );
   TRACE( "set input channel: %d\n", channel );
   return( _vid21394_send_fcp_command( vid21394handle, fcp_input_select, bit, NULL ) );
}

unicap_status_t vid21394_get_input_channel( vid21394handle_t vid21394handle, 
					    enum vid21394_input_channel *channel )
{
   unsigned long long fcp = CHK_VIDEO_LOCK_MSG << 24;
   int bit = CHK_VIDEO_LOCK_MSG - 0x10;
   unsigned long response;
   unicap_status_t status;
	
   status = _vid21394_send_fcp_command( vid21394handle, fcp, bit, &response );
   response = ( response >> 8 ) & 0xff;
	
   *channel = response;
	
	
   return status;
}


/*
  Set brightness level

  Input: vid21394handle
  brightness

  Returns: STATUS_SUCCESS
  STATUS_TIMEOUT
  STATUS_NO_DEVICE
  STATUS_FAILURE
  STATUS_INVALID_PARAMETER: brightness level not in valid range

*/
unicap_status_t vid21394_set_brightness( vid21394handle_t vid21394handle, unsigned int brightness )
{
   unsigned long long fcp_write_i2c = WRITE_I2C_BYTE_MSG << 24;	
   int bit = WRITE_I2C_BYTE_MSG - 0x10;

   fcp_write_i2c |= I2C_SAA7112_ID;
   fcp_write_i2c |= I2C_BRIGHTNESS;
   fcp_write_i2c |= brightness & 0xff;

   return( _vid21394_send_fcp_command( vid21394handle, fcp_write_i2c, bit, NULL ) );
}

unicap_status_t vid21394_get_brightness( vid21394handle_t vid21394handle, 
					 unsigned int *brightness )
{
   unsigned long long fcp = READ_I2C_BYTE_MSG << 24;
   int bit = READ_I2C_BYTE_MSG - 0x10;
   unicap_status_t status;
   unsigned long response;
	
   fcp |= I2C_SAA7112_ID | I2C_BRIGHTNESS;
	
   status = _vid21394_send_fcp_command( vid21394handle, fcp, bit, &response );
	
   *brightness = response & 0xff;
	
   return status;
}


/*
  Set video standard ( PAL = 50 Hz, NTSC = 60 Hz )

  Input: vid21394handle
  freq: VID21394_FREQ_50, VID21394_FREQ_60, VID21394_FREQ_AUTO

  Returns: STATUS_SUCCESS
  STATUS_TIMEOUT
  STATUS_NO_DEVICE
  STATUS_FAILURE
  STATUS_INVALID_PARAMETER

*/
unicap_status_t vid21394_set_frequency( vid21394handle_t vid21394handle, 
					enum vid21394_frequency freq )
{
   unsigned long long fcp_write_freq = SET_VIDEO_FREQUENCY_MSG << 24;
   int bit = SET_VIDEO_FREQUENCY_MSG - 0x10;
   fcp_write_freq |= freq <<16;

   return( _vid21394_send_fcp_command( vid21394handle, fcp_write_freq, bit, NULL ) );
}

unicap_status_t vid21394_get_frequency( vid21394handle_t vid21394handle, 
					enum vid21394_frequency *freq )
{
   unsigned long long fcp = CHK_VIDEO_LOCK_MSG << 24;
   int bit = CHK_VIDEO_LOCK_MSG - 0x10;
   unsigned long response;
   unicap_status_t status;
	
   status = _vid21394_send_fcp_command( vid21394handle, fcp, bit, &response );
   if( response & 2 )
   {
      *freq = VID21394_FREQ_60;
   }
   else
   {
      *freq = VID21394_FREQ_50;
   }

   return status;
}

/*
  Set byte order

   See Table 88, page 103
 */
unicap_status_t vid21394_set_byte_order( vid21394handle_t vid21394handle, 
					 vid21394_byte_order_t order )
{
   unsigned long long fcp_write_i2c = WRITE_I2C_BYTE_MSG << 24;	
   int bit = WRITE_I2C_BYTE_MSG - 0x10;

   fcp_write_i2c |= I2C_SAA7112_ID;
   fcp_write_i2c |= I2C_BYTE_ORDER;
   fcp_write_i2c |= ( order << 6 ) & 0xff;

   return( _vid21394_send_fcp_command( vid21394handle, fcp_write_i2c, bit, NULL ) );   
}


/*
  Set contrast level

  Input: vid21394handle
  contrast

  Returns: STATUS_SUCCESS
  STATUS_TIMEOUT
  STATUS_NO_DEVICE
  STATUS_FAILURE
  STATUS_INVALID_PARAMETER: contrast level not in valid range

*/
unicap_status_t vid21394_set_contrast( vid21394handle_t vid21394handle, 
				       unsigned int contrast )
{
   unsigned long long fcp_write_i2c = WRITE_I2C_BYTE_MSG << 24;	
   int bit = WRITE_I2C_BYTE_MSG - 0x10;

   fcp_write_i2c |= I2C_SAA7112_ID;
   fcp_write_i2c |= I2C_CONTRAST;
   fcp_write_i2c |= contrast & 0xff;

   return( _vid21394_send_fcp_command( vid21394handle, fcp_write_i2c, bit, NULL ) );
}

/*
  Get contrast level 

  Output: contrast : Current contrast level
*/
unicap_status_t vid21394_get_contrast( vid21394handle_t vid21394handle, 
				       unsigned int *contrast )
{
   unsigned long long fcp = READ_I2C_BYTE_MSG << 24;
   int bit = READ_I2C_BYTE_MSG - 0x10;
   unicap_status_t status;
   unsigned long response;
	
   fcp |= I2C_SAA7112_ID | I2C_CONTRAST;
	
   status = _vid21394_send_fcp_command( vid21394handle, fcp, bit, &response );
	
   *contrast = response & 0xff;
	
   return status;
}

/*
  Force the device to tag fields as odd/even even if the input only provides one field type

  Input: force : 1 to enable; 0 to disable
*/
unicap_status_t vid21394_set_force_odd_even( vid21394handle_t vid21394handle, 
					     unsigned int force )
{
   unsigned long long fcp_write_i2c = READ_I2C_BYTE_MSG << 24;	
	
   int bit = READ_I2C_BYTE_MSG - 0x10;
   unsigned long response;

   force = force?1:0;

   fcp_write_i2c |= I2C_SAA7112_ID;
   fcp_write_i2c |= I2C_SYNCCONTROL;

   _vid21394_send_fcp_command( vid21394handle, fcp_write_i2c, bit, &response );

   response &= ~(1<<5);
   response |= force<<5;

   bit = WRITE_I2C_BYTE_MSG - 0x10;
   fcp_write_i2c = WRITE_I2C_BYTE_MSG << 24;	
	
   fcp_write_i2c |= I2C_SAA7112_ID;
   fcp_write_i2c |= I2C_SYNCCONTROL;
   fcp_write_i2c |= response & 0xff;

   return( _vid21394_send_fcp_command( vid21394handle, fcp_write_i2c, bit, NULL ) );
}

/*
  Get current state of odd/even flag
*/
unicap_status_t vid21394_get_force_odd_even( vid21394handle_t vid21394handle, 
					     unsigned int *force )
{
   unsigned long long fcp = READ_I2C_BYTE_MSG << 24;	
	
   int bit = READ_I2C_BYTE_MSG - 0x10;
   unsigned long response;
   unicap_status_t status;

   fcp |= I2C_SAA7112_ID;
   fcp |= I2C_SYNCCONTROL;

   status = _vid21394_send_fcp_command( vid21394handle, fcp, bit, &response );

   *force = (response>>5)&1;
	
   return status;
}



unicap_status_t vid21394_force_s200( vid21394handle_t vid21394handle )
{
   unsigned long long fcp = READ_LINK_REG_MSG << 24;	
   int bit = READ_LINK_REG_MSG - 0x10;
   unsigned long response;
   unicap_status_t status;

   fcp |= 0x34 << 8; // ITXCTL register base address

   status = _vid21394_send_fcp_command( vid21394handle, fcp, bit, &response );
/* 	printf( "link reg1: %08x\n", response ); */
	
   fcp = WRITE_LINK_REG_MSG << 24;
   fcp |= 0x34 << 8; // ITXCTL register base address

	
   response &= ~0xf0; // Mask out SPD
   response |= 1<<4; // 0 == S100 , 1 == S200 , 2 == S400

   _vid21394_send_fcp_command_ext( vid21394handle, 
				   fcp, 
				   response, 
				   bit, 
				   NULL );

   fcp = READ_LINK_REG_MSG << 24;	
   fcp |= 0x34 << 8; // ITXCTL register base address

   status = _vid21394_send_fcp_command( vid21394handle, fcp, bit, &response );
/* 	printf( "link reg2: %08x\n", response ); */
	

   return status;
	
}

#if VID21394_BOOTLOAD
unicap_status_t vid21394_enter_bootload( vid21394handle_t vid21394handle )
{
   unsigned long long fcp = ENTER_BOOTLOAD_MSG << 24;
   int bit = READ_LINK_REG_MSG - 0x10;
   unsigned long response;
   unicap_status_t status;
	
   TRACE( "enter bootload\n" );

   status = _vid21394_send_fcp_command( vid21394handle, fcp, bit, &response );

   return status;
}
#endif

unicap_status_t vid21394_rs232_io( vid21394handle_t vid21394handle, 
				   unsigned char *out_data, 
				   int out_data_length,
				   unsigned char *in_data, 
				   int in_data_length )
{
   unsigned long long fcp = RS232_IO_MSG << 24;	
   int bit = RS232_IO_MSG - 0x10;
   fcp |= out_data_length << 8;
   fcp |= in_data_length;
	
/*    TRACE( "rs232io fcp: %08llx out_data_length: %d, in_data_length: %d\n", fcp, out_data_length, in_data_length ); */

   return( _vid21394_send_fcp_command_new( vid21394handle, fcp, bit, out_data, out_data_length, in_data, (unsigned int *)&in_data_length ) );
}

unicap_status_t vid21394_read_rs232( vid21394handle_t vid21394handle, 
				     unsigned char *data, 
				     int *datalen )
{
   unsigned long long fcp = RS232_IO_MSG << 24;	
   int bit = RS232_IO_MSG - 0x10;
   int offset = 0;
   unsigned long poll_len = 1;

   while( poll_len && ( *datalen > (offset + 4) ) )
   {
      int i;
		
      fcp = RS232_IO_MSG << 24;

      if( !SUCCESS( _vid21394_send_fcp_command_ext( vid21394handle, 
						    fcp, 
						    0, 
						    bit, 
						    &poll_len ) ) )
      {
	 TRACE( "failed\n" );
	 return STATUS_FAILURE;
      }
		
      usleep( 100 );
		
      if( poll_len )
      {
/* 			TRACE( "offset: %d *datalen: %d poll_len: %d\n", offset, *datalen, poll_len ); */
			
	 if( poll_len > 4 )
	 {
/* 			TRACE( "len > 4!!!!\n\n\n\n\n\n\n\n\n\n\n\n" ); */
	    return STATUS_FAILURE;
	 }

	 for( i = 0; i < poll_len; i++ )
	 {
	    *(data+offset+i) = vid21394handle->fcp_ext_data & 0xff;
	    vid21394handle->fcp_ext_data = vid21394handle->fcp_ext_data>>8;
	 }
	 offset += poll_len;
      }
   }
   *datalen = offset;
	
   return STATUS_SUCCESS;
}

unicap_status_t vid21394_rs232_set_baudrate( vid21394handle_t vid21394handle, 
					     int rate )
{
   unsigned long long fcp = RS232_CONFIG_MSG << 24;	
   int bit = RS232_CONFIG_MSG - 0x10;
   unsigned long fcp_data = 0;
   fcp |= 1<<8;

   TRACE( "SET BAUD RATE\n" );
	
   switch( rate )
   {
      case 2400: 
	 fcp_data = 1<<24;
	 break;
      case 4800:
	 fcp_data = 2<<24;
	 break;
      case 9600:
	 fcp_data = 3<<24;
	 break;
      case 19200:
	 fcp_data = 4<<24;
	 break;
      case 38400:
	 fcp_data = 5<<24;
	 break;
   }
	

   return( _vid21394_send_fcp_command_ext( vid21394handle, fcp, fcp_data, bit, NULL ) );
	
}

unicap_status_t vid21394_set_link_speed( vid21394handle_t vid21394handle, 
					 int speed )
{
   unsigned long long fcp = LINK_SPEED_MSG << 24;	
   int bit = LINK_SPEED_MSG - 0x10;
   fcp |= 1<<8;

   TRACE( "SET SPEED\n" );

   fcp |= speed;	

   return( _vid21394_send_fcp_command( vid21394handle, fcp, bit, NULL ) );
}



/*
  Set video mode

  Input: vid21394handle
  video_mode: 
  VID21394_Y_320x240   
  VID21394_Y41P_320x240
  VID21394_UYVY_320x240
  VID21394_Y_640x480   
  VID21394_Y41P_640x480
  VID21394_UYVY_640x480
  VID21394_Y_768x576   
  VID21394_Y41P_768x576
  VID21394_UYVY_768x576
		
  Returns: STATUS_SUCCESS
  STATUS_TIMEOUT
  STATUS_NO_DEVICE
  STATUS_FAILURE
  STATUS_INVALID_PARAMETER: unknown mode

*/
unicap_status_t vid21394_set_video_mode( vid21394handle_t vid21394handle, 
					 enum vid21394_video_mode video_mode )
{
   unsigned long long fcp_set_video_mode = SET_VIDEO_MODE_MSG << 24;
   int status = STATUS_FAILURE;
   int bit = SET_VIDEO_MODE_MSG - 0x10;
   int result;

   fcp_set_video_mode = fcp_set_video_mode | ( ( video_mode & 0xff ) <<16);

   result = _vid21394_send_fcp_command( vid21394handle, fcp_set_video_mode, bit, NULL );
   if( !SUCCESS( result ) )
   {
      TRACE( "set_video mode failed!\n" );
      return result;
   }

   if( video_mode & 0xff00 )
   {
      vid21394_set_byte_order( vid21394handle, VID21394_BYTE_ORDER_YUYV );
   }
   else
   {
      vid21394_set_byte_order( vid21394handle, VID21394_BYTE_ORDER_UYVY );
   }

   vid21394handle->current_offset = 0;
   vid21394handle->current_field = 0;
   vid21394handle->current_line_offset = 0;
   vid21394handle->current_line_length = vid21394_video_mode_line_lengths[ video_mode & 0xff ];
   vid21394handle->current_buffer_size = vid21394_video_mode_sizes[ video_mode & 0xff ];

   vid21394handle->current_line_to_copy = vid21394handle->current_line_length;
	   
   vid21394handle->current_bytes_copied = 0;

   vid21394handle->copy_done = 0;
   vid21394handle->start_copy = 0;	   
	
   TRACE( "set video_mode: %d\n", video_mode );
   vid21394handle->video_mode = video_mode;

   status = STATUS_SUCCESS;
   return status;
}

/*
  Queue a buffer to get filled by the device. 
  Queued buffers should not be touched until returned by vid21394_wait_buffer() ( especially they must not be freed ).
  Queued buffers can be dequeued one after the other with vid21394_dequeue_buffer() ( reception needs to be stopped first )

  Input: vid21394handle
  buffer: buffer for the image, needs to be large enough to store complete frame
*/
void vid21394_queue_buffer( vid21394handle_t vid21394handle, void * buffer )
{
   struct _unicap_queue *entry;
	
   entry = malloc( sizeof( struct _unicap_queue ) );
   entry->data = buffer;
   
   ucutil_insert_back_queue( &vid21394handle->queued_buffers, entry );
}

/*
  Removes the first buffer from the queue. Can only be called if reception is stopped! 
  Once queued with vid21394_queue_buffer, only free buffers returned by vid21394_dequeue_buffer() or vid2134_wait_buffer()!
  
  Input: vid21394handle

  Returns: pointer to the buffer or NULL if no buffer was dequeued
*/
void *vid21394_dequeue_buffer( vid21394handle_t vid21394handle )
{
   void *retval;
	
   if( vid21394handle->is_receiving )
   {
      // device has to be stopped first
      return NULL;
   }

   retval = ucutil_get_front_queue( &vid21394handle->queued_buffers )->data;
	
   return retval;
}

/*
  Wait until one buffer got filled by the device. 
  May return immediately ( if a buffer already was filled by the device ) 
  or block until the buffer got filled.
*/
int vid21394_wait_buffer( vid21394handle_t vid21394handle, void **buffer )
{
   int retval = STATUS_SUCCESS;
	
   if( !vid21394handle->ready_buffers.next && !vid21394handle->is_receiving )
   {
      return STATUS_IS_STOPPED;
   }
	
   if( !vid21394handle->ready_buffers.next && !vid21394handle->device_present )
   {
      return STATUS_NO_DEVICE;
   }
	
   if( vid21394handle->ready_buffers.next )
   {
      unicap_queue_t *first_entry;
      // return immediately if a buffer is already in the ready queue
      first_entry = ucutil_get_front_queue( &vid21394handle->ready_buffers );
      *buffer = first_entry->data;
      free( first_entry );
   }
   else
   {
      if( vid21394handle->queued_buffers.next )
      {
	 //block until a buffer got filled by the device
	 unicap_queue_t *first_entry;
	 struct timeval timeout_time, cur_time;
			
	 if( gettimeofday( &timeout_time, NULL ) < 0 )
	 {
	    return STATUS_FAILURE;
	 }
			
	 timeout_time.tv_sec += 1; // 1 second timeout
	 while( !vid21394handle->ready_buffers.next )
	 {
	    // raw1394_loop_iterate is waked regularly by a seperate thread ( every 500ms )
	    if( gettimeofday( &cur_time, NULL ) < 0 )
	    {
	       return STATUS_FAILURE;
	    }
				
	    if( timercmp( &cur_time, &timeout_time, > ) )
	    {	
	       return STATUS_TIMEOUT;
	    }
/*
            // dcm: don't enter into loop_iterate since, it can block even if a frame arrives
	    raw1394_loop_iterate( vid21394handle->raw1394handle );
*/
            // 1 millisecond
            struct timespec delay = {0, 1000000};
	    nanosleep(&delay, NULL);
	 }
	 first_entry = ucutil_get_front_queue( &vid21394handle->ready_buffers );
	 *buffer = first_entry->data;
			
	 free( first_entry );
      }
      else
      {
	 // no buffers queued -> no buffers to return
	 return STATUS_NO_BUFFERS;
      }
   }
	

   return retval;
}



/*
  returns the number of completed buffers
*/
int vid21394_poll_buffer( vid21394handle_t vid21394handle )
{
   int buffers = 0;
/*    unicap_queue_t *entry = &vid21394handle->ready_buffers; */

/*    while( entry->next ) */
/*    { */
/*       buffers++; */
/*       entry = entry->next; */
/*    } */

   return ucutil_queue_get_size( &vid21394handle->ready_buffers );
}		

void _vid21394_add_to_queue( struct _unicap_queue *queue, struct _unicap_queue *entry )
{
   ucutil_insert_back_queue( queue, entry );
}

#define START_COPY_ON_NEXT_SY    0
#define CONTINUE_COPY_DURING_SY  1
#define COPY_NOSY                2
#define SKIP_TO_NEXT_SY          3

/*
  Iso callback

  Weaves one odd and one even field into a frame
*/
enum raw1394_iso_disposition 
_vid21394_new_iso_handler( raw1394handle_t handle, 
			   unsigned char *data, 
			   unsigned int length, 
			   unsigned char channel, 
			   unsigned char tag, 
			   unsigned char sy, 
			   unsigned int cycle, 
			   unsigned int dropped ) 
{
   vid21394handle_t vid21394handle = raw1394_get_userdata( handle );
   unsigned int field_type;
   int remaining_length;

#ifdef DEBUG
   static unsigned int last_cycle = -1;

   if( last_cycle != -1 && ( ( ( last_cycle + 1 ) % 8000 ) != cycle ) )
   {
      TRACE( "packet discontinuity: %u, last:%u\n", cycle, last_cycle );
      last_cycle = cycle;
   }

   vid21394handle->nr_pkts += 1;
#endif

   /* bail out on empty or bogus/corrupt packets */
   if( ( length <= 8 ) ||
       ( length >4096 ) ) 
   {
#ifdef DEBUG
      if( length > 4096 )
      {
	 TRACE( "invalid length in iso callback!\n" );
      }
#endif
      return 0;
   }

   /* 
      Check for bogus values
   */
   if( ( ( sy != 0 ) && ( sy != 1 ) && ( sy != 4 ) ) ||
       ( channel != vid21394handle->channel ) || 
       ( !data ) )
   {
      TRACE( "corrupt packet in iso callback!\n" );
      return 0;
   }

   if( ( vid21394handle->firmware_version & 0xff00 ) < 0x200 )
   {
      field_type = vid21394handle->last_field_type == 0 ? 0:1;
   } else {
      field_type = *(data+7) == 4 ? 0:1;
   }
   
   if( sy == 1 )
   {
      switch( vid21394handle->start_copy )
      {			

	 case COPY_NOSY:
	 { 
	    // SY field is set while we where still waiting for data to finish a field
	    // => Not enough data transmitted
	    vid21394handle->start_copy = 0;
	    vid21394handle->nr_pkts = 0;
	    //start over with this field
	 }
/* 			break; */
			
	 case START_COPY_ON_NEXT_SY:
	 {
	    // A new field starts here
	    if( !vid21394handle->current_data_buffer )
	    {
	       if( vid21394handle->current_format.buffer_type == UNICAP_BUFFER_TYPE_USER )
	       {
		  vid21394handle->current_data_buffer = 
		     ucutil_get_front_queue( &vid21394handle->queued_buffers );
		  if( !vid21394handle->current_data_buffer )
		  {
		     TRACE( "No buffer!\n" );
		     vid21394handle->start_copy = SKIP_TO_NEXT_SY;
		     return 0;
		  }
	       }
	       else
	       {
		  vid21394handle->current_data_buffer = & vid21394handle->system_buffer_entry;
	       }
	    }
#ifdef DEBUG
	    vid21394handle->nr_pkts = 0;
#endif
	    vid21394handle->current_offset = 0;
	    vid21394handle->current_line_to_copy = vid21394handle->current_line_length;
	    vid21394handle->current_offset += field_type * vid21394handle->current_line_length;
	    vid21394handle->start_copy = 1;
	 }
	 break;
		
	 case SKIP_TO_NEXT_SY:
	 {
	    return 0;
	 }
	 break;
		
	 case CONTINUE_COPY_DURING_SY:
	 {
	 }
	 break;
		
	 default:
	 {
/* 			TRACE( "Unknown state while synching to the next field!\n" ); */
	    return 0;
	 }
	 break;
      }
		
   }
   else
   {
      vid21394handle->last_field_type = sy;
      if( vid21394handle->start_copy == START_COPY_ON_NEXT_SY )
      {
	 return 0;
      }
      else if( vid21394handle->start_copy == SKIP_TO_NEXT_SY )
      {
	 vid21394handle->start_copy = START_COPY_ON_NEXT_SY;
	 return 0;
      } else {
	 vid21394handle->start_copy = COPY_NOSY;
      }
   }

   remaining_length = ( length - 8 );

   // This copy routine weaves two fields into a frame
   if( ( vid21394handle->current_bytes_copied + remaining_length ) 
       <= (vid21394handle->current_buffer_size/2) )
   {
      int tmplen = 0;
      int tmpoffset = 0;

      while( remaining_length )
      {
	 
	 if( remaining_length >= vid21394handle->current_line_to_copy )
	 {
	    tmplen = vid21394handle->current_line_to_copy;
	 }
	 else
	 {
	    tmplen = remaining_length;
	 }
	 memcpy( ((char*)vid21394handle->current_data_buffer->data) + 
		 vid21394handle->current_offset, 
		 (data+8) + tmpoffset, 
		 tmplen );

	 vid21394handle->current_offset += tmplen;
	 tmpoffset += tmplen;
	 remaining_length -= tmplen;
	 vid21394handle->current_bytes_copied += tmplen;
		   
	 vid21394handle->current_line_to_copy -= tmplen;

	 if( vid21394handle->current_line_to_copy <= 0 )
	 {
	    vid21394handle->current_line_to_copy = vid21394handle->current_line_length;
	    vid21394handle->current_offset += 
	       vid21394handle->current_line_length; // skip one line
	 }
      }
	   
      if( vid21394handle->current_bytes_copied == (vid21394handle->current_buffer_size/2) )
      {
	 // complete field got copied

	 if( field_type == 0 )
	 {
	    vid21394handle->copied_field_0 = 1;
	 }
	 else
	 {
	    vid21394handle->copied_field_1 = 1;
	 }
		   
			
	 if( ( vid21394handle->copied_field_0 ) &&
	     ( vid21394handle->copied_field_1 ) )
	 {
	    // complete frame ( 2 fields ) got copied
	    unicap_data_buffer_t data_buffer;
	    unicap_copy_format( &data_buffer.format, &vid21394handle->current_format );
	    data_buffer.buffer_size = data_buffer.format.buffer_size;
	    data_buffer.data = vid21394handle->current_data_buffer->data;
	    gettimeofday( &data_buffer.fill_time, NULL );

	    if( field_type != 0 ){
	       data_buffer.flags = UNICAP_FLAGS_BUFFER_TYPE_SYSTEM | UNICAP_FLAGS_BUFFER_INTERLACED | UNICAP_FLAGS_BUFFER_ODD_FIELD_FIRST;
	       unicap_data_buffer_t *b;
	       b = ( unicap_data_buffer_t * )vid21394handle->current_data_buffer->data;
	       if( b )
		  b->flags |= UNICAP_FLAGS_BUFFER_INTERLACED | UNICAP_FLAGS_BUFFER_ODD_FIELD_FIRST;
	    } else {
	       data_buffer.flags = UNICAP_FLAGS_BUFFER_TYPE_SYSTEM | UNICAP_FLAGS_BUFFER_INTERLACED;
	       unicap_data_buffer_t *b;
	       b = ( unicap_data_buffer_t * )vid21394handle->current_data_buffer->data;
	       if( b ){
		  b->flags |= UNICAP_FLAGS_BUFFER_INTERLACED;
		  b->flags &= ~UNICAP_FLAGS_BUFFER_ODD_FIELD_FIRST;
	       }
	    }

	    vid21394handle->event_callback( vid21394handle->unicap_handle, UNICAP_EVENT_NEW_FRAME, &data_buffer );
	    
	    if( vid21394handle->current_format.buffer_type == UNICAP_BUFFER_TYPE_USER )
	    {
	       ucutil_insert_back_queue( &vid21394handle->ready_buffers,
				   vid21394handle->current_data_buffer );
	    }
	    vid21394handle->current_data_buffer = 0;
	    vid21394handle->copied_field_0 = vid21394handle->copied_field_1 = 0;
	 }
	 vid21394handle->start_copy = 0;
	 vid21394handle->current_bytes_copied = 0;
      }
      else if( vid21394handle->current_bytes_copied > vid21394handle->current_buffer_size/2 )
      {
/* 			TRACE( "Tried to copy too much data: %d\n", vid21394handle->current_offset ); */
      }
   }
   else
   {
/* 		TRACE( "Out of bounds\n" ); */
      vid21394handle->start_copy = START_COPY_ON_NEXT_SY;
      vid21394handle->copied_field_0 = vid21394handle->copied_field_1 = 0;
      vid21394handle->current_bytes_copied = 0;		
   }
   
   return 0;
}


void * vid21394_capture_thread( vid21394handle_t vid21394handle )
{
   raw1394handle_t raw1394handle = raw1394_new_handle_on_port( vid21394handle->port );
   unicap_status_t status = STATUS_SUCCESS;
   pthread_t wakeup_thread;
   timeout_data_t timeout_data;
   
   if( 0 > raw1394_iso_recv_init( raw1394handle,
				  _vid21394_new_iso_handler,
				  2000,
				  3100,
				  vid21394handle->channel,
#if RAW1394_1_1_API
				  RAW1394_DMA_PACKET_PER_BUFFER,
#endif
				  100 ) )
   {
      TRACE( "iso_recv_init failed: %s\n", strerror(errno) );
      raw1394_destroy_handle( raw1394handle );
      return NULL;
   }

   if( 0 > raw1394_iso_recv_start( raw1394handle, -1, -1, 0 ) )
   {
      TRACE( "iso_recv_start failed\n" );
      raw1394_destroy_handle( raw1394handle );
      return NULL;
   }

   vid21394handle->is_receiving = SUCCESS( status );

   timeout_data.raw1394handle = raw1394handle;
   timeout_data.quit = 0;

   if( pthread_create( &wakeup_thread, NULL, (void*(*)(void*))_vid21394_timeout_thread, (void*)&timeout_data ) )
   {
      perror( "create wakeup thread\n" );
   }
	
   raw1394_set_userdata( raw1394handle, vid21394handle );

   while( !vid21394handle->stop_capture )
   {
      raw1394_loop_iterate( raw1394handle );
   }
   
   timeout_data.quit = 1;
   pthread_join( wakeup_thread, NULL );

   vid21394handle->is_receiving = 0;
   raw1394_iso_stop( raw1394handle );
   raw1394_iso_shutdown( raw1394handle );
   if( vid21394handle->bandwidth )
   {
      _1394util_free_bandwidth( raw1394handle, vid21394handle->bandwidth );
      vid21394handle->bandwidth = 0;
   }
	
   if( vid21394handle->channel )
   {
      _1394util_free_channel( raw1394handle, vid21394handle->channel );
      vid21394handle->channel = -1;
   }

   raw1394_destroy_handle( raw1394handle );
   
   return NULL;
}



/*
  vid21394_start_receive

  start reception of the isochronous stream
  channel has to be allocated and set in the handle
*/
int vid21394_start_receive( vid21394handle_t vid21394handle )
{
   raw1394handle_t raw1394handle = vid21394handle->raw1394handle;
   unicap_status_t status = STATUS_SUCCESS;
	
   TRACE( "recv init channel: %d\n", vid21394handle->channel );
	
   vid21394handle->timeout_data.capture_running = 1;
	
   if( 0 > raw1394_iso_recv_init( raw1394handle,
				  _vid21394_new_iso_handler,
				  2000,
				  3100,
				  vid21394handle->channel,
#if RAW1394_1_1_API
				  RAW1394_DMA_PACKET_PER_BUFFER,
#endif
				  100 ) )
   {
      TRACE( "iso_recv_init failed: %s\n", strerror(errno) );
      return STATUS_FAILURE;
   }

   if( 0 > raw1394_iso_recv_start( raw1394handle, -1, -1, -1 ) )
   {
      TRACE( "iso_recv_start failed\n" );
      return STATUS_FAILURE;
   }

   vid21394handle->is_receiving = SUCCESS( status );
	
   return status;
}

/*
  stops reception of isochronous data, frees allocated bandwidth and channel
*/
int vid21394_stop_receive( vid21394handle_t vid21394handle )
{
   vid21394handle->is_receiving = 0;
   vid21394handle->timeout_data.capture_running = 0;
   raw1394_iso_stop( vid21394handle->raw1394handle );
   raw1394_iso_shutdown( vid21394handle->raw1394handle );
   if( vid21394handle->bandwidth )
   {
      _1394util_free_bandwidth( vid21394handle->raw1394handle, vid21394handle->bandwidth );
      vid21394handle->bandwidth = 0;
   }
	
   if( vid21394handle->channel )
   {
      _1394util_free_channel( vid21394handle->raw1394handle, vid21394handle->channel );
      vid21394handle->channel = -1;
   }

   return STATUS_SUCCESS;
}

