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

#ifndef __DCAM_PROPERTY_H__
#define __DCAM_PROPERTY_H__

unicap_status_t _dcam_prepare_property_table( dcam_handle_t dcamhandle, dcam_property_t **property_table );
unicap_status_t dcam_set_property( dcam_handle_t dcamhandle, 
								   unicap_property_t *property, 
								   dcam_property_t *dcam_property );

unicap_status_t dcam_set_frame_rate_property( dcam_handle_t dcamhandle, 
											  unicap_property_t *property, 
											  dcam_property_t *dcam_property );

unicap_status_t dcam_set_white_balance_mode_property( dcam_handle_t dcamhandle, 
													  unicap_property_t *property, 
													  dcam_property_t *dcam_property );

unicap_status_t dcam_get_property( dcam_handle_t dcamhandle, 
								   unicap_property_t *property, 
								   dcam_property_t *dcam_property );

unicap_status_t dcam_get_frame_rate_property( dcam_handle_t dcamhandle, 
											  unicap_property_t *property, 
											  dcam_property_t *dcam_property );

unicap_status_t dcam_get_white_balance_mode_property( dcam_handle_t dcamhandle, 
													  unicap_property_t *property, 
													  dcam_property_t *dcam_property );

unicap_status_t dcam_init_shutter_property( dcam_handle_t dcamhandle, 
											unicap_property_t *p, 
											dcam_property_t *dcam_property );

unicap_status_t dcam_init_brightness_property( dcam_handle_t dcamhandle, 
											   unicap_property_t *p, 
											   dcam_property_t *dcam_property );

unicap_status_t dcam_init_white_balance_mode_property( dcam_handle_t dcamhandle, 
													   unicap_property_t *p, 
													   dcam_property_t *dcam_property );

unicap_status_t dcam_init_white_balance_property( dcam_handle_t dcamhandle, 
													unicap_property_t *p, 
													dcam_property_t *dcam_property );

unicap_status_t dcam_init_trigger_property( dcam_handle_t dcamhandle, 
											unicap_property_t *p, 
											dcam_property_t *dcam_property );

unicap_status_t dcam_init_frame_rate_property( dcam_handle_t dcamhandle, 
											   unicap_property_t *p, 
											   dcam_property_t *dcam_property );

unicap_status_t dcam_init_rw_register_property( dcam_handle_t dcamhandle, 
											   unicap_property_t *p, 
												dcam_property_t *dcam_property );


unicap_status_t dcam_init_timeout_property( dcam_handle_t dcamhandle, 
											unicap_property_t *p, 
											dcam_property_t *dcam_property );

unicap_status_t dcam_get_timeout_property( dcam_handle_t dcamhandle, 
										   unicap_property_t *property, 
										   dcam_property_t *dcam_property );

unicap_status_t dcam_set_timeout_property( dcam_handle_t dcamhandle, 
										   unicap_property_t *property, 
										   dcam_property_t *dcam_property );

unicap_status_t dcam_init_gpio_property( dcam_handle_t dcamhandle, 
										 unicap_property_t *p, 
										 dcam_property_t *dcam_property );

unicap_status_t dcam_set_gpio_property( dcam_handle_t dcamhandle, 
					unicap_property_t *property, 
					dcam_property_t *dcam_property );

unicap_status_t dcam_get_gpio_property( dcam_handle_t dcamhandle, 
					unicap_property_t *property, 
					dcam_property_t *dcam_property );

unicap_status_t dcam_init_software_trigger_property( dcam_handle_t dcamhandle, 
						     unicap_property_t *p, 
						     dcam_property_t *dcam_property );

unicap_status_t dcam_set_software_trigger_property( dcam_handle_t dcamhandle, 
						    unicap_property_t *property, 
						    dcam_property_t *dcam_property );


unicap_status_t dcam_get_software_trigger_property( dcam_handle_t dcamhandle, 
						    unicap_property_t *property, 
						    dcam_property_t *dcam_property );


unicap_status_t dcam_set_strobe_mode_property( dcam_handle_t dcamhandle, 
					       unicap_property_t *property, 
					       dcam_property_t *dcam_property );

unicap_status_t dcam_get_strobe_mode_property( dcam_handle_t dcamhandle, 
					       unicap_property_t *property, 
					       dcam_property_t *dcam_property );

unicap_status_t dcam_set_strobe_duration_property( dcam_handle_t dcamhandle, 
						   unicap_property_t *property, 
						   dcam_property_t *dcam_property );

unicap_status_t dcam_get_strobe_duration_property( dcam_handle_t dcamhandle, 
						   unicap_property_t *property, 
						   dcam_property_t *dcam_property );

unicap_status_t dcam_set_strobe_delay_property( dcam_handle_t dcamhandle, 
						   unicap_property_t *property, 
						   dcam_property_t *dcam_property );

unicap_status_t dcam_get_strobe_delay_property( dcam_handle_t dcamhandle, 
						   unicap_property_t *property, 
						   dcam_property_t *dcam_property );

unicap_status_t dcam_set_strobe_polarity_property( dcam_handle_t dcamhandle, 
						   unicap_property_t *property, 
						   dcam_property_t *dcam_property );

unicap_status_t dcam_get_strobe_polarity_property( dcam_handle_t dcamhandle, 
						   unicap_property_t *property, 
						   dcam_property_t *dcam_property );



int _dcam_check_frame_rate_available( quadlet_t quad, int rate );

dcam_property_t *dcam_copy_property( dcam_property_t *dest, dcam_property_t *src );


#endif//__DCAM_PROPERTY_H__
