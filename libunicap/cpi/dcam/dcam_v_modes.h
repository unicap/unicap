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

#ifndef __DCAM_V_MODES_H__
#define __DCAM_V_MODES_H__

#include <unicap_status.h>

unicap_status_t _dcam_prepare_format_array( dcam_handle_t, int node, int directory, unicap_format_t *format_array, int *format_count );
int _dcam_count_v_modes( dcam_handle_t, int node, int directory );
int _dcam_get_mode_index( unsigned int format, unsigned int mode );
unicap_status_t _dcam_set_mode_and_format( dcam_handle_t dcamhandle, int index );
unicap_status_t _dcam_get_current_v_mode( dcam_handle_t dcamhandle, int *mode );
unicap_status_t _dcam_get_current_v_format( dcam_handle_t dcamhandle, int *format );
quadlet_t _dcam_get_supported_frame_rates( dcam_handle_t dcamhandle );

#endif//__DCAM_V_MODES_H__
