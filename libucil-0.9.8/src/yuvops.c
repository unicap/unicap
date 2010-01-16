/*
** yuvops.c
** 
** Made by (Arne Caspari)
** Login   <arne@arne-laptop>
** 
** Started on  Mon Oct 30 07:20:28 2006 Arne Caspari
** Last update Wed Jan  3 18:31:42 2007 Arne Caspari
*/

#include "yuvops.h"
#include "unicap.h"
#include "ucil.h"

#include <sys/types.h>
#include <linux/types.h>



#define CLIP(v,l,h) (((v)<(l))?(l):(v)>(h)?(h):(v))



void ucil_blend_uyvy_a( unicap_data_buffer_t *dest, 
			unicap_data_buffer_t *img1, 
			unicap_data_buffer_t *img2, 
			int alpha )
{
   int offset = 0;
   int af = alpha;
   int ab = ( 1 << 16 ) - alpha;
   
   while( offset < img1->format.buffer_size )
   {
      int bu  = ( ( ((int)img1->data[offset])-127 ) * ab ) >> 16;
      int by1 = ( ((int)img1->data[offset+1]) * ab ) >> 16;
      int bv  = ( ( ((int)img1->data[offset+2])-127 ) * ab ) >> 16;
      int by2 = ( ((int)img1->data[offset+3]) * ab ) >> 16;

      int fu  = ( ( ((int)img2->data[offset])-127 ) * af ) >> 16;
      int fy1 = ( ((int)img2->data[offset+1]) * af ) >> 16;
      int fv  = ( ( ((int)img2->data[offset+2])-127 ) * af ) >> 16;
      int fy2 = ( ((int)img2->data[offset+3]) * af ) >> 16;
      
      dest->data[offset++]  = ( CLIP( ( bu + fu ), -127, 128 ) + 127 );
      dest->data[offset++] = CLIP( ( by1 + fy1 ), 0, 255 );
      dest->data[offset++]  = ( CLIP( ( bv + fv ), -127, 128 ) + 127 );
      dest->data[offset++] = CLIP( ( by2 + fy2 ), 0, 255 );      
   }
}

void ucil_composite_UYVY_YUVA( unicap_data_buffer_t *dest, 
			       unicap_data_buffer_t *img, 
			       int xpos, 
			       int ypos, 
			       double scalex, 
			       double scaley, 
			       ucil_interpolation_type_t interp )
{
   double w, h;

   if( ( scalex <= 0.0 ) || ( scaley <= 0.0 ) )
   {
      return;
   }
   
   w = img->format.size.width;
   h = img->format.size.height;
	 

   if( ( scalex == 1.0 ) && ( scaley == 1.0 ) )
   {
      int i,j;
      if( ( xpos + img->format.size.width ) > ( dest->format.size.width ) )
      {
	 w = img->format.size.width - ( ( xpos + img->format.size.width ) - dest->format.size.width );
      }
      
      if( ( ypos + img->format.size.height ) > ( dest->format.size.height ) )
      {
	 h = img->format.size.height - ( ( ypos + img->format.size.height ) - ( dest->format.size.height ) );
      }

      w /= 2.0;

      for( j = 0; j < h; j++ )
      {
	 int boff = 0;
	 int ioff = 0;
	 int doff = 0;

	 boff = doff = ( ypos + j ) * dest->format.size.width * 2 + xpos * 2;
	 ioff = j * img->format.size.width * 4;
	 
	 for( i = 0; i < w; i++ )
	 {
	    
	    int y, u, v, a, xa;
	    
	    int bu  = dest->data[boff++];
	    int by1 = dest->data[boff++];
	    int bv  = dest->data[boff++];
	    int by2 = dest->data[boff++];
	    
	    y  = img->data[ioff++];
	    u  = img->data[ioff];
	    ioff += 2;
	    a  = img->data[ioff++];
	    xa = 255 - a;
	    
	    dest->data[doff++] = ( bu * xa + u * a ) >> 8;
	    dest->data[doff++] = ( by1 * xa + y * a ) >> 8;
	    
	    y  = img->data[ioff];
	    ioff += 2;
	    v  = img->data[ioff++];
	    a  = img->data[ioff++];
	    xa = 255 - a;
	    
	    dest->data[doff++] = ( bv * xa + v * a ) >> 8;
	    dest->data[doff++] = ( by2 * xa + y * a ) >> 8;
	 }
      }
   }
   else
   {
      double xstep, ystep;
      double i, j;
      int k, l;
      
      xstep = 1 / scalex;
      ystep = 1 / scaley;

      for( j = 0, l = ypos; 
	   ( j < h ) && ( l < dest->format.size.height ); 
	   j += ystep, l++ )
      {
	 int tmpoff = 0;
	 int doff = 0;

	 doff = ( l ) * dest->format.size.width * 2 + xpos * 2;
	 tmpoff = (int)j * ( img->format.size.width * 4 );
	 
	 for( i = 0, k = 0; 
	      ( i < w ) && ( k < dest->format.size.width ) ; 
	      i += xstep, k++ )
	 {
	    
	    int y, u, v, a, xa;
	    int ioff;
	    
	    int bu  = dest->data[doff];
	    int by1 = dest->data[doff+1];
	    int bv  = dest->data[doff+2];
	    int by2 = dest->data[doff+3];

	    ioff = tmpoff + (int)i * 4;
	    
	    y  = img->data[ioff];
	    u  = img->data[ioff+1];
	    a  = img->data[ioff+3];
	    xa = 255 - a;
	    
	    i += xstep;
	    ioff = tmpoff + (int)i * 4;

	    dest->data[doff++] = ( bu * xa + u * a ) >> 8;
	    dest->data[doff++] = ( by1 * xa + y * a ) >> 8;
	    
	    y  = img->data[ioff];
	    v  = img->data[ioff+2];
	    a  = img->data[ioff+3];
	    xa = 255 - a;
	    
	    dest->data[doff++] = ( bv * xa + v * a ) >> 8;
	    dest->data[doff++] = ( by2 * xa + y * a ) >> 8;
	 }
      }
   }
}

void ucil_composite_YUYV_YUVA( unicap_data_buffer_t *dest, 
			       unicap_data_buffer_t *img, 
			       int xpos, 
			       int ypos, 
			       double scalex, 
			       double scaley, 
			       ucil_interpolation_type_t interp )
{
   double w, h;

   if( ( scalex <= 0.0 ) || ( scaley <= 0.0 ) )
   {
      return;
   }
   
   w = img->format.size.width;
   h = img->format.size.height;
	 

   if( ( scalex == 1.0 ) && ( scaley == 1.0 ) )
   {
      int i,j;
      if( ( xpos + img->format.size.width ) > ( dest->format.size.width ) )
      {
	 w = img->format.size.width - ( ( xpos + img->format.size.width ) - dest->format.size.width );
      }
      
      if( ( ypos + img->format.size.height ) > ( dest->format.size.height ) )
      {
	 h = img->format.size.height - ( ( ypos + img->format.size.height ) - ( dest->format.size.height ) );
      }

      w /= 2.0;

      for( j = 0; j < h; j++ )
      {
	 int boff = 0;
	 int ioff = 0;
	 int doff = 0;

	 boff = doff = ( ypos + j ) * dest->format.size.width * 2 + xpos * 2;
	 ioff = j * img->format.size.width * 4;
	 
	 for( i = 0; i < w; i++ )
	 {
	    
	    int y, u, v, a, xa;
	    
	    int by1 = dest->data[boff++];
	    int bu  = dest->data[boff++];
	    int by2 = dest->data[boff++];
	    int bv  = dest->data[boff++];
	    
	    y  = img->data[ioff++];
	    u  = img->data[ioff];
	    ioff += 2;
	    a  = img->data[ioff++];
	    xa = 255 - a;
	    
	    dest->data[doff++] = ( by1 * xa + y * a ) >> 8;
	    dest->data[doff++] = ( bu * xa + u * a ) >> 8;
	    
	    y  = img->data[ioff];
	    ioff += 2;
	    v  = img->data[ioff++];
	    a  = img->data[ioff++];
	    xa = 255 - a;
	    
	    dest->data[doff++] = ( by2 * xa + y * a ) >> 8;
	    dest->data[doff++] = ( bv * xa + v * a ) >> 8;
	 }
      }
   }
   else
   {
      double xstep, ystep;
      double i, j;
      int k, l;
      
      xstep = 1 / scalex;
      ystep = 1 / scaley;

      for( j = 0, l = ypos; 
	   ( j < h ) && ( l < dest->format.size.height ); 
	   j += ystep, l++ )
      {
	 int tmpoff = 0;
	 int doff = 0;

	 doff = ( l ) * dest->format.size.width * 2 + xpos * 2;
	 tmpoff = (int)j * ( img->format.size.width * 4 );
	 
	 for( i = 0, k = 0; 
	      ( i < w ) && ( k < dest->format.size.width ) ; 
	      i += xstep, k++ )
	 {
	    
	    int y, u, v, a, xa;
	    int ioff;
	    
	    int bu  = dest->data[doff];
	    int by1 = dest->data[doff+1];
	    int bv  = dest->data[doff+2];
	    int by2 = dest->data[doff+3];

	    ioff = tmpoff + (int)i * 4;

	    y  = img->data[ioff];
	    u  = img->data[ioff+1];
	    a  = img->data[ioff+3];
	    xa = 255 - a;
	    
	    i += xstep;
	    ioff = tmpoff + (int)i * 4;

	    dest->data[doff++] = ( bu * xa + u * a ) >> 8;
	    dest->data[doff++] = ( by1 * xa + y * a ) >> 8;
	    
	    y  = img->data[ioff];
	    v  = img->data[ioff+2];
	    a  = img->data[ioff+3];
	    xa = 255 - a;
	    
	    dest->data[doff++] = ( bv * xa + v * a ) >> 8;
	    dest->data[doff++] = ( by2 * xa + y * a ) >> 8;
	 }
      }
   }
}


void ucil_fill_uyvy( unicap_data_buffer_t *buffer, 
		     ucil_color_t *color )
{
   int offset;

   __u32 word;
   
   word = ( color->yuv.y << 24 ) | (color->yuv.v << 16 ) | ( color->yuv.y << 8 ) | ( color->yuv.u );
   
   for( offset = 0; offset < ( buffer->format.buffer_size / 4 ); offset++ )
   {
      *(((__u32 *)buffer->data) + offset ) = word;
   }
}

void ucil_convolution_mask_uyvy( unicap_data_buffer_t *dest, 
				 unicap_data_buffer_t *src, 
				 ucil_convolution_mask_t *mask )
{
   int ix, iy;
   int mx, my;
   int mmoff;
   
   static int printed = 0;
   
   mmoff = ( ( (float)mask->size + 0.5 ) / 2 ) -1;

   for( iy = 1; iy < ( src->format.size.height - 1 ); iy++ )
   {
      for( ix = 1; ix < ( src->format.size.width - 1 ); ix++ )
      {
	 int val = 0;
	 int stride = src->format.size.width * 2;
	 int iyoff = iy * stride;
	 int ixoff = ix * 2;
	 
	 for( my = 0; my < mask->size; my++ )
	 {

	    for( mx = 0; mx < mask->size; mx++ )
	    {
	       int p, m;
	       int offset;
/* 	       if( !printed ) */
/* 	       { */
/* 		  printf( "%d ", mask->mask[ my * mask->size + mx ].yuv.y ); */
/* 	       } */
	       
	       offset = iyoff + my * stride + ixoff + mx * 2;
	       m = (signed char)mask->mask[ my * mask->size + mx ].yuv.y;
	       p = src->data[ offset + 1];	       
	       
	       val += ( p * m );

/* 	       if( !printed ) */
/* 	       { */
/* 		  printf( "[%d,%d] ", p,val ); */
/* 	       } */

	    }
/* 	    if( !printed ) */
/* 	    { */
/* 	       printf( "\n" ); */
/* 	    } */
	 }	 

	 dest->data[ iyoff + mmoff * stride + ixoff + mmoff * 2 ] = src->data[ iyoff + mmoff * stride + ixoff + mmoff * 2 ];
	 dest->data[ iyoff + mmoff * stride + ixoff + mmoff * 2 + 1] = CLIP( val, 0, 255 );
	 
	 printed = 1;
	 
      }
   }
}
