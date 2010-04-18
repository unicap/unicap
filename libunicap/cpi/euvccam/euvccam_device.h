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

#ifndef __EUVCCAM_DEVICE_H__
#define __EUVCCAM_DEVICE_H__

#include <unicap.h>

#define EUVCCAM_SYSTEM_BUFFER_COUNT 8

struct probe_control
{
   unsigned short bmHint;
   unsigned char bFormatIndex;
   unsigned char bFrameIndex;
   unsigned long dwFrameInterval;
   unsigned short wKeyFrameRate;
   unsigned short wPFrameRate;
   unsigned short wCompQuality;
   unsigned short wCompWindowSize;
   unsigned short wDelay;
   unsigned long dwMaxVideoFrameSize;
   unsigned long dwMaxPayloadTransferSize;
   unsigned long dwClockFrequency;
   unsigned char bmFramingInfo;
   unsigned char bPreferedVersion;
   unsigned char bMinVersion;
   unsigned char bMaxVersion;
};

struct euvccam_iic_command
{
   unsigned char devaddr;
   unsigned char addr;
   unsigned short value;
};

   

struct usb_device *euvccam_find_device( int index );
struct usb_device *euvccam_find_device_by_id( char *identifier );
unicap_status_t euvccam_fill_device_struct( struct usb_device *dev, unicap_device_t *device );

unicap_status_t euvccam_read_vendor_register( int fd, int index, unsigned char *val );
unicap_status_t euvccam_write_vendor_register( int fd, int index, unsigned char val );

unicap_status_t euvccam_device_get_format( euvccam_handle_t handle, struct euvccam_video_format_description **format );
unicap_status_t euvccam_device_set_format( euvccam_handle_t handle, unicap_format_t *format );

unicap_status_t euvccam_device_set_exposure( euvccam_handle_t handle, unicap_property_t *property );
unicap_status_t euvccam_device_get_exposure( euvccam_handle_t handle, unicap_property_t *property );
unicap_status_t euvccam_device_set_gain( euvccam_handle_t handle, unicap_property_t *property );
unicap_status_t euvccam_device_get_gain( euvccam_handle_t handle, unicap_property_t *property );
unicap_status_t euvccam_device_set_white_balance( euvccam_handle_t handle, unicap_property_t *property );
unicap_status_t euvccam_device_get_white_balance( euvccam_handle_t handle, unicap_property_t *property );
unicap_status_t euvccam_device_set_white_balance_mode( euvccam_handle_t handle, unicap_property_t *property );
unicap_status_t euvccam_device_get_white_balance_mode( euvccam_handle_t handle, unicap_property_t *property );
unicap_status_t euvccam_device_set_frame_rate( euvccam_handle_t handle, unicap_property_t *property );
unicap_status_t euvccam_device_get_frame_rate( euvccam_handle_t handle, unicap_property_t *property );
unicap_status_t euvccam_device_enumerate_frame_rate( euvccam_handle_t handle, unicap_property_t *property );
unicap_status_t euvccam_device_set_trigger( euvccam_handle_t handle, unicap_property_t *property );
unicap_status_t euvccam_device_get_trigger( euvccam_handle_t handle, unicap_property_t *property );
unicap_status_t euvccam_device_enumerate_trigger( euvccam_handle_t handle, unicap_property_t *property );
unicap_status_t euvccam_device_set_software_trigger( euvccam_handle_t handle, unicap_property_t *property );
unicap_status_t euvccam_device_get_software_trigger( euvccam_handle_t handle, unicap_property_t *property );
unicap_status_t euvccam_device_enumerate_software_trigger( euvccam_handle_t handle, unicap_property_t *property );
unicap_status_t euvccam_device_set_pixel_clock( euvccam_handle_t handle, unicap_property_t *property );
unicap_status_t euvccam_device_get_pixel_clock( euvccam_handle_t handle, unicap_property_t *property );
unicap_status_t euvccam_device_set_reset_mt9v024( euvccam_handle_t handle, unicap_property_t *property );
unicap_status_t euvccam_device_get_reset_mt9v024( euvccam_handle_t handle, unicap_property_t *property );
unicap_status_t euvccam_device_write_iic( euvccam_handle_t handle, unicap_property_t *property );
unicap_status_t euvccam_device_read_iic( euvccam_handle_t handle, unicap_property_t *property );
unicap_status_t euvccam_device_enumerate_gpout( euvccam_handle_t handle, unicap_property_t *property );
unicap_status_t euvccam_device_get_gpout( euvccam_handle_t handle, unicap_property_t *property );
unicap_status_t euvccam_device_set_gpout( euvccam_handle_t handle, unicap_property_t *property );



#endif//__EUVCCAM_DEVICE_H__

