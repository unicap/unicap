#ifndef __VISCA_PRIVATE_H__
#define __VISCA_PRIVATE_H__

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

#define VISCA_COMMAND_ZOOM           0x07
#define VISCA_COMMAND_ZOOM_DIRECT    0x47

#define VISCA_COMMAND_FOCUS          0x08
#define VISCA_COMMAND_FOCUS_AUTO     0x38
#define VISCA_COMMAND_FOCUS_DIRECT   0x48
#define VISCA_COMMAND_FOCUS_MODE_INQ 0x57
#define VISCA_FOCUS_AUTO             0x02
#define VISCA_FOCUS_MANUAL           0x03

#define VISCA_COMMAND_AE_MODE        0x39
#define VISCA_AE_MODE_FULL_AUTO      0x00
#define VISCA_AE_MODE_MANUAL         0x03
#define VISCA_AE_MODE_SHUTTER_PRIO   0x0A
#define VISCA_AE_MODE_IRIS_PRIO      0x0B
#define VISCA_AE_MODE_BRIGHT         0x0D
#define VISCA_AE_MENU_ITEM_FULL_AUTO     "Full Auto"
#define VISCA_AE_MENU_ITEM_MANUAL        "Manual"
#define VISCA_AE_MENU_ITEM_SHUTTER_PRIO  "Shutter Priority"
#define	VISCA_AE_MENU_ITEM_IRIS_PRIO     "Iris Priority"
#define	VISCA_AE_MENU_ITEM_BRIGHT        "Bright Mode"

#define VISCA_COMMAND_SHUTTER_DIRECT 0x4A

#define VISCA_COMMAND_IRIS_DIRECT    0x4B

#define VISCA_COMMAND_GAIN_DIRECT    0x4C

#define VISCA_COMMAND_WHITE_BALANCE  0x35
#define VISCA_WHITE_BALANCE_AUTO     0x00
#define VISCA_WHITE_BALANCE_INDOOR   0x01
#define VISCA_WHITE_BALANCE_OUTDOOR  0x02

#define VISCA_COMMAND_COMPLETE 0x50



unicap_status_t visca_set_white_balance( vid21394handle_t vid21394handle, 
										 unicap_property_t *property );
unicap_status_t visca_get_white_balance( vid21394handle_t vid21394handle, 
										 unicap_property_t *property );

unicap_status_t visca_set_zoom( vid21394handle_t vid21394handle, 
								unicap_property_t *property );
unicap_status_t visca_get_zoom( vid21394handle_t vid21394handle, 
								unicap_property_t *property );

unicap_status_t visca_set_focus( vid21394handle_t vid21394handle, 
								 unicap_property_t *property );
unicap_status_t visca_get_focus( vid21394handle_t vid21394handle, 
								 unicap_property_t *property );

unicap_status_t visca_set_gain( vid21394handle_t vid21394handle, 
								unicap_property_t *property );
unicap_status_t visca_get_gain( vid21394handle_t vid21394handle, 
								unicap_property_t *property );

unicap_status_t visca_set_shutter( vid21394handle_t vid21394handle, 
								   unicap_property_t *property );
unicap_status_t visca_get_shutter( vid21394handle_t vid21394handle, 
								   unicap_property_t *property );

unicap_status_t visca_set_iris( vid21394handle_t vid21394handle, 
								unicap_property_t *property );
unicap_status_t visca_get_iris( vid21394handle_t vid21394handle, 
								unicap_property_t *property );

unicap_status_t visca_set_ae_mode( vid21394handle_t vid21394handle, 
								   unicap_property_t *property );
unicap_status_t visca_get_ae_mode( vid21394handle_t vid21394handle, 
								   unicap_property_t *property );

#endif//__VISCA_PRIVATE_H__
