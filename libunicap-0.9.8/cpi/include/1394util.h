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

#ifndef __UTIL_H__
#define __UTIL_H__

#include <config.h>

#include <unicap.h>
#ifdef RAW1394_1_1_API
#include <libraw1394/raw1394.h>
#include <libraw1394/csr.h>
#else
#include <raw1394.h>
#include <csr.h>
#endif


int cooked1394_read( raw1394handle_t handle, 
		     nodeid_t node, 
		     nodeaddr_t addr,
		     size_t length, 
		     quadlet_t *buffer );

int cooked1394_write( raw1394handle_t handle, 
		      nodeid_t node, 
		      nodeaddr_t addr,
		      size_t length, 
		      quadlet_t *data);

unsigned long long get_guid( raw1394handle_t handle, int phyID );
unsigned int get_unit_spec_ID( raw1394handle_t handle, int phyID );
unsigned int get_unit_sw_version( raw1394handle_t handle, int phyID );

int _1394util_find_free_channel( raw1394handle_t raw1394handle );
int _1394util_free_channel( raw1394handle_t raw1394handle, int channel );
int _1394util_allocate_channel( raw1394handle_t raw1394handle, int channel );
int _1394util_allocate_bandwidth( raw1394handle_t raw1394handle, int bandwidth );
int _1394util_free_bandwidth( raw1394handle_t raw1394handle, int bandwidth );
int _1394util_get_available_bandwidth( raw1394handle_t raw1394handle );
unicap_status_t _1394util_send_fcp_command_ext( raw1394handle_t raw1394handle, 
												nodeid_t nodeid,
												unsigned long long fcp_command,
												unsigned long *sync_bit_store,
												void *data, 
												size_t data_length, 
												unsigned long *response );
unsigned long bswap( unsigned long v );
quadlet_t bitswap( quadlet_t value );




#endif //__UTIL_H__
