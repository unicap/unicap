/*
** rgbops.h
** 
** Made by Arne Caspari
** Login   <arne@arne-laptop>
** 
** Started on  Tue Oct 31 06:19:16 2006 Arne Caspari
** Last update Tue Oct 31 06:53:52 2006 Arne Caspari
*/

#ifndef   	RGBOPS_H_
# define   	RGBOPS_H_

#include <unicap.h>
#include <ucil.h>

void ucil_blend_rgb24_a( unicap_data_buffer_t *dest, 
			 unicap_data_buffer_t *img1, 
			 unicap_data_buffer_t *img2, 
			 int alpha );

void ucil_blend_rgb32_a( unicap_data_buffer_t *dest, 
			 unicap_data_buffer_t *img1, 
			 unicap_data_buffer_t *img2, 
			 int alpha );

void ucil_fill_rgb24( unicap_data_buffer_t *buffer, 
		      ucil_color_t *color );

void ucil_fill_rgb32( unicap_data_buffer_t *buffer, 
		      ucil_color_t *color );


void ucil_copy_color_plane_by8( unicap_data_buffer_t *destbuf, 
				unicap_data_buffer_t *srcbuf, 
				ucil_color_plane_t plane );



#endif 	    /* !RGBOPS_H_ */
