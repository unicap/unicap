#ifndef __VISCA_PROPERTY_TABLE_H__
#define __VISCA_PROPERTY_TABLE_H__
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

#include <unicap.h>
#include "vid21394_base.h"
#include "visca_private.h"

typedef unicap_status_t (*visca_property_function_t) ( vid21394handle_t vid21394handle, unicap_property_t *property );

struct visca_property
{
		unicap_property_t property;
		visca_property_function_t set_function;
		visca_property_function_t get_function;
};

typedef struct visca_property visca_property_t;


#define VISCA_CAT_COLOR     "color"
#define VISCA_CAT_LENS      "lens control"
#define VISCA_CAT_EXPOSURE  "exposure"

#define VISCA_UNIT_KELVIN "k"
#define VISCA_UNIT_NONE   ""

#define VISCA_ID_WHITE_BALANCE "White Balance"
#define VISCA_WB_DEFAULT       5200
#define VISCA_WB_DEFAULT_FLAGS 

static double visca_white_balance_values[] = 
{
	3800.0f, 
	5200.0f
};

#define VISCA_ID_ZOOM            "Zoom"
#define VISCA_ZOOM_DEFAULT       0
#define VISCA_ZOOM_RANGE_MIN     0.0f
#define VISCA_ZOOM_RANGE_MAX     50000.0f

#define VISCA_ID_IRIS            "Iris"
#define VISCA_IRIS_DEFAULT       16
#define VISCA_IRIS_RANGE_MIN     0.0f
#define VISCA_IRIS_RANGE_MAX    16.0f

#define VISCA_ID_FOCUS            "Focus"
#define VISCA_FOCUS_DEFAULT       0
#define VISCA_FOCUS_RANGE_MIN    0.0f
#define VISCA_FOCUS_RANGE_MAX    50000.0f

#define VISCA_ID_SHUTTER            "Shutter"
#define VISCA_SHUTTER_DEFAULT       0
#define VISCA_SHUTTER_RANGE_MIN     0.0f
#define VISCA_SHUTTER_RANGE_MAX     255.0f

#define VISCA_ID_GAIN            "Gain"
#define VISCA_GAIN_DEFAULT       0
#define VISCA_GAIN_RANGE_MIN     0.0f
#define VISCA_GAIN_RANGE_MAX     255.0f

#define VISCA_ID_AUTOEXPOSURE_MODE      "Auto Exposure Mode"
#define VISCA_AUTOEXPOSURE_MODE_DEFAULT "Full Auto"
#define VISCA_AUTOEXPOSURE_MODE_FLAGS   UNICAP_FLAGS_MANUAL

char *visca_autoexposure_mode_menu_items[] = 
{
	VISCA_AE_MENU_ITEM_FULL_AUTO, 
	VISCA_AE_MENU_ITEM_MANUAL, 
	VISCA_AE_MENU_ITEM_SHUTTER_PRIO, 
	VISCA_AE_MENU_ITEM_IRIS_PRIO, 
	VISCA_AE_MENU_ITEM_BRIGHT
};



visca_property_t visca_property_table[] =
{
	{ 
		property: { identifier: VISCA_ID_WHITE_BALANCE, 
					category:   VISCA_CAT_COLOR, 
					unit:       VISCA_UNIT_KELVIN, 
					relations:  0, 
					relations_count: 0, 
					{value:VISCA_WB_DEFAULT}, 
					{value_list:{visca_white_balance_values,sizeof( visca_white_balance_values ) / sizeof( double )}},
					stepping:   0,
					type:       UNICAP_PROPERTY_TYPE_VALUE_LIST, 
					flags:      UNICAP_FLAGS_AUTO, 
					flags_mask: UNICAP_FLAGS_MANUAL | UNICAP_FLAGS_AUTO, 
					property_data: 0,
					property_data_size: 0 // data 
		}, 
		set_function: visca_set_white_balance, 
		get_function: visca_get_white_balance,
	},

	{ 
		property: { identifier: VISCA_ID_ZOOM, 
					category:   VISCA_CAT_LENS, 
					unit:       VISCA_UNIT_NONE, 
					relations:  0, 
					relations_count: 0, 
					{value: VISCA_ZOOM_DEFAULT }, 
					{range:{ VISCA_ZOOM_RANGE_MIN, VISCA_ZOOM_RANGE_MAX }},
					stepping: 1.0f, 
					type:       UNICAP_PROPERTY_TYPE_RANGE, 
					flags:      UNICAP_FLAGS_MANUAL, 
					flags_mask: UNICAP_FLAGS_MANUAL, 
					property_data:      0, 
					property_data_size: 0 
		}, 
		set_function: visca_set_zoom, 
		get_function: visca_get_zoom, 
	},
	
	{ 
		property: { identifier: VISCA_ID_FOCUS,
					category:   VISCA_CAT_LENS,
					unit:       VISCA_UNIT_NONE,
					relations:  0,
					relations_count: 0,
					{value: VISCA_FOCUS_DEFAULT},
					{range:{ VISCA_FOCUS_RANGE_MIN, VISCA_FOCUS_RANGE_MAX }},
					stepping:   1.0f,
					type:       UNICAP_PROPERTY_TYPE_RANGE,
					flags:      UNICAP_FLAGS_AUTO,
					flags_mask: UNICAP_FLAGS_MANUAL | UNICAP_FLAGS_AUTO,
					property_data: 0,
					property_data_size: 0
		},
		set_function: visca_set_focus, 
		get_function: visca_get_focus, 
	},
	
	{ 
		property: { identifier: VISCA_ID_IRIS,
					category:   VISCA_CAT_LENS,
					unit:       VISCA_UNIT_NONE,
					relations:  0,
					relations_count: 0,
					{value: VISCA_IRIS_DEFAULT },
					{range:{ VISCA_IRIS_RANGE_MIN, VISCA_IRIS_RANGE_MAX }},
					stepping: 1.0f,
					type:       UNICAP_PROPERTY_TYPE_RANGE,
					flags:      UNICAP_FLAGS_MANUAL,
					flags_mask: UNICAP_FLAGS_MANUAL,
					property_data: 0,
					property_data_size: 0
		}, 
		set_function: visca_set_focus, 
		get_function: visca_get_focus, 
	},
	
	{ 
		property: { identifier: VISCA_ID_GAIN,
					category:   VISCA_CAT_EXPOSURE,
					unit:       VISCA_UNIT_NONE,
					relations:       0,
					relations_count: 0,
					{value: VISCA_GAIN_DEFAULT },
					{range:{ VISCA_GAIN_RANGE_MIN, VISCA_GAIN_RANGE_MAX }},
					stepping: 1.0f,
					type:       UNICAP_PROPERTY_TYPE_RANGE,
					flags:      UNICAP_FLAGS_AUTO,
					flags_mask: UNICAP_FLAGS_MANUAL | UNICAP_FLAGS_AUTO,
					property_data:      0,
					property_data_size: 0
		},
		set_function: visca_set_gain, 
		get_function: visca_get_gain, 
	}, 
	
	{ 
		property: { identifier: VISCA_ID_SHUTTER,
					category:   VISCA_CAT_EXPOSURE,
					unit:       VISCA_UNIT_NONE,
					relations: 0,
					relations_count: 0,
					{value: VISCA_SHUTTER_DEFAULT },
					{range:{ VISCA_SHUTTER_RANGE_MIN, VISCA_SHUTTER_RANGE_MAX }},
					stepping: 1.0f,
					type:       UNICAP_PROPERTY_TYPE_RANGE,
					flags:      UNICAP_FLAGS_MANUAL,
					flags_mask: UNICAP_FLAGS_MANUAL,
					property_data: 0,
					property_data_size: 0
		}, 
		set_function: visca_set_shutter, 
		get_function: visca_get_shutter, 
	},
	
	{ 
		property: { identifier: VISCA_ID_AUTOEXPOSURE_MODE,
					category:   VISCA_CAT_EXPOSURE,
					unit:       VISCA_UNIT_NONE,
					relations: 0,
					relations_count: 0,
					{menu_item:{ VISCA_AUTOEXPOSURE_MODE_DEFAULT}},
					{menu:{ visca_autoexposure_mode_menu_items, 
							sizeof( visca_autoexposure_mode_menu_items ) / sizeof ( char* ) } },
					stepping: 0,
					type:       UNICAP_PROPERTY_TYPE_MENU,
					flags:      UNICAP_FLAGS_MANUAL,
					flags_mask: UNICAP_FLAGS_MANUAL,
					property_data:      0,
					property_data_size: 0
		}, 
		set_function: visca_set_ae_mode, 
		get_function: visca_get_ae_mode, 
	},
	
};


#endif//__VISCA_PROPERTY_TABLE_H__
