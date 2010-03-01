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

#include <sys/types.h>
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unicap.h>
#include <stdint.h>
#include <string.h>
#include <linux/usbdevice_fs.h>

#include <unicap.h>

#include "euvccam_cpi.h"
#include "euvccam_usb.h"



#ifdef DEBUG
#include <execinfo.h>
#endif


#define N_ELEMENTS(x) (sizeof(x)/sizeof(x[0]))


static char *usb_path = NULL;
static char *usb_search_path[]= { "/dev/bus/usb", "/proc/bus/usb", NULL };

static uint16_t supported_pids[] = { 0x8201, 0x8202, 0x8203, 0x8204, 0x8205, 0x8206, 0x8207 };


static char *get_usb_path( void )
{
   int i;
   char *path = NULL;
   
   for( i = 0; (path == NULL) && usb_search_path[i]; i++ ){
      DIR *dir = opendir( usb_search_path[i] );
      if( dir ){
	 struct dirent *dirent;
	 while(( dirent = readdir( dir ) ) != NULL ){
	    if( dirent->d_name[0] == '.' ){
	       path = usb_search_path[i];
	       break;
	    }
	 }
	 closedir( dir );
      }
   }
	 
   return path;
}

static int is_supported( uint16_t vid, uint16_t pid )
{
   TRACE( "vid = %x, pid = %x\n", vid, pid );

   if( vid == 0x199e ){
      int i;
      for( i = 0; i < N_ELEMENTS( supported_pids ); i++ ){
	 if( supported_pids[i] == pid )
	    return 1;
      }
   }
	
   return 0;
}

static unsigned long long string_to_number( char *string )
{
   int i;
   unsigned long long ret = 0;

   for( i = 0; string[i]; i++ ){
      if( !isdigit(string[i]) ){
	 return 0;
      }
      
      ret = (ret<<8) | (string[i]-'0');
   }
   
   return ret; 
}

static unicap_status_t claim_device( int fd )
{
   int tmp;
   unicap_status_t status = STATUS_SUCCESS;
	
   tmp = 1;
   if( ioctl( fd, USBDEVFS_SETCONFIGURATION, &tmp ) < 0 )
      status = STATUS_FAILURE;
   tmp = 0;
   if( ioctl( fd, USBDEVFS_CLAIMINTERFACE, &tmp ) < 0 )
      status = STATUS_FAILURE;
   tmp = 1;
   if( ioctl( fd, USBDEVFS_CLAIMINTERFACE, &tmp ) < 0 )
      status = STATUS_FAILURE;
	
   return status;
}

unicap_status_t euvccam_usb_init( void )
{
   if( usb_path ){
      TRACE( "EUVCCAM_USB already initialized\n" );
      return STATUS_FAILURE;
   }
	
   usb_path = get_usb_path();

   TRACE( "euvccam_usb_init - usb_path = %s\n", usb_path );
	
   return ( usb_path != NULL ) ? STATUS_SUCCESS : STATUS_FAILURE;
}

void print_caller( int depth )
{
#ifdef DEBUG
   void *array[10];
   size_t size;
   char **strings;
   size_t i;
     
   size = backtrace (array, 10);
   strings = backtrace_symbols (array, size);
   
   if( size > depth ){
      TRACE( "Caller: %s\n", strings[depth ] );
   }

   free (strings);
#else
   return;
#endif
}

unicap_status_t euvccam_usb_ctrl_msg( int fd, uint8_t reqtype, uint8_t req, uint16_t val, uint16_t index, void *buf, uint16_t size )
{
   struct usbdevfs_ctrltransfer xfer;
   int ret;
   unicap_status_t status = STATUS_SUCCESS;

   xfer.bRequestType = reqtype;
   xfer.bRequest = req;
   xfer.wValue = val;
   xfer.wIndex = index;
   xfer.wLength = size;
   xfer.data = buf;
   xfer.timeout = 10000;
	
   TRACE( "ctrl msg: %x %x %x %x %x\n", reqtype, req, val, index, size );
   print_caller( 2 );

   ret = ioctl( fd, USBDEVFS_CONTROL, &xfer );
   if( ret<0 ){
      TRACE( "control message failed!\n" );
      status = STATUS_FAILURE;
   }
	
   return status;
}

static unicap_status_t euvccam_usb_read_ascii_string( int fd, int index, char *buf, size_t len )
{
   char unicode[256];
   int i,j;
	
	
   if( !SUCCESS( euvccam_usb_ctrl_msg( fd, EP0_IN, GET_DESCRIPTOR, (DT_STRING << 8) + index, 0, unicode, sizeof( unicode ) ) ) ){
      TRACE( "Failed to read string descriptor" );
      return STATUS_FAILURE;
   }
	
   if( unicode[1] != DT_STRING )
      return STATUS_FAILURE;
	
   if( unicode[0] > ((len-1)*2) )
      return STATUS_FAILURE;
	
   for( i = 0, j = 2; j < unicode[0]; i++, j+=2 )
      buf[i] = unicode[j];
	
   buf[i] = 0;
	
   return STATUS_SUCCESS;
}


euvccam_usb_device_t *euvccam_usb_find_device( int index )
{
   DIR *usbdir = opendir( usb_path );
   int cind = -1;
   static euvccam_usb_device_t dev;
   euvccam_usb_device_t *ret = NULL;

   TRACE( "euvccam_usb_find_device index=%d\n", index );
   
   if( usbdir ){
      struct dirent *usbdirent;
      
      while((cind != index) && ((usbdirent = readdir( usbdir )) != NULL )){
	 if( usbdirent->d_name[0] == '.' )
	    continue;
	 
	 char buspath[PATH_MAX + 1];			
	 sprintf( buspath, "%s/%s", usb_path, usbdirent->d_name );
	 TRACE( "buspath: %s\n", buspath );
	 
	 DIR *busdir = opendir( buspath );
	 if( busdir ){
	    struct dirent *busdirent;
	    
	    while((cind != index) && ((busdirent = readdir( busdir )) != NULL )){
	       if( busdirent->d_name[0] == '.' )
		  continue;
	       
	       char devpath[PATH_MAX + 1];
	       int fd;
	       
	       sprintf( devpath, "%s/%s", buspath, busdirent->d_name );
	       
	       fd = open( devpath, O_RDWR, 0 );
	       if( fd < 0 )
		  continue;
					
	       usb_device_descriptor_t descriptor;
	       int rd = read( fd, (void*)&descriptor, sizeof( descriptor ) );
	       if( rd < 0 ){
		  TRACE( "Failed to read descriptor for device: %s\n", devpath );
		  close( fd );
		  continue;
	       }
	       
	       TRACE( "Read descriptor of device: %s len=%d\n", devpath, rd );

	       if( is_supported( descriptor.idVendor, descriptor.idProduct ) ){
		  cind++;
		  if( cind == index ){
		     dev.idVendor = descriptor.idVendor;
		     dev.idProduct = descriptor.idProduct;
		     dev.fd = -1;
		     if( !SUCCESS( euvccam_usb_read_ascii_string( fd, descriptor.iManufacturer, dev.strVendor, sizeof( dev.strVendor ) ) ) ){
			strcpy( dev.strVendor, "The Imaging Source" );
		     }
		     if( !SUCCESS( euvccam_usb_read_ascii_string( fd, descriptor.iProduct, dev.strProduct, sizeof( dev.strProduct ) ) ) ){
			strcpy( dev.strProduct, "CMOS camera" );
		     }
		     if( !SUCCESS( euvccam_usb_read_ascii_string( fd, descriptor.iSerialNumber, dev.strSerialNumber, sizeof( dev.strSerialNumber ) ) ) ){
			strcpy( dev.strSerialNumber, "0" );
		     }
		     strcpy( dev.devpath, devpath );
		     sprintf( dev.identifier, "%s %s %s", dev.strVendor, dev.strProduct, dev.strSerialNumber );
		     dev.serial = string_to_number( dev.strSerialNumber );
		     ret = &dev;
		     TRACE( "Device found %x:%x\n", dev.idVendor, dev.idProduct );
		  }
	       }
	       
	       close( fd );
	    }
	    closedir( busdir );
	 }
      }
      closedir( usbdir );
   }
   
   return ret;
}

unicap_status_t euvccam_usb_open_device( unicap_device_t *unicap_device, euvccam_usb_device_t *dev )
{
   int i;
   unicap_status_t status = STATUS_FAILURE;
   euvccam_usb_device_t *tmp = NULL;
	
   for( i = 0; ( tmp = euvccam_usb_find_device( i ) ); i++ ){
      if( !strcmp( tmp->identifier, unicap_device->identifier ) ){
	 memcpy( dev, tmp, sizeof( euvccam_usb_device_t ) );
	 dev->fd = open( dev->devpath, O_RDWR, 0 );
	 if( dev->fd < 0 ){
	    TRACE( "Failed to open device: %s\n", unicap_device->identifier );
	    return STATUS_FAILURE;
	 }
	 claim_device( dev->fd );
	 strcpy( unicap_device->vendor_name, dev->strVendor );
	 strcpy( unicap_device->model_name, dev->strProduct );
	 unicap_device->vendor_id = dev->idVendor;
	 unicap_device->model_id = string_to_number( dev->strSerialNumber );
	 strcpy( unicap_device->device, dev->devpath );
	 unicap_device->flags = 0;
	 status = STATUS_SUCCESS;
	 break;
      }
   }
	
   return status;
}		

unicap_status_t euvccam_usb_close_device( euvccam_usb_device_t *dev )
{
   unicap_status_t status = STATUS_FAILURE;
	
   if( dev->fd >= 0 ){
      close( dev->fd );
      dev->fd = -1;
      status = STATUS_SUCCESS;
   }
	
   return status;
}




