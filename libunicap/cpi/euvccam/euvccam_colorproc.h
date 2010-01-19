/*
  unicap euvccam plugin

  Copyright (C) 2009  Arne Caspari

  Compilation, installation or use of this program requires a written license. 

  By compilling, installing or using this software you agree to accept the terms 
  of the license as specified in the COPYING file in the root folder of this 
  software package.
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
