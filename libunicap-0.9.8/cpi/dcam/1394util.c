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
#include <netinet/in.h> // for ntohl
#include <errno.h>
#ifdef RAW1394_1_1_API
#include <libraw1394/raw1394.h>
#include <libraw1394/csr.h>
#endif
#ifdef RAW1394_1_0_API
#include <raw1394.h>
#include <csr.h>
#endif

#include <unistd.h>
#include <stdio.h>
#include <errno.h>

#include "1394util.h"
#include <unicap_status.h>
#if DCAM_DEBUG
#define DEBUG
#endif
#include <debug.h>


#define MAXTRIES 20
#define DELAY 1000000

#define EXTCODE_COMPARE_SWAP     0x2

#define MAX_BANDWIDTH            0x1333 // Check this! Should be 0x1000 ???

int cooked1394_read( raw1394handle_t handle, 
		     nodeid_t node, 
		     nodeaddr_t addr,
		     size_t length, 
		     quadlet_t *buffer ) 
{
	int retval, i;
	for( i=0; i < MAXTRIES; i++ ) 
	{
		retval = raw1394_read( handle, node, addr, length, buffer );
		if( retval >= 0 )
		{
		   return retval;	/* Everything is OK */
		}
		
		if( errno != EAGAIN )
		{
		   break;
		}
		
/* 		usleep(DELAY); */
	}
#ifdef DEBUG
	perror("Error while reading from IEEE1394: ");
#endif
	return retval;
}

int cooked1394_write( raw1394handle_t handle, 
		      nodeid_t node, 
		      nodeaddr_t addr,
		      size_t length, 
		      quadlet_t *data) 
{
	int retval, i;
	for( i=0; i < MAXTRIES; i++ ) 
	{
/* 		usleep( DELAY ); */
		sleep( 1 );
		retval = raw1394_write( handle, node, addr, length, data );
		if( retval >= 0 )
		{
		   return retval;	/* Everything is OK */
		}
		
		if( errno != EAGAIN ){ 
		   break;
		}
		
	}
#ifdef DEBUG
	perror( "Error while writing to IEEE1394: " );
#endif
	return retval;
}

/*
 * Obtain the global unique identifier of a node from its configuration ROM.
 * The GUID can also be read from a filled out Rom_info structure, but this
 * method is of course faster than reading the whole configuration ROM and can
 * for instance be used to obtain a hash key for caching Rom_info structures
 * in memory.
 * IN:  phyID:	Physical ID of the node to read from
 *      hi:	Pointer to an integer which should receive the HI quadlet
 *      hi:	Pointer to an integer which should receive the LOW quadlet
 */
unsigned long long get_guid(raw1394handle_t handle, int phyID ) 
{
	
	unsigned int hiquad;
	unsigned int loquad;

	unsigned long long guid = 0;

	if ( cooked1394_read( handle, 
			      0xffC0 | phyID, 
			      CSR_REGISTER_BASE + CSR_CONFIG_ROM + 0x0C, 
			      4, 
			      &hiquad ) < 0 ) 
	{ 
		return 0; 
	}

	if ( cooked1394_read( handle, 
			      0xffC0 | phyID, 
			      CSR_REGISTER_BASE + CSR_CONFIG_ROM + 0x10, 
			      4, 
			      &loquad ) < 0 ) 
	{ 
		return 0; 
	}

	hiquad = ntohl( hiquad );
	loquad = ntohl( loquad );

	guid = ( ( unsigned long long ) hiquad ) << 32;
	guid += loquad;
	
	return guid;
}

unsigned int get_unit_spec_ID( raw1394handle_t handle, int phyID )
{
	unsigned int unit_dir_offset;
	unsigned int spec_ID;
	
	if( cooked1394_read( handle, 
			     0xffc0 | phyID, 
			     CSR_REGISTER_BASE + CSR_CONFIG_ROM + 0x24, 
			     4, 
			     &unit_dir_offset ) < 0 )
	{
		TRACE( "failed to read unit_dir _offset\n" );
		return 0;
	}
	
	unit_dir_offset = ntohl( unit_dir_offset );
	unit_dir_offset &= 0x00ffffff;

	TRACE( "unit_dir_offset: %x\n", unit_dir_offset );
	
	if( cooked1394_read( handle, 
			     0xffc0 | phyID, 
			     CSR_REGISTER_BASE + CSR_CONFIG_ROM + 0x24 + unit_dir_offset + 0x8, 
			     4, 
			     &spec_ID ) < 0 )
	{
		return 0;
	}
	
	spec_ID = ntohl( spec_ID );
	spec_ID &= 0x00ffffff;
	
	return spec_ID;
}

unsigned int get_unit_sw_version( raw1394handle_t handle, int phyID )
{
	unsigned int unit_dir_offset;
	unsigned int sw_version;
	
	if( cooked1394_read( handle, 
			     0xffc0 | phyID, 
			     CSR_REGISTER_BASE + CSR_CONFIG_ROM + 0x24, 
			     4, 
			     &unit_dir_offset ) < 0 )
	{
		TRACE( "failed to read unit_dir _offset\n" );
		return 0;
	}
	
	unit_dir_offset = ntohl( unit_dir_offset );
	unit_dir_offset &= 0x00ffffff;

	TRACE( "unit_dir_offset: %x\n", unit_dir_offset );
	
	if( cooked1394_read( handle, 
			     0xffc0 | phyID, 
			     CSR_REGISTER_BASE + CSR_CONFIG_ROM + 0x24 + unit_dir_offset + 0xc, 
			     4, 
			     &sw_version ) < 0 )
	{
		return 0;
	}
	
	sw_version = ntohl( sw_version );
	sw_version &= 0x00ffffff;
	
	return sw_version;
}


/*
  allocate the first free channel for isochronous transmission using compare & swap
  
  returns: < 0 if channel could not be allocated
           or the allocated channel
 */
int _1394util_find_free_channel( raw1394handle_t raw1394handle )
{
	quadlet_t buffer;
	int channel = -1;
	
	if( 0 > cooked1394_read( raw1394handle, 
				 raw1394_get_irm_id( raw1394handle ), 
				 CSR_REGISTER_BASE + CSR_CHANNELS_AVAILABLE_LO, 
				 sizeof( quadlet_t ), 
				 &buffer ) )
	{
		TRACE( "Failed to get CSR_CHANNELS_AVAILABLE_LO\n" );
		return -1;
	}

	buffer = ntohl( buffer );

	for( channel = 0; ( !( buffer & ( 1 << channel ) ) ) && channel < 32; channel++ )
	{
	}

	
	if( channel > 31 ) // none found in the lower range
	{
		if( 0 > cooked1394_read( raw1394handle, 
					 raw1394_get_irm_id( raw1394handle ), 
					 CSR_REGISTER_BASE + CSR_CHANNELS_AVAILABLE_HI, 
					 sizeof( quadlet_t ), 
					 &buffer ) )
		{
			TRACE( "Failed to get CSR_CHANNELS_AVAILABLE_HI\n" );
			return -1;
		}
		
		buffer = ntohl( buffer );
		for( channel = 0; ( !( buffer & ( 1<<channel ) ) ) && ( channel < 32 ); channel++ )
		{
		}
		channel+=32;
	}//if( channel > 31
	
	if( channel > 63 )
	{
		TRACE( "No free channel found\n" );
		return -1;
	}
	else
	{
		quadlet_t data, arg, result;
		arg   = htonl( buffer );
		data  = htonl( buffer & ~( 1<<channel ) );


		if( 0 > raw1394_lock( raw1394handle, 
				      raw1394_get_irm_id( raw1394handle ), 
				      channel < 32 ? CSR_REGISTER_BASE + CSR_CHANNELS_AVAILABLE_LO : CSR_REGISTER_BASE + CSR_CHANNELS_AVAILABLE_HI, 
				      EXTCODE_COMPARE_SWAP, 
				      data, 
				      arg, 
				      &result ) )
		{
			TRACE( "Channel allocation failed\n" );
			return -1;
		}		
		
// is the following thread safe? I do not think so!

		if( 0 > cooked1394_read( raw1394handle, raw1394_get_irm_id( raw1394handle ), 
					 channel < 32 ? CSR_REGISTER_BASE + CSR_CHANNELS_AVAILABLE_LO : CSR_REGISTER_BASE + CSR_CHANNELS_AVAILABLE_HI, 
					 sizeof( quadlet_t ), 
					 &buffer ) )
		{
			TRACE( "Failed to read back available channels\n" );
			return -1;
		}
		if( buffer != data )
		{
			TRACE( "Validation of channel failed\n" );
			return -1;
		}
	}
	
	return channel;
	
}


/*
  free a channel
 */
unicap_status_t _1394util_free_channel( raw1394handle_t raw1394handle, int channel )
{
	quadlet_t buffer;
	
	if( 0 <= cooked1394_read( raw1394handle, raw1394_get_irm_id( raw1394handle ),
				  channel < 32 ? CSR_REGISTER_BASE + CSR_CHANNELS_AVAILABLE_LO : CSR_REGISTER_BASE + CSR_CHANNELS_AVAILABLE_HI,
				  sizeof( quadlet_t ),
				  &buffer ) )
	{
		int channel_bit = channel > 31 ? channel - 32 : channel;
		quadlet_t data, arg, result;

		buffer = htonl( buffer );

		if( buffer & ( 1<<channel_bit ) )
		{ // channel already marked as free
			TRACE( "Trying to free an unallocated channel\n" );
			TRACE( "channels: %x\n", buffer );
			return STATUS_CHANNEL_ALREADY_FREE;
		}

		arg   = htonl( buffer );
		data  = htonl( buffer | ( 1<<channel_bit ) );
		if( 0 > raw1394_lock( raw1394handle, 
				      raw1394_get_irm_id( raw1394handle ), 
				      channel < 32 ? CSR_REGISTER_BASE + CSR_CHANNELS_AVAILABLE_LO : CSR_REGISTER_BASE + CSR_CHANNELS_AVAILABLE_HI, 
				      EXTCODE_COMPARE_SWAP, 
				      data, 
				      arg, 
				      &result ) )
		{
			TRACE( "Failed to free channel\n" );
			return STATUS_FAILURE;
		}		
		
		if( arg != htonl( buffer ) )
		{
			TRACE( "Free channel validation failed\n" );
			return STATUS_FAILURE;
		}
		

		return STATUS_SUCCESS;
	}

	TRACE( "Read failed\n" );
	return STATUS_FAILURE;
}

/*
  try to allocate channel on the bus
  returns: status
 */
unicap_status_t _1394util_allocate_channel( raw1394handle_t raw1394handle, int channel )
{
	quadlet_t buffer;
	
	if( 0 > cooked1394_read( raw1394handle, 
				 raw1394_get_irm_id( raw1394handle ), 
				 CSR_REGISTER_BASE + CSR_CHANNELS_AVAILABLE_LO, 
				 sizeof( quadlet_t ), 
				 &buffer ) )
	{
		TRACE( "Failed to get CSR_CHANNELS_AVAILABLE_LO\n" );
		return STATUS_FAILURE;
	}

	buffer = ntohl( buffer );

	
	if( channel > 31 ) // none found in the lower range
	{
		if( 0 > cooked1394_read( raw1394handle, 
					 raw1394_get_irm_id( raw1394handle ), 
					 CSR_REGISTER_BASE + CSR_CHANNELS_AVAILABLE_HI, 
					 sizeof( quadlet_t ), 
					 &buffer ) )
		{
			TRACE( "Failed to get CSR_CHANNELS_AVAILABLE_HI\n" );
			return -1;
		}
		
		buffer = ntohl( buffer );
	}//if( channel > 31
	
	if( channel > 63 )
	{
		TRACE( "Channel number too high\n" );
		return STATUS_INVALID_PARAMETER;
	}
	else
	{
		quadlet_t data, arg, result;
		arg   = htonl( buffer );
		data  = htonl( buffer & ~( 1<<channel ) );


		if( 0 > raw1394_lock( raw1394handle, 
				      raw1394_get_irm_id( raw1394handle ), 
				      channel < 32 ? CSR_REGISTER_BASE + CSR_CHANNELS_AVAILABLE_LO : CSR_REGISTER_BASE + CSR_CHANNELS_AVAILABLE_HI, 
				      EXTCODE_COMPARE_SWAP, 
				      data, 
				      arg, 
				      &result ) )
		{
			TRACE( "Channel allocation failed\n" );
			return STATUS_FAILURE;
		}		
		
		if( arg != buffer )
		{
			TRACE( "Allocate channel validation failed\n" );
			return STATUS_FAILURE;
		}
	}
	
	return STATUS_SUCCESS;
}

unicap_status_t _1394util_allocate_bandwidth( raw1394handle_t raw1394handle, int bandwidth )
{
	quadlet_t buffer;
	
	if( 0 > cooked1394_read( raw1394handle, 
				 raw1394_get_irm_id( raw1394handle ), 
				 CSR_REGISTER_BASE + CSR_BANDWIDTH_AVAILABLE, 
				 sizeof( quadlet_t ), 
				 &buffer ) )
	{
		TRACE( "Failed to get CSR_BANDWIDTH_AVAILABLE\n" );
		return STATUS_FAILURE;
	}

	buffer = ntohl( buffer );


	if( (int)(buffer - bandwidth) < 0 )
	{
		TRACE( "Insufficient bandwidth\n" );
		return STATUS_FAILURE;
	}
	else
	{
		quadlet_t data, arg, result;
		
		arg = htonl( buffer );
		data = htonl( buffer - bandwidth );
		
		if( 0 > raw1394_lock( raw1394handle, 
				      raw1394_get_irm_id( raw1394handle ), 
				      CSR_REGISTER_BASE + CSR_BANDWIDTH_AVAILABLE,
				      EXTCODE_COMPARE_SWAP, 
				      data, 
				      arg, 
				      &result ) )
		{
			TRACE( "Failed to allocate bandwidth\n" );
			return STATUS_FAILURE;
		}

		if( arg != htonl( buffer ) )
		{
			TRACE( "Allocate bandwidth validation failed\n" );
			return STATUS_FAILURE;
		}
	}
	return STATUS_SUCCESS;
}

/*
  free bandwidth
 
  no checking if more bw than allocated is freed!
 */
int _1394util_free_bandwidth( raw1394handle_t raw1394handle, int bandwidth )
{
	quadlet_t buffer;
	
	if( 0 > cooked1394_read( raw1394handle, 
				 raw1394_get_irm_id( raw1394handle ), 
				 CSR_REGISTER_BASE + CSR_BANDWIDTH_AVAILABLE, 
				 sizeof( quadlet_t ), 
				 &buffer ) )
	{
		TRACE( "Failed to get CSR_BANDWIDTH_AVAILABLE\n" );
		return STATUS_FAILURE;
	}

	buffer = ntohl( buffer );

	TRACE( "Available bandwidth: 0x%x ( %d )\n", buffer, buffer );

	if( (int)(buffer + bandwidth) > MAX_BANDWIDTH )
	{ // just a warning!
		TRACE( "Freeing of bandwidth gives more than the maximum allowed ( or the bus is > 400Mbps )\n" );
	}
	else
	{
		quadlet_t data, arg, result;
		
		arg = htonl( buffer );
		data = htonl( buffer + bandwidth );
		
		if( 0 > raw1394_lock( raw1394handle, 
				      raw1394_get_irm_id( raw1394handle ), 
				      CSR_REGISTER_BASE + CSR_BANDWIDTH_AVAILABLE,
				      EXTCODE_COMPARE_SWAP, 
				      data, 
				      arg, 
				      &result ) )
		{
			TRACE( "Failed to allocate bandwidth\n" );
			return STATUS_FAILURE;
		}

		if( arg != htonl( buffer ) )
		{
			TRACE( "Allocate bandwidth validation failed: arg = %x, buffer = %x\n", arg, htonl( buffer ) );
			return STATUS_FAILURE;
		}	
		
	}
	return STATUS_SUCCESS;
}

/*
  return the bw available on the bus. for convenience only, value could change any time
 */
int _1394util_get_available_bandwidth( raw1394handle_t raw1394handle )
{
	quadlet_t buffer;
	
	if( 0 > cooked1394_read( raw1394handle, 
				 raw1394_get_irm_id( raw1394handle ), 
				 CSR_REGISTER_BASE + CSR_BANDWIDTH_AVAILABLE, 
				 sizeof( quadlet_t ), 
				 &buffer ) )
	{
		TRACE( "Failed to get CSR_BANDWIDTH_AVAILABLE\n" );
		return -1;
	}

	buffer =  ntohl( buffer );
	
	return buffer;
}

unsigned long bswap( unsigned long v )
{
	unsigned long r;
	
	r = ( v>>24 ) | 
		( ( ( v>>16 ) & 0xff ) << 8 ) |
		( ( ( v>>8 ) & 0xff ) << 16 ) |
		( ( ( v ) & 0xff ) << 24 );
	
	return r;
}

quadlet_t bitswap( quadlet_t value )
{
	quadlet_t tmp = 0;
	int i;
	
	for( i = 0; i < 32; i++ )
	{
		tmp |= ( ( ( value >>( 31-i ) ) & 1 ) << i );
	}
	
	return tmp;
}
