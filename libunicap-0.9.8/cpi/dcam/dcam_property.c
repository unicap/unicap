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

#include <stdlib.h>

#include "dcam.h"
#include "dcam_functions.h"
#include "dcam_property_table.h"
#include "dcam_v_modes.h"
#include "dcam_defs.h"

#include "1394util.h"

#if DCAM_DEBUG
#define DEBUG
#endif
#include "debug.h"


struct _dcam_raw_register
{
      nodeaddr_t address;
      quadlet_t  value;
};

typedef struct _dcam_raw_register dcam_raw_register_t;

/*
  returns 1 if the given property is enabled in either feature_hi or feature_lo
*/
int _dcam_check_property_supported( quadlet_t feature_hi, 
				    quadlet_t feature_lo, 
				    dcam_property_t *property )
{
   int supported = 0;
   switch( property->register_offset )
   {
      case 0:
	 supported = (feature_hi>>31)&1; //.brightness;
	 break;
			
      case 0x4:
	 supported = (feature_hi>>30)&1; //.auto_exposure;
	 break;
			
      case 0x8:
	 supported = (feature_hi>>29)&1; //.sharpness;
	 break;
			
      case 0xc:
	 supported = (feature_hi>>28)&1; //.white_balance;
	 break;
			
      case 0x10:
	 supported = (feature_hi>>27)&1; //.hue;
	 break;
			
      case 0x14:
	 supported = (feature_hi>>26)&1; //.saturation;
	 break;
			
      case 0x18:
	 supported = (feature_hi>>25)&1; //.gamma;
	 break;
			
      case 0x1c:
	 supported = (feature_hi>>24)&1; //.shutter;
	 break;
			
      case 0x20:
	 supported = (feature_hi>>23)&1; //.gain;
	 break;
			
      case 0x24:
	 supported = (feature_hi>>22)&1; //.iris;
	 break;
			
      case 0x28:
	 supported = (feature_hi>>21)&1; //.focus;
	 break;
			
      case 0x2c:
	 supported = (feature_hi>>20)&1; //.temperature;
	 break;
			
      case 0x30:
	 supported = (feature_hi>>19)&1; //.trigger;
	 break;
			
      case 0x80:
	 supported = (feature_lo>>31)&1; //.zoom;
	 break;
			
      case 0x84:
	 supported = (feature_lo>>30)&1; //.pan;
	 break;
			
      case 0x88:
	 supported = (feature_lo>>29)&1; //.tilt;
	 break;
			
      case 0x8c:
	 supported = (feature_lo>>28)&1; //.optical_filter;
	 break;
			
      case 0xc0:
	 supported = (feature_lo>>16)&1; //.capture_size;
	 break;
			
      case 0xc4:
	 supported = (feature_lo>>15)&1; //.capture_quality;
	 break;
			
      default:
	 supported = 0;
	 break;
   }
	
   return supported;
}

unicap_status_t dcam_init_property_std_flags( dcam_handle_t dcamhandle, 
					      dcam_property_t *dcam_property )
{
   quadlet_t val = dcam_property->register_inq;

   dcam_property->unicap_property.flags_mask = 0;
	
   if( GETFLAG_ON_OFF_INQ( val ) ) // ON_OFF inq
   {
      dcam_property->unicap_property.flags_mask |= UNICAP_FLAGS_ON_OFF;
   }
   if( GETFLAG_AUTO_INQ( val ) && // AUTO inq
       (dcam_property->type != PPTY_TYPE_TRIGGER ) && 
        (dcam_property->type != PPTY_TYPE_TRIGGER_POLARITY ))
   {
      dcam_property->unicap_property.flags_mask |= UNICAP_FLAGS_AUTO;
   }
   if( GETFLAG_MANUAL_INQ( val ) && // MANUAL inq
       (dcam_property->type != PPTY_TYPE_TRIGGER ) && 
       (dcam_property->type != PPTY_TYPE_TRIGGER_POLARITY ) )
   {
      dcam_property->unicap_property.flags_mask |= UNICAP_FLAGS_MANUAL;
   }
   if( GETFLAG_ONE_PUSH_INQ( val ) && // ONE_PUSH inq
       (dcam_property->type != PPTY_TYPE_TRIGGER ) && 
       (dcam_property->type != PPTY_TYPE_TRIGGER_POLARITY ) )
   {
      dcam_property->unicap_property.flags_mask |= UNICAP_FLAGS_ONE_PUSH;
   }

   return STATUS_SUCCESS;
}

unicap_status_t dcam_read_default_and_inquiry( dcam_handle_t dcamhandle, 
					       dcam_property_t *dcam_property )
{
   quadlet_t val;
   quadlet_t default_val;

   // read inquiry register for this ppty
   if( _dcam_read_register( dcamhandle->raw1394handle, 
			    dcamhandle->node, 
			    dcamhandle->command_regs_base + 0x500 + dcam_property->register_offset, 
			    &val ) < 0 )
   {
      dcam_property->register_inq = 0;
      return STATUS_FAILURE;
   }

   dcam_property->register_inq = val;
		
   TRACE( "Inquiry[%x]: %08x\n", dcam_property->register_offset, dcam_property->register_inq );

   // 
   // only consider properties if: 
   // - the property inquiry register has presence flag set
   if( GETFLAG_PRESENCE_INQ( val )  )
   {
      if( _dcam_read_register( dcamhandle->raw1394handle, 
			       dcamhandle->node, 
			       dcamhandle->command_regs_base + 0x800 + dcam_property->register_offset, 
			       &default_val ) < 0 )
      {
	 dcam_property->register_inq = 0;
	 return STATUS_FAILURE;
      }
			
      if( !GETFLAG_PRESENCE_INQ( default_val ) )
      {
	 TRACE( "dcam: property_inq reports presence but property_cntl does not! disabling property\n" );
	 dcam_property->register_inq = 0;
	 return STATUS_NOT_IMPLEMENTED;
      }
			
      dcam_property->register_default = default_val;
      dcam_property->register_value = default_val;
   }
   else
   {
      return STATUS_NOT_IMPLEMENTED;
   }

   return STATUS_SUCCESS;
}

unicap_status_t dcam_init_gpio_property( dcam_handle_t dcamhandle, 
					 unicap_property_t *p, 
					 dcam_property_t *dcam_property )
{
   if( dcamhandle->unicap_device.vendor_id != 0x748 )
   {
      return STATUS_NOT_IMPLEMENTED;
   }
	
   if( ( ( dcamhandle->unicap_device.model_id >> 32 ) & 0xff ) != 0x12 )
   {
      return STATUS_NOT_IMPLEMENTED;
   }
	
   return STATUS_SUCCESS;
}


unicap_status_t dcam_set_gpio_property( dcam_handle_t dcamhandle, 
					unicap_property_t *property, 
					dcam_property_t *dcam_property )
{
   quadlet_t quad;
   unicap_status_t status;
	
   if( property->property_data_size < ( sizeof( unsigned int ) ) )
   {
      return STATUS_INVALID_PARAMETER;
   }
	
   quad = *(quadlet_t *)property->property_data;

   status = _dcam_write_register( dcamhandle->raw1394handle, 
				  dcamhandle->node, 
				  dcamhandle->command_regs_base + 0x1000000 + dcam_property->register_offset, 
				  quad );

   return status;
}

unicap_status_t dcam_get_gpio_property( dcam_handle_t dcamhandle, 
					unicap_property_t *property, 
					dcam_property_t *dcam_property )
{
   quadlet_t quad;
   unicap_status_t status;
	
   if( property->property_data_size < ( sizeof( unsigned int ) ) )
   {
      return STATUS_INVALID_PARAMETER;
   }
	

   status = _dcam_read_register( dcamhandle->raw1394handle, 
				 dcamhandle->node, 
				 dcamhandle->command_regs_base + 0x1000000 + dcam_property->register_offset, 
				 &quad );

   *(quadlet_t *)property->property_data = quad;

   return status;
}

unicap_status_t dcam_init_software_trigger_property( dcam_handle_t dcamhandle, 
						     unicap_property_t *p, 
						     dcam_property_t *dcam_property )
{
   if( dcamhandle->unicap_device.vendor_id != 0x748 )
   {
      return STATUS_NOT_IMPLEMENTED;
   }
	
   if( ( ( dcamhandle->unicap_device.model_id >> 32 ) & 0xff ) != 0x12 )
   {
      return STATUS_NOT_IMPLEMENTED;
   }
	
   return STATUS_SUCCESS;
}


unicap_status_t dcam_set_software_trigger_property( dcam_handle_t dcamhandle, 
					unicap_property_t *property, 
					dcam_property_t *dcam_property )
{
   quadlet_t quad = 0;
   unicap_status_t status;
	
   if( property->flags & UNICAP_FLAGS_ONE_PUSH )
   {
      quad = SETFLAG_ONE_PUSH( quad, 1 );
   }
   else
   {
      quad = SETFLAG_ONE_PUSH( quad, 0 );
   }
   
      
   status = _dcam_write_register( dcamhandle->raw1394handle, 
				  dcamhandle->node, 
				  dcamhandle->command_regs_base + 0x1000000 + dcam_property->register_offset, 
				  quad );

   return status;
}

unicap_status_t dcam_get_software_trigger_property( dcam_handle_t dcamhandle, 
						    unicap_property_t *property, 
						    dcam_property_t *dcam_property )
{
	
   property->flags = UNICAP_FLAGS_MANUAL;

   return STATUS_SUCCESS;
}

unicap_status_t dcam_set_strobe_mode_property( dcam_handle_t dcamhandle, 
					       unicap_property_t *property, 
					       dcam_property_t *dcam_property )
{
   quadlet_t quad;
   unicap_status_t status = STATUS_SUCCESS;
   
   quad = 0;

   status = _dcam_read_register( dcamhandle->raw1394handle, 
				 dcamhandle->node, 
				 dcamhandle->command_regs_base + 0x1000000 + dcam_property->register_offset, 
				 &quad );
   
   quad = SETFLAG_PRESENCE( quad, 1 );
   quad = SETFLAG_ON_OFF( quad, 1 );
   

   if( !strcmp( property->menu_item, "constant low" ) )
   {
      quad = SETFLAG_ONE_PUSH( quad, 0 ); // polarity 0 
      quad = SETVAL_V_VALUE( quad, 0 ); // duration 0
      quad = SETVAL_U_VALUE( quad, 0 ); // delay 0
      quad = SETFLAG_AUTO( quad, 0 ); // mode 0 = fixed duration
   }
   else if( !strcmp( property->menu_item, "constant high" ) )
   {
      quad = SETFLAG_ONE_PUSH( quad, 1 ); // polarity 1
      quad = SETVAL_V_VALUE( quad, 0 ); // duration 0
      quad = SETVAL_U_VALUE( quad, 0 ); // delay 0
      quad = SETFLAG_AUTO( quad, 0 ); // mode 0 = fixed duration
   }
   else if( !strcmp( property->menu_item, "fixed duration" ) )
   {
      quad = SETFLAG_AUTO( quad, 0 ); // mode 0 = fixed duration
      quad = SETVAL_V_VALUE( quad, 1 );
   }
   else if( !strcmp( property->menu_item, "exposure" ) )
   {
      quad = SETFLAG_AUTO( quad, 1 ); // mode 1 = strobe follows exposure
   }
   else
   {
      status = STATUS_INVALID_PARAMETER;
      TRACE( "unknown strobe mode\n" );
   }

   if( SUCCESS( status ) )
   {
      status = _dcam_write_register( dcamhandle->raw1394handle, 
				     dcamhandle->node, 
				     dcamhandle->command_regs_base + 0x1000000 + dcam_property->register_offset, 
				     quad );
   }
   
   return status;
}

unicap_status_t dcam_get_strobe_mode_property( dcam_handle_t dcamhandle, 
					       unicap_property_t *property, 
					       dcam_property_t *dcam_property )
{
   unicap_status_t status;
   quadlet_t quad;
   
   status = _dcam_read_register( dcamhandle->raw1394handle, 
				 dcamhandle->node, 
				 dcamhandle->command_regs_base + 0x1000000 + dcam_property->register_offset, 
				 &quad );
   
   if( GETFLAG_AUTO( quad ) )
   {
      // mode = 1 --> strobe follows exposure
      strcpy( property->menu_item, dcam_property->unicap_property.menu.menu_items[3] );
   }
   else if( GETVAL_V_VALUE( quad ) == 0 )
   {
      if( GETFLAG_ONE_PUSH( quad ) )
      {
	 // polarity = 1 --> constant high
	 strcpy( property->menu_item, dcam_property->unicap_property.menu.menu_items[1] );
      }
      else
      {
	 // polarity == 0 -> constant lo
	 strcpy( property->menu_item, dcam_property->unicap_property.menu.menu_items[0] );
      }
   }
   else
   {
      strcpy( property->menu_item, dcam_property->unicap_property.menu.menu_items[2] );
   }
   
   return status;
}

unicap_status_t dcam_set_strobe_duration_property( dcam_handle_t dcamhandle, 
						   unicap_property_t *property, 
						   dcam_property_t *dcam_property )
{
   quadlet_t quad;
   unicap_status_t status;
   
   status = _dcam_read_register( dcamhandle->raw1394handle, 
				 dcamhandle->node, 
				 dcamhandle->command_regs_base + 0x1000000 + dcam_property->register_offset, 
				 &quad );

   quad = SETFLAG_ON_OFF( quad, 1 );
   quad = SETVAL_V_VALUE( quad, (int) ( property->value / 10.0 ) );
   
   if( SUCCESS( status ) )
   {
      status = _dcam_write_register( dcamhandle->raw1394handle, 
				     dcamhandle->node, 
				     dcamhandle->command_regs_base + 0x1000000 + dcam_property->register_offset, 
				     quad );
   }
   
   return status;
}

unicap_status_t dcam_get_strobe_duration_property( dcam_handle_t dcamhandle, 
						   unicap_property_t *property, 
						   dcam_property_t *dcam_property )
{
   quadlet_t quad;
   double value;
   unicap_status_t status;
   
   status = _dcam_read_register( dcamhandle->raw1394handle, 
				 dcamhandle->node, 
				 dcamhandle->command_regs_base + 0x1000000 + dcam_property->register_offset, 
				 &quad );

   value = GETVAL_V_VALUE( quad );
   property->value = value * 10.0;
   
   property->flags_mask = UNICAP_FLAGS_MANUAL;

   if( GETVAL_V_VALUE(quad) == 0 )
   {
      property->flags = UNICAP_FLAGS_READ_ONLY;
   }
   else
   {
      property->flags = UNICAP_FLAGS_MANUAL;
   }
   return status;
}

unicap_status_t dcam_set_strobe_delay_property( dcam_handle_t dcamhandle, 
						unicap_property_t *property, 
						dcam_property_t *dcam_property )
{
   quadlet_t quad;
   unicap_status_t status;
   
   status = _dcam_read_register( dcamhandle->raw1394handle, 
				 dcamhandle->node, 
				 dcamhandle->command_regs_base + 0x1000000 + dcam_property->register_offset, 
				 &quad );

   quad = SETFLAG_ON_OFF( quad, 1 );
   quad = SETVAL_U_VALUE( quad, (int) ( property->value / 10.0 ) );
   
   if( SUCCESS( status ) )
   {
      status = _dcam_write_register( dcamhandle->raw1394handle, 
				     dcamhandle->node, 
				     dcamhandle->command_regs_base + 0x1000000 + dcam_property->register_offset, 
				     quad );
   }
   
   return status;
}

unicap_status_t dcam_get_strobe_delay_property( dcam_handle_t dcamhandle, 
						unicap_property_t *property, 
						dcam_property_t *dcam_property )
{
   quadlet_t quad;
   double value;
   unicap_status_t status;
   
   status = _dcam_read_register( dcamhandle->raw1394handle, 
				 dcamhandle->node, 
				 dcamhandle->command_regs_base + 0x1000000 + dcam_property->register_offset, 
				 &quad );

   value = GETVAL_U_VALUE( quad );
   property->value = value * 10.0;

   property->flags_mask = UNICAP_FLAGS_MANUAL;

   if( GETVAL_V_VALUE(quad) == 0 )
   {
      property->flags = UNICAP_FLAGS_READ_ONLY;
   }
   else
   {
      property->flags = UNICAP_FLAGS_MANUAL;
   }
   return status;
}

unicap_status_t dcam_set_strobe_polarity_property( dcam_handle_t dcamhandle, 
						   unicap_property_t *property, 
						   dcam_property_t *dcam_property )
{
   quadlet_t quad;
   unicap_status_t status = STATUS_SUCCESS;
   
   quad = 0;

   status = _dcam_read_register( dcamhandle->raw1394handle, 
				 dcamhandle->node, 
				 dcamhandle->command_regs_base + 0x1000000 + dcam_property->register_offset, 
				 &quad );
   
   quad = SETFLAG_PRESENCE( quad, 1 );
   quad = SETFLAG_ON_OFF( quad, 1 );
   

   if( !strcmp( property->menu_item, "active low" ) )
   {
      quad = SETFLAG_ONE_PUSH( quad, 0 ); // polarity 0 
   }
   else if( !strcmp( property->menu_item, "active high" ) )
   {
      quad = SETFLAG_ONE_PUSH( quad, 1 ); // polarity 1
   }
   else
   {
      status = STATUS_INVALID_PARAMETER;
      TRACE( "unknown strobe mode\n" );
   }

   if( SUCCESS( status ) )
   {
      status = _dcam_write_register( dcamhandle->raw1394handle, 
				     dcamhandle->node, 
				     dcamhandle->command_regs_base + 0x1000000 + dcam_property->register_offset, 
				     quad );
   }
   
   return status;
}

unicap_status_t dcam_get_strobe_polarity_property( dcam_handle_t dcamhandle, 
						   unicap_property_t *property, 
						   dcam_property_t *dcam_property )
{
   unicap_status_t status;
   quadlet_t quad;
   
   status = _dcam_read_register( dcamhandle->raw1394handle, 
				 dcamhandle->node, 
				 dcamhandle->command_regs_base + 0x1000000 + dcam_property->register_offset, 
				 &quad );
   
   if( GETFLAG_ONE_PUSH( quad ) )
   {
      // polarity = 1 
      strcpy( property->menu_item, dcam_property->unicap_property.menu.menu_items[1] );
   }
   else
   {
      // polarity == 0 -> constant lo
      strcpy( property->menu_item, dcam_property->unicap_property.menu.menu_items[0] );
   }
   
   property->flags_mask = UNICAP_FLAGS_MANUAL;

   if( GETVAL_V_VALUE(quad) == 0 )
   {
      property->flags = UNICAP_FLAGS_READ_ONLY;
   }
   else
   {
      property->flags = UNICAP_FLAGS_MANUAL;
   }
   return status;
}

unicap_status_t dcam_init_timeout_property( dcam_handle_t dcamhandle, 
					    unicap_property_t *p, 
					    dcam_property_t *dcam_property )
{
   dcamhandle->timeout_seconds = 1.0;
   return STATUS_SUCCESS;
}

unicap_status_t dcam_get_timeout_property( dcam_handle_t dcamhandle, 
					   unicap_property_t *property, 
					   dcam_property_t *dcam_property )
{
   unicap_copy_property( property, &dcam_property->unicap_property );
   property->value = dcamhandle->timeout_seconds;
   
   return STATUS_SUCCESS;
}


unicap_status_t dcam_set_timeout_property( dcam_handle_t dcamhandle, 
					   unicap_property_t *property, 
					   dcam_property_t *dcam_property )
{
   unicap_status_t status = STATUS_SUCCESS;
	
   if( ( property->value >= dcam_property->unicap_property.range.min ) &&
       ( property->value <= dcam_property->unicap_property.range.max ) )
   {
      dcamhandle->timeout_seconds = property->value;
   }
   else
   {
      status = STATUS_INVALID_PARAMETER;
   }
	
   return status;
}

static unicap_status_t dcam_set_shutter_abs( dcam_handle_t dcamhandle, 
					     unicap_property_t *property, 
					     dcam_property_t *dcam_property )
{
   unicap_status_t status = STATUS_SUCCESS;
   abs_value_t value;
   quadlet_t quad = 0;
	
   quad = SETFLAG_PRESENCE( quad, 1 );

   if( property->flags & UNICAP_FLAGS_AUTO )
   {
      quad = SETFLAG_AUTO( quad, 1 );
   }
   else
   {
      quad = SETFLAG_ABS_CONTROL( quad, 1 );
   }

   quad = SETFLAG_ON_OFF( quad, 1 );
	
   if( _dcam_write_register( dcamhandle->raw1394handle, 
			     dcamhandle->node, 
			     dcamhandle->command_regs_base + 0x800 + dcam_property->register_offset, 
			     quad ) < 0 )
   {
      TRACE( "failed to write shutter setting\n" );
      status = STATUS_FAILURE;
   }	

   if( SUCCESS( status ) )
   {
      value.f = property->value;	
		
      if( _dcam_write_register( dcamhandle->raw1394handle, 
				dcamhandle->node, 
				CSR_REGISTER_BASE + dcam_property->absolute_offset + 8, 
				value.quad ) < 0 ) 
      {
	 status = STATUS_FAILURE;
      }
   }
	
   return status;
}

static unicap_status_t dcam_get_shutter_abs( dcam_handle_t dcamhandle, 
					     unicap_property_t *property, 
					     dcam_property_t *dcam_property )
{
   unicap_status_t status = STATUS_SUCCESS;
   float value;
   quadlet_t quad;
	
   if( _dcam_read_register( dcamhandle->raw1394handle, 
			    dcamhandle->node, 
			    dcamhandle->command_regs_base + 0x800 + dcam_property->register_offset, 
			    &quad ) < 0 )
   {
      TRACE( "failed to read shutter setting\n" );
      status = STATUS_FAILURE;
   }

   if( GETFLAG_AUTO( quad ) )
   {
      property->flags = UNICAP_FLAGS_AUTO;
   }
   else
   {
      property->flags = UNICAP_FLAGS_MANUAL;
   }
	
   if( _dcam_read_register( dcamhandle->raw1394handle, 
			    dcamhandle->node, 
			    CSR_REGISTER_BASE + dcam_property->absolute_offset + 8, 
			    (quadlet_t *)&value ) < 0 ) 
   {
      status = STATUS_FAILURE;
   }
   else
   {
      property->value = value;
   }
	
   return status;
}

unicap_status_t dcam_init_shutter_property( dcam_handle_t dcamhandle, 
					    unicap_property_t *p, 
					    dcam_property_t *dcam_property )
{
   unicap_status_t status = STATUS_FAILURE;
   status = dcam_read_default_and_inquiry( dcamhandle, dcam_property );
   status = dcam_init_property_std_flags( dcamhandle, dcam_property );

   if( SUCCESS( status ) )
   {
      if( GETFLAG_ABS_CONTROL_INQ( dcam_property->register_inq ) )
      {
	 quadlet_t abs_offset;
	 float min;
	 float max;
	 float def;
	 quadlet_t quad;
	 // read inquiry register for this ppty
	 if( _dcam_read_register( dcamhandle->raw1394handle, 
				  dcamhandle->node, 
				  dcamhandle->command_regs_base + 0x700 + dcam_property->register_offset, 
				  &abs_offset ) < 0 )
	 {
	    TRACE( "failed to read abs offset\n" );
	    status = STATUS_FAILURE;
	 }
			
	 abs_offset *= 4;
	 dcam_property->absolute_offset = abs_offset;

	 TRACE( "abs_offset: %x\n", abs_offset );

	 if( SUCCESS( status ) )
	 {
	    if( _dcam_read_register( dcamhandle->raw1394handle, 
				     dcamhandle->node, 
				     CSR_REGISTER_BASE + abs_offset, 
				     (quadlet_t *)&min ) < 0 ) 
	    {
	       TRACE( "failed to read abs min : %llx\n", dcamhandle->command_regs_base );
	       status = STATUS_FAILURE;
	    }
	    else
	    {
	       dcam_property->unicap_property.range.min = min;
	       TRACE( "abs shutter min: %f\n", min );
	    }
	 }
			
	 if( SUCCESS( status ) )
	 {
	    if( _dcam_read_register( dcamhandle->raw1394handle, 
				     dcamhandle->node, 
				     CSR_REGISTER_BASE + abs_offset + 4, 
				     (quadlet_t *)&max ) < 0 ) 
	    {
	       TRACE( "failed to read abs max\n" );
	       status = STATUS_FAILURE;
	    }
	    else
	    {
	       dcam_property->unicap_property.range.max = max;
	       TRACE( "abs shutter max: %f\n", max );
	    }
	 }
			
	 if( _dcam_read_register( dcamhandle->raw1394handle, 
				  dcamhandle->node, 
				  dcamhandle->command_regs_base + 0x800 + dcam_property->register_offset, 
				  &quad ) < 0 )
	 {
	    TRACE( "failed to read shutter setting\n" );
	    status = STATUS_FAILURE;
	 }

	 if( SUCCESS( status ) )
	 {
	    quad = SETFLAG_ABS_CONTROL( quad, 1 );
	    if( _dcam_write_register( dcamhandle->raw1394handle, 
				      dcamhandle->node, 
				      dcamhandle->command_regs_base + 0x800 + dcam_property->register_offset, 
				      quad ) < 0 )
	    {
	       TRACE( "failed to write shutter setting\n" );
	       status = STATUS_FAILURE;
	    }
	 }

	 if( SUCCESS( status ) )
	 {
	    if( _dcam_read_register( dcamhandle->raw1394handle, 
				     dcamhandle->node, 
				     CSR_REGISTER_BASE + abs_offset + 8, 
				     (quadlet_t *)&def ) < 0 ) 
	    {
	       TRACE( "failed to read abs value\n" );
	       status = STATUS_FAILURE;
	    }
	    else
	    {
	       dcam_property->unicap_property.value = def;
	    }
	 }
			
	 if( SUCCESS( status ) )
	 {
	    dcam_property->unicap_property.stepping = 0.005;
	    dcam_property->set_function = dcam_set_shutter_abs;
	    dcam_property->get_function = dcam_get_shutter_abs;
	    strcpy( dcam_property->unicap_property.unit, "s" );
	 }
      }
      else
      {
	 TRACE( "Abs shutter not supported\n" );
	 status = STATUS_FAILURE;
      }
   }
	
   if( !SUCCESS( status ) )
   {
      status = dcam_init_brightness_property( dcamhandle, 
					      p, 
					      dcam_property );
   }
	
   return status;
}

unicap_status_t dcam_init_brightness_property( dcam_handle_t dcamhandle, 
					       unicap_property_t *p, 
					       dcam_property_t *dcam_property )
{
   unicap_status_t status;
	
   status = dcam_read_default_and_inquiry( dcamhandle, dcam_property );

   if( SUCCESS( status ) )
   {
      status = dcam_init_property_std_flags( dcamhandle, dcam_property );

      dcam_property->unicap_property.value = GETVAL_VALUE( dcam_property->register_default );
      dcam_property->unicap_property.range.min = GETVAL_MIN_VALUE( dcam_property->register_inq );
      dcam_property->unicap_property.range.max = GETVAL_MAX_VALUE( dcam_property->register_inq );
      dcam_property->unicap_property.stepping = 1.0f;
   }
	
   return status;
}

unicap_status_t dcam_init_white_balance_mode_property( dcam_handle_t dcamhandle, 
						       unicap_property_t *p, 
						       dcam_property_t *dcam_property )
{
   unicap_status_t status;
	
   status = dcam_read_default_and_inquiry( dcamhandle, dcam_property );

   if( SUCCESS( status ) )
   {
      status = dcam_init_property_std_flags( dcamhandle, dcam_property );
   }
	
   return status;
}

unicap_status_t dcam_init_white_balance_property( dcam_handle_t dcamhandle, 
						  unicap_property_t *p, 
						  dcam_property_t *dcam_property )
{
   unicap_status_t status;
	
   status = dcam_read_default_and_inquiry( dcamhandle, dcam_property );

   if( SUCCESS( status ) )
   {

      // FLAGS are handled via "white_balance_mode" property 
      dcam_property->unicap_property.flags = 0;
      dcam_property->unicap_property.flags_mask = UNICAP_FLAGS_MANUAL;
		
      if( dcam_property->type == PPTY_TYPE_WHITEBAL_U )
      {
	 dcam_property->unicap_property.value = GETVAL_U_VALUE( dcam_property->register_default );
      }
      else
      {
	 dcam_property->unicap_property.value = GETVAL_V_VALUE( dcam_property->register_default );
      }
      dcam_property->unicap_property.range.min = GETVAL_MIN_VALUE( dcam_property->register_inq );
      dcam_property->unicap_property.range.max = GETVAL_MAX_VALUE( dcam_property->register_inq );
      dcam_property->unicap_property.stepping = 1.0f;
   }
	
   return status;
}

unicap_status_t dcam_init_trigger_property( dcam_handle_t dcamhandle, 
					    unicap_property_t *p, 
					    dcam_property_t *dcam_property )
{
   unicap_status_t status;	
   int trigger_mode_count = 1;

   dcamhandle->trigger_modes[0] = dcam_trigger_modes[0];
   dcamhandle->trigger_modes[1] = dcam_trigger_modes[1];
   dcamhandle->trigger_polarities[0] = dcam_trigger_polarity[0];
   dcamhandle->trigger_polarities[1] = dcam_trigger_polarity[1];

   status = dcam_read_default_and_inquiry( dcamhandle, dcam_property );

   if( SUCCESS( status ) )
   {
      status = dcam_init_property_std_flags( dcamhandle, dcam_property );
      if( dcam_property->id == DCAM_PPTY_TRIGGER_MODE )
      {
	 if( ( ( dcam_property->register_inq >> 15 ) & 1 ) ) // trigger mode 0
	 {
	    dcamhandle->trigger_modes[trigger_mode_count++] = dcam_trigger_modes[1];
	 }
	 if( ( ( dcam_property->register_inq >> 16 ) & 1 ) ) // trigger mode 1
	 {
	    dcamhandle->trigger_modes[trigger_mode_count++] = dcam_trigger_modes[2];
	 }
	 if( ( ( dcam_property->register_inq >> 17 ) & 1 ) ) // trigger mode 2
	 {
	    dcamhandle->trigger_modes[trigger_mode_count++] = dcam_trigger_modes[3];
	 }
	 if( ( ( dcam_property->register_inq >> 18 ) & 1 ) ) // trigger mode 3
	 {
	    dcamhandle->trigger_modes[trigger_mode_count++] = dcam_trigger_modes[4];
	 }
	 dcam_property->unicap_property.menu.menu_items = dcamhandle->trigger_modes;
	 dcam_property->unicap_property.menu.menu_item_count = trigger_mode_count;
	 dcamhandle->trigger_mode_count = trigger_mode_count;

/* 	 dcam_property->unicap_property.value = ( dcam_property->register_default >> 16 ) & 0xf; */
	 if( ( ( dcam_property->register_default >> 16 ) & 0xf ) < trigger_mode_count )
	 {
	    strcpy( dcam_property->unicap_property.menu_item, dcam_trigger_modes[ ( dcam_property->register_default>>16 ) & 0xf ] );
	 }
	 else
	 {
	    TRACE( "Trigger default value out of range\n" );
	 }
	 dcam_property->unicap_property.property_data_size = 4;
	 dcam_property->unicap_property.property_data = ( &dcamhandle->trigger_parameter );
	 dcamhandle->trigger_parameter = ( dcam_property->register_default & 0xfff);
	 dcam_property->unicap_property.flags_mask = UNICAP_FLAGS_MANUAL;
      }
      else
      {
	 // TRIGGER_POLARITY
	 if( ( dcam_property->register_inq >> 26 ) & 0x1 )
	 {
	    int i;
	    strcpy( dcam_property->unicap_property.menu_item, dcamhandle->trigger_polarities[( dcam_property->register_default >> 26 ) & 1] );
	    dcam_property->unicap_property.menu.menu_item_count = 2;
	    dcam_property->unicap_property.menu.menu_items = dcamhandle->trigger_polarities;
	    dcam_property->unicap_property.flags_mask = UNICAP_FLAGS_MANUAL;
	 }
	 else
	 {
	    status = STATUS_FAILURE;
	 }
      }
   }

   return status;
}

unicap_status_t dcam_init_frame_rate_property( dcam_handle_t dcamhandle, 
					       unicap_property_t *p, 
					       dcam_property_t *dcam_property )
{
   quadlet_t quad;
   quad = _dcam_get_supported_frame_rates( dcamhandle );
	
   if( dcam_property->unicap_property.value_list.value_count > 0 )
   {
      free( dcam_property->unicap_property.value_list.values );
   }
	
   // 8 == max frame rates supported in DCAM 1.3
   dcam_property->unicap_property.value_list.values = malloc( sizeof( double ) * 8 ); 
   dcam_property->unicap_property.value_list.value_count = 0;

   if( quad )
   {
      if( ( quad >> 31 ) &1 )
      {
	 dcam_property->unicap_property.value_list.values[dcam_property->unicap_property.value_list.value_count++] = 
	    1.875;
      }
      if( ( quad >> 30 ) &1 )
      {
	 dcam_property->unicap_property.value_list.values[dcam_property->unicap_property.value_list.value_count++] = 
	    3.75;
      }
      if( ( quad >> 29 ) &1 )
      {
	 dcam_property->unicap_property.value_list.values[dcam_property->unicap_property.value_list.value_count++] = 
	    7.5;
      }
      if( ( quad >> 28 ) &1 )
      {
	 dcam_property->unicap_property.value_list.values[dcam_property->unicap_property.value_list.value_count++] = 
	    15;
      }
      if( ( quad >> 27 ) &1 )
      {
	 dcam_property->unicap_property.value_list.values[dcam_property->unicap_property.value_list.value_count++] = 
	    30;
      }
      if( ( quad >> 26 ) & 1 )
      {
	 dcam_property->unicap_property.value_list.values[dcam_property->unicap_property.value_list.value_count++] = 
	    60;
      }
   }
   dcam_property->unicap_property.flags_mask = UNICAP_FLAGS_MANUAL;

   TRACE( "count: %d\n", dcam_property->unicap_property.value_list.value_count );

   return STATUS_SUCCESS;
}

unicap_status_t dcam_init_rw_register_property( dcam_handle_t dcamhandle, 
						unicap_property_t *p, 
						dcam_property_t *dcam_property )
{
   return STATUS_SUCCESS;
}



/*
  fill property_table with properties supported by this camera. set _property_count to the number of entries entered into the table

  input: 
  property_table: pointer to an allocated memory area
  _property_count: number of dcam_properties which are allocated in property_table
*/
unicap_status_t _dcam_prepare_property_table( dcam_handle_t dcamhandle, dcam_property_t **new_table )
{
   quadlet_t feature_hi, feature_lo;
   dcam_property_t *property_table;
   int i;

   _dcam_read_register( dcamhandle->raw1394handle, dcamhandle->node, dcamhandle->command_regs_base + 0x404, &feature_hi );
   _dcam_read_register( dcamhandle->raw1394handle, dcamhandle->node, dcamhandle->command_regs_base + 0x408, &feature_lo );
	
   TRACE( "feature hi:\n brightness: %d\n auto_exp: %d\n sharpness: %d\n whitebal: %d\n hue: %d\n" \
	  " saturation: %d\n gamma: %d\n shutter: %d\n iris: %d\n focus: %d\n temperature: %d\n trigger: %d\n",
	  (feature_hi>>31)&1,
	  (feature_hi>>30)&1,
	  (feature_hi>>29)&1,
	  (feature_hi>>27)&1,
	  (feature_hi>>26)&1,
	  (feature_hi>>25)&1,
	  (feature_hi>>24)&1,
	  (feature_hi>>23)&1,
	  (feature_hi>>22)&1,
	  (feature_hi>>21)&1,
	  (feature_hi>>20)&1,
	  (feature_hi>>19)&1 );

   property_table = malloc( sizeof( _dcam_properties ) );

   for( i = 0; i < ( sizeof( _dcam_properties ) / sizeof( struct _dcam_property ) ); i++ )
   {
      dcam_copy_property( &property_table[i], &_dcam_properties[i] );
   }

   *new_table = property_table;

   return STATUS_SUCCESS;
}

unicap_status_t dcam_set_frame_rate_property( dcam_handle_t dcamhandle, 
					      unicap_property_t *property, 
					      dcam_property_t *dcam_property )
{
   unicap_status_t status = STATUS_SUCCESS;
   quadlet_t quad = 0;

   if( property->value <= 1.875 )
   {
      quad = SETVAL_FRAME_RATE( quad, 0 );
   }
   else if( property->value <= 3.75 )
   {
      quad = SETVAL_FRAME_RATE( quad, 1 );
   }
   else if( property->value <= 7.5 )
   {
      quad = SETVAL_FRAME_RATE( quad, 2 );
   }
   else if( property->value <= 15 )
   {
      quad = SETVAL_FRAME_RATE( quad, 3 );
   }
   else if( property->value <= 30 )
   {
      quad = SETVAL_FRAME_RATE( quad, 4 );
   }
   else if( property->value <= 60 )
   {
      quad = SETVAL_FRAME_RATE( quad, 5 );
   }
   else if( property->value <= 120 )
   {
      quad = SETVAL_FRAME_RATE( quad, 6 );
   }
   else if( property->value <= 240 )
   {
      quad = SETVAL_FRAME_RATE( quad, 7 );
   }
   else
   {
      status = STATUS_INVALID_PARAMETER;
      TRACE( "dcam: invalid frame rate" );
   }

   if( SUCCESS( status ) )
   {
      int was_running = 0;
      if( dcamhandle->capture_running )
      {
	 status = dcam_capture_stop( dcamhandle );
	 was_running = 1;
      }
      if( SUCCESS( status ) )
      {
	 status = _dcam_write_register( dcamhandle->raw1394handle, 
					dcamhandle->node, 
					dcamhandle->command_regs_base + 0x600, 
					quad );
	 if( SUCCESS( status ) )
	 {
	    dcamhandle->current_frame_rate = GETVAL_FRAME_RATE( quad );
	    if( was_running )
	    {
	       status = dcam_capture_start( dcamhandle );
	    }
	 }
      }
   }
	
   return status;
}


unicap_status_t dcam_get_frame_rate_property( dcam_handle_t dcamhandle, 
					      unicap_property_t *property, 
					      dcam_property_t *dcam_property )
{
   unicap_status_t status = STATUS_SUCCESS;
   quadlet_t rate;
   status = _dcam_read_register( dcamhandle->raw1394handle, 
				 dcamhandle->node, 
				 dcamhandle->command_regs_base + 0x600, 
				 &rate );
   if( !SUCCESS( status ) )
   {
      return status;
   }
			
   switch( GETVAL_FRAME_RATE( rate ) )
   {
      case 0:
	 property->value = 1.875;
	 break;
      case 1:
	 property->value = 3.75;
	 break;
      case 2:
	 property->value = 7.5;
	 break;
      case 3:
	 property->value = 15;
	 break;
      case 4:
	 property->value = 30;
	 break;
      case 5:
	 property->value = 60;
	 break;
      case 6:
	 property->value = 120;
	 break;
      case 7:
	 property->value = 240;
	 break;
      default:
	 TRACE( "dcam: unsupported frame rate\n" );
	 status = STATUS_FAILURE;
	 break;
   }

   return status;
}

/*
  Set white balance mode: AUTO or ONE_PUSH
*/
unicap_status_t dcam_set_white_balance_mode_property( dcam_handle_t dcamhandle, 
						      unicap_property_t *property, 
						      dcam_property_t *dcam_property )
{
   unicap_status_t status = STATUS_FAILURE;
	
   u_int64_t flags = property->flags;
   quadlet_t quad = 0;

   status = _dcam_read_register( dcamhandle->raw1394handle, 
				 dcamhandle->node, 
				 ( dcamhandle->command_regs_base + 
				   0x800 + 
				   dcam_property->register_offset ), 
				 &quad );
		
   flags &= property->flags_mask;
   quad = SETFLAG_AUTO( quad, 0 );

   quad = SETFLAG_ONE_PUSH( quad, 0 );


   if( SUCCESS( status ) )
   {
      if( flags & UNICAP_FLAGS_AUTO )
      {
	 quad = SETFLAG_AUTO( quad, 1 ); // set auto mode flag
      }
		
      if( flags & UNICAP_FLAGS_ONE_PUSH )
      {
	 quad = SETFLAG_ONE_PUSH( quad, 1); // set one_push flag
      }
		
      quad = SETFLAG_ON_OFF( quad, 1 ); // set on_off flag
      quad = SETFLAG_PRESENCE( quad, 1 ); // set presence flag
		
      status = _dcam_write_register( dcamhandle->raw1394handle, 
				     dcamhandle->node, 
				     ( dcamhandle->command_regs_base + 
				       0x800 + 
				       dcam_property->register_offset ), 
				     quad );
   }
	
   return status;
}

/*
  set a property to the camera
  
  input: 
  property
  dcam_property
  
  output: 
  dcam_property: the cached value of the property is updated
*/
unicap_status_t dcam_set_property( dcam_handle_t dcamhandle, 
				   unicap_property_t *property, 
				   dcam_property_t *dcam_property )
{
   unicap_status_t status = STATUS_FAILURE;

   switch( dcam_property->type )
   {
      case PPTY_TYPE_BRIGHTNESS:
      case PPTY_TYPE_WHITEBAL_U:
      case PPTY_TYPE_WHITEBAL_V:
      case PPTY_TYPE_TEMPERATURE:
      {
	 u_int64_t flags = property->flags;
	 quadlet_t quad = 0;
	 status = _dcam_read_register( dcamhandle->raw1394handle, 
				       dcamhandle->node, 
				       ( dcamhandle->command_regs_base + 
					 0x800 + 
					 dcam_property->register_offset ), 
				       &quad );
		
	 flags &= property->flags_mask;
	 quad = SETFLAG_AUTO( quad, 0 );
	 quad = SETFLAG_ONE_PUSH( quad, 0 );
			
	 if( flags & UNICAP_FLAGS_MANUAL )
	 {
	    unsigned int register_value;
				
/* 				register_value = (property->value * (max-min)) + min; */
	    register_value = property->value;
				
	    if( ( dcam_property->type != PPTY_TYPE_WHITEBAL_U ) && 
		( dcam_property->type != PPTY_TYPE_TEMPERATURE ) )
	    {
	       quad = SETVAL_VALUE( quad, register_value );
	    }
	    else if( ( dcam_property->type == PPTY_TYPE_WHITEBAL_U ) ||
		     ( dcam_property->type == PPTY_TYPE_TEMPERATURE ) )
	    {
	       quad = SETVAL_U_VALUE( quad, register_value );
	    }
	 }
			
	 if( flags & UNICAP_FLAGS_AUTO )
	 {
	    quad = SETFLAG_AUTO( quad, 1 ); // set auto mode flag
	 }
			
	 if( flags & UNICAP_FLAGS_ONE_PUSH )
	 {
	    quad = SETFLAG_ONE_PUSH( quad, 1); // set one_push flag
	 }
			
	 quad = SETFLAG_ON_OFF( quad, 1 ); // set on_off flag
	 quad = SETFLAG_PRESENCE( quad, 1 ); // set presence flag

	 TRACE( "dcam_set_property quad: %08x\n", quad );
			
	 status = _dcam_write_register( dcamhandle->raw1394handle, 
					dcamhandle->node, 
					( dcamhandle->command_regs_base + 
					  0x800 + 
					  dcam_property->register_offset ), 
					quad );
      }
      break;
      case PPTY_TYPE_TRIGGER:
      {
	 int i;
	 quadlet_t quad = 0;
			
	 quad = SETFLAG_PRESENCE( quad, 1 );

	 if( !strncmp( property->menu_item, dcam_trigger_modes[0], 127 ) )
	 {
	    quad = SETFLAG_ON_OFF( quad, 0 );
	 }
	 else
	 {
	    for( i = 1; i < dcamhandle->trigger_mode_count; i++ )
	    {

	       if( !strncmp( property->menu_item, dcam_trigger_modes[i], 127 ) )
	       {
		  // set mode
		  quad |= ( ( i - 1 ) << 12 );
		  quad = SETFLAG_ON_OFF( quad, 1 );
		  break;
	       }
	    }
	 }
			
	 status = _dcam_write_register( dcamhandle->raw1394handle, 
					dcamhandle->node, 
					( dcamhandle->command_regs_base + 
					  0x830 ), 
					quad );
      }
      break;

      case PPTY_TYPE_TRIGGER_POLARITY:
      {
	 int i;
	 quadlet_t quad = 0;
			
	 status = _dcam_read_register( dcamhandle->raw1394handle, 
				       dcamhandle->node, 
				       dcamhandle->command_regs_base + 0x830, 
				       &quad );
	 if( !SUCCESS( status ) )
	 {
	    break;
	 }
	 
	 quad &= ~(1<<26);
	 
	 if( !strcmp( property->menu_item, dcamhandle->trigger_polarities[1] ) )
	 {
	    quad |= (1<<26);
	 }	 
			
	 status = _dcam_write_register( dcamhandle->raw1394handle, 
					dcamhandle->node, 
					( dcamhandle->command_regs_base + 
					  0x830 ), 
					quad );
      }
      break;
		
      case PPTY_TYPE_REGISTER:
      {
	 dcam_raw_register_t *data = ( dcam_raw_register_t *)property->property_data;
	 if( property->property_data_size < sizeof( dcam_raw_register_t ) )
	 {
	    status = STATUS_INVALID_PARAMETER;
	 }
	 else
	 {
	    status = _dcam_write_register( dcamhandle->raw1394handle, 
					   dcamhandle->node, 
					   dcamhandle->command_regs_base + data->address,
					   data->value );
	 }							   
      }
      break;

      default:
      {
	 TRACE( "dcam: Unknown property type\n" );
      }
      break;
   }
	
   return status;
}

unicap_status_t dcam_get_white_balance_mode_property( dcam_handle_t dcamhandle, 
						      unicap_property_t *property, 
						      dcam_property_t *dcam_property )
{
   quadlet_t val = 0;
   unicap_status_t status = STATUS_SUCCESS;

   if( _dcam_read_register( dcamhandle->raw1394handle, dcamhandle->node, 
			    dcamhandle->command_regs_base + 
			    0x800 + 
			    dcam_property->register_offset, 
			    &val )
       < 0 )
   {
      return STATUS_FAILURE;
   }

   if( GETFLAG_AUTO(val) )
   {
      property->flags |= UNICAP_FLAGS_AUTO;
   }
   if( GETFLAG_ON_OFF(val) )
   {
      property->flags |= UNICAP_FLAGS_ON_OFF;
   }
   if( GETFLAG_ONE_PUSH(val) )
   {
      property->flags |= UNICAP_FLAGS_ONE_PUSH;
   }

   return status;
}


/*
  get property from the camera
  input: 
  property: property->identifier specifies property to get
  output: 
  property: all values updated to represent current state of camera
*/
unicap_status_t dcam_get_property( dcam_handle_t dcamhandle, unicap_property_t *property, dcam_property_t *dcam_property )
{
   quadlet_t val = 0;
   unicap_status_t status = STATUS_SUCCESS;
	
   if( ( dcam_property->type != PPTY_TYPE_REGISTER ) &&
       ( dcam_property->type != PPTY_TYPE_FRAMERATE ) )
   {
      if( _dcam_read_register( dcamhandle->raw1394handle, dcamhandle->node, 
			       dcamhandle->command_regs_base + 
			       0x800 + 
			       dcam_property->register_offset, 
			       &val )
	  < 0 )
      {
	 return STATUS_FAILURE;
      }
   }


   // initialize property struct
   // TODO: correct initialization
   if( strcmp( property->identifier, "register" ) )
   {
      memcpy( property, &dcam_property->unicap_property, sizeof( unicap_property_t ) );
   }
	

   switch( dcam_property->type )
   {
      case PPTY_TYPE_BRIGHTNESS:
      case PPTY_TYPE_WHITEBAL_U:
      case PPTY_TYPE_WHITEBAL_V:
      case PPTY_TYPE_TEMPERATURE:
      {
	 if( GETFLAG_AUTO(val) )
	 {
	    property->flags |= UNICAP_FLAGS_AUTO;
	    property->flags &= ~UNICAP_FLAGS_MANUAL;
	 }
	 else
	 {
	    property->flags |= UNICAP_FLAGS_MANUAL;
	 }
	 if( GETFLAG_ON_OFF(val) )
	 {
	    property->flags |= UNICAP_FLAGS_ON_OFF;
	 }
	 if( GETFLAG_ONE_PUSH(val) )
	 {
	    property->flags |= UNICAP_FLAGS_ONE_PUSH;
	 }

	 if( ( dcam_property->type == PPTY_TYPE_BRIGHTNESS ) ||
	     ( dcam_property->type == PPTY_TYPE_WHITEBAL_U ) || 
	     ( dcam_property->type == PPTY_TYPE_WHITEBAL_V ) )
	 {
	    unsigned int min = GETVAL_MIN_VALUE( dcam_property->register_inq);
				
	    unsigned int max = GETVAL_MAX_VALUE( dcam_property->register_inq);
				
	    if( ( dcam_property->type != PPTY_TYPE_WHITEBAL_U ) &&
		( dcam_property->type != PPTY_TYPE_TEMPERATURE ) )
	    {
	       unsigned int register_value = GETVAL_VALUE( val );
/* 					double d_value = ( double )( register_value - min ) / ( max - min ); */
	       property->value = register_value;
	    }
	    else if ( dcam_property->type == PPTY_TYPE_WHITEBAL_U )
	    {
	       unsigned int register_value;
					
	       register_value = GETVAL_U_VALUE( val );
/* 					d_value = ( double )( register_value - min ) / (double)( max - min ); */
	       property->value = register_value;
	    }
	    else if ( dcam_property->type == PPTY_TYPE_TEMPERATURE )
	    {
	       double d_value;
	       unsigned int register_value;
					
	       register_value = GETVAL_VALUE( val );
	       d_value = ( double )( register_value - min ) / ( max - min );
	       property->value = d_value;
	    }
	 }
      }
      break;

      case PPTY_TYPE_FRAMERATE:
      {
      }
      break;

      case PPTY_TYPE_TRIGGER:
      {
	 quadlet_t trigger;
			
	 status = _dcam_read_register( dcamhandle->raw1394handle, 
				       dcamhandle->node, 
				       dcamhandle->command_regs_base + 0x830, 
				       &trigger );
	 if( !SUCCESS( status ) )
	 {
	    break;
	 }

	 // if we have a data buffer for the optional parameter
	 if( property->property_data_size >= 4 )
	 {
	    if( property->property_data )
	    {
	       ((unsigned int *)property->property_data)[0] = GETVAL_VALUE( trigger );
	    }
	    else
	    {
	       status = STATUS_INVALID_PARAMETER;
	       break;
	    }
	 }

	 if( !GETFLAG_ON_OFF( trigger ) )
	 {
	    strncpy( property->menu_item, dcam_trigger_modes[0], 127 );
	 }
	 else
	 {
	    int mode = 0;
	    mode = ( trigger >> 12 ) & 0xf;
	    strncpy( property->menu_item, dcam_trigger_modes[ mode+1 ], 127 );
	 }

	 property->flags = UNICAP_FLAGS_MANUAL;
	 property->flags_mask = UNICAP_FLAGS_MANUAL;
      }
      break;

      case PPTY_TYPE_TRIGGER_POLARITY:
      {
	 quadlet_t trigger;
			
	 status = _dcam_read_register( dcamhandle->raw1394handle, 
				       dcamhandle->node, 
				       dcamhandle->command_regs_base + 0x830, 
				       &trigger );
	 if( !SUCCESS( status ) )
	 {
	    break;
	 }

	 strcpy( property->menu_item, dcamhandle->trigger_polarities[( dcam_property->register_default >> 26 ) & 1] );	 

	 property->flags = UNICAP_FLAGS_MANUAL;
	 property->flags_mask = UNICAP_FLAGS_MANUAL;
      }
      break;

      case PPTY_TYPE_REGISTER:
      {
	 dcam_raw_register_t *data = ( dcam_raw_register_t * )property->property_data;
	 if( property->property_data_size < sizeof( dcam_raw_register_t ) )
	 {
	    status = STATUS_INVALID_PARAMETER;
	 }
	 else
	 {
	    status = _dcam_read_register( dcamhandle->raw1394handle, 
					  dcamhandle->node, 
					  dcamhandle->command_regs_base + data->address,
					  &data->value );
	 }							   
      }
      break;

      default:
      {
	 TRACE( "dcam: unknown ppty type\n" );
      }
      break;
   }
   return status;
}

/*
  returns 1 if the given rate is available in the frame rate inquiry register quad, 0 otherwise
*/
int _dcam_check_frame_rate_available( quadlet_t quad, int rate )
{
   TRACE( "check frame rate avail: quad= %08x, rate: %d := %d\n", 
	  quad, 
	  rate, 
	  ( quad >> (31-rate) ) & 1 );

   return ( quad >> ( 31 - rate ) ) & 1;	
}

dcam_property_t *dcam_copy_property( dcam_property_t *dest, dcam_property_t *src )
{
   dest->id = src->id;
   unicap_copy_property( &dest->unicap_property, &src->unicap_property );

   dest->register_offset = src->register_offset;
   dest->register_inq = src->register_inq;
   dest->register_default = src->register_default;
   dest->register_value = src->register_value;
	
   dest->type = src->type;
	
   dest->feature_hi_mask = src->feature_hi_mask;
   dest->feature_lo_mask = src->feature_lo_mask;

   dest->set_function = src->set_function;
   dest->get_function = src->get_function;
   dest->init_function = src->init_function;
	
   return dest;
}
