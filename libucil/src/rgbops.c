/*
** rgbops.c
** 
** Made by (Arne Caspari)
** Login   <arne@arne-laptop>
** 
** Started on  Tue Oct 31 06:19:12 2006 Arne Caspari
** Last update Tue Oct 31 17:20:40 2006 Arne Caspari
*/

#include "rgbops.h"
#include "unicap.h"
#include "ucil.h"

#include <sys/types.h>
#include <linux/types.h>


void ucil_blend_rgb24_a( unicap_data_buffer_t *dest, 
			 unicap_data_buffer_t *img1, 
			 unicap_data_buffer_t *img2, 
			 int alpha )
{
   int offset = 0;

   int af = alpha;
   int ab = ( 1 << 16 ) - alpha;

   while( offset < img1->buffer_size )
   {
      int br, bg, bb, fr, fg, fb;
      
      br = ( (int)img1->data[offset] * ab ) >> 16;
      bg = ( (int)img1->data[offset+1] * ab ) >> 16;
      bb = ( (int)img1->data[offset+2] * ab ) >> 16;
      
      fr = ( (int)img2->data[offset] * af ) >> 16;
      fg = ( (int)img2->data[offset+1] * af ) >> 16;
      fb = ( (int)img2->data[offset+2] * af ) >> 16;
      
      dest->data[offset++] = (char)(br + fr);
      dest->data[offset++] = (char)(bg + fg);
      dest->data[offset++] = (char)(bb + fb);
   }
}

void ucil_blend_rgb32_a( unicap_data_buffer_t *dest, 
			 unicap_data_buffer_t *img1, 
			 unicap_data_buffer_t *img2, 
			 int alpha )
{
   int offset = 0;

   int af = alpha;
   int ab = ( 1 << 16 ) - alpha;

   while( offset < img1->buffer_size )
   {
      int br, bg, bb, fr, fg, fb;
      
      br = ( (int)img1->data[offset] * ab ) >> 16;
      bg = ( (int)img1->data[offset+1] * ab ) >> 16;
      bb = ( (int)img1->data[offset+2] * ab ) >> 16;
      
      fr = ( (int)img2->data[offset] * af ) >> 16;
      fg = ( (int)img2->data[offset+1] * af ) >> 16;
      fb = ( (int)img2->data[offset+2] * af ) >> 16;
      
      dest->data[offset++] = (char)(br + fr);
      dest->data[offset++] = (char)(bg + fg);
      dest->data[offset++] = (char)(bb + fb);
      dest->data[offset++] = 255;
   }
}


void ucil_fill_rgb24( unicap_data_buffer_t *buffer, 
		      ucil_color_t *color )
{
   int offset = 0;
   
   while( offset < buffer->buffer_size )
   {
      buffer->data[offset++] = color->rgb24.r;
      buffer->data[offset++] = color->rgb24.g;
      buffer->data[offset++] = color->rgb24.b;
   }
}

void ucil_fill_rgb32( unicap_data_buffer_t *buffer, 
		      ucil_color_t *color )
{
   int offset = 0;
   
   while( offset < buffer->buffer_size )
   {
      buffer->data[offset++] = color->rgb32.r;
      buffer->data[offset++] = color->rgb32.g;
      buffer->data[offset++] = color->rgb32.b;
      buffer->data[offset++] = color->rgb32.a;
   }
}

void ucil_copy_color_plane_by8( unicap_data_buffer_t *destbuf, 
				unicap_data_buffer_t *srcbuf, 
				ucil_color_plane_t plane )
{
   int i,j;
   __u8 *dest = destbuf->data;
   __u8 *src = srcbuf->data;
   
   switch( plane ){
   case UCIL_COLOR_PLANE_RED:
      for( j = 0; j < srcbuf->format.size.height; j+=2 ){
	 src += srcbuf->format.size.width; // Skip 'blue' rows
	 for( i = 1; i < srcbuf->format.size.width; i+=2 ){
	    *dest++ = *src;
	    src+=2;
	 }
      }
      break;
      
   case UCIL_COLOR_PLANE_GREEN:
      for( j = 0; j < srcbuf->format.size.height; j+=2 ){
	 for( i = 0; i < srcbuf->format.size.width; i+=2 ){
	    // GB row
	    *dest++ = *src; 
	    src+=2;
	 }
	 for( i = 1; i < srcbuf->format.size.width; i+=2 ){
	    // RG row
	    *dest++ = *src;
	    src+=2;
	 }
      }
      break;
   case UCIL_COLOR_PLANE_BLUE:
      for( j = 0; j < srcbuf->format.size.height; j+=2 ){
	 for( i = 0; i < srcbuf->format.size.width; i+=2 ){
	    // GB row
	    *dest++ = *src;
	    src+=2;
	 }
	 src += srcbuf->format.size.width; // Skip 'red' rows
      }
      break;
   default:
      break;
   }

}
