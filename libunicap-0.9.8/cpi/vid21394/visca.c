/*
  unicap
  Copyright (C) 2004  Arne Caspari  ( arne_caspari@users.sourceforge.net )

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

#include <string.h>
#include <netinet/in.h>

#include "unicap.h"

#include "vid21394_base.h"
#include "visca.h"
#include "visca_private.h"
#include "visca_property_table.h"

#if VID21394_DEBUG
#define DEBUG
#endif
#include "debug.h"

// --------------------------------------------------------------------------
// Constants and Definitions
// --------------------------------------------------------------------------
struct ae_mode
{
      char param;
      char *menu_item;
};

struct ae_mode ae_modes[] =
{
   { VISCA_AE_MODE_FULL_AUTO,    VISCA_AE_MENU_ITEM_FULL_AUTO }, 
   { VISCA_AE_MODE_MANUAL,       VISCA_AE_MENU_ITEM_MANUAL }, 
   { VISCA_AE_MODE_SHUTTER_PRIO, VISCA_AE_MENU_ITEM_SHUTTER_PRIO }, 
   { VISCA_AE_MODE_IRIS_PRIO,    VISCA_AE_MENU_ITEM_IRIS_PRIO }, 
   { VISCA_AE_MODE_BRIGHT,       VISCA_AE_MENU_ITEM_BRIGHT },
   { 0xff, 0 }
};

// --------------------------------------------------------------------------
// Helper functions
// --------------------------------------------------------------------------
static void visca_init_command_packet_1( unsigned char *packet_data, unsigned char command, unsigned char param1 )
{
   packet_data[0] = 0x81;
   packet_data[1] = 0x01;
   packet_data[2] = 0x04;
   packet_data[3] = command;
   packet_data[4] = param1;
   packet_data[5] = 0xff;
}

static void visca_init_command_packet_4( unsigned char *packet_data, unsigned char command, 
					 unsigned char param1, unsigned char param2, 
					 unsigned char param3, unsigned char param4 )
{
   packet_data[0] = 0x81;
   packet_data[1] = 0x01;
   packet_data[2] = 0x04;
   packet_data[3] = command;
   packet_data[4] = param1;
   packet_data[5] = param2;
   packet_data[6] = param3;
   packet_data[7] = param4;
   packet_data[8] = 0xff;
}

static void visca_init_inq_packet( unsigned char *packet_data, unsigned char command )
{
   packet_data[0] = 0x81;
   packet_data[1] = 0x09;
   packet_data[2] = 0x04;
   packet_data[3] = command;
   packet_data[4] = 0xff;
}

static void visca_init_device_type_inq_packet( unsigned char *packet_data )
{
   packet_data[0] = 0x81;
   packet_data[1] = 0x09;
   packet_data[2] = 0x00;
   packet_data[3] = 0x02;
   packet_data[4] = 0xff;
}
	
static void visca_htofla( unsigned char *ar, size_t s )
{
   unsigned int *i_ar;
   int i;
	
   i_ar = ( unsigned int *)ar;

   if( s % 4 )
   {
      s += 4 - ( s%4 );
   }

   for( i = 0; i < ( s/4 ); i++ )
   {
      i_ar[i] = htonl( i_ar[i] );
   }
}



// --------------------------------------------------------------------------
// Functions
// --------------------------------------------------------------------------

unicap_status_t visca_check_inq_response( unsigned char *response )
{
   unicap_status_t status = STATUS_SUCCESS;
	
   if( ( response[1] & 0xf0 ) != VISCA_COMMAND_COMPLETE )
   {
      status = STATUS_FAILURE;
   }
	
   return status;
}
	
unicap_status_t visca_set_white_balance( vid21394handle_t vid21394handle, 
					 unicap_property_t *property )
{
   int out_bytes, in_bytes;
   unsigned char out_data[8];
   unsigned char in_data[8];
   unicap_status_t status;
		
   out_bytes = 6;
   in_bytes = 6;
		
   if( property->flags & UNICAP_FLAGS_AUTO )
   {
      visca_init_command_packet_1( out_data, VISCA_COMMAND_WHITE_BALANCE, VISCA_WHITE_BALANCE_AUTO );
   }
   else			
   {	
      if( property->value == 3200 )
      {
	 visca_init_command_packet_1( out_data, VISCA_COMMAND_WHITE_BALANCE, VISCA_WHITE_BALANCE_INDOOR );
      }
      else
      {
	 visca_init_command_packet_1( out_data, VISCA_COMMAND_WHITE_BALANCE, VISCA_WHITE_BALANCE_OUTDOOR );
      }
   }
		
   visca_htofla( out_data, out_bytes );
	
   status = vid21394_rs232_io( vid21394handle, 
			       out_data, out_bytes, 
			       in_data, in_bytes );

   return status;
}

unicap_status_t visca_get_white_balance( vid21394handle_t vid21394handle, 
					 unicap_property_t *property )
{
   int out_bytes, in_bytes;
   unsigned char out_data[8];
   unsigned char in_data[8];
   unicap_status_t status;
		
   out_bytes = 5;
   in_bytes = 4;
	
   visca_init_inq_packet( out_data, VISCA_COMMAND_WHITE_BALANCE );
		
   visca_htofla( out_data, out_bytes );
	
   status = vid21394_rs232_io( vid21394handle, 
			       out_data, out_bytes, 
			       in_data, in_bytes );

   if( SUCCESS( status ) )
   {
      status = visca_check_inq_response( in_data );

      property->flags = UNICAP_FLAGS_MANUAL;
			
      switch( in_data[2] )
      {
	 case VISCA_WHITE_BALANCE_AUTO:
	 {
	    property->flags = UNICAP_FLAGS_AUTO;
	 }
	 break;
				
	 case VISCA_WHITE_BALANCE_INDOOR:
	 {
	    property->value = 3200;
	 }
	 break;
				
	 case VISCA_WHITE_BALANCE_OUTDOOR:
	 {
	    property->value = 5600;
	 }
	 break;
      }
   }		

   return status;
}

unicap_status_t visca_set_zoom( vid21394handle_t vid21394handle, 
				unicap_property_t *property )
{
   int out_bytes, in_bytes;
   unsigned char out_data[12];
   unsigned char in_data[8];
   unicap_status_t status;

   unsigned char p1, p2, p3, p4;

   unsigned int value;
		
   out_bytes = 9;
   in_bytes = 3;

   value = property->value;

   p1 = ( value >> 12 ) & 0xf;
   p2 = ( value >> 8 ) & 0xf;
   p3 = ( value >> 4 ) & 0xf ; 
   p4 = value & 0xf;
		
   visca_init_command_packet_4( out_data, VISCA_COMMAND_ZOOM_DIRECT, p1, p2, p3, p4 );
		
   visca_htofla( out_data, out_bytes );
	
   status = vid21394_rs232_io( vid21394handle, 
			       out_data, out_bytes, 
			       in_data, in_bytes );

   return status;
}

unicap_status_t visca_get_zoom( vid21394handle_t vid21394handle, 
				unicap_property_t *property )
{
   int out_bytes, in_bytes;
   unsigned char out_data[8];
   unsigned char in_data[8];
   unicap_status_t status;
		
   out_bytes = 5;
   in_bytes = 7;
	
   visca_init_inq_packet( out_data, VISCA_COMMAND_ZOOM_DIRECT );
	
   visca_htofla( out_data, out_bytes );
	
   status = vid21394_rs232_io( vid21394handle, 
			       out_data, out_bytes, 
			       in_data, in_bytes );

   if( SUCCESS( status ) )
   {
      property->value = ( in_data[2] * 0x1000 + 
			  in_data[3] * 0x100 + 
			  in_data[4] * 0x10 + 
			  in_data[5] );
   }			

   return status;
}

unicap_status_t visca_set_focus( vid21394handle_t vid21394handle, 
				 unicap_property_t *property )
{
   int out_bytes, in_bytes;
   unsigned char out_data[12];
   unsigned char in_data[8];
   unicap_status_t status;

		

   if( property->flags & UNICAP_FLAGS_AUTO )
   {
      out_bytes = 6;
      in_bytes = 6;
			
      visca_init_command_packet_1( out_data, VISCA_COMMAND_FOCUS_AUTO, VISCA_FOCUS_AUTO );
   }
   else
   {
      out_bytes = 6;
      in_bytes = 6;

      visca_init_command_packet_1( out_data, VISCA_COMMAND_FOCUS_AUTO, VISCA_FOCUS_MANUAL );
			
      visca_htofla( out_data, out_bytes );
	
      status = vid21394_rs232_io( vid21394handle, 
				  out_data, out_bytes, 
				  in_data, in_bytes );

      if( SUCCESS( status ) )
      {
	 unsigned char p1, p2, p3, p4;
			
	 unsigned int value;

	 out_bytes = 9;
	 in_bytes = 6;

	 value = property->value;
				
	 p1 = ( value >> 12 ) & 0xf;
	 p2 = ( value >> 8 ) & 0xf;
	 p3 = ( value >>4 ) & 0xf;
	 p4 = value & 0xf;
				
	 visca_init_command_packet_4( out_data, VISCA_COMMAND_FOCUS_DIRECT, p1, p2, p3, p4 );
      }
      else
      {
	 return status;
      }
   }
		
   visca_htofla( out_data, out_bytes );
	
   status = vid21394_rs232_io( vid21394handle, 
			       out_data, out_bytes, 
			       in_data, in_bytes );


   return status;
}

unicap_status_t visca_get_focus( vid21394handle_t vid21394handle, 
				 unicap_property_t *property )
{
   int out_bytes, in_bytes;
   unsigned char out_data[8];
   unsigned char in_data[8];
   unicap_status_t status;
		
   out_bytes = 5;
   in_bytes = 4;
	
   visca_init_inq_packet( out_data, VISCA_COMMAND_FOCUS_AUTO );
		
   visca_htofla( out_data, out_bytes );
	
   status = vid21394_rs232_io( vid21394handle, 
			       out_data, out_bytes, 
			       in_data, in_bytes );


   if( SUCCESS( status ) )
   {
      status = visca_check_inq_response( in_data );

      property->flags = UNICAP_FLAGS_MANUAL;
			
      switch( in_data[2] )
      {
	 case VISCA_FOCUS_AUTO:
	 {
	    property->flags = UNICAP_FLAGS_AUTO;
/* 	    TRACE( "AUTO FOCUS enabled\n" ); */
	 }
	 break;
				
	 default:
	 case VISCA_FOCUS_MANUAL:
	 {
/* 	    TRACE( "MANUAL FOCUS enabled\n" ); */
	 }
	 break;				
      }
      out_bytes = 5;
      in_bytes = 7;
		
      visca_init_inq_packet( out_data, VISCA_COMMAND_FOCUS_DIRECT );
		
      visca_htofla( out_data, out_bytes );
	
      status = vid21394_rs232_io( vid21394handle, 
				  out_data, out_bytes, 
				  in_data, in_bytes );


      if( SUCCESS( status ) )
      {
	 property->value = ( in_data[2] * 0x1000 + 
			     in_data[3] * 0x100 + 
			     in_data[4] * 0x10 + 
			     in_data[5] );
      }
   }		

   return status;
}

unicap_status_t visca_set_gain( vid21394handle_t vid21394handle, 
				unicap_property_t *property )
{
   int out_bytes, in_bytes;
   unsigned char out_data[12];
   unsigned char in_data[8];
   unicap_status_t status;	

   unsigned char p3, p4;
   unsigned int value = property->value;
		
		
   out_bytes = 9;
   in_bytes = 6;
				
   p3 = ( value & 0xf0 ) >> 8;
   p4 = value & 0xf;
		
   visca_init_command_packet_4( out_data, VISCA_COMMAND_FOCUS_DIRECT, 0, 0, p3, p4 );
		
   visca_htofla( out_data, out_bytes );
	
   status = vid21394_rs232_io( vid21394handle, 
			       out_data, out_bytes, 
			       in_data, in_bytes );

   return status;
}

unicap_status_t visca_get_gain( vid21394handle_t vid21394handle, 
				unicap_property_t *property )
{
   int out_bytes, in_bytes;
   unsigned char out_data[8];
   unsigned char in_data[8];
   unicap_status_t status;
		
   out_bytes = 5;
   in_bytes = 7;

   visca_init_inq_packet( out_data, VISCA_COMMAND_GAIN_DIRECT );
		
   visca_htofla( out_data, out_bytes );
	
   status = vid21394_rs232_io( vid21394handle, 
			       out_data, out_bytes, 
			       in_data, in_bytes );

   if( SUCCESS( status ) )
   {
      property->value = ( in_data[4] * 0x10 + 
			  in_data[5] );
   }
		
   property->flags = UNICAP_FLAGS_MANUAL;

   return status;
}

unicap_status_t visca_set_shutter( vid21394handle_t vid21394handle, 
				   unicap_property_t *property )
{
   int out_bytes, in_bytes;
   unsigned char out_data[12];
   unsigned char in_data[8];
   unicap_status_t status;

		

   unsigned char p3, p4;
		
   unsigned int value = property->value;
		
   out_bytes = 9;
   in_bytes = 6;
				
   p3 = ( value & 0xf0 ) >> 8;
   p4 = value & 0xf;
		
   visca_init_command_packet_4( out_data, VISCA_COMMAND_SHUTTER_DIRECT, 0, 0, p3, p4 );
		
   visca_htofla( out_data, out_bytes );
	
   status = vid21394_rs232_io( vid21394handle, 
			       out_data, out_bytes, 
			       in_data, in_bytes );

   return status;
}

unicap_status_t visca_get_shutter( vid21394handle_t vid21394handle, 
				   unicap_property_t *property )
{
   int out_bytes, in_bytes;
   unsigned char out_data[8];
   unsigned char in_data[8];
   unicap_status_t status;
		
   out_bytes = 5;
   in_bytes = 7;

   visca_init_inq_packet( out_data, VISCA_COMMAND_SHUTTER_DIRECT );
		
   visca_htofla( out_data, out_bytes );

   status = vid21394_rs232_io( vid21394handle, 
			       out_data, out_bytes, 
			       in_data, in_bytes );

   if( SUCCESS( status ) )
   {
      property->value = ( in_data[4] * 0x10 + 
			  in_data[5] );
   }
		
   property->flags = UNICAP_FLAGS_MANUAL;

   return status;
}

unicap_status_t visca_set_iris( vid21394handle_t vid21394handle, 
				unicap_property_t *property )
{
   int out_bytes, in_bytes;
   unsigned char out_data[12];
   unsigned char in_data[8];
   unicap_status_t status;

		

   unsigned char p3, p4;
		
   unsigned int value = property->value;
		
   out_bytes = 9;
   in_bytes = 6;
				
   p3 = ( value & 0xf0 ) >> 8;
   p4 = value & 0xf;
		
   visca_init_command_packet_4( out_data, VISCA_COMMAND_IRIS_DIRECT, 0, 0, p3, p4 );
		
   visca_htofla( out_data, out_bytes );
	
   status = vid21394_rs232_io( vid21394handle, 
			       out_data, out_bytes, 
			       in_data, in_bytes );

   return status;
}

unicap_status_t visca_get_iris( vid21394handle_t vid21394handle, 
				unicap_property_t *property )
{
   int out_bytes, in_bytes;
   unsigned char out_data[8];
   unsigned char in_data[8];
   unicap_status_t status;
		
   out_bytes = 5;
   in_bytes = 7;

   visca_init_inq_packet( out_data, VISCA_COMMAND_IRIS_DIRECT );
		
   visca_htofla( out_data, out_bytes );
	
   status = vid21394_rs232_io( vid21394handle, 
			       out_data, out_bytes, 
			       in_data, in_bytes );

   if( SUCCESS( status ) )
   {
      property->value = ( in_data[4] * 0x10 + 
			  in_data[5] );
   }
		
   property->flags = UNICAP_FLAGS_MANUAL;

   return status;
}



unicap_status_t visca_set_ae_mode( vid21394handle_t vid21394handle, 
				   unicap_property_t *property )
{
   int out_bytes, in_bytes;
   unsigned char out_data[8];
   unsigned char in_data[8];
   unicap_status_t status;
   unsigned char param = 0xff;
   int i;

   out_bytes = 6;
   in_bytes = 6;
	
   for( i = 0; ae_modes[i].param != -1; i++ )
   {
      if( !strcmp( property->menu_item, ae_modes[i].menu_item ) )
      {
	 param = ae_modes[i].param;
	 break;
      }
   }
	
   if( param == 0xff )
   {
      return STATUS_INVALID_PARAMETER;
   };
	
   visca_init_command_packet_1( out_data, VISCA_COMMAND_AE_MODE, param );
	
   visca_htofla( out_data, out_bytes );

   status = vid21394_rs232_io( vid21394handle, 
			       out_data, out_bytes, 
			       in_data, in_bytes );
	
   return status;
}

unicap_status_t visca_get_ae_mode( vid21394handle_t vid21394handle, 
				   unicap_property_t *property )
{
   int out_bytes, in_bytes;
   unsigned char out_data[8];
   unsigned char in_data[8];
   unicap_status_t status;
	
   out_bytes = 5;
   in_bytes = 4;

   visca_init_inq_packet( out_data, VISCA_COMMAND_AE_MODE );
		
   visca_htofla( out_data, out_bytes );
	
   status = vid21394_rs232_io( vid21394handle, 
			       out_data, out_bytes, 
			       in_data, in_bytes );

   if( SUCCESS( status ) )
   {
      status = visca_check_inq_response( in_data );

      property->flags = UNICAP_FLAGS_MANUAL;
			
      switch( in_data[2] )
      {
	 case VISCA_AE_MODE_MANUAL:
	 {
	    strcpy( property->menu_item, VISCA_AE_MENU_ITEM_MANUAL );
	 }
	 break;
				
	 case VISCA_AE_MODE_FULL_AUTO:
	 {
	    strcpy( property->menu_item, VISCA_AE_MENU_ITEM_FULL_AUTO );
	 }
	 break;
				
	 case VISCA_AE_MODE_SHUTTER_PRIO:
	 {
	    strcpy( property->menu_item, VISCA_AE_MENU_ITEM_SHUTTER_PRIO );
	 }
	 break;

	 case VISCA_AE_MODE_IRIS_PRIO:
	 {
	    strcpy( property->menu_item, VISCA_AE_MENU_ITEM_IRIS_PRIO );
	 }
	 break;
			
	 case VISCA_AE_MODE_BRIGHT:
	 {
	    strcpy( property->menu_item, VISCA_AE_MENU_ITEM_BRIGHT );
	 }
	 break;
			
	 default:
	 {
	    strcpy( property->menu_item, "Unknown" );
	 }
	 break;
      }
   }		

   return status;
}

unicap_status_t visca_reenumerate_properties( vid21394handle_t vid21394handle, int *count )
{
   *count = ( sizeof( visca_property_table ) / sizeof( visca_property_t ) );
	
   return STATUS_SUCCESS;
}

unicap_status_t visca_enumerate_properties( unicap_property_t *property, int index )
{
   if( ( index < 0 ) ||
       ( index >= ( sizeof( visca_property_table ) / sizeof( visca_property_t ) ) ) )
   {
      return STATUS_NO_MATCH;
   }
	
   unicap_copy_property( property, &visca_property_table[index].property );
	
   return STATUS_SUCCESS;
}

unicap_status_t visca_set_property( vid21394handle_t vid21394handle, unicap_property_t *property )
{
   int i;
   unicap_status_t status = STATUS_INVALID_PARAMETER;
	
   for( i = 0; i < ( sizeof( visca_property_table ) / sizeof( visca_property_t ) ); i++ )
   {
      if( !strcmp( visca_property_table[i].property.identifier, property->identifier ) )
      {
	 status = visca_property_table[i].set_function( vid21394handle, property );
	 break;
      }
   }
	
   return status;
}

unicap_status_t visca_get_property( vid21394handle_t vid21394handle, unicap_property_t *property )
{
   int i;
   unicap_status_t status = STATUS_INVALID_PARAMETER;
	
   for( i = 0; i < ( sizeof( visca_property_table ) / sizeof( visca_property_t ) ); i++ )
   {
      if( !strcmp( visca_property_table[i].property.identifier, property->identifier ) )
      {
	 unicap_copy_property( property, & visca_property_table[i].property );
	 status = visca_property_table[i].get_function( vid21394handle, property );
	 break;
      }
   }
	
   return status;
}
	
unicap_status_t visca_check_camera( vid21394handle_t vid21394handle, visca_camera_type_t *type )
{
   int out_bytes, in_bytes;
   unsigned char out_data[8];
   unsigned char in_data[10];
   unicap_status_t status = STATUS_FAILURE;
	
   *type = VISCA_CAMERA_TYPE_NONE;

   out_bytes = 5;
   in_bytes = 10;

   visca_init_device_type_inq_packet( out_data );
	
   vid21394_rs232_set_baudrate( vid21394handle, 9600 );

   visca_htofla( out_data, out_bytes );
	
   status = vid21394_rs232_io( vid21394handle, 
			       out_data, out_bytes, 
			       in_data, in_bytes );
	
   if( !SUCCESS( status ) )
   {
      TRACE( "Get camera type failed\n" );
      return status;
   }

   // Check Vendor ID
   if( ( in_data[2] != 0x0 ) ||
       ( in_data[3] != 0x20 ) )
   {
      TRACE( "Wrong Vendor ID\n" );
   } 
   else if( ( in_data[4] != 0x4 ) ) 	// Check Model ID
   {
      TRACE( "Unknown camera type!\n" );
      *type = VISCA_CAMERA_TYPE_UNKNOWN;
   }
   else
   {
      *type = VISCA_CAMERA_TYPE_FCB_IX47;
   }
	
   return status;
}
