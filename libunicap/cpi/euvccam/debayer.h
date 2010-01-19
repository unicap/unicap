/*
  unicap euvccam plugin

  Copyright (C) 2009  Arne Caspari

  Compilation, installation or use of this program requires a written license. 

  By compilling, installing or using this software you agree to accept the terms 
  of the license as specified in the COPYING file in the root folder of this 
  software package.
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
