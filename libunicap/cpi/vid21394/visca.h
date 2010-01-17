#ifndef __VISCA_H__
#define __VISCA_H__

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

enum visca_camera_type
{
	VISCA_CAMERA_TYPE_NONE = 0, 
	VISCA_CAMERA_TYPE_FCB_IX47, 
	VISCA_CAMERA_TYPE_UNKNOWN
};

typedef enum visca_camera_type visca_camera_type_t;


unicap_status_t visca_reenumerate_properties( vid21394handle_t vid21394handle, int *count );
unicap_status_t visca_enumerate_properties( unicap_property_t *property, int index );
unicap_status_t visca_set_property( vid21394handle_t vid21394handle, unicap_property_t *property );
unicap_status_t visca_get_property( vid21394handle_t vid21394handle, unicap_property_t *property );
unicap_status_t visca_check_camera( vid21394handle_t vid21394handle, visca_camera_type_t *type );


#endif//__VISCA_H__
