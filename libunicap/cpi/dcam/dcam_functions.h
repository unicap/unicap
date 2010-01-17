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

#ifndef __DCAM_FUNCTIONS_H__
#define __DCAM_FUNCTIONS_H__


#include <config.h>

#ifdef RAW1394_1_1_API
#include <libraw1394/raw1394.h>
#endif
#ifdef RAW1394_1_0_API
#include <raw1394.h>
#endif

#include <unicap.h>

int _dcam_read_register( raw1394handle_t raw1394handle,
			 int node,
			 nodeaddr_t address,
			 quadlet_t *value );

int _dcam_write_register( raw1394handle_t raw1394handle,
			  int node,
			  nodeaddr_t address,
			  quadlet_t value );

int _dcam_get_directory_count( raw1394handle_t raw1394handle, int node );

nodeaddr_t _dcam_get_unit_directory_address( raw1394handle_t raw1394handle, int node, int directory );

nodeaddr_t _dcam_calculate_address( raw1394handle_t raw1394handle, int node, nodeaddr_t addr, quadlet_t key );

nodeaddr_t _dcam_get_unit_dependent_directory_address( raw1394handle_t raw1394handle, int node, int directory );

unsigned int _dcam_get_spec_ID( raw1394handle_t raw1394handle, int node, nodeaddr_t unit_directory_addr );

unsigned int _dcam_get_sw_version( raw1394handle_t raw1394handle, int node, nodeaddr_t unit_directory_addr );

nodeaddr_t _dcam_get_command_regs_base( raw1394handle_t raw1394handle, int node, nodeaddr_t unit_dependend_directory_addr );

int _dcam_get_vendor_id( raw1394handle_t raw1394handle, int node, int directory, unsigned long *vendor_id );

int _dcam_get_model_id( raw1394handle_t raw1394handle, int node, int directory, unsigned long *model_id_hi, unsigned long *model_id_lo );

int _dcam_read_name_leaf( raw1394handle_t raw1394handle, int node, nodeaddr_t addr, char *buffer, size_t *buffer_len );

int _dcam_is_compatible( raw1394handle_t raw1394handle, int node, int directory );

void _dcam_create_device_identifier( char *buffer, size_t size, char *vendor_name, char *model_name, unsigned long model_id_hi, unsigned long model_id_lo );

unicap_status_t _dcam_get_device_info( raw1394handle_t raw1394handle, int node, int directory, unicap_device_t *device );

unicap_status_t _dcam_find_device( unicap_device_t *device, int *port, int *node, int *directory );

#endif //__DCAM_FUNCTIONS_H__
