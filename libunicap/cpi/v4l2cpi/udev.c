/*
  unicap
  Copyright (C) 2011  Arne Caspari

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

#include <libudev.h>
#include <string.h>
#include <stdlib.h>



char *v4l2cpi_udev_get_serial (const char* devfile)
{
	struct udev *udev;
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices, *dev_list_entry;
	struct udev_device *dev;
	char *ret = NULL;
	
	udev = udev_new ();
	if (!udev)
		return NULL;
	
	enumerate = udev_enumerate_new (udev);
	udev_enumerate_add_match_subsystem (enumerate, "video4linux");
	udev_enumerate_scan_devices (enumerate);
	devices = udev_enumerate_get_list_entry (enumerate);
	udev_list_entry_foreach (dev_list_entry, devices){
		const char *path;
		
		path = udev_list_entry_get_name (dev_list_entry);
		dev = udev_device_new_from_syspath (udev, path);

		if (!strcmp (udev_device_get_devnode (dev), devfile)){
			dev = udev_device_get_parent_with_subsystem_devtype (dev, "usb", "usb_device");
			if (dev){
				const char *serial;
				
				serial = udev_device_get_sysattr_value(dev, "serial");
				if (serial){
					ret = malloc (strlen (serial)+1);
					strcpy (ret, serial);
				}
				udev_device_unref (dev);
			}
		}
	}
	/* Free the enumerator object */
	udev_enumerate_unref(enumerate);

	udev_unref(udev);

	return ret;
}
