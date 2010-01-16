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

#ifdef RAW1394_1_1_API
#include <libraw1394/raw1394.h>
#include <libraw1394/csr.h>
#else
#include <raw1394.h>
#include <csr.h>
#endif

#include <netinet/in.h> // ntohl()
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <unicap.h>
#include <unicap_status.h>
#include <errno.h>
#include "dcam.h"
#include "dcam_functions.h"

#if DCAM_DEBUG
#define DEBUG
#endif
#include "debug.h"

#define MAX_RETRIES 5
#define REGISTER_DELAY 5000 // usec

static nodeaddr_t _dcam_get_vendor_name_leaf_address( raw1394handle_t raw1394handle, 
						      int node, 
						      nodeaddr_t logical_unit_dir_addr );
static nodeaddr_t _dcam_get_model_name_leaf_address( raw1394handle_t raw1394handle, 
						     int node, 
						     nodeaddr_t logical_unit_dir_addr );


int _dcam_read_register( raw1394handle_t raw1394handle, 
			 int node,
			 nodeaddr_t address, 
			 quadlet_t *value )
{
	int retval;
	int retries = MAX_RETRIES;
	dcam_handle_t dcamhandle = raw1394_get_userdata( raw1394handle );
	if( dcamhandle != NULL )
	{
		struct timeval ctime;
		unsigned long time = 0;
		
		gettimeofday( &ctime, 0 );
		time = 1000000 * ( ctime.tv_sec - dcamhandle->last_register_access.tv_sec );
		time += ctime.tv_usec - dcamhandle->last_register_access.tv_usec;
		
		if( time < REGISTER_DELAY )
		{
/* 			TRACE( "Trying to access camera too fast. Sleeping: %d\n", REGISTER_DELAY - time ); */
			usleep( REGISTER_DELAY - time );
		}
		
		memcpy( &dcamhandle->last_register_access, &ctime, sizeof( struct timeval ) );
	}
	
	while( retries-- )
	{
		retval = raw1394_read( raw1394handle, 0xffc0 | node, address, 4, value );
		TRACE( "read[%d] addr: %llx : %08x\n", node, address, *value );
		if( !retval )
		{
			*value = ntohl( *value );
			return 0;
		}

		if( errno == 22 )
		{
			// invalid arg
			return -1;
		}
		TRACE( "retval: %d, error: %d %s\n", retval, errno, strerror( errno ) )
		usleep( REGISTER_DELAY );
	}
	
	return -1;
}

int _dcam_write_register( raw1394handle_t raw1394handle, 
						  int node, 
						  nodeaddr_t address, 
						  quadlet_t value )
{
	int retval;
	int retries = MAX_RETRIES;

	dcam_handle_t dcamhandle = raw1394_get_userdata( raw1394handle );
	if( dcamhandle )
	{
		struct timeval ctime;
		unsigned long time = 0;
		
		gettimeofday( &ctime, 0 );
		time = 1000000 * ( ctime.tv_sec - dcamhandle->last_register_access.tv_sec );
		time += ctime.tv_usec - dcamhandle->last_register_access.tv_usec;
		
		if( time < REGISTER_DELAY )
		{
/* 			TRACE( "Trying to access camera too fast. Sleeping: %d\n", REGISTER_DELAY - time ); */
			usleep( REGISTER_DELAY - time );
		}
		
		memcpy( &dcamhandle->last_register_access, &ctime, sizeof( struct timeval ) );
	}

	value = ntohl( value );

	while( retries-- )
	{
		TRACE( "write addr: %llx : %08x\n", address, value );
		retval = raw1394_write( raw1394handle, 0xffc0 | node, address, 4, &value );
		if( !retval )
		{
			return 0;
		}

		usleep( REGISTER_DELAY );
	}
	
	return -1;
}
		

int _dcam_get_directory_count( raw1394handle_t raw1394handle, int node )
{
	nodeaddr_t addr = CSR_REGISTER_BASE + CSR_CONFIG_ROM;
	quadlet_t root_dir_length, val;
	unsigned int offset;
	int count = 0;

	if( _dcam_read_register( raw1394handle, node, addr, &root_dir_length ) < 0 )
	{
		TRACE( "failed to read length register %llx\n", addr );
		return 0;
	}

/* 	TRACE( "root_dir_length: %llx %x\n", addr, root_dir_length ); */

	root_dir_length &=0x00ff0000;
	root_dir_length = root_dir_length>>16;
	for( offset = 8; offset < (root_dir_length*4); offset+=4 )
	{
		if( _dcam_read_register( raw1394handle, node, addr + offset, &val ) == 0 )
		{
/* 			TRACE( "offset: %x  \t%x\n", offset, val ); */
			if( ( val & 0xff000000 )>>24 == 0xd1 )
			{
				count++;
			}
		}
	}
	return count;
}

nodeaddr_t _dcam_get_unit_directory_address( raw1394handle_t raw1394handle, int node, int directory )
{
	nodeaddr_t addr = CSR_REGISTER_BASE + CSR_CONFIG_ROM;
	quadlet_t root_dir_length, val;
	unsigned int offset;
	int count = 0;


	if( _dcam_read_register( raw1394handle, node, addr, &root_dir_length ) < 0 )
	{
		return 0;
	}

	root_dir_length &= 0x00ff0000;
	root_dir_length = root_dir_length >> 16;
	
	// scan through the root directory
	for( offset = 8; offset < (root_dir_length*4); offset+=4 )
	{
	   if( _dcam_read_register( raw1394handle, node, addr + offset, &val ) == 0 )
	   {
	      if( (( val & 0xff000000 )>>24) == 0xd1 )
	      {
		 //found a unit directory
		 if( count == directory )
		 {
		    break;
		 }
		 count++;
	      }
	   }
	   else
	   {
	      return 0;
	   }
	}

	// calculate unit directory address
	addr += offset;
	addr += (val & 0xffffff)*4;

	return addr;
}

nodeaddr_t _dcam_calculate_address( raw1394handle_t raw1394handle, int node, nodeaddr_t addr, quadlet_t key )
{
	quadlet_t dir_length, val;
	unsigned int offset;
	
	if( _dcam_read_register( raw1394handle, node, addr, &dir_length ) < 0 )
	{
		return 0;
	}
/* 	TRACE( "calc addr: %llx\t%08x\n", addr, dir_length ); */
	dir_length = dir_length >> 16;

	for( offset = 0; offset < ( dir_length * 4 ); offset += 4 )
	{
		if( _dcam_read_register( raw1394handle, node, addr + offset, &val ) < 0 )
		{
			return 0;
		}

/* 		TRACE( "off: %x\t%08x\n", offset, val ); */
		if( (( val & 0xff000000 )>>24) == key )
		{
			val &= ~0xff000000;
			break;
		}
	}
	if( offset > ( dir_length * 4 ) )
	{
		return 0;
	}
	addr += offset;
	return addr;
}


nodeaddr_t _dcam_get_unit_dependent_directory_address( raw1394handle_t raw1394handle, int node, int directory )
{
	nodeaddr_t addr = _dcam_get_unit_directory_address( raw1394handle, node, directory );
			// val now contains 0xd4xxxxxx where xxxxxx is unit_dependent_directory offset
	addr = _dcam_calculate_address( raw1394handle, node, addr, 0xd4 );
	return addr;
}

static nodeaddr_t _dcam_get_vendor_name_leaf_address( raw1394handle_t raw1394handle, 
													  int node, 
													  nodeaddr_t logical_unit_dir_addr )
{
	nodeaddr_t addr;
	quadlet_t val;

	if( !logical_unit_dir_addr )
	{
		return 0;
	}
	
	logical_unit_dir_addr = _dcam_calculate_address( raw1394handle, node, logical_unit_dir_addr, 0xd4 );
	if( !logical_unit_dir_addr )
	{
		return 0;
	}

	if( _dcam_read_register( raw1394handle, node, logical_unit_dir_addr, &val ) < 0 )
	{
		return 0;
	}

	logical_unit_dir_addr += val &0xffff*4;
	addr = _dcam_calculate_address( raw1394handle, node, logical_unit_dir_addr, 0x81 );
	
	if( !addr )
	{
		return 0;
	}
	
	if( _dcam_read_register( raw1394handle, node, addr, &val ) < 0 )
	{
		return 0;
	}
	addr += (val&0xffff)*4;

	return addr;
}

static nodeaddr_t _dcam_get_model_name_leaf_address( raw1394handle_t raw1394handle, 
													 int node, 
													 nodeaddr_t logical_unit_dir_addr )
{
	nodeaddr_t addr;
	quadlet_t val;
	
	logical_unit_dir_addr = _dcam_calculate_address( raw1394handle, node, logical_unit_dir_addr, 0xd4 );
	_dcam_read_register( raw1394handle, node, logical_unit_dir_addr, &val );
	logical_unit_dir_addr += val &0xffff*4;
	addr = _dcam_calculate_address( raw1394handle, node, logical_unit_dir_addr, 0x81 );
	addr += 4; // hack!! assume model name leaf addr follows vendor name leaf addr.
	_dcam_read_register( raw1394handle, node, addr, &val );
	addr += (val&0xffff)*4;

	return addr;
}

unsigned int _dcam_get_spec_ID( raw1394handle_t raw1394handle, int node, nodeaddr_t unit_directory_addr )
{
	quadlet_t spec_ID;
	nodeaddr_t addr;
	
	TRACE( "Get spec ID\n" );
	
	addr = _dcam_calculate_address( raw1394handle, node, unit_directory_addr, 0x12 );

	if( addr == 0 )
	{
		TRACE( "!addr\n" );
		return 0;
	}
	

	_dcam_read_register( raw1394handle, node, addr, &spec_ID );
	// spec_ID now contains 0x12xxxxxx where xxxxxx is spec_ID
	spec_ID &= ~0xff000000;
	
	return spec_ID;
}

unsigned int _dcam_get_sw_version( raw1394handle_t raw1394handle, int node, nodeaddr_t unit_directory_addr )
{
	quadlet_t sw_version;
	nodeaddr_t addr;

	addr = _dcam_calculate_address( raw1394handle, node, unit_directory_addr, 0x13 );
	if( !addr )
	{
		return 0;
	}
	_dcam_read_register( raw1394handle, node, addr, &sw_version );
	// sw_version now contains 0x13xxxxxx where xxxxxx is sw_version
	sw_version &= ~0xff000000;
	return sw_version;
}

nodeaddr_t _dcam_get_command_regs_base( raw1394handle_t raw1394handle, int node, nodeaddr_t unit_dependend_directory_addr )
{
	quadlet_t offset;
	nodeaddr_t addr;

	addr = _dcam_calculate_address( raw1394handle, node, unit_dependend_directory_addr, 0xd4 );
	if( !addr )
	{
		return 0;
	}
	_dcam_read_register( raw1394handle, node, addr, &offset );
	offset &= ~0xff000000;

	addr = _dcam_calculate_address( raw1394handle, node, addr + (offset*4), 0x40 );
	_dcam_read_register( raw1394handle, node, addr, &offset );
	offset &= ~0xff000000;

	addr = CSR_REGISTER_BASE;
	addr += offset*4;
	
	return addr;
}

int _dcam_get_vendor_id( raw1394handle_t raw1394handle, int node, int directory, unsigned long *vendor_id )
{
	quadlet_t node_vendor_id;
	
	_dcam_read_register( raw1394handle, node, CSR_REGISTER_BASE + CSR_CONFIG_ROM + 0xc, &node_vendor_id );
	node_vendor_id = node_vendor_id >> 8;

	*vendor_id = node_vendor_id;
	
	return STATUS_SUCCESS;
}

int _dcam_get_model_id( raw1394handle_t raw1394handle, int node, int directory, unsigned long *model_id_hi, unsigned long *model_id_lo )
{
	quadlet_t node_model_id;
	_dcam_read_register( raw1394handle, node, CSR_REGISTER_BASE + CSR_CONFIG_ROM + 0xc, &node_model_id );
	*model_id_hi = node_model_id & 0xff;
	
	_dcam_read_register( raw1394handle, node, CSR_REGISTER_BASE + CSR_CONFIG_ROM + 0x10, &node_model_id );
	*model_id_lo = node_model_id;
	
	return STATUS_SUCCESS;
}

int _dcam_read_name_leaf( raw1394handle_t raw1394handle, int node, nodeaddr_t addr, char *buffer, size_t *buffer_len )
{
	quadlet_t leaf_length;
	int i;
	
	if( _dcam_read_register( raw1394handle, node, addr, &leaf_length ) < 0 )
	{
		return -1;
	}

	leaf_length = leaf_length >> 16;
	leaf_length -= 2;

	if( ( leaf_length * 4 +1) > *buffer_len )
	{
		*buffer_len = leaf_length * 4;
		return -1;
	}

	for( i=0; ( i < leaf_length ) && ( i < (*buffer_len / 4 ) ); i++ )
	{
		quadlet_t val;
		if( _dcam_read_register( raw1394handle, node, addr + 0xc + (i*4), &val ) < 0 ) 
		{
			return -1;
		}
		
		val = ntohl( val );
		memcpy(buffer+(i*4), &val, 4 );		
	}
	buffer[leaf_length*4]=0;
	
	*buffer_len = leaf_length * 4;

	return *buffer_len;
}


static int _dcam_check_compat_fast( raw1394handle_t raw1394handle, int node )
{
	nodeaddr_t addr = CSR_REGISTER_BASE + CSR_CONFIG_ROM;

	unsigned int spec_ID;
	unsigned int sw_version;
	quadlet_t quad;
	int unit_offset;
	int key;

	addr += 0x24;
	
	if( _dcam_read_register( raw1394handle, node, addr, &quad ) < 0 )
	{
		return 0;
	}
	
	unit_offset = ( quad & 0xffffff ) * 4;
	
	addr += unit_offset;
	
	if( _dcam_read_register( raw1394handle, node, addr + 0x4, &quad ) < 0 )
	{
		return 0;
	}
	
	spec_ID = quad & 0xffffff;
	key = ( quad >> 24 ) & 0xff;
	if( key != 12 )
	{
		return 0;
	}
	if( spec_ID != 0xA02D )
	{
		return -1;
	}
	
	if( _dcam_read_register( raw1394handle, node, addr + 0x8, &quad ) < 0 )
	{
		return 0;
	}
	
	sw_version = quad & 0xffffff;
	
	if( ( sw_version < 0x100 ) || ( sw_version > 0x103 ) )
	{
		return -1;
	}
	
	return 1;
}

int _dcam_is_compatible( raw1394handle_t raw1394handle, int node, int directory )
{
	unsigned int spec_ID;
	unsigned int sw_version;
	int fastcheck;
	

	if( directory == 0 )
	{
		fastcheck = _dcam_check_compat_fast( raw1394handle, node );
		
		if( fastcheck == 1 )
		{
			TRACE( "fast check success\n" );
			return 1;
		}
		else if( fastcheck < 0 )
		{
			TRACE( "fastcheck incompatible\n" );
			return 0;
		}
	}
	
	spec_ID = _dcam_get_spec_ID( raw1394handle, node, _dcam_get_unit_directory_address( raw1394handle, node, directory ) );
	if( spec_ID != 0xa02d )
	{
		TRACE( "!spec_id\n" );
		return 0;
	}

	sw_version = _dcam_get_sw_version( raw1394handle, node, _dcam_get_unit_directory_address( raw1394handle, node, directory ) );	
	
	if( ( sw_version < 0x100 ) || ( sw_version > 0x103 ) )
	{
		TRACE( "!sw_ver\n" );
		return 0;
	}
	
	return 1;
}

void _dcam_create_device_identifier( char *buffer, size_t size, char *vendor_name, char *model_name, unsigned long model_id_hi, unsigned long model_id_lo )
{
	snprintf( buffer, size, "%s %s %lu%lu", vendor_name, model_name, model_id_hi, model_id_lo );
}

unicap_status_t _dcam_get_device_info( raw1394handle_t raw1394handle, int node, int directory, unicap_device_t *device )
{
	char buffer[128];
	size_t buffer_size = 128;
	unsigned long model_id_hi;
	unsigned long model_id_lo;
	unsigned long vendor_id;
	nodeaddr_t addr;
	nodeaddr_t logical_unit_dir_addr;
	
	TRACE( "GetDevInfo\n" );

	strcpy( device->device, "/dev/raw1394" );

	logical_unit_dir_addr = _dcam_get_unit_directory_address( raw1394handle, node, directory );
	if( !logical_unit_dir_addr )
	{
		return STATUS_FAILURE;
	}
	
	addr = _dcam_get_vendor_name_leaf_address( raw1394handle, node, logical_unit_dir_addr );
	if( !addr )
	{
		TRACE( "Get vendor name lead addr failed\n" );
		return STATUS_FAILURE;
	}

	if( _dcam_read_name_leaf( raw1394handle, 
							  node, 
							  addr, 
							  buffer, 
							  &buffer_size ) < 0 )
	{
		TRACE( "Read name leaf failed\n" );
		return STATUS_FAILURE;
	}

	strcpy( device->vendor_name, buffer );

	buffer_size = 128; 

	addr = _dcam_get_model_name_leaf_address( raw1394handle, node, logical_unit_dir_addr );
	if( !addr )
	{
		return STATUS_FAILURE;
	}

	if( _dcam_read_name_leaf( raw1394handle, 
							  node, 
							  addr, 
							  buffer, 
							  &buffer_size ) < 0 )
	{
		TRACE( "-->\n" );
		return STATUS_FAILURE;
	}
	
	strcpy( device->model_name, buffer );

	_dcam_get_vendor_id( raw1394handle, node, directory, &vendor_id );
	_dcam_get_model_id( raw1394handle, node, directory, &model_id_hi, &model_id_lo );
	
	_dcam_create_device_identifier( buffer, 128, device->vendor_name, device->model_name, model_id_hi, model_id_lo );
	strcpy( device->identifier, buffer );
	
	device->model_id = ((unsigned long long )model_id_hi << 32) | (unsigned long long )model_id_lo;
	device->vendor_id = vendor_id;
	
	TRACE( "-->\n" );

	return STATUS_SUCCESS;
}


unicap_status_t _dcam_find_device( unicap_device_t *device, int *_port, int *_node, int *_directory )
{
	raw1394handle_t raw1394handle;
	int port;
	int node;
	int directory;
	struct raw1394_portinfo portinfo[16];
	int port_count;
	
	raw1394handle = raw1394_new_handle( );
	if( !raw1394handle )
	{
		return STATUS_FAILURE;
	}
	
	port_count = raw1394_get_port_info( raw1394handle, portinfo, 16 );
	if( port_count < 0 )
	{
		raw1394_destroy_handle( raw1394handle );
		return STATUS_FAILURE;
	}
	if( port_count == 0 )
	{
		raw1394_destroy_handle( raw1394handle );
		return STATUS_NO_DEVICE;
	}
	raw1394_destroy_handle( raw1394handle );
	
	for( port = 0; port < port_count; port++ )
	{
		if( ( raw1394handle = raw1394_new_handle_on_port( port ) ) == NULL )
		{
			continue;
		}
		raw1394_set_userdata( raw1394handle, 0 );

		for( node = 0; node < raw1394_get_nodecount( raw1394handle ); node ++ )
		{
			for( directory = 0; directory < _dcam_get_directory_count( raw1394handle, node ); directory++ )
			{
				if( _dcam_is_compatible( raw1394handle, node, directory ) )
				{
					int match = 1;
					if( strlen( device->identifier ) )
					{
						char identifier[128];
						char vendor_name[128];
						char model_name[128];
						size_t size = 128;
						unsigned long model_id_hi, model_id_lo;
						nodeaddr_t logical_unit_dir_addr;
						nodeaddr_t addr;
						
						logical_unit_dir_addr = _dcam_get_unit_directory_address( raw1394handle, node, directory );
						if( !logical_unit_dir_addr )
						{
							TRACE( "Failed to get logical unit addr\n" );
							match = 0;
							continue;
						}
						
						addr = _dcam_get_vendor_name_leaf_address( raw1394handle, node, logical_unit_dir_addr );
						if( !addr )
						{
							TRACE( "Failed to get vendor name leaf addr\n" );
							match = 0;
							continue;
						}

						_dcam_get_model_id( raw1394handle, 
											node, 
											directory, 
											&model_id_hi, 
											&model_id_lo );
						_dcam_read_name_leaf( raw1394handle, 
											  node, 
											  addr, 
											  vendor_name, 
											  &size );
						size = 128;
						addr = _dcam_get_model_name_leaf_address( raw1394handle, node, logical_unit_dir_addr );
						if( !addr )
						{
							TRACE( "Failed to get model name leaf addr\n" );
							match = 0;
							continue;
						}

						_dcam_read_name_leaf( raw1394handle, 
											  node, 
											  addr, 
											  model_name, 
											  &size );
						_dcam_create_device_identifier( identifier, 
														128, 
														vendor_name, 
														model_name, 
														model_id_hi, 
														model_id_lo );
						if( !strcmp( identifier, device->identifier ) )
						{
							match = 1;
							*_port = port;
							*_node = node;
							*_directory = directory;
							raw1394_destroy_handle( raw1394handle );
							return STATUS_SUCCESS;
						}
						else
						{
							match = 0;
							continue;
						}
					}// strlen( device->identifier

					if( strlen( device->vendor_name ) )
					{
						char vendor_name[128];
						size_t size = 128;

						nodeaddr_t logical_unit_dir_addr;
						nodeaddr_t addr;
						
						logical_unit_dir_addr = _dcam_get_unit_directory_address( raw1394handle, node, directory );
						if( !logical_unit_dir_addr )
						{
							TRACE( "Failed to get logical unit addr\n" );
							match = 0;
							continue;
						}

						addr = _dcam_get_vendor_name_leaf_address( raw1394handle, 
																   node, 
																   logical_unit_dir_addr );
						
						_dcam_read_name_leaf( raw1394handle, 
											  node, 
											  addr, 
											  vendor_name, 
											  &size );
						if( !strcmp( vendor_name, device->vendor_name ) )
						{
							match = 1;
						}
						else
						{
							match = 0;
							continue;
						}
					}//strlen( device->vendor_name
					
					if( strlen( device->model_name ) )
					{
						char model_name[128];
						size_t size = 128;
						nodeaddr_t logical_unit_dir_addr;
						nodeaddr_t addr;
						
						logical_unit_dir_addr = _dcam_get_unit_directory_address( raw1394handle, node, directory );
						if( !logical_unit_dir_addr )
						{
							TRACE( "Failed to get logical unit addr\n" );
							match = 0;
							continue;
						}

						addr = _dcam_get_model_name_leaf_address( raw1394handle, 
																  node, 
																  logical_unit_dir_addr );
						if( !addr )
						{
							match = 0;
							continue;
						}

						_dcam_read_name_leaf( raw1394handle, 
											  node, 
											  addr, 
											  model_name, 
											  &size );
						if( !strcmp( model_name, device->model_name ) )
						{
							match = 1;
						}
						else
						{
							match = 0;
							continue;
						}
						
					}
					
					if( device->vendor_id != -1 )
					{
						unsigned long vendor_id;
						_dcam_get_vendor_id( raw1394handle, 
								     node, 
								     directory, 
								     &vendor_id );
						if( device->vendor_id == vendor_id )
						{
							match = 1;
						}
						else
						{
							match = 0;
							continue;
						}
					}
					
					if( device->model_id != -1 )
					{
						unsigned long long model_id;
						unsigned long model_id_hi;
						unsigned long model_id_lo;
						
						_dcam_get_model_id( raw1394handle, 
								    node, 
								    directory, 
								    &model_id_hi, 
								    &model_id_lo );
						model_id = (unsigned long long)model_id_hi << 32 | (unsigned long long )model_id_lo;
						if( device->model_id == model_id )
						{
							match = 1;
						}
						else
						{
							match = 0;
							continue;
						}
					}	

					if( match )
					{
						*_port = port;
						*_node = node;
						*_directory = directory;
						raw1394_destroy_handle( raw1394handle );
						return STATUS_SUCCESS;
					}
				}
			}
		}
		if( raw1394handle )
		{
			raw1394_destroy_handle( raw1394handle );
		}
	}
	return STATUS_NO_DEVICE;
}

