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

#ifndef __EUVCCAM_USB_H__
#define __EUVCCAM_USB_H__

#include <limits.h>
#include <stdint.h>

#define EP0_IN	        0x80
#define EP0_OUT	        0x00

#define USB_TYPE_CLASS	(0x01 << 5)
#define USB_TYPE_VENDOR	(0x02 << 5)

#define USB_RECIP_DEVICE    0x00
#define USB_RECIP_INTERFACE 0x01
#define USB_RECIP_ENDPOINT  0x02

#define GET_DESCRIPTOR  0x06

#define DT_STRING       0x03


struct euvccam_usb_device 
{
      int fd;
      unsigned short idProduct;
      unsigned short idVendor;
      
      char strProduct[64];
      char strVendor[64];
      char strSerialNumber[64];

      char devpath[PATH_MAX +1];
      char identifier[128];
      unsigned long long serial;
};

typedef struct euvccam_usb_device euvccam_usb_device_t;

struct _usb_device_descriptor {
	uint8_t  bLength;
	uint8_t  bDescriptorType;
	uint16_t bcdUSB;
	uint8_t  bDeviceClass;
	uint8_t  bDeviceSubClass;
	uint8_t  bDeviceProtocol;
	uint8_t  bMaxPacketSize0;
	uint16_t idVendor;
	uint16_t idProduct;
	uint16_t bcdDevice;
	uint8_t  iManufacturer;
	uint8_t  iProduct;
	uint8_t  iSerialNumber;
	uint8_t  bNumConfigurations;
} __attribute__ ((packed));

typedef struct _usb_device_descriptor usb_device_descriptor_t;


unicap_status_t euvccam_usb_init( void );
euvccam_usb_device_t *euvccam_usb_find_device( int index );
unicap_status_t euvccam_usb_open_device( unicap_device_t *unicap_device, euvccam_usb_device_t *dev );
unicap_status_t euvccam_usb_close_device( euvccam_usb_device_t *dev );
unicap_status_t euvccam_usb_ctrl_msg( int fd, uint8_t reqtype, uint8_t req, uint16_t val, uint16_t index, void *buf, uint16_t size );

#endif //__EUVCCAM_USB_H__
