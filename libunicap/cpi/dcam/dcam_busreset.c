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
#include "dcam.h"
#include "dcam_functions.h"
#include "dcam_busreset.h"
#include "1394util.h"
#include <unicap.h>
#include <unicap_status.h>
#if DCAM_DEBUG
#define DEBUG
#endif
#include "debug.h"

extern struct _dcam_isoch_mode dcam_isoch_table[];

static void dcam_device_removed_event( dcam_handle_t dcamhandle )
{
	if( dcamhandle->event_callback )
	{
		dcamhandle->event_callback( dcamhandle->unicap_handle, UNICAP_EVENT_DEVICE_REMOVED );
	}
}

/*
  bus reset handler checks if device is still on the bus. 
  if it is removed
    -> set device_present to 0 . bandwidth and channel are remotely freed on bus reset
  if device still present
    -> reallocate bw and channel
    -> channel is not guarranted to be the same after reallocation -> set new channel to camera if changed  
 */
int dcam_busreset_handler( raw1394handle_t raw1394handle, unsigned int generation )
{
	dcam_handle_t dcamhandle = ( dcam_handle_t ) raw1394_get_userdata( raw1394handle );
	int port;
	
	int channel;
	
	raw1394_update_generation( raw1394handle, generation );

	// check if the device is still there or if it has changed
	if( _dcam_find_device( &dcamhandle->unicap_device, 
			       &port, 
			       &dcamhandle->node, 
			       &dcamhandle->directory ) != STATUS_SUCCESS )
	{
		dcamhandle->device_present = 0;
		dcam_device_removed_event( dcamhandle );
		return 0;
	}
	
	// what happens if a PC-Card is removed ? does this change the port???
	if( port != dcamhandle->port )
	{
		if( raw1394_set_port( raw1394handle, port ) < 0 )
		{
			dcamhandle->device_present = 0;
			dcam_device_removed_event( dcamhandle );
			return 0;
		}
		dcamhandle->port = port;
	}
	
	// reallocate bandwidth 
	// ( reallocation should always succeed )
	if( dcamhandle->allocate_bandwidth )
	{
		if( !SUCCESS(_1394util_allocate_bandwidth( dcamhandle->raw1394handle, dcam_isoch_table[dcamhandle->current_iso_index].bytes_per_packet ) ) )
		{
			TRACE( "dcam.cpi: failed to reallocate bandwidth\n" );
			dcam_capture_stop( dcamhandle );
			return 0;
		}
	}
	
	// channel is freed on busreset
	// -> reallocate channel
	//    check if newly allocated channel matches previous channel
	//    -> conditionaly set new channel on the camera
	if( !SUCCESS(_1394util_allocate_channel(dcamhandle->raw1394handle, dcamhandle->channel_allocated ) ) )
	{
		
		if( ( channel = _1394util_find_free_channel( dcamhandle->raw1394handle ) ) < 0 )
		{
			TRACE( "dcam.cpi: failed to allocate channel\n");
			_1394util_free_bandwidth( dcamhandle->raw1394handle, dcam_isoch_table[dcamhandle->current_iso_index].bytes_per_packet );
			dcam_capture_stop( dcamhandle );
			return 0;
		}
		
		if( channel != dcamhandle->channel_allocated )
		{
			quadlet_t quad = 0;
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
				_1394util_free_bandwidth( dcamhandle->raw1394handle, 
										  dcam_isoch_table[dcamhandle->current_iso_index].bytes_per_packet );
				return 0;
			}
		}
	}
	
	return 0;
}
