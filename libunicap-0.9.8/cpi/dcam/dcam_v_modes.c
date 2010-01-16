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


#include <unicap_status.h>
#include <unicap.h>

#include <string.h>

#include "dcam.h"
#include "dcam_functions.h"
#include "dcam_format_table.h"
#include "dcam_isoch_table.h"
#include "dcam_v_modes.h"
#include "dcam_offsets.h"

#include "1394util.h"

#if DCAM_DEBUG
#define DEBUG
#endif
#include "debug.h"

/*
  returns the number of video modes supported by the device described by (node,directory)

  returns 0 on errors
*/
int _dcam_count_v_modes( dcam_handle_t dcamhandle, int node, int directory )
{
   nodeaddr_t addr = dcamhandle->command_regs_base;
   quadlet_t formats;
   quadlet_t modes;
   int count = 0;
	
   if( _dcam_read_register( dcamhandle->raw1394handle, node, addr + 0x100, &formats ) < 0 )
   {
      TRACE( "Failed to read formats\n" );
      return 0;
   }	

   if( formats & ( 1 << 31 ) )
   {
      if( _dcam_read_register( dcamhandle->raw1394handle, node, addr + 0x180, &modes ) == 0 )
      {
	 int i;
	 for( i = 0; i < 8; i++ )
	 {
	    if( modes & ( 1<<(31-i) ) )
	    {
	       count++;
/* 					TRACE( "Mode: %d\n", i ); */
	    }
	 }
      }
/* 		TRACE( "format0: %d\n", count ); */
   }
   if( formats & ( 1 << 30 ) )
   {
      if( _dcam_read_register( dcamhandle->raw1394handle, node, addr + 0x180, &modes ) == 0 )
      {
	 int i;
	 for( i = 0; i < 8; i++ )
	 {
	    if( modes & ( 1<<(31-i) ) )
	    {
	       count++;
	    }
	 }
      }
/* 		TRACE( "format1: %d\n", count ); */
   }
   if( formats & ( 1 << 29 ) )
   {
      if( _dcam_read_register( dcamhandle->raw1394handle, node, addr + 0x180, &modes ) == 0 )
      {
	 int i;
	 for( i = 0; i < 8; i++ )
	 {
	    if( modes & ( 1<<(31-i) ) )
	    {
	       count++;
	    }
	 }
      }
/* 		TRACE( "format2: %d\n", count ); */
   }
	
   return count;
}

/*
  returns index of the mode in _dcam_isoch_table and _dcam_formats_table
*/
int _dcam_get_mode_index( unsigned int format, unsigned int mode )
{
   if( mode > 7 )
   {
      return -1;
   }
   if( format > 2 )
   {
      return -1;
   }
	
   return (format * 8 ) + mode;
};

/*
  create array of unicap video formats supported by the camera described by (node,directory)
  input: 
  format_array: allocated memory area
  format_count: number of entries allocated in format_array
  output:
  format_array: initialized table of supported video formats
  format_count: number of entries actually used in the array
*/
unicap_status_t _dcam_prepare_format_array( dcam_handle_t dcamhandle, 
					    int node, 
					    int directory, 
					    unicap_format_t *format_array, 
					    int *format_count )
{
   nodeaddr_t addr = dcamhandle->command_regs_base;
   quadlet_t formats;
   quadlet_t modes;
   int current_format=0;
   
   int f;
   
   if( *format_count < _dcam_count_v_modes( dcamhandle, node, directory ) )
   {
      *format_count = 0;
      return STATUS_BUFFER_TOO_SMALL;
   }
	
   // read format inquiry register
   if( _dcam_read_register( dcamhandle->raw1394handle, node, addr + O_FORMAT_INQ, &formats ) < 0 )
   {
      *format_count = 0;
      return STATUS_FAILURE;
   }

   for( f = 0; f < 3; f++ )
   {
      if( formats & ( 1<<(31-f) ) )
      {
	 // read mode inquiry register
	 if( _dcam_read_register( dcamhandle->raw1394handle, node, addr + O_MODE_INQ_BASE + (f*4), &modes ) == 0 )
	 {
	    int i;
	    for( i = 0; i < 8; i++ )
	    {
	       if( modes & ( 1<<(31-i) ) )
	       {
		  int index = _dcam_get_mode_index( f, i );
		  TRACE( "f: %d m: %d  index: %d (%s)\n", f, i, index, _dcam_unicap_formats[index].identifier );
		  TRACE( "size: %d x %d\n", _dcam_unicap_formats[index].size.width, _dcam_unicap_formats[index].size.height );
		  memcpy( format_array + current_format, &_dcam_unicap_formats[index], sizeof( unicap_format_t ) );
		  current_format++;
	       }
	    }
	 }
      }
   }

   *format_count = current_format;

   return STATUS_SUCCESS;
}


unicap_status_t _dcam_set_mode_and_format( dcam_handle_t dcamhandle, int index )
{
   int format;
   int mode;

   quadlet_t quad;
	
   format = index / 8;
   mode = index % 8;

   TRACE( "format: %d, mode: %d\n", format, mode );


   quad = mode << 29;

   if( _dcam_write_register( dcamhandle->raw1394handle, 
			     dcamhandle->node, 
			     dcamhandle->command_regs_base + O_CUR_MODE, 
			     quad )< 0 )
   {
      return STATUS_FAILURE;
   }

   quad = format << 29;

   if( _dcam_write_register( dcamhandle->raw1394handle, 
			     dcamhandle->node, 
			     dcamhandle->command_regs_base + O_CUR_FORMAT, 
			     quad ) < 0 )
   {
      return STATUS_FAILURE;
   }
	
   return STATUS_SUCCESS;
}
	
/*
  returns the video mode currently set on the camera
*/
unicap_status_t _dcam_get_current_v_mode( dcam_handle_t dcamhandle, int *mode )
{
   quadlet_t quad;
	
   if( _dcam_read_register( dcamhandle->raw1394handle, 
			    dcamhandle->node, 
			    dcamhandle->command_regs_base + O_CUR_MODE, 
			    &quad ) < 0 )
   {
      return STATUS_FAILURE;
   }
	
   *mode = quad >> 29;

   return STATUS_SUCCESS;
}

/*
  returns the video format currently set on the camera
*/
unicap_status_t _dcam_get_current_v_format( dcam_handle_t dcamhandle, int *format )
{
   quadlet_t quad;
	
   if( _dcam_read_register( dcamhandle->raw1394handle, 
			    dcamhandle->node, 
			    dcamhandle->command_regs_base + O_CUR_FORMAT, 
			    &quad ) < 0 )
   {
      return STATUS_FAILURE;
   }
	
   *format = quad >> 29;
	
   return STATUS_SUCCESS;
}

/*
  returns the frame rates supported by the mode & format combination currently set on the camera
  ( return value is of type frame_rate_inq_t.quadlet )
*/
quadlet_t _dcam_get_supported_frame_rates( dcam_handle_t dcamhandle )
{
   int mode, format;

   unsigned int offset = 0;
	
   quadlet_t quad;
	
   if( !SUCCESS( _dcam_get_current_v_mode( dcamhandle, &mode ) ) )
   {
      return 0;
   }
	
   if( !SUCCESS( _dcam_get_current_v_format( dcamhandle, &format ) ) )
   {
      return 0;
   }
	
   if( format == 0 )
   {
      if( mode < 7 )
      {
	 offset = 0x200 + ( 4*mode );
      }
   }
   else if( format == 1 )
   {
      if( mode < 8 )
      {
	 offset = 0x220 + ( 4*mode );
      }
   }
   else if( format == 2 )
   {
      if( mode < 8 )
      {
	 offset = 0x240 + ( 4 * mode );
      }
   }
	
   if( offset == 0 ) // unsupported mode
   {
      return 0;
   }
	
   if( !SUCCESS( _dcam_read_register( dcamhandle->raw1394handle, 
				      dcamhandle->node, 
				      dcamhandle->command_regs_base + offset,
				      &quad ) ) )
   {
      return 0;
   }
	
   return quad;
}
