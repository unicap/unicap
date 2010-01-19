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

#ifndef __EUVCCAM_COLORPROC_H__
#define __EUVCCAM_COLORPROC_H__

#include "euvccam_cpi.h"

void euvccam_colorproc_by8_rgb24_nn( euvccam_handle_t handle, unicap_data_buffer_t *dest, unicap_data_buffer_t *src );
void euvccam_colorproc_by8_rgb24_nn_be( euvccam_handle_t handle, unicap_data_buffer_t *dest, unicap_data_buffer_t *src );
void euvccam_colorproc_by8_gr_rgb24_nn( euvccam_handle_t handle, unicap_data_buffer_t *dest, unicap_data_buffer_t *src );
void euvccam_colorproc_auto_wb( euvccam_handle_t handle, unicap_data_buffer_t *buffer );


unicap_status_t euvccam_colorproc_set_wbgain( euvccam_handle_t handle, unicap_property_t *property );
unicap_status_t euvccam_colorproc_get_wbgain( euvccam_handle_t handle, unicap_property_t *property );

unicap_status_t euvccam_colorproc_set_wbgain_mode( euvccam_handle_t handle, unicap_property_t *property );
unicap_status_t euvccam_colorproc_get_wbgain_mode( euvccam_handle_t handle, unicap_property_t *property );

#endif//__EUVCCAM_COLORPROC_H__
