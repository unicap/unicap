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

#include "euvccam_cpi.h"
#include "debayer.h"

void euvccam_colorproc_by8_rgb24_nn( euvccam_handle_t handle, unicap_data_buffer_t *dest, unicap_data_buffer_t *src )
{
#ifdef __SSE2__
   debayer_sse2( dest, src, &handle->debayer_data );
#else
   debayer_ccm_rgb24_nn( dest, src, &handle->debayer_data );
#endif
}

void euvccam_colorproc_by8_rgb24_nn_be( euvccam_handle_t handle, unicap_data_buffer_t *dest, unicap_data_buffer_t *src )
{
   debayer_ccm_rgb24_nn_be( dest, src, &handle->debayer_data );
}

void euvccam_colorproc_by8_gr_rgb24_nn( euvccam_handle_t handle, unicap_data_buffer_t *dest, unicap_data_buffer_t *src )
{
   debayer_ccm_rgb24_gr_nn( dest, src, &handle->debayer_data );
}

void euvccam_colorproc_by8_sse2( euvccam_handle_t handle, unicap_data_buffer_t *dest, unicap_data_buffer_t *src )
{
#ifdef __SSE2__
   debayer_sse2( dest, src, &handle->debayer_data );
#else
   debayer_ccm_rgb24_nn( dest, src, &handle->debayer_data );
#endif
}

unicap_status_t euvccam_colorproc_set_wbgain( euvccam_handle_t handle, unicap_property_t *property )
{
   if( !strcmp( property->identifier, "White Balance Blue" ) ){
      handle->debayer_data.bgain = property->value * 4096.0;
   } else {
      handle->debayer_data.rgain = property->value * 4096.0;
   }

   return STATUS_SUCCESS;
}

unicap_status_t euvccam_colorproc_get_wbgain( euvccam_handle_t handle, unicap_property_t *property )
{
   if( !strcmp( property->identifier, "White Balance Blue" ) ){
      property->value = handle->debayer_data.bgain / 4096.0;
   }else{
      property->value = handle->debayer_data.rgain / 4096.0;
   }

   return STATUS_SUCCESS;
}

unicap_status_t euvccam_colorproc_set_wbgain_mode( euvccam_handle_t handle, unicap_property_t *property )
{
   if( property->flags & UNICAP_FLAGS_AUTO )
      handle->debayer_data.wb_auto_mode = WB_MODE_AUTO;
   else if( property->flags & UNICAP_FLAGS_ONE_PUSH )
      handle->debayer_data.wb_auto_mode = WB_MODE_ONE_PUSH;
   else
      handle->debayer_data.wb_auto_mode = WB_MODE_MANUAL;

   return STATUS_SUCCESS;
}

unicap_status_t euvccam_colorproc_get_wbgain_mode( euvccam_handle_t handle, unicap_property_t *property )
{
   switch( handle->debayer_data.wb_auto_mode ){
   case WB_MODE_AUTO:
      property->flags = UNICAP_FLAGS_AUTO;
      break;
   case WB_MODE_ONE_PUSH:
      property->flags = UNICAP_FLAGS_ONE_PUSH;
      break;
   default:
      property->flags = UNICAP_FLAGS_MANUAL;
      break;
   }
   
   return STATUS_SUCCESS;
}


void euvccam_colorproc_auto_wb( euvccam_handle_t handle, unicap_data_buffer_t *buffer )
{
   int x,y;
   const int step = 32;
   unsigned int rval = 0, bval = 0, gval = 0;
   
   for( y = step; y < buffer->format.size.height - step; y+=step ){
      int lineoffset = y * buffer->format.size.width;

      for( x = step; x < buffer->format.size.width - step; x+=step ){
	 gval += buffer->data[ lineoffset + x ];
	 bval += buffer->data[ lineoffset + x + 1];
	 rval += buffer->data[ lineoffset + buffer->format.size.width + x ];
      }
   }
   
   handle->debayer_data.rgain = ( (double)gval / (double)rval ) * 4096;
   handle->debayer_data.bgain = ( (double)gval / (double)bval ) * 4096;
}

