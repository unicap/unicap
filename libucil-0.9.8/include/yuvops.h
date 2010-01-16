/*
** yuvops.h
** 
** Made by Arne Caspari
** Login   <arne@arne-laptop>
** 
** Started on  Mon Oct 30 07:20:36 2006 Arne Caspari
** Last update Mon Oct 30 18:33:39 2006 Arne Caspari
*/

#ifndef   	YUVOPS_H_
# define   	YUVOPS_H_

#include "unicap.h"
#include "ucil.h"

void ucil_blend_uyvy_a( unicap_data_buffer_t *dest, 
			unicap_data_buffer_t *img1, 
			unicap_data_buffer_t *img2, 
			int alpha );

void ucil_fill_uyvy( unicap_data_buffer_t *buffer, 
		     ucil_color_t *color );


void ucil_composite_UYVY_YUVA( unicap_data_buffer_t *dest, 
			       unicap_data_buffer_t *img, 
			       int xpos, 
			       int ypos, 
			       double scalex, 
			       double scaley, 
			       ucil_interpolation_type_t interp );

void ucil_composite_YUYV_YUVA( unicap_data_buffer_t *dest, 
			       unicap_data_buffer_t *img, 
			       int xpos, 
			       int ypos, 
			       double scalex, 
			       double scaley, 
			       ucil_interpolation_type_t interp );

void ucil_convolution_mask_uyvy( unicap_data_buffer_t *dest, 
				 unicap_data_buffer_t *src, 
				 ucil_convolution_mask_t *mask );

#endif 	    /* !YUVOPS_H_ */
