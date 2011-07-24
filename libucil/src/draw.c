/*
** draw.c
** 
** Made by (Arne Caspari)
** Login   <arne@arne-laptop>
** 
** Started on  Mon Sep 18 18:14:12 2006 Arne Caspari
** Last update Tue Feb 13 16:03:58 2007 Arne Caspari
*/

#include "config.h"
#include "ucil.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "unicap.h"

#define FOURCC(a,b,c,d) (unsigned int)((((unsigned int)a))+(((unsigned int)b)<<8)+(((unsigned int)c)<<16)+(((unsigned int)d)<<24))
#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) (((a)<(b))?(b):(a))
#endif
#define CLIP(a,l,u) ((a)<(l)?(l):(((a)>(u)) ? (u) : (a) ) )
#define NORMALIZE(dt, a, b) if ((a)>(b)) { dt t; t = a; a = b; b = t; }
#define OOB(buf, x1, y1, x2, y2) ((x2)<0 || (x1)>=buf->format.size.width || \
                                  (y2)<0 || (y1)>=buf->format.size.height)

static __inline__ 
void draw_hline( unicap_data_buffer_t *data_buffer, ucil_color_t *color, unsigned int fill, int y, int x1, int x2 );


void ucil_convert_color( ucil_color_t *dest, ucil_color_t *src )
{
   int r,g,b,a,y,u,v;
   r=g=b=a=y=u=v = 0;

   switch( src->colorspace )
   {
      case UCIL_COLORSPACE_Y8:
	 r = g = b = y = src->y8.y;
	 u = v = 128;
	 a = 255;
	 break;
	 
      case UCIL_COLORSPACE_RGB24:
	 r = src->rgb24.r;
	 g = src->rgb24.g;
	 b = src->rgb24.b;
	 y = ( 0.2990 * (float)r + 0.5870 * (float)g + 0.1140 * (float)b );
	 u = (-0.1687 * (float)r - 0.3313 * (float)g + 0.5000 * (float)b + 128.0 );
	 v = ( 0.5000 * (float)r - 0.4187 * (float)g - 0.0813 * (float)b + 128.0);
	 break;
	 
      case UCIL_COLORSPACE_RGB32:
	 r = src->rgb32.r;
	 g = src->rgb32.g;
	 b = src->rgb32.b;
	 a = src->rgb32.a;
	 y = ( 0.2990 * (float)r + 0.5870 * (float)g + 0.1140 * (float)b );
	 u = (-0.1687 * (float)r - 0.3313 * (float)g + 0.5000 * (float)b + 128.0 );
	 v = ( 0.5000 * (float)r - 0.4187 * (float)g - 0.0813 * (float)b + 128.0);
	 break;
	 
      case UCIL_COLORSPACE_YUV:
	 y = src->yuv.y;
	 u = src->yuv.u;
	 v = src->yuv.v;
	 a = 255;
	 r = y - 0.0009267 * ( u - 128.0 ) + 1.4016868 * ( v - 128.0 );
	 g = y - 0.3436954 * ( u - 128.0 ) - 0.7141690 * ( v - 128.0 );
	 b = y + 1.7721604 * ( u - 128.0 ) + 0.0009902 * ( v - 128.0 );
	 r = CLIP(r,0,255);
	 g = CLIP(g,0,255);
	 b = CLIP(b,0,255);
	 break;

      default:
	 break;
   }

   switch( dest->colorspace )
   {
      case UCIL_COLORSPACE_Y8:
	 dest->y8.y = y;
	 break;
	 
      case UCIL_COLORSPACE_RGB24:
	 dest->rgb24.r = r;
	 dest->rgb24.g = g;
	 dest->rgb24.b = b;
	 break;
	 
      case UCIL_COLORSPACE_RGB32:
	 dest->rgb32.r = r;
	 dest->rgb32.g = g;
	 dest->rgb32.b = b;
	 dest->rgb32.a = a;
	 break;
	 
      case UCIL_COLORSPACE_YUV:
	 dest->yuv.y = (char)y;
	 dest->yuv.u = (char)u;
	 dest->yuv.v = (char)v;
	 break;

      default:
	 TRACE( "color format not supported for this operation\n" );
	 break;
   }
}

void ucil_get_pixel( unicap_data_buffer_t *data_buffer, ucil_color_t *color, int x, int y )
{
   int width = data_buffer->format.size.width;
   switch( data_buffer->format.fourcc )
   {
      case FOURCC( 'Y', '8', '0', '0' ):
      case FOURCC( 'G', 'R', 'E', 'Y' ):
      {
	 color->colorspace = UCIL_COLORSPACE_Y8;
	 color->y8.y = data_buffer->data[ y * width + x ];
      }
      break;      
      
      case FOURCC( 'R', 'G', 'B', ' ' ):
      case FOURCC( 'R', 'G', 'B', '3' ):
      {
	 int o = y * width * 3 + x * 3;
	 color->colorspace = UCIL_COLORSPACE_RGB24;
	 color->rgb24.r = data_buffer->data[ o ];
	 color->rgb24.g = data_buffer->data[ o+1 ];
	 color->rgb24.b = data_buffer->data[ o+2 ];
      }
      break;
      
      case FOURCC( 'R', 'G', 'B', 'A' ):
      case FOURCC( 'R', 'G', 'B', '4' ):
      {
	 int o = y * width * 4 + x * 4;
	 color->colorspace = UCIL_COLORSPACE_RGB32;
	 color->rgb32.r = data_buffer->data[ o ];
	 color->rgb32.g = data_buffer->data[ o+1 ];
	 color->rgb32.b = data_buffer->data[ o+2 ];
	 color->rgb32.a = data_buffer->data[ o+3 ];
      }
      break;
      
      case FOURCC( 'U', 'Y', 'V', 'Y' ):
      {
	 int o = y * width * 2 + (x & ~1) * 2;
	 int p = ( x & 1 ) * 2;

	 color->colorspace = UCIL_COLORSPACE_YUV;
	 
	 color->yuv.y = data_buffer->data[ o + p + 1 ];
	 color->yuv.u = data_buffer->data[ o ];
	 color->yuv.v = data_buffer->data[ o + 2 ];
      }
      break;

      case FOURCC( 'Y', 'U', 'Y', '2' ):
      case FOURCC( 'Y', 'U', 'Y', 'V' ):
      {
	 int o = y * width * 2 + (x & ~1) * 2;
	 int p = ( x & 1 ) * 2;

	 color->colorspace = UCIL_COLORSPACE_YUV;
	 
	 color->yuv.y = data_buffer->data[ o + p ];
	 color->yuv.u = data_buffer->data[ o + 1 ];
	 color->yuv.v = data_buffer->data[ o + 3 ];
      }
      break;

   }
}
   



void ucil_set_pixel( unicap_data_buffer_t *data_buffer, ucil_color_t *color, int x, int y )
{
   int width = data_buffer->format.size.width;

   switch( data_buffer->format.fourcc )
   {
      case UCIL_FOURCC( 'B', 'Y', '8', ' ' ):
      case UCIL_FOURCC( 'Y', '8', '0', '0' ):
      case UCIL_FOURCC( 'G', 'R', 'E', 'Y' ):
      {
	 data_buffer->data[ y * width + x ] = color->y8.y;
      }
      break;      
      
      case UCIL_FOURCC( 'I', '4', '2', '0' ):
      {
	 int height = data_buffer->format.size.height;
	 int uo = width * height;
	 int vo = uo + ( ( width / 2 ) * ( height / 2 ) );
	 int xo = y / 2 * width / 2 + x / 2;
	 data_buffer->data[ y * width + x ] = color->yuv.y;
	 data_buffer->data[ uo + xo ] = color->yuv.u;
	 data_buffer->data[ vo + xo ] = color->yuv.v;	 
      }
      break;      

      case UCIL_FOURCC( 'R', 'G', 'B', 0 ):
      case UCIL_FOURCC( 'R', 'G', 'B', ' ' ):
      case UCIL_FOURCC( 'R', 'G', 'B', '3' ):
      {
	 int o = y * width * 3 + x * 3;
	 if( color->colorspace == UCIL_COLORSPACE_RGB24 )
	 {
	    data_buffer->data[ o ] = color->rgb24.r;
	    data_buffer->data[ o+1 ] = color->rgb24.g;
	    data_buffer->data[ o+2 ] = color->rgb24.b;
	 }
	 else
	 {
	    data_buffer->data[ o ] = MIN( (int)data_buffer->data[ o ] * color->rgb32.a + (int)color->rgb32.r, 255 );
	    data_buffer->data[ o+1 ] = MIN( (int)data_buffer->data[ o+1 ] * color->rgb32.a + (int)color->rgb32.g, 255 );
	    data_buffer->data[ o+2 ] = MIN( (int)data_buffer->data[ o+2 ] * color->rgb32.a + (int)color->rgb32.b, 255 );
	 }
      }
      break;
      
      case UCIL_FOURCC( 'R', 'G', 'B', 'A' ):
      case UCIL_FOURCC( 'R', 'G', 'B', '4' ):
      {
	 int o = y * width * 4 + x * 4;
	 if( color->colorspace == UCIL_COLORSPACE_RGB24 )
	 {
	    data_buffer->data[ o ] = color->rgb24.r;
	    data_buffer->data[ o+1 ] = color->rgb24.g;
	    data_buffer->data[ o+2 ] = color->rgb24.b;
	 }
	 else
	 {
	    data_buffer->data[ o ] = MIN( (int)data_buffer->data[ o ] * color->rgb32.a + (int)color->rgb32.r, 255 );
	    data_buffer->data[ o+1 ] = MIN( (int)data_buffer->data[ o+1 ] * color->rgb32.a + (int)color->rgb32.g, 255 );
	    data_buffer->data[ o+2 ] = MIN( (int)data_buffer->data[ o+2 ] * color->rgb32.a + (int)color->rgb32.b, 255 );
	    data_buffer->data[ o+3 ] = color->rgb32.a;
	 }
      }
      break;
      
      case UCIL_FOURCC( 'U', 'Y', 'V', 'Y' ):
      {
	 int o = y * width * 2 + (x & ~1) * 2;
	 int p = ( x & 1 ) * 2;
	 
	 data_buffer->data[ o + p + 1 ] = color->yuv.y;
	 data_buffer->data[ o ] = color->yuv.u;
	 data_buffer->data[ o + 2 ] = color->yuv.v;
	 
      }
      break;

      case UCIL_FOURCC( 'Y', 'U', 'Y', '2' ):
      case UCIL_FOURCC( 'Y', 'U', 'Y', 'V' ):
      {
	 int o = y * width * 2 + (x & ~1) * 2;
	 int p = ( x & 1 ) * 2;
	 
	 data_buffer->data[ o + p ] = color->yuv.y;
	 data_buffer->data[ o + 1] = color->yuv.u;
	 data_buffer->data[ o + 3 ] = color->yuv.v;
	 
      }
      break;

      
      

   }
}

void ucil_set_pixel_alpha( unicap_data_buffer_t *data_buffer, ucil_color_t *color, int alpha, int x, int y )
{
   int width = data_buffer->format.size.width;

   switch( data_buffer->format.fourcc )
   {
      case UCIL_FOURCC( 'B', 'Y', '8', ' ' ):
      case UCIL_FOURCC( 'Y', '8', '0', '0' ):
      case UCIL_FOURCC( 'G', 'R', 'E', 'Y' ):
      {
	 data_buffer->data[ y * width + x ] = ( ( data_buffer->data[ y * width + x ] * (255 - alpha) ) + ( color->y8.y * alpha ) ) / 256;
      }
      break;      
      
      case UCIL_FOURCC( 'I', '4', '2', '0' ):
      {
	 int height = data_buffer->format.size.height;
	 int uo = width * height;
	 int vo = uo + ( ( width / 2 ) * ( height / 2 ) );
	 int xo = y / 2 * width / 2 + x / 2;
	 data_buffer->data[ y * width + x ] = ( ( data_buffer->data[ y * width + x ] * (255 - alpha) ) + ( color->y8.y * alpha ) ) / 256;
	 data_buffer->data[ uo + xo ] = color->yuv.u;
	 data_buffer->data[ vo + xo ] = color->yuv.v;	 
      }
      break;

      case UCIL_FOURCC( 'R', 'G', 'B', ' ' ):
      case UCIL_FOURCC( 'R', 'G', 'B', '3' ):
      {
	 int o = y * width * 3 + x * 3;
	 data_buffer->data[ o ] = ((data_buffer->data[ o ] * (255-alpha) ) + ( color->rgb24.r * alpha ) ) / 256;
	 data_buffer->data[ o+1 ] = ((data_buffer->data[ o+1 ] * (255-alpha) ) + ( color->rgb24.g * alpha ) ) / 256;
	 data_buffer->data[ o+2 ] = ((data_buffer->data[ o+2 ] * (255-alpha) ) + ( color->rgb24.b * alpha ) ) / 256;
      }
      break;
      
      case UCIL_FOURCC( 'R', 'G', 'B', 'A' ):
      case UCIL_FOURCC( 'R', 'G', 'B', '4' ):
      {
	 int o = y * width * 4 + x * 4;
	 data_buffer->data[ o ] = ((data_buffer->data[ o ] * (255-alpha) ) + ( color->rgb24.r * alpha ) ) / 256;
	 data_buffer->data[ o+1 ] = ((data_buffer->data[ o+1 ] * (255-alpha) ) + ( color->rgb24.g * alpha ) ) / 256;
	 data_buffer->data[ o+2 ] = ((data_buffer->data[ o+2 ] * (255-alpha) ) + ( color->rgb24.b * alpha ) ) / 256;
      }
      break;
      
      case UCIL_FOURCC( 'U', 'Y', 'V', 'Y' ):
      {
	 int o = y * width * 2 + (x & ~1) * 2;
	 int p = ( x & 1 ) * 2;
	 
	 data_buffer->data[ o + p + 1 ] = ( ( data_buffer->data[ o + p + 1 ] * ( 255 - alpha ) ) + ( color->yuv.y * alpha ) ) / 256;
	 data_buffer->data[ o ] = ( ( data_buffer->data[ o ] * ( 255 - alpha ) ) + ( color->yuv.u * alpha ) ) / 256;
	 data_buffer->data[ o + 2 ] = ( ( data_buffer->data[ o + 2 ] * ( 255 - alpha ) ) + ( color->yuv.v * alpha ) ) / 256;
      }
      break;

      case UCIL_FOURCC( 'Y', 'U', 'Y', '2' ):
      case UCIL_FOURCC( 'Y', 'U', 'Y', 'V' ):
      {
	 int o = y * width * 2 + (x & ~1) * 2;
	 int p = ( x & 1 ) * 2;
	 
	 data_buffer->data[ o + p ] = ( ( data_buffer->data[ o + p ] * ( 255 - alpha ) ) + ( color->yuv.y * alpha ) ) / 256;
	 data_buffer->data[ o + 1 ] = ( ( data_buffer->data[ o + 1 ] * ( 255 - alpha ) ) + ( color->yuv.u * alpha ) ) / 256;
	 data_buffer->data[ o + 3 ] = ( ( data_buffer->data[ o + 3 ] * ( 255 - alpha ) ) + ( color->yuv.v * alpha ) ) / 256;
      }
      break;

      default:
	 TRACE( "Unhandled color format!\n" );
	 break;

   }
}

enum
{
   REGION_NONE = 0, 
   REGION_BOTTOM = 1<<0, 
   REGION_TOP    = 1<<1,
   REGION_RIGHT  = 1<<2, 
   REGION_LEFT   = 1<<3,
};


static int find_region( unicap_data_buffer_t *buffer, int x, int y)
{
    int region = REGION_NONE;
    
    if( y > buffer->format.size.height )
    {
       region |= REGION_BOTTOM;
    }
    else if( y < 0 )
    {
       region |= REGION_TOP;
    }
    
    if( x > buffer->format.size.width )
    {
       region |= REGION_RIGHT;
    }
    else if ( x < 0 )
    {
       region |= REGION_LEFT;
    }
    
    return region;
}

// Cohen-Sutherland clipping: http://de.wikipedia.org/wiki/Cohen-Sutherland-Algorithmus
static int clip_line( unicap_data_buffer_t *buffer, int x1, int y1, int x2, int y2, int *x3, int *y3, int *x4, int *y4 )
{
    int region1, region2, region;

    region1 = find_region( buffer, x1, y1 ); 
    region2 = find_region( buffer, x2, y2 );


    while( region1 | region2 )
    {
	int x, y;
	if( region1 & region2 )
	{
	   // line not inside clip region
	   *x3 = *x4 = *y3 = *y4 = 0;
	   return 0;
	}

	region = region1 ? region1 : region2;
	if( region & REGION_BOTTOM )
	{
	    x = x1 + ( x2 - x1 ) * ( buffer->format.size.height - y1 ) / ( y2 - y1 );
	    y = buffer->format.size.height - 1;
	}
	else if( region & REGION_TOP )
	{
	    x = x1 + ( x2 - x1 ) * -y1 / ( y2 - y1 );
	    y = 0;
	}
	else if( region & REGION_RIGHT)
	{
	    y = y1 + ( y2 - y1 ) * ( buffer->format.size.width - x1) / ( x2 - x1 );
	    x = buffer->format.size.width - 1;
	}
	else 
	{
	    y = y1 + ( y2 - y1 ) * -x1 / ( x2 - x1 );
	    x = 0;
	}
	if( region == region1) //first endpoint was clipped
	{
	    x1 = x; 
	    y1 = y;
	    region1 = find_region( buffer, x1, y1 );
	}
	else //second endpoint was clipped
	{
	    x2 = x; 
	    y2 = y;
	    region2 = find_region( buffer, x2, y2);
	}
    }

    *x3 = x1;
    *x4 = x2;
    *y3 = y1;
    *y4 = y2;
    return 1;
}



void ucil_draw_line( unicap_data_buffer_t *data_buffer, ucil_color_t *color, 
		     int x0, int y0, 
		     int x1, int y1 )
{
   int steep; 
   int deltax;
   int deltay;
   int error;
   int ystep;
   int x,y;

   if( !clip_line( data_buffer, x0, y0, x1, y1, &x0, &y0, &x1, &y1 ) )
   {
      // line out of clipping region
      return;
   }

   steep = abs(y1-y0)>abs(x1-x0);
   if( steep )
   {
      int tmp;
      
      tmp = x0;
      x0 = y0;
      y0 = tmp;
      
      tmp = x1;
      x1 = y1;
      y1 = tmp;
   }
   
   if( x0 > x1 )
   {
      int tmp;
      
      tmp = x0;
      x0 = x1;
      x1 = tmp;
      tmp = y0;
      y0 = y1;
      y1 = tmp;
   }
   
   deltax = x1 - x0;
   deltay = abs(y1-y0);
   error = 0;
   y = y0;
   
   if( y0 < y1 )
   {
      ystep = 1;
   }
   else
   {
      ystep = -1;
   }

   
   
   for( x = x0; x < x1; x++ )
   {
      if( steep )
      {
	 ucil_set_pixel( data_buffer, color, y, x );
      }
      else
      {
	 ucil_set_pixel( data_buffer, color, x, y );
      }
      
      error = error + deltay;
      
      if( 2 * error >= deltax )
      {
	 y = y + ystep;
	 error = error - deltax;
      }
   }
}

static __inline__ 
void draw_hline( unicap_data_buffer_t *data_buffer, ucil_color_t *color, unsigned int fill, int y, int x1, int x2 )
{
   // Coordinates should already have been normalised and clipped:
   //    0 <= x1 <= x2 <= width-1
   //    0 <= y <= height-1
   //
   // For 16/32bpp, works best if data start is aligned to at least the pixel
   // size, so we can do aligned 32-bit writes most/all of the time.
   //
   // Fill generation (and rolling for 24bpp) is little-endian specific, but
   // that is relatively easy to fix, we could also try to do 64-bit writes
   // on 64-bit machines at the cost of an extra decision point.

   int bytespp;
   unsigned char *p;
   unsigned char *q;
   unsigned int fill24[3];
   int o;

   bytespp = data_buffer->format.bpp/8;
   p = data_buffer->data + (y*data_buffer->format.size.width + x1)*bytespp;

   switch( bytespp )
   {
      case 1:
         // Assume this is more optimised than anything else we could do
         memset( p, fill, x2-x1+1 );
         break;

      case 2:
         q = p + (x2-x1)*2;

         if( (intptr_t)p & 2 )
         {
            // Align start to next word boundary
            *(unsigned short*)p = fill;
            p += 2;
            if (p>q) return;
         }
         if( ((intptr_t)q & 2)==0 )
         {
            // Align end to previous word boundary
            *(unsigned short*)q = fill;
            q -= 4;
         }
         while( p<=q )
         {
            *(unsigned int*)p = fill;
            p += 4;
         }
         break;

      case 3:
         // These aren't *quite* rotations: RBGR, GRBG, BGRB
         fill24[0] = fill;
         fill24[1] = (fill>>8) | ((fill & 0xff00)<<16);
         fill24[2] = (fill<<8) | ((fill & 0xff0000)>>16);
         q = p + (x2-x1)*3;
         o = 0;

         // Align start to next word boundary, and choose correct fill value
         // for that position
         switch( (intptr_t)p & 3 )
         {
            case 1:
               *p = fill;
               *(unsigned short*)(p+1) = fill>>8;
               p += 3;
               if (p>q) return;
               break;
            case 2:
               *(unsigned short*)p = fill;
               p += 2;
               o = 2;
               break;
            case 3:
               *p = fill;
               p++;
               o = 1;
               break;
         }

         // Align end to previous word boundary
         switch( (intptr_t)q & 3 )
         {
            case 0:
               *q = fill;
               *(unsigned short*)(q+1) = fill>>8;
               break;
            case 1:
               q += 3;
               break;
            case 2:
               q += 2;
               *q = fill>>16;
               break;
            case 3:
               q += 1;
               *(unsigned short*)q = fill>>8;
               break;
         }

         while( p<q )
         {
            *(unsigned int*)p = fill24[o++];
            if (o==3) o = 0;
            p += 4;
         }
         break;

      case 4:
         q = p + (x2-x1)*4;

         while( p<=q )
         {
            *(unsigned int*)p = fill;
            p += 4;
         }
         break;
   }
}

static __inline__
int get_colorspace_fill( unicap_data_buffer_t *data_buffer, ucil_color_t *color , unsigned int *fill )
{
   switch( color->colorspace )
   {
      default:
         TRACE( "color format not supported for this operation\n" );
         return 0;

      case UCIL_COLORSPACE_Y8: 
         *fill = color->y8.y | ( color->y8.y << 8 ) | ( color->y8.y << 16 ) | ( color->y8.y << 24 );
         break;

      case UCIL_COLORSPACE_RGB24:
         *fill = color->rgb24.r | ( color->rgb24.g << 8 ) | ( color->rgb24.b << 16 ) | ( color->rgb24.r << 24 );
         break;
      
      case UCIL_COLORSPACE_RGB32: 
         *fill = color->rgb32.r | ( color->rgb32.g << 8 ) | ( color->rgb32.b << 16 ) | ( color->rgb32.a << 24 );
         break;

      case UCIL_COLORSPACE_YUV:
         switch( data_buffer->format.fourcc )
         {
            default:
               TRACE( "color format not supported for this operation\n" );
               return 0;
            case FOURCC('U','Y','V','Y'):
               *fill = color->yuv.u + ( color->yuv.y << 8 ) + ( color->yuv.v << 16 ) + ( color->yuv.y << 24 );
               break;
            case FOURCC('Y','U','Y','V'):
            case FOURCC('Y','U','Y','2'):
               *fill = (color->yuv.u<<8) + ( color->yuv.y  ) + ( color->yuv.v << 24 ) + ( color->yuv.y << 16 );
               break;
         }
         break;
   }

   return 1;
}

void ucil_draw_rect( unicap_data_buffer_t *data_buffer, ucil_color_t *color, int x1, int y1, int x2, int y2 )
{
   unsigned int fill;
   int y;

   NORMALIZE( int, x1, x2 );
   NORMALIZE( int, y1, y2 );

   if( OOB( data_buffer, x1, y1, x2, y2 ) ) return;

   x1 = CLIP( x1, 0, data_buffer->format.size.width-1 );
   x2 = CLIP( x2, 0, data_buffer->format.size.width-1 );
   y1 = CLIP( y1, 0, data_buffer->format.size.height-1 );
   y2 = CLIP( y2, 0, data_buffer->format.size.height-1 );
      
   if( !get_colorspace_fill( data_buffer, color, &fill ) ) return;

   for( y = y1; y <= y2; y++ )
   {
      draw_hline( data_buffer, color, fill, y, x1, x2 );
   }
}

void ucil_draw_box( unicap_data_buffer_t *data_buffer, ucil_color_t *color, int x1, int y1, int x2, int y2 )
{
   int xl, xr, yt, yb, y;
   unsigned int fill;

   NORMALIZE( int, x1, x2 );
   NORMALIZE( int, y1, y2 );

   if( OOB( data_buffer, x1, y1, x2, y2 ) ) return;

   xl = CLIP( x1, 0, data_buffer->format.size.width-1 );
   xr = CLIP( x2, 0, data_buffer->format.size.width-1 );
   yt = CLIP( y1+1, 0, data_buffer->format.size.height-1 );
   yb = CLIP( y2-1, 0, data_buffer->format.size.height-1 );

   if( !get_colorspace_fill(data_buffer, color, &fill ) ) return;

   if( y1>=0 )
   {
      draw_hline( data_buffer, color, fill, y1, xl, xr );
   }
   if( y2<data_buffer->format.size.height )
   {
      draw_hline( data_buffer, color, fill, y2, xl, xr );
   }
   for( y = yt; y <= yb; y++ )
   {
      if( x1>=0 )
      {
         ucil_set_pixel( data_buffer, color, x1, y );
      }
      if( x2<data_buffer->format.size.width )
      {
         ucil_set_pixel( data_buffer, color, x2, y );
      }
   }   
}

void ucil_draw_circle( unicap_data_buffer_t *dest, ucil_color_t *color, int cx, int cy, int r )
{
   int x = 0;
   int y = r;
   int e = 1-r;
   int dx = 1;
   int dy = -2*r;

   ucil_set_pixel( dest, color, cx, cy-r );
   ucil_set_pixel( dest, color, cx, cy+r );
   ucil_set_pixel( dest, color, cx-r, cy );
   ucil_set_pixel( dest, color, cx+r, cy );

   for( ;; )
   {
      if( e>=0 )
      {
         y--;
         dy += 2;
         e += dy;
      }
      x++;
      if( x>=y ) break;
      dx += 2;
      e += dx;

      ucil_set_pixel( dest, color, cx+x, cy-y );
      ucil_set_pixel( dest, color, cx+x, cy+y );
      ucil_set_pixel( dest, color, cx-x, cy-y );
      ucil_set_pixel( dest, color, cx-x, cy+y );
      ucil_set_pixel( dest, color, cx+y, cy-x );
      ucil_set_pixel( dest, color, cx+y, cy+x );
      ucil_set_pixel( dest, color, cx-y, cy-x );
      ucil_set_pixel( dest, color, cx-y, cy+x );
   }

   if( x==y )
   {
      ucil_set_pixel( dest, color, cx+x, cy-y );
      ucil_set_pixel( dest, color, cx+x, cy+y );
      ucil_set_pixel( dest, color, cx-x, cy-y );
      ucil_set_pixel( dest, color, cx-x, cy+y );
   }
}

static __inline__
void draw_hline_clipped( unicap_data_buffer_t *data_buffer, ucil_color_t *color, unsigned int fill, int y, int x1, int x2 )
{
   if( OOB( data_buffer, x1, y, x2, y ) ) return;

   x1 = CLIP( x1, 0, data_buffer->format.size.width-1 );
   x2 = CLIP( x2, 0, data_buffer->format.size.width-1 );

   draw_hline( data_buffer, color, fill, y, x1, x2 );
}

void ucil_draw_filled_circle( unicap_data_buffer_t *dest, ucil_color_t *color, int cx, int cy, int r )
{
   unsigned int fill;
   int x, y, e, dx, dy;

   if( r<0 ) r = -r;
   x = 0;
   y = r;
   e = 1-r;
   dx = 1;
   dy = -2*r;

   if( !get_colorspace_fill( dest, color, &fill ) ) return;

   draw_hline_clipped( dest, color, fill, cy, cx-r, cx+r );

   for( ;; )
   {
      if( e>=0 )
      {
         draw_hline_clipped( dest, color, fill, cy-y, cx-x, cx+x );
         draw_hline_clipped( dest, color, fill, cy+y, cx-x, cx+x );
         y--;
         dy += 2;
         e += dy;
      }
      x++;
      if( x>=y ) break;
      dx += 2;
      e += dx;

      draw_hline_clipped( dest, color, fill, cy-x, cx-y, cx+y );
      draw_hline_clipped( dest, color, fill, cy+x, cx-y, cx+y );
   }

   if( x==y )
   {
      draw_hline_clipped( dest, color, fill, cy-y, cx-x, cx+x );
      draw_hline_clipped( dest, color, fill, cy+y, cx-x, cx+x );
   }
}
