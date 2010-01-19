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

#include "config.h"

#include <unicap.h>
#ifdef __SSE2__
#include <emmintrin.h>
#endif
#include "debayer.h"

union _rgb24pixel
{
      struct
      {
	    unsigned char r;
	    unsigned char g;
	    unsigned char b;
      }c;
      
      unsigned int combined:24;
}__attribute__((packed));

typedef union _rgb24pixel rgb24pixel_t;


#define YUV_Y(r,g,b) ( ( (  66 * r + 129 * g +  25 * b + 128) >> 8) +  16 )
#define YUV_U(r,g,b) ( ( ( -38 * r -  74 * g + 112 * b + 128) >> 8) + 128 )
#define YUV_V(r,g,b) ( ( ( 112 * r -  94 * g -  18 * b + 128) >> 8) + 128 )

#define CRR(x) (x[0][0])
#define CRG(x) (x[0][1])
#define CRB(x) (x[0][2])
#define CGR(x) (x[1][0])
#define CGG(x) (x[1][1])
#define CGB(x) (x[1][2])
#define CBR(x) (x[2][0])
#define CBG(x) (x[2][1])
#define CBB(x) (x[2][2])

#define CLIP(x) (x>255?255:x<0?0:x)
#define ABS(x) ((x)<0?(-1*x):(x))


void debayer_calculate_rbgain( unicap_data_buffer_t *buffer, int *rgain, int *bgain, int *combined )
{
   int x,y;
   int stepx, stepy;
   int rval = 0, gval = 0, bval = 0;

   stepx = ( buffer->format.size.width / 64 ) & ~1;
   stepy = ( buffer->format.size.height / 64 ) & ~1;   

   for( y = 0; y < buffer->format.size.height; y += stepy )
   {
      for( x = 0; x < buffer->format.size.width; x+= stepx )
      {
	 gval += buffer->data[ y * buffer->format.size.width + x ];
	 bval += buffer->data[ y * buffer->format.size.width + x + 1 ];
	 rval += buffer->data[ (y+1) * buffer->format.size.width + x  ];
      }
   }
   
   *rgain = (int)( ((double)gval  * 4096.0) / (double)rval );
   *bgain = (int)( ((double)gval  * 4096.0) / (double)bval );

   *combined = rval + gval + bval;
}


void debayer_ccm_rgb24_nn( unicap_data_buffer_t *destbuf, unicap_data_buffer_t *srcbuf, debayer_data_t *data )
{
   int i, j;
   int dest_offset = 0;
   unsigned char *dest = destbuf->data;
   unsigned char *source = srcbuf->data;
   int width = srcbuf->format.size.width;
   int height = srcbuf->format.size.height;   
   int rgain, bgain;

   if( data->use_rbgain )
   {
      rgain = data->rgain;
      bgain = data->bgain;
   }
   else
   {
      rgain = 4096;
      bgain = 4096;
   }

   for( j = 1; j < height-1; j+=2 )
   {
      int lineoffset = j*width;

      //RGRGR
      //GBGBG
      //RGRGR

      for( i = 0; i < width -1;  )
      {
	 
	 
	 *dest++ = CLIP(( (unsigned int)source[ lineoffset + i ] * rgain ) / 4096);
	 *dest++ = ( (unsigned int)source[ lineoffset + i + width ] + (unsigned int)source[ lineoffset + i + 1]) / 2;
	 *dest++ = CLIP(( (unsigned int)source[ lineoffset + i + width + 1] * bgain ) / 4096);

	 i++;
	 
	 *dest++ = CLIP(( (unsigned int)source[ lineoffset + i + 1 ] *rgain ) / 4096);
	 *dest++ = ( (unsigned int)source[ lineoffset + i + width + 1 ] + (int)source[ lineoffset + i ]) / 2;
	 *dest++ = CLIP(( (unsigned int)source[ lineoffset + i + width ] * bgain ) / 4096);
	 
	 i++;
      }

      lineoffset = (j+1)*width;

      //GBGBG
      //RGRGR
      //GBGBG

      for( i = 0; i < width -1;  )
      {
	 
	 *dest++ = CLIP(( (unsigned int)source[ lineoffset + i + width ] * rgain ) / 4096);
	 *dest++ = ( (unsigned int)source[ lineoffset + i + width + 1 ] + (int)source[ lineoffset + i ]) / 2;
	 *dest++ = CLIP(( (unsigned int)source[ lineoffset + i + 1 ] * bgain ) / 4096);	 

	 i++;

	 *dest++ = CLIP(( (unsigned int)source[ lineoffset + i + width + 1 ] * rgain ) / 4096);
	 *dest++ = ( (unsigned int)source[ lineoffset + i + width ] + (int)source[ lineoffset + i + 1 ]) / 2;
	 *dest++ = CLIP(( (unsigned int)source[ lineoffset + i ] * bgain ) / 4096);
	 

	 i++;
      }
   }
}

void debayer_ccm_rgb24_nn_be( unicap_data_buffer_t *destbuf, unicap_data_buffer_t *srcbuf, debayer_data_t *data )
{
   int i, j;
   int dest_offset = 0;
   unsigned char *dest = destbuf->data;
   unsigned char *source = srcbuf->data;
   int width = srcbuf->format.size.width;
   int height = srcbuf->format.size.height;   
   int rgain, bgain;

   static int odd = 0;

   if( data->use_rbgain )
   {
      rgain = data->rgain;
      bgain = data->bgain;
   }
   else
   {
      rgain = 4096;
      bgain = 4096;
   }

/*    if( !odd ){ */
/*    for( i = 0; i < ( width* height ) - 2; i+=2 ){ */
/*       *(unsigned short *)(source+i) = ntohs( *(unsigned short *)(source+i) ); */
/*    } */
/*       odd = 1; */
/*       printf( "odd\n" ); */
/*    }else{ */
/*       odd = 0; */
/*    } */

   for( j = 1; j < height-1; j+=2 )
   {
      int lineoffset = j*width;

      //RGRGR
      //GBGBG
      //RGRGR

      for( i = 0; i < width -1;  )
      {
	 
	 
	 *dest++ = CLIP(( (unsigned int)source[ lineoffset + i ] * rgain ) / 4096);
	 *dest++ = ( (unsigned int)source[ lineoffset + i + width ] + (unsigned int)source[ lineoffset + i + 1]) / 2;
	 *dest++ = CLIP(( (unsigned int)source[ lineoffset + i + width + 1] * bgain ) / 4096);

	 i++;
	 
	 *dest++ = CLIP(( (unsigned int)source[ lineoffset + i + 1 ] *rgain ) / 4096);
	 *dest++ = ( (unsigned int)source[ lineoffset + i + width + 1 ] + (int)source[ lineoffset + i ]) / 2;
	 *dest++ = CLIP(( (unsigned int)source[ lineoffset + i + width ] * bgain ) / 4096);
	 
	 i++;
      }

      lineoffset = (j+1)*width;

      //GBGBG
      //RGRGR
      //GBGBG

      for( i = 0; i < width -1;  )
      {
	 
	 *dest++ = CLIP(( (unsigned int)source[ lineoffset + i + width ] * rgain ) / 4096);
	 *dest++ = ( (unsigned int)source[ lineoffset + i + width + 1 ] + (int)source[ lineoffset + i ]) / 2;
	 *dest++ = CLIP(( (unsigned int)source[ lineoffset + i + 1 ] * bgain ) / 4096);	 

	 i++;

	 *dest++ = CLIP(( (unsigned int)source[ lineoffset + i + width + 1 ] * rgain ) / 4096);
	 *dest++ = ( (unsigned int)source[ lineoffset + i + width ] + (int)source[ lineoffset + i + 1 ]) / 2;
	 *dest++ = CLIP(( (unsigned int)source[ lineoffset + i ] * bgain ) / 4096);
	 

	 i++;
      }
   }
}

void debayer_ccm_rgb24_gr_nn( unicap_data_buffer_t *destbuf, unicap_data_buffer_t *srcbuf, debayer_data_t *data )
{
   int i, j;
   int dest_offset = 0;
   unsigned char *dest = destbuf->data;
   unsigned char *source = srcbuf->data;
   int width = srcbuf->format.size.width;
   int height = srcbuf->format.size.height;   
   int rgain, bgain;

   if( data->use_rbgain )
   {
      rgain = data->rgain;
      bgain = data->bgain;
   }
   else
   {
      rgain = 4096;
      bgain = 4096;
   }

   for( j = 1; j < height-1; j+=2 )
   {
      int lineoffset = j*width;

      //RGRGR
      //GBGBG
      //RGRGR

      for( i = 0; i < width -1;  )
      {
	 unsigned char r,b;
	 b = CLIP(( (unsigned int)source[ lineoffset + i ] * bgain ) / 4096);
	 r = CLIP(( (unsigned int)source[ lineoffset + i + width + 1] * rgain ) / 4096);
	 
	 *dest++ = r;
	 *dest++ = ( (unsigned int)source[ lineoffset + i + width ] + (unsigned int)source[ lineoffset + i + 1]) / 2;
	 *dest++ = b;

	 i++;
	 
	 *dest++ = r;
	 *dest++ = ( (unsigned int)source[ lineoffset + i + width + 1 ] + (int)source[ lineoffset + i ]) / 2;
	 *dest++ = b;
	 
	 i++;
      }

      lineoffset = (j+1)*width;

      //GBGBG
      //RGRGR
      //GBGBG

      for( i = 0; i < width -1;  )
      {
	 unsigned char r,b;
	 b = CLIP(( (unsigned int)source[ lineoffset + i + width ] * bgain ) / 4096);
	 r = CLIP(( (unsigned int)source[ lineoffset + i + 1 ] * rgain ) / 4096);

	 *dest++ = r;
	 *dest++ = ( (unsigned int)source[ lineoffset + i + width + 1 ] + (int)source[ lineoffset + i ]) / 2;
	 *dest++ = b;

	 i++;

	 *dest++ = r;
	 *dest++ = ( (unsigned int)source[ lineoffset + i + width ] + (int)source[ lineoffset + i + 1 ]) / 2;
	 *dest++ = b;
	 

	 i++;
      }
   }
}

void debayer_ccm_rgb24_edge( unicap_data_buffer_t *destbuf, unicap_data_buffer_t *srcbuf, debayer_data_t *data )
{
   int i,j;
   int dest_offset = 0;   
   rgb24pixel_t *dest = (rgb24pixel_t*)destbuf->data;
   unsigned char *source = srcbuf->data;
   int width = destbuf->format.size.width;
   int height = destbuf->format.size.height;
   
   for( j = 2; j < height - 2; j+=2 )
   {
      int lineoffset = j * width;
      dest_offset = j * width + 1;

      for( i = 1; i < width - 2; )
      {
	 rgb24pixel_t pixel;
	 
	 int g1 = source[ lineoffset + i - 1 ];
	 int g2 = source[ lineoffset + i + 1 ];
	 int g3 = source[ ( lineoffset + i ) - width ];
	 int g4 = source[ lineoffset + i + width ];
	 
	 int d1 = ABS( g1 - g2 );
	 int d2 = ABS( g3 - g4 );

	 pixel.c.b = source[ lineoffset + i ];
	 pixel.c.r = ( ( (int)source[ ( ( lineoffset + i ) - width ) + 1 ] + 
			 (int)source[ ( lineoffset + i + width ) + 1 ] + 
			 (int)source[ ( ( lineoffset + i ) - width ) - 1 ] + 
			 (int)source[ ( lineoffset + i + width ) - 1 ] ) / 4 );
	 
	 if( d1 < d2 )
	 {
	    pixel.c.g = ( g1 + g2 ) / 2;
	 }
	 else
	 {
	    pixel.c.g = ( g3 + g4 ) / 2;
	 }

	 if( data->use_ccm )
	 {
	    pixel.c.r = CLIP( (pixel.c.r * CRR(data->ccm) + pixel.c.g * CRG(data->ccm) + pixel.c.b * CRB(data->ccm))/1024 );
	    pixel.c.g = CLIP( (pixel.c.r * CGR(data->ccm) + pixel.c.g * CGG(data->ccm) + pixel.c.b * CGB(data->ccm))/1024 );
	    pixel.c.b = CLIP( (pixel.c.r * CBR(data->ccm) + pixel.c.g * CBG(data->ccm) + pixel.c.b * CBB(data->ccm))/1024 );
	 }
	 else if( data->use_rbgain )
	 {
	    pixel.c.r = CLIP(( pixel.c.r * data->rgain ) / 4096);
	    pixel.c.b = CLIP(( pixel.c.b * data->bgain ) / 4096);
	 }
	 
	 
	 dest[dest_offset++].combined = pixel.combined;
	 
	 i++;
	 
	 pixel.c.g = g2;
	 pixel.c.b = ( (int)source[ ( lineoffset + i ) - 1 ] + (int)source[ ( lineoffset + i ) + 1 ] ) / 2;
	 pixel.c.r = ( (int)source[ ( lineoffset + i ) - width ] + (int)source[ lineoffset + i + width ] ) / 2;

	 if( data->use_ccm )
	 {
	    pixel.c.r = CLIP( (pixel.c.r * CRR(data->ccm) + pixel.c.g * CRG(data->ccm) + pixel.c.b * CRB(data->ccm))/1024 );
	    pixel.c.g = CLIP( (pixel.c.r * CGR(data->ccm) + pixel.c.g * CGG(data->ccm) + pixel.c.b * CGB(data->ccm))/1024 );
	    pixel.c.b = CLIP( (pixel.c.r * CBR(data->ccm) + pixel.c.g * CBG(data->ccm) + pixel.c.b * CBB(data->ccm))/1024 );
	 }
	 else if( data->use_rbgain )
	 {
	    pixel.c.r = CLIP(( pixel.c.r * data->rgain ) / 4096);
	    pixel.c.b = CLIP(( pixel.c.b * data->bgain ) / 4096);
	 }

	 dest[dest_offset++].combined = pixel.combined;
	 
	 i++;
      }
    
      lineoffset += width;
      dest_offset = (j+1) * width + 2;
      
      for( i = 2; i < width - 3; )
      {
	 rgb24pixel_t pixel;
	 int g1 = source[ lineoffset + i - 1 ];
	 int g2 = source[ lineoffset + i + 1 ];
	 int g3 = source[ ( lineoffset + i ) - width ];
	 int g4 = source[ lineoffset + i + width ];
	 
	 int d1 = ABS( g1 - g2 );
	 int d2 = ABS( g3 - g4 );

	 pixel.c.r = source[ lineoffset + i ];

	 if( d1 < d2 )
	 {
	    pixel.c.g = ( g1 + g2 ) / 2;
	 }	    
	 else
	 {
	    pixel.c.g = ( g3 + g4 ) / 2;
	 }

	 pixel.c.b = ( ( (int)source[ ( ( lineoffset + i ) - width ) + 1 ] + 
		       (int)source[ ( lineoffset + i + width ) + 1 ] + 
		       (int)source[ ( ( lineoffset + i ) - width ) - 1 ] + 
		       (int)source[ ( lineoffset + i + width ) - 1 ] ) / 4 );
	 
	 if( data->use_ccm )
	 {
	    pixel.c.r = CLIP( (pixel.c.r * CRR(data->ccm) + pixel.c.g * CRG(data->ccm) + pixel.c.b * CRB(data->ccm))/1024 );
	    pixel.c.g = CLIP( (pixel.c.r * CGR(data->ccm) + pixel.c.g * CGG(data->ccm) + pixel.c.b * CGB(data->ccm))/1024 );
	    pixel.c.b = CLIP( (pixel.c.r * CBR(data->ccm) + pixel.c.g * CBG(data->ccm) + pixel.c.b * CBB(data->ccm))/1024 );
	 }
	 else if( data->use_rbgain )
	 {
	    pixel.c.r = CLIP(( pixel.c.r * data->rgain ) / 4096);
	    pixel.c.b = CLIP(( pixel.c.b * data->bgain ) / 4096);
	 }

	 dest[dest_offset++].combined = pixel.combined;

	 i++;
	 
	 pixel.c.g = g2;
	 pixel.c.r = ( (int)source[ ( lineoffset + i ) - 1 ] + (int)source[ ( lineoffset + i ) + 1 ] ) / 2;
	 pixel.c.b = ( (int)source[ ( lineoffset + i ) - width ] + (int)source[ lineoffset + i + width ] ) / 2;
	 
	 if( data->use_ccm )
	 {
	    pixel.c.r = CLIP( (pixel.c.r * CRR(data->ccm) + pixel.c.g * CRG(data->ccm) + pixel.c.b * CRB(data->ccm))/1024 );
	    pixel.c.g = CLIP( (pixel.c.r * CGR(data->ccm) + pixel.c.g * CGG(data->ccm) + pixel.c.b * CGB(data->ccm))/1024 );
	    pixel.c.b = CLIP( (pixel.c.r * CBR(data->ccm) + pixel.c.g * CBG(data->ccm) + pixel.c.b * CBB(data->ccm))/1024 );
	 }
	 else if( data->use_rbgain )
	 {
	    pixel.c.r = CLIP(( pixel.c.r * data->rgain ) / 4096);
	    pixel.c.b = CLIP(( pixel.c.b * data->bgain ) / 4096);
	 }

	 dest[dest_offset++].combined = pixel.combined;

	 i++; 
      }
   }
   
}



void debayer_ccm_uyvy( unicap_data_buffer_t *destbuf, unicap_data_buffer_t *srcbuf, debayer_data_t *data )
{
   int i,j;
   int dest_offset = 0;   
   unsigned char *dest = destbuf->data;
   unsigned char *source = srcbuf->data;
   int width = destbuf->format.size.width;
   int height = destbuf->format.size.height;
   
   for( j = 2; j < height - 2; j+=2 )
   {
      int lineoffset = j * width;
      dest_offset = j * width * 2 + 2;

      for( i = 1; i < width - 2; )
      {
	 int r, g, b;
	 int g1 = source[ lineoffset + i - 1 ];
	 int g2 = source[ lineoffset + i + 1 ];
	 int g3 = source[ ( lineoffset + i ) - width ];
	 int g4 = source[ lineoffset + i + width ];
	 
	 int d1 = ABS( g1 - g2 );
	 int d2 = ABS( g3 - g4 );

	 b = source[ lineoffset + i ];
	 r = ( ( (int)source[ ( ( lineoffset + i ) - width ) + 1 ] + 
		 (int)source[ ( lineoffset + i + width ) + 1 ] + 
		 (int)source[ ( ( lineoffset + i ) - width ) - 1 ] + 
		 (int)source[ ( lineoffset + i + width ) - 1 ] ) / 4 );
	 
	 if( d1 < d2 )
	 {
	    g = ( g1 + g2 ) / 2;
	 }
	 else
	 {
	    g = ( g3 + g4 ) / 2;
	 }

	 if( data->use_ccm )
	 {
	    r = CLIP( (r * CRR(data->ccm) + g * CRG(data->ccm) + b * CRB(data->ccm))/1024 );
	    g = CLIP( (r * CGR(data->ccm) + g * CGG(data->ccm) + b * CGB(data->ccm))/1024 );
	    b = CLIP( (r * CBR(data->ccm) + g * CBG(data->ccm) + b * CBB(data->ccm))/1024 );
	 }
	 else if( data->use_rbgain )
	 {
	    r = CLIP(( r * data->rgain ) / 1024);
	    b = CLIP(( b * data->bgain ) / 1024);
	 }

	 dest[dest_offset++] = YUV_V(r,g,b);	 
	 dest[dest_offset++] = YUV_Y(r,g,b);
	 
	 i++;
	 
	 g = g2;
	 b = ( (int)source[ ( lineoffset + i ) - 1 ] + (int)source[ ( lineoffset + i ) + 1 ] ) / 2;
	 r = ( (int)source[ ( lineoffset + i ) - width ] + (int)source[ lineoffset + i + width ] ) / 2;

	 if( data->use_ccm )
	 {
	    r = CLIP( (r * CRR(data->ccm) + g * CRG(data->ccm) + b * CRB(data->ccm))/1024 );
	    g = CLIP( (r * CGR(data->ccm) + g * CGG(data->ccm) + b * CGB(data->ccm))/1024 );
	    b = CLIP( (r * CBR(data->ccm) + g * CBG(data->ccm) + b * CBB(data->ccm))/1024 );
	 }
	 else if( data->use_rbgain )
	 {
	    r = CLIP(( r * data->rgain ) / 1024);
	    b = CLIP(( b * data->bgain ) / 1024);
	 }

	 dest[dest_offset++] = YUV_U(r,g,b);
	 dest[dest_offset++] = YUV_Y(r,g,b);
	 
	 i++;
      }
    
      lineoffset += width;
      dest_offset = (j+1) * width * 2 + 4;
      
      for( i = 2; i < width - 3; )
      {
	 int r, g, b;
	 int g1 = source[ lineoffset + i - 1 ];
	 int g2 = source[ lineoffset + i + 1 ];
	 int g3 = source[ ( lineoffset + i ) - width ];
	 int g4 = source[ lineoffset + i + width ];
	 
	 int d1 = ABS( g1 - g2 );
	 int d2 = ABS( g3 - g4 );

	 r = source[ lineoffset + i ];

	 if( d1 < d2 )
	 {
	    g = ( g1 + g2 ) / 2;
	 }
	    
	 else
	 {
	    g = ( g3 + g4 ) / 2;
	 }

	 b = ( ( (int)source[ ( ( lineoffset + i ) - width ) + 1 ] + 
		 (int)source[ ( lineoffset + i + width ) + 1 ] + 
		 (int)source[ ( ( lineoffset + i ) - width ) - 1 ] + 
		 (int)source[ ( lineoffset + i + width ) - 1 ] ) / 4 );
	 

	 if( data->use_ccm )
	 {
	    r = CLIP( (r * CRR(data->ccm) + g * CRG(data->ccm) + b * CRB(data->ccm))/1024 );
	    g = CLIP( (r * CGR(data->ccm) + g * CGG(data->ccm) + b * CGB(data->ccm))/1024 );
	    b = CLIP( (r * CBR(data->ccm) + g * CBG(data->ccm) + b * CBB(data->ccm))/1024 );
	 }
	 else if( data->use_rbgain )
	 {
	    r = CLIP(( r * data->rgain ) / 1024);
	    b = CLIP(( b * data->bgain ) / 1024);
	 }

	 dest[dest_offset++] = YUV_U(r,g,b);
	 dest[dest_offset++] = YUV_Y(r,g,b);

	 i++;
	 
	 g = g2;
	 r = ( (int)source[ ( lineoffset + i ) - 1 ] + (int)source[ ( lineoffset + i ) + 1 ] ) / 2;
	 b = ( (int)source[ ( lineoffset + i ) - width ] + (int)source[ lineoffset + i + width ] ) / 2;
	 
	 if( data->use_ccm )
	 {
	    r = CLIP( (r * CRR(data->ccm) + g * CRG(data->ccm) + b * CRB(data->ccm))/1024 );
	    g = CLIP( (r * CGR(data->ccm) + g * CGG(data->ccm) + b * CGB(data->ccm))/1024 );
	    b = CLIP( (r * CBR(data->ccm) + g * CBG(data->ccm) + b * CBB(data->ccm))/1024 );
	 }
	 else if( data->use_rbgain )
	 {
	    r = CLIP(( r * data->rgain ) / 1024);
	    b = CLIP(( b * data->bgain ) / 1024);
	 }

	 dest[dest_offset++] = YUV_V(r,g,b);	 
	 dest[dest_offset++] = YUV_Y(r,g,b);

	 i++; 
      }
   }   
}

#ifdef __SSE2__

typedef union
{
   __m128i m128i;
   unsigned char i8[16];
}m128iu;

void debayer_sse2( unicap_data_buffer_t *destbuf, unicap_data_buffer_t* srcbuf, debayer_data_t *data )
{
   unsigned char *dest = destbuf->data;
   unsigned char *source = srcbuf->data;
   int width = srcbuf->format.size.width;
   int height = srcbuf->format.size.height;
   int i,j;
   int dest_offset = 0;      
   int imgwidth = width;

   for( j = 2; j < (height); j += 2 )
   {
      int lineoffset = j * imgwidth;
      dest_offset = (j) * width * 3;


      // RGRGRGRGRGRG 
      // GBGBGBGBGBGB <<-
      // RGRGRGRGRGRG 
      for( i = 0; i < (width ); i += 16)
      {
	 int k;
	 m128iu b1, b2, r1, r2;
	 m128iu avg1, avg2, avg3;

	 r1.m128i = _mm_loadu_si128( (__m128i*)(source + lineoffset - imgwidth + i) );
	 r2.m128i = _mm_loadu_si128( (__m128i*)(source + lineoffset + imgwidth + i) );
	 // r1 = RG
	 avg1.m128i = _mm_avg_epu8( b1.m128i, b2.m128i );
	 // avg1 = RG'
	 b1.m128i = _mm_loadu_si128( (__m128i*)(source + lineoffset + i - 1) );
	 b2.m128i = _mm_loadu_si128( (__m128i*)(source + lineoffset + i + 1) );
	 // b1 = BG
	 avg2.m128i = _mm_avg_epu8( b1.m128i, b2.m128i );
	 // avg2 = B'G'
	 avg3.m128i = _mm_avg_epu8( avg1.m128i, avg2.m128i );
	 // avg3 = XG
	 
	 for( k = 0; k < 16; k+=2 ){
	    dest[dest_offset++] = r1.i8[k];
	    dest[dest_offset++] = b1.i8[k+1];
	    dest[dest_offset++] = avg2.i8[k];

	    dest[dest_offset++] = r1.i8[k];
	    dest[dest_offset++] = avg2.i8[k+1];
	    dest[dest_offset++] = b2.i8[k];	 
	 }
      }
      
      lineoffset += imgwidth;
      dest_offset = ( j + 1 ) * width * 3;
      
      // GBGBGBGBGBGB 
      // RGRGRGRGRGRG <<-
      // GBGBGBGBGBGB 
      for( i = 0; i < (width); i += 16)
      {
	 m128iu b1, b2, r1, r2;
	 m128iu avg1, avg2, avg3;
	 int k;

	 b1.m128i = _mm_loadu_si128( (__m128i*)(source + lineoffset - imgwidth + i) );
	 b2.m128i = _mm_loadu_si128( (__m128i*)(source + lineoffset + imgwidth + i) );
	 avg1.m128i = _mm_avg_epu8( b1.m128i, b2.m128i );
	 // avg1 = G'B
	 r1.m128i = _mm_loadu_si128( (__m128i*)(source + lineoffset + i - 1) );
	 r2.m128i = _mm_loadu_si128( (__m128i*)(source +lineoffset + i + 1 ) );
	 // r1 = GRGR
	 // r2 = GRGR
	 avg2.m128i = _mm_avg_epu8( r1.m128i, r2.m128i );
	 // avg2 = G'R'
	 avg3.m128i = _mm_avg_epu8( avg1.m128i, avg2.m128i );
	 // avg3 = GX

	 for( k = 0; k < 16; k += 2 ){
	    dest[dest_offset++] = r1.i8[k+1];
	    dest[dest_offset++] = avg3.i8[k];
	    dest[dest_offset++] = avg1.i8[k+1];

	    dest[dest_offset++] = avg2.i8[k+1];
	    dest[dest_offset++] = r2.i8[k];
	    dest[dest_offset++] = avg1.i8[k+1];
	 }
      }
   }
   
}
#endif
