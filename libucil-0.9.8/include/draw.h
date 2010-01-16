/*
** draw.h
** 
** Made by Arne Caspari
** Login   <arne@arne-laptop>
** 
** Started on  Tue Sep 19 07:19:30 2006 Arne Caspari
** Last update Wed Oct 25 18:25:32 2006 Arne Caspari
*/

#ifndef   	DRAW_H_
# define   	DRAW_H_

#include <unicap.h>

void ucil_convert_color( ucil_color_t *src, ucil_color_t *dest );
void ucil_set_pixel( unicap_data_buffer_t *data_buffer, ucil_color_t *color, int x, int y );
void ucil_draw_line( unicap_data_buffer_t *data_buffer, ucil_color_t *color, int x1, int y1, int x2, int y2 );
void ucil_draw_rect( unicap_data_buffer_t *data_buffer, ucil_color_t *color, int x1, int y1, int x2, int y2 );
void ucil_fill( unicap_data_buffer_t *data_buffer, ucil_color_t *color );
void ucil_draw_box( unicap_data_buffer_t *data_buffer, ucil_color_t *color, int x1, int y1, int x2, int y2 );

#endif 	    /* !DRAW_H_ */
