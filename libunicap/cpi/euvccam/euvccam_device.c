/*
  unicap euvccam plugin

  Copyright (C) 2009  Arne Caspari

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

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "euvccam_cpi.h"
#include "euvccam_device.h"
#include "euvccam_capture.h"
#include "euvccam_devspec.h"
#include "euvccam_usb.h"

#define EUVCCAM_VENDOR_ID 0x199e
#define N_ELEMENTS(a) (sizeof( a )/ sizeof(( a )[0] ))
#define FOURCC(a,b,c,d) (unsigned int)((((unsigned int)d)<<24)+(((unsigned int)c)<<16)+(((unsigned int)b)<<8)+a)

#define SET_CUR 0x01
#define GET_CUR 0x81
#define GET_DEF 0x87

#define VS_PROBE_CONTROL 0x01
#define VS_COMMIT_CONTROL 0x02


#define SC_VIDEOSTREAMING 0x02


#define CT_AE_MODE_CONTROL                0x02
#define CT_EXPOSURE_TIME_ABSOLUTE_CONTROL 0x04
#define CT_PRIVACY_CONTROL                0x11
#define CT_TIS_PIXEL_CLOCK                0x24
#define CT_TIS_PARTIAL_SCAN_WIDTH         0x25
#define CT_TIS_PARTIAL_SCAN_HEIGHT        0x26
#define CT_TIS_BINNING                    0x2a
#define CT_TIS_SOFTWARE_TRIGGER           0x2b
#define CT_TIS_SENSOR_RESET               0x2c
#define CT_TIS_FIRMWARE_REVISION          0x2d
#define CT_TIS_GPOUT                      0x2e


#define PU_GAIN_CONTROL                         0x04
#define PU_WHITE_BALANCE_COMPONENT_CONTROL      0x0c
#define PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL 0x0d
#define PU_TIS_COLOR_FORMAT                     0x21

#define CAMERA_TERMINAL 1
#define PROCESSING_UNIT 3

#define COLOR_FORMAT_MONO8   0
#define COLOR_FORMAT_UYVY    1
#define COLOR_FORMAT_BAYER1  3



/* static unsigned long long euvccam_usb_string_to_number( char *string ); */


unicap_status_t euvccam_read_vendor_register( int fd, int index, unsigned char *val )
{
   unsigned char length = 1;
   unicap_status_t status = STATUS_SUCCESS;

   status = euvccam_usb_ctrl_msg(fd,
				 EP0_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
				 0x0, // UVCD Register Request
				 0x0, 
				 index, (char*)val, length);
   
   return status;
}

unicap_status_t euvccam_write_vendor_register( int fd, int index, unsigned char val )
{
   unicap_status_t status = STATUS_SUCCESS;

   status = euvccam_usb_ctrl_msg(fd,
				 EP0_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE,
				 0x0, // UVCD Register Request
				 0x0, 
				 index, (char*)&val, 1);

   return status;
}
   
unicap_status_t euvccam_device_get_format( euvccam_handle_t handle, euvccam_video_format_description_t **format )
{
   unicap_status_t status = STATUS_SUCCESS;
   struct probe_control probe;
   unsigned short val;
   int i;

   memset( &probe, 0x0, sizeof( struct probe_control ) );
   
   status = euvccam_usb_ctrl_msg( handle->dev.fd, 
				  EP0_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE, 
				  GET_DEF,
				  VS_PROBE_CONTROL << 8, 
				  1, 
				  (char*)&probe, sizeof( probe ));

   if( !SUCCESS( status ) ){
      TRACE( "probe control failed!\n" );
      return STATUS_FAILURE;
   }

   for( i = 0; i < euvccam_devspec[handle->devspec_index].format_count; i++ ){
      if( ( probe.bFormatIndex == euvccam_devspec[handle->devspec_index].format_list[i].format_index ) && 
	  ( probe.bFrameIndex == euvccam_devspec[handle->devspec_index].format_list[i].frame_index ) ){

	 *format = &euvccam_devspec[handle->devspec_index].format_list[i];
	 break;
      }
   }

   if( i == euvccam_devspec[handle->devspec_index].format_count ){
      TRACE( "Invalid format returned by camera! FormatIndex = %d FrameIndex = %d\n", probe.bFormatIndex, probe.bFrameIndex );
      *format = &euvccam_devspec[handle->devspec_index].format_list[0];
   }  

   status |= euvccam_usb_ctrl_msg( handle->dev.fd,
				   EP0_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
				   GET_CUR,
				   CT_TIS_PARTIAL_SCAN_WIDTH << 8,
				   CAMERA_TERMINAL << 8,
				   (char*)&val, 2 );

   if( ( val >= (*format)->format.min_size.width ) && 
       ( val <= (*format)->format.max_size.width ) ){
      (*format)->format.size.width = val;
   }

   status |= euvccam_usb_ctrl_msg( handle->dev.fd,
				   EP0_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
				   GET_CUR,
				   CT_TIS_PARTIAL_SCAN_HEIGHT << 8,
				   CAMERA_TERMINAL << 8,
				   (char*)&val, 2 );

   if( ( val >= (*format)->format.min_size.height ) && 
       ( val <= (*format)->format.max_size.height ) ){
      (*format)->format.size.height = val;
   }

  
   return status;  
}

unicap_status_t euvccam_device_set_format( euvccam_handle_t handle, unicap_format_t *format )
{
   unicap_status_t status = STATUS_SUCCESS;
   struct probe_control commit;
   int i;
   int old_capture = handle->capture_running;
   struct euvccam_video_format_description *dscr = NULL;
   unsigned short cf = COLOR_FORMAT_UYVY;
   unsigned char binning = 1;

   if( old_capture )
      euvccam_capture_stop_capture( handle );

   memset( &commit, 0x0, sizeof( struct probe_control ) );

   for( i = 0; i < euvccam_devspec[handle->devspec_index].format_count; i++ ){
      struct euvccam_video_format_description *tmp = &euvccam_devspec[handle->devspec_index].format_list[i];
      
      if( ( tmp->format.size.width <= format->max_size.width ) && 
	  ( tmp->format.size.height <= format->max_size.height ) &&
	  ( tmp->format.size.width >= format->min_size.width ) && 
	  ( tmp->format.size.height >= format->min_size.height ) &&
	  ( tmp->format.fourcc == format->fourcc ) ){

	 commit.bFormatIndex = tmp->format_index;
	 commit.bFrameIndex = tmp->frame_index;

	 dscr = tmp;
	 break;
      }
   }

   if( !dscr ){
      TRACE( "No match for format: %dx%d  %s\n", format->size.width, format->size.height, format->identifier );
      return STATUS_NO_MATCH;
   }
   
   status = euvccam_usb_ctrl_msg( handle->dev.fd, 
				  EP0_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE, 
				  SET_CUR,
				  VS_COMMIT_CONTROL<<8, 
				  1, 
				  (char*)&commit, sizeof( commit ));
   usleep( 100000 );
   
   if( dscr->flags & EUVCCAM_FORMAT_IS_PARTIAL_SCAN ){
      unsigned short val = format->size.width;
      TRACE( "partial scan %dx%d\n", format->size.width, format->size.height );
      
/*       status = euvccam_usb_ctrl_msg( handle->dev.fd,  */
/* 				     EP0_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE,  */
/* 				     SET_CUR,  */
/* 				     CT_AE_MODE_CONTROL << 8,  */
/* 				     CAMERA_TERMINAL << 8,  */
/* 				     &handle->current_ae_mode, 1); */
      status |= euvccam_usb_ctrl_msg( handle->dev.fd,
				      EP0_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
				      SET_CUR,
				      CT_TIS_PARTIAL_SCAN_WIDTH << 8,
				      CAMERA_TERMINAL << 8,
				      (char*)&val, 2 );
      TRACE( "Set partial scan width: %d\n", status );
      val = format->size.height;
      status |= euvccam_usb_ctrl_msg( handle->dev.fd, 
					 EP0_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE, 
					 SET_CUR, 
					 CT_TIS_PARTIAL_SCAN_HEIGHT << 8, 
					 CAMERA_TERMINAL << 8, 
					 (char*)&val, 2 );
      TRACE( "Set partial scan height: %d\n", status );
   }

   switch( format->fourcc ){
   case FOURCC( 'U', 'Y', 'V', 'Y' ):
      cf = COLOR_FORMAT_UYVY;
      break;
   case FOURCC( 'R', 'G', 'B', '3' ):
      cf = COLOR_FORMAT_BAYER1;
      break;
   }

   if( strstr( format->identifier, "2x Binning" ) ){
      binning = 2;
   }
   if( strstr( format->identifier, "4x Binning" ) ){
      binning = 4;
   }
      
   euvccam_usb_ctrl_msg( handle->dev.fd, 
			 EP0_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE, 
			 SET_CUR, 
			 CT_TIS_BINNING << 8,
			 CAMERA_TERMINAL << 8, 
			 (char *)&binning, 1 );

   TRACE( "Binning: %d\n", binning );

   TRACE( "cf: %d\n", cf );

   //status |= euvccam_usb_ctrl_msg( handle->dev.fd,
//				   EP0_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
//				   SET_CUR,
//				   PU_TIS_COLOR_FORMAT << 8,
//				   PROCESSING_UNIT << 8,
//				   (char*)&cf, 2 );
   
   if( SUCCESS( status ) ){
      handle->current_format = dscr;
      handle->current_format->format.size.width = format->size.width;
      handle->current_format->format.size.height = format->size.height;
   }

   if( old_capture )
      euvccam_capture_start_capture( handle );
  
   return status;  
}

unicap_status_t euvccam_device_set_reset_mt9v024( euvccam_handle_t handle, unicap_property_t *property )
{
   unicap_status_t status;
   unsigned char val = 1;
   status = euvccam_usb_ctrl_msg( handle->dev.fd, 
				   EP0_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE, 
				   SET_CUR, 
				   CT_TIS_SENSOR_RESET << 8, 
				   CAMERA_TERMINAL << 8, 
				   (char*)&val, 1);

   return status;
}

unicap_status_t euvccam_device_get_reset_mt9v024( euvccam_handle_t handle, unicap_property_t *property )
{
   return STATUS_SUCCESS;
}

unicap_status_t euvccam_device_write_iic( euvccam_handle_t handle, unicap_property_t *property )
{
   unicap_status_t status;

   if( property->property_data_size < sizeof( struct euvccam_iic_command ) ){
      return STATUS_INVALID_PARAMETER;
   }

   struct euvccam_iic_command *iiccmd = ( struct euvccam_iic_command * )property->property_data;

   // set IIC device address
   euvccam_usb_ctrl_msg( handle->dev.fd, 
			 EP0_OUT | USB_TYPE_CLASS | USB_RECIP_DEVICE, 
			 SET_CUR, 
			 0x40 << 8, 
			 0x1 << 8, 
			 &iiccmd->devaddr, 1 );
   

   // set IIC address
   status = euvccam_usb_ctrl_msg( handle->dev.fd, 
				EP0_OUT | USB_TYPE_CLASS | USB_RECIP_DEVICE, 
				SET_CUR, 
				0x21 << 8, 
				0x1 << 8, 
				&iiccmd->addr, 1 );

   // write IIC value
   status |= euvccam_usb_ctrl_msg( handle->dev.fd, 
				EP0_OUT | USB_TYPE_CLASS | USB_RECIP_DEVICE, 
				SET_CUR, 
				0x22 << 8, 
				0x1 << 8, 
				&iiccmd->value, 2 );

   return status;
}

unicap_status_t euvccam_device_read_iic( euvccam_handle_t handle, unicap_property_t *property )
{
   unicap_status_t status;
   static unsigned char devaddr = 0xff;

   if( property->property_data_size < sizeof( struct euvccam_iic_command ) ){
      return STATUS_INVALID_PARAMETER;
   }

   struct euvccam_iic_command *iiccmd = ( struct euvccam_iic_command * )property->property_data;

   if( devaddr != iiccmd->devaddr ){
      // set IIC device address
      euvccam_usb_ctrl_msg( handle->dev.fd,
			    EP0_OUT | USB_TYPE_CLASS | USB_RECIP_DEVICE,
			    SET_CUR,
			    0x40 << 8,
			    0x1 << 8,
			    &iiccmd->devaddr, 1 );
      devaddr = iiccmd->devaddr;
   }
   
   // set IIC address
   status = euvccam_usb_ctrl_msg( handle->dev.fd, 
				  EP0_OUT | USB_TYPE_CLASS | USB_RECIP_DEVICE, 
				  SET_CUR, 
				  0x21 << 8, 
				  0x1 << 8, 
				  &iiccmd->addr, 1 );


   // read IIC value
   status |= euvccam_usb_ctrl_msg( handle->dev.fd, 
				   EP0_IN | USB_TYPE_CLASS | USB_RECIP_DEVICE, 
				   GET_CUR, 
				   0x22 << 8, 
				   0x1 << 8, 
				   &iiccmd->value, 2 );
   return status;
}


unicap_status_t euvccam_device_set_exposure( euvccam_handle_t handle, unicap_property_t *property )
{
   unicap_status_t status = STATUS_SUCCESS;
   unsigned int val = property->value * 10000.0;
   char old_ae_mode = handle->current_ae_mode;

   if( property->flags & UNICAP_FLAGS_AUTO ){
      handle->current_ae_mode |= 1<<1;
   }else{
      handle->current_ae_mode &= ~(1<<1);
   }
   
   if( (euvccam_devspec[handle->devspec_index].flags & EUVCCAM_HAS_AUTO_EXPOSURE ) && ( old_ae_mode != handle->current_ae_mode ) ){
      status = euvccam_usb_ctrl_msg( handle->dev.fd, 
				     EP0_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE, 
				     SET_CUR, 
				     CT_AE_MODE_CONTROL << 8, 
				     CAMERA_TERMINAL << 8, 
				     &handle->current_ae_mode, 1);
   }
   
   status |= euvccam_usb_ctrl_msg( handle->dev.fd, 
				   EP0_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE, 
				   SET_CUR, 
				   CT_EXPOSURE_TIME_ABSOLUTE_CONTROL << 8, 
				   CAMERA_TERMINAL << 8, 
				   (char*)&val, 4);

   TRACE( "set_exposure va: %d\n", val );

   return status;
}

unicap_status_t euvccam_device_get_exposure( euvccam_handle_t handle, unicap_property_t *property )
{
   unicap_status_t status = STATUS_SUCCESS;
   unsigned int val;

   if( euvccam_devspec[handle->devspec_index].flags & EUVCCAM_HAS_AUTO_EXPOSURE ){
      status = euvccam_usb_ctrl_msg( handle->dev.fd, 
				     EP0_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE, 
				     SET_CUR, 
				     CT_AE_MODE_CONTROL << 8, 
				     CAMERA_TERMINAL << 8, 
				     &handle->current_ae_mode, 1);
   }

   if( handle->current_ae_mode & ( 1<<1 ) ){
      property->flags = UNICAP_FLAGS_AUTO;
   }else{
      property->flags = UNICAP_FLAGS_MANUAL;
   }  
   
   status = euvccam_usb_ctrl_msg( handle->dev.fd, 
				  EP0_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE, 
				  GET_CUR, 
				  CT_EXPOSURE_TIME_ABSOLUTE_CONTROL << 8, 
				  CAMERA_TERMINAL << 8, 
				  (char*)&val, 4);

   property->value = (double)val / 10000.0;
   
   return status;
}

unicap_status_t euvccam_device_set_gain( euvccam_handle_t handle, unicap_property_t *property )
{
   unicap_status_t status = STATUS_SUCCESS;
   unsigned int val = property->value;
   char old_ae_mode = handle->current_ae_mode;

   if( property->flags & UNICAP_FLAGS_AUTO ){
      handle->current_ae_mode |= 1<<2;
   }else{
      handle->current_ae_mode &= ~(1<<2);
   }
   
   if( (euvccam_devspec[handle->devspec_index].flags & EUVCCAM_HAS_AUTO_GAIN ) && ( old_ae_mode != handle->current_ae_mode ) ){
      status = euvccam_usb_ctrl_msg( handle->dev.fd, 
				     EP0_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE, 
				     SET_CUR, 
				     CT_AE_MODE_CONTROL << 8, 
				     CAMERA_TERMINAL << 8, 
				     &handle->current_ae_mode, 1);
   }
   
   status |= euvccam_usb_ctrl_msg( handle->dev.fd, 
				   EP0_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE, 
				   SET_CUR, 
				   PU_GAIN_CONTROL << 8, 
				   PROCESSING_UNIT << 8, 
				   (char*)&val, 4);

   return status;
}

unicap_status_t euvccam_device_get_gain( euvccam_handle_t handle, unicap_property_t *property )
{
   unicap_status_t status = STATUS_SUCCESS;
   unsigned int val;

   if( euvccam_devspec[handle->devspec_index].flags & EUVCCAM_HAS_AUTO_GAIN ){
      status = euvccam_usb_ctrl_msg( handle->dev.fd, 
				     EP0_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE, 
				     GET_CUR, 
				     CT_AE_MODE_CONTROL << 8, 
				     CAMERA_TERMINAL << 8, 
				     &handle->current_ae_mode, 1);
   }

   if( handle->current_ae_mode & ( 1<<2 ) ){
      property->flags = UNICAP_FLAGS_AUTO;
   }else{
      property->flags = UNICAP_FLAGS_MANUAL;
   }  
   
   status = euvccam_usb_ctrl_msg( handle->dev.fd, 
				  EP0_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE, 
				  GET_CUR, 
				  PU_GAIN_CONTROL << 8, 
				  PROCESSING_UNIT << 8, 
				  (char*)&val, 4);

   property->value = val;
   
   return status;
}

unicap_status_t euvccam_device_set_white_balance( euvccam_handle_t handle, unicap_property_t *property )
{
   unicap_status_t status = STATUS_SUCCESS;
   unsigned int val = 0;

   if( !strcmp( property->identifier, "White Balance Red" ) ){
      val &= 0xffff;
      val |= ((int)property->value) << 16;
   }else{
      val &= 0xffff0000;
      val |= ((int)property->value) & 0xffff;
   }

   status = euvccam_usb_ctrl_msg( handle->dev.fd, 
				  EP0_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE, 
				  SET_CUR, 
				  PU_WHITE_BALANCE_COMPONENT_CONTROL << 8, 
				  PROCESSING_UNIT << 8, 
				  (char*)&val, 4);

   return status;
}

unicap_status_t euvccam_device_get_white_balance( euvccam_handle_t handle, unicap_property_t *property )
{
   unicap_status_t status = STATUS_SUCCESS;
   unsigned int val;
   
   status = euvccam_usb_ctrl_msg( handle->dev.fd, 
				  EP0_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE, 
				  GET_CUR, 
				  PU_WHITE_BALANCE_COMPONENT_CONTROL << 8, 
				  PROCESSING_UNIT << 8, 
				  (char*)&val, 4);

   if( !strcmp( property->identifier, "White Balance Red" ) ){
      property->value = ( val >> 16 ) & 0xffff;
   } else {
      property->value = val & 0xffff;
   }
   
   return status;
}

unicap_status_t euvccam_device_set_white_balance_mode( euvccam_handle_t handle, unicap_property_t *property )
{
   unicap_status_t status = STATUS_SUCCESS;
   unsigned char val = 0;

   if( property->flags & UNICAP_FLAGS_AUTO ){
      val = 1;
   }

   status = euvccam_usb_ctrl_msg( handle->dev.fd, 
				  EP0_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE, 
				  SET_CUR, 
				  PU_WHITE_BALANCE_COMPONENT_AUTO_CONTROL << 8, 
				  PROCESSING_UNIT << 8, 
				  (char*)&val, 1);

   return status;
}

unicap_status_t euvccam_device_get_white_balance_mode( euvccam_handle_t handle, unicap_property_t *property )
{
   unicap_status_t status = STATUS_SUCCESS;
   unsigned char val;
   
   status = euvccam_usb_ctrl_msg( handle->dev.fd, 
				  EP0_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE, 
				  GET_CUR, 
				  PU_WHITE_BALANCE_COMPONENT_CONTROL << 8, 
				  PROCESSING_UNIT << 8, 
				  (char*)&val, 1);

   if( val ){
      property->flags = UNICAP_FLAGS_AUTO;
   }else{
      property->flags = UNICAP_FLAGS_MANUAL;
   }
   
   return status;
}

unicap_status_t euvccam_device_set_trigger( euvccam_handle_t handle, unicap_property_t *property )
{
   unicap_status_t status = STATUS_SUCCESS;
   unsigned char val = 0;
   
   if( strcmp( property->menu_item, "free running" ) ){
      val = 1;
   }

   status = euvccam_usb_ctrl_msg( handle->dev.fd, 
				  EP0_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE, 
				  SET_CUR, 
				  CT_PRIVACY_CONTROL << 8, 
				  CAMERA_TERMINAL << 8, 
				  (char*)&val, 1);


   return status;
}

unicap_status_t euvccam_device_get_trigger( euvccam_handle_t handle, unicap_property_t *property )
{
   unicap_status_t status = STATUS_SUCCESS;
   unsigned char val;

   
   status = euvccam_usb_ctrl_msg( handle->dev.fd, 
				  EP0_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE, 
				  GET_CUR, 
				  CT_PRIVACY_CONTROL << 8, 
				  CAMERA_TERMINAL << 8, 
				  (char*)&val, 1);
   TRACE( "trigger val: %d\n", val );

   if( val ){
      strcpy( property->menu_item, "trigger on rising edge" );
   }else{
      strcpy( property->menu_item, "free running" );
   }
   
   return status;
}

unicap_status_t euvccam_device_enumerate_trigger( euvccam_handle_t handle, unicap_property_t *property )
{
   unicap_status_t status = STATUS_NO_MATCH;

   if( SUCCESS( euvccam_device_get_trigger( handle, property ) ) ){
      status = STATUS_SUCCESS;
   }
   
   return status;
}

unicap_status_t euvccam_device_set_software_trigger( euvccam_handle_t handle, unicap_property_t *property )
{
   unicap_status_t status = STATUS_SUCCESS;
   unsigned char val = 1;
   

   status = euvccam_usb_ctrl_msg( handle->dev.fd, 
				  EP0_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE, 
				  SET_CUR, 
				  CT_TIS_SOFTWARE_TRIGGER << 8, 
				  CAMERA_TERMINAL << 8, 
				  (char*)&val, 1);


   return status;
}

unicap_status_t euvccam_device_get_software_trigger( euvccam_handle_t handle, unicap_property_t *property )
{
   unicap_status_t status = STATUS_SUCCESS;
   unsigned char val;

   
   status = euvccam_usb_ctrl_msg( handle->dev.fd, 
				  EP0_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE, 
				  GET_CUR, 
				  CT_TIS_SOFTWARE_TRIGGER << 8, 
				  CAMERA_TERMINAL << 8, 
				  (char*)&val, 1);

   TRACE( "Get software trigger: %d\n", val );
   
   return status;
}

unicap_status_t euvccam_device_enumerate_software_trigger( euvccam_handle_t handle, unicap_property_t *property )
{
   unicap_status_t status = STATUS_NO_MATCH;

   if( SUCCESS( euvccam_device_get_trigger( handle, property ) ) ){
      status = STATUS_SUCCESS;
   }
   
   return status;
}

unicap_status_t euvccam_device_set_frame_rate( euvccam_handle_t handle, unicap_property_t *property )
{
   unicap_status_t status = STATUS_SUCCESS;
   unsigned char regval = 0;
   int i;
   
   for( i = 0; i < handle->current_format->frame_rate_count; i++ ){
      if( handle->current_format->frame_rates[i] == property->value ){
	 regval = handle->current_format->frame_rate_map[i];
      }
   }
   
   property->value_list.values = handle->current_format->frame_rates;
   property->value_list.value_count = handle->current_format->frame_rate_count;

   status = euvccam_write_vendor_register( handle->dev.fd, 0x3a, regval );
   
   return status;
}

unicap_status_t euvccam_device_get_frame_rate( euvccam_handle_t handle, unicap_property_t *property )
{
   unicap_status_t status = STATUS_SUCCESS;
   unsigned char regval;
   double val = 0.0;

   if( !handle->current_format ){
      return STATUS_FAILURE;
   }

   status = euvccam_read_vendor_register( handle->dev.fd, 0x3a, &regval );
   if( SUCCESS( status ) ){
      int i;
      for( i = 0; i < handle->current_format->frame_rate_count; i++ ){
	 if( handle->current_format->frame_rate_map[i] == regval ){
	    val = handle->current_format->frame_rates[i];
	 }
      }
   }

   property->value_list.values = handle->current_format->frame_rates;
   property->value_list.value_count = handle->current_format->frame_rate_count;
   property->value = val;
   
   return status;
}

unicap_status_t euvccam_device_enumerate_frame_rate( euvccam_handle_t handle, unicap_property_t *property )
{
   unicap_status_t status;

   status = euvccam_device_get_frame_rate( handle, property );
   if( SUCCESS( status ) ){
      property->value = handle->current_format->frame_rates[0];
   }
   
   return status;
}

unicap_status_t euvccam_device_set_pixel_clock( euvccam_handle_t handle, unicap_property_t *property )
{
   unicap_status_t status = STATUS_SUCCESS;
   unsigned int val = property->value * 1000000.0;
   
   status = euvccam_usb_ctrl_msg( handle->dev.fd, 
				   EP0_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE, 
				   SET_CUR, 
				   CT_TIS_PIXEL_CLOCK << 8, 
				   CAMERA_TERMINAL << 8, 
				   (char*)&val, 4);

   return status;
}

unicap_status_t euvccam_device_get_pixel_clock( euvccam_handle_t handle, unicap_property_t *property )
{
   unicap_status_t status = STATUS_SUCCESS;
   unsigned int val;

   status = euvccam_usb_ctrl_msg( handle->dev.fd, 
				  EP0_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE, 
				  GET_CUR, 
				  CT_TIS_PIXEL_CLOCK << 8, 
				  CAMERA_TERMINAL << 8, 
				  (char*)&val, 4);

   property->value = (double)val / 1000000.0;
   
   return status;
}

unicap_status_t euvccam_device_set_gpout( euvccam_handle_t handle, unicap_property_t *property )
{
   unicap_status_t status = STATUS_SUCCESS;
   unsigned char val = (property->flags & UNICAP_FLAGS_ON_OFF)?1:0;
   
   printf( "%lld %d\n", property->flags, val );

   status = euvccam_usb_ctrl_msg( handle->dev.fd, 
				  EP0_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE, 
				  SET_CUR, 
				  CT_TIS_GPOUT << 8, 
				  CAMERA_TERMINAL << 8, 
				  (char*)&val, 1);


   return status;
}

unicap_status_t euvccam_device_get_gpout( euvccam_handle_t handle, unicap_property_t *property )
{
   unicap_status_t status = STATUS_SUCCESS;
   unsigned char val;

   
   status = euvccam_usb_ctrl_msg( handle->dev.fd, 
				  EP0_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE, 
				  GET_CUR, 
				  CT_TIS_GPOUT << 8, 
				  CAMERA_TERMINAL << 8, 
				  (char*)&val, 1);

   property->flags = UNICAP_FLAGS_MANUAL | ( val ? UNICAP_FLAGS_ON_OFF : 0);
   
   return status;
}

unicap_status_t euvccam_device_enumerate_gpout( euvccam_handle_t handle, unicap_property_t *property )
{
   unicap_status_t status = STATUS_NO_MATCH;

   if( SUCCESS( euvccam_device_get_gpout( handle, property ) ) ){
      status = STATUS_SUCCESS;
   }
   
   return status;
}
