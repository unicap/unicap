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
 */

#ifndef __DEBAYER_H__
#define __DEBAYER_H__

enum{
   WB_MODE_MANUAL = 0, 
   WB_MODE_AUTO, 
   WB_MODE_ONE_PUSH
};


struct _debayer_data
{
   int use_ccm;
   int use_rbgain;
   
   int wb_auto_mode;

   // Color Correction Matrix
   int ccm[3][3];
   
   // red / blue gain
   int rgain;
   int bgain;
};

typedef struct _debayer_data debayer_data_t;


void debayer_ccm_rgb24_nn( unicap_data_buffer_t *destbuf, unicap_data_buffer_t *srcbuf, debayer_data_t *data );
void debayer_ccm_rgb24_gr_nn( unicap_data_buffer_t *destbuf, unicap_data_buffer_t *srcbuf, debayer_data_t *data );
void debayer_ccm_rgb24_edge( unicap_data_buffer_t *destbuf, unicap_data_buffer_t *srcbuf, debayer_data_t *data );
void debayer_ccm_uyvy( unicap_data_buffer_t *destbuf, unicap_data_buffer_t *srcbuf, debayer_data_t *data );
void debayer_calculate_rbgain( unicap_data_buffer_t *buffer, int *rgain, int *bgain, int *combined );
void debayer_ccm_rgb24_nn_be( unicap_data_buffer_t *destbuf, unicap_data_buffer_t *srcbuf, debayer_data_t *data );
void debayer_sse2( unicap_data_buffer_t *destbuf, unicap_data_buffer_t* srcbuf, debayer_data_t *data );

#endif//__DEBAYER_H__
