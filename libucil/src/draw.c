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
   int o;
   int x;
   int bytespp;

   bytespp = data_buffer->format.bpp/8;

   o = y * data_buffer->format.size.width * bytespp;

   for( x = x1; x < ( ( x1 & ~0x3 ) + 4 ); x++ )
   {
      ucil_set_pixel( data_buffer, color, x, y );
   }
   for( x = x2 & ~0x3; x <= x2; x++ )
   {
      ucil_set_pixel( data_buffer, color, x, y );
   }
   for( x = ((x1&~0x3)+4); x < ((x2)&~03); x+=4/bytespp )
   {
      *(int*)(data_buffer->data + o + ( x * bytespp)) = fill;
   }
}

static __inline__ 
void draw_hline_rgb24( unicap_data_buffer_t *data_buffer, ucil_color_t *color, int y, int x1, int x2 )
{
   int xa;
   int xb;
   int xc;
   unsigned int *o;
   unsigned int fill1 = ( color->rgb24.r << 24 ) | ( color->rgb24.g << 16 ) | ( color->rgb24.b << 8 ) | ( color->rgb24.r );
   unsigned int fill2 = ( color->rgb24.g << 24 ) | ( color->rgb24.b << 16 ) | ( color->rgb24.r << 8 ) | ( color->rgb24.g );
   unsigned int fill3 = ( color->rgb24.b << 24 ) | ( color->rgb24.r << 16 ) | ( color->rgb24.g << 8 ) | ( color->rgb24.b );
   
   o = (unsigned int *) ( data_buffer->data + ( y * data_buffer->format.size.width * 3 ) );
   
   for( xa = x1; xa < ( x1 + ( x1 % 3 ) ); xa++ )
   {
      ucil_set_pixel( data_buffer, color, xa, y );
   }
   for( xb = x2 - 4; xb <= x2; xb++ )
   {
      ucil_set_pixel( data_buffer, color, xb, y );
   }
   for( xc = xa; xc < x2; xc += 4 )
   {
      *o++ = fill1;
      *o++ = fill2;
      *o++ = fill3;
   }
}


void ucil_draw_rect( unicap_data_buffer_t *data_buffer, ucil_color_t *color, int x1, int y1, int x2, int y2 )
{
   if( x1 > x2 )
   {
      int t;
      t = x2;
      x2 = x1;
      x1 = t;
   }
   if( y1 > y2 )
   {
      int t;
      t = y2;
      y2 = y1;
      y1 = t;
   }

   x1 = CLIP( x1, 0, data_buffer->format.size.width-1 );
   x2 = CLIP( x2, 0, data_buffer->format.size.width-1 );
   y1 = CLIP( y1, 0, data_buffer->format.size.height-1 );
   y2 = CLIP( y2, 0, data_buffer->format.size.height-1 );
   
   
   switch( color->colorspace )
   {
      default:
	 TRACE( "color format not supported for this operation\n" );
	 break;

      case UCIL_COLORSPACE_Y8: 
      {
	 unsigned int fill;
	 int y;
	 for( y = y1; y <= y2; y++ )
	 {
	    fill = color->y8.y | ( color->y8.y << 8 ) | ( color->y8.y << 16 ) | ( color->y8.y << 24 );
	    draw_hline( data_buffer, color, fill, y, x1, x2 );
	 }
      }
      break;

      case UCIL_COLORSPACE_RGB24:
      {
	 int x,y;
	 for( y = y1; y < y2; y++ )
	 {
	    for( x = x1; x <= x2; x++ )
	    {
	       ucil_set_pixel( data_buffer, color, x, y );
	    }
	    
/* 	    draw_hline_rgb24( data_buffer, color, y, x1, x2 ); */
	 }
      }
      break;
      
      case UCIL_COLORSPACE_RGB32: 
      {
	 unsigned int fill;
	 int y;
	 for( y = y1; y <= y2; y++ )
	 {
	    fill = color->rgb32.r | ( color->rgb32.g << 8 ) | ( color->rgb32.b << 16 ) | ( color->rgb32.a << 24 );
	    draw_hline( data_buffer, color, fill, y, x1, x2 );
	 }
      }
      break;

      case UCIL_COLORSPACE_YUV:
      {
	 switch( data_buffer->format.fourcc )
	 {
	    case FOURCC('U','Y','V','Y'):
	    {
	       unsigned int fill;
	       int y;
	       for( y = y1; y <= y2; y++ )
	       {
		  fill = color->yuv.u + ( color->yuv.y << 8 ) + ( color->yuv.v << 16 ) + ( color->yuv.y << 24 );
		  draw_hline( data_buffer, color, fill, y, x1, x2 );		  
	       }
	    }
	    case FOURCC('Y','U','Y','V'):
	    case FOURCC('Y','U','Y','2'):
	    {
	       unsigned int fill;
	       int y;
	       for( y = y1; y <= y2; y++ )
	       {
		  fill = (color->yuv.u<<8) + ( color->yuv.y  ) + ( color->yuv.v << 24 ) + ( color->yuv.y << 16 );
		  draw_hline( data_buffer, color, fill, y, x1, x2 );		  
	       }
	    }
	 }
      }
      break;
   }   
}

void ucil_draw_box( unicap_data_buffer_t *data_buffer, ucil_color_t *color, int x1, int y1, int x2, int y2 )
{
   if( x1 > x2 )
   {
      int t;
      t = x2;
      x2 = x1;
      x1 = t;
   }
   if( y1 > y2 )
   {
      int t;
      t = y2;
      y2 = y1;
      y1 = t;
   }
   
   switch( color->colorspace )
   {
      default:
	 TRACE( "color format not supported for this operation\n" );
	 break;

      case UCIL_COLORSPACE_Y8: 
      {
	 unsigned int fill;
	 int y;
	 fill = color->y8.y | ( color->y8.y << 8 ) | ( color->y8.y << 16 ) | ( color->y8.y << 24 );
	 draw_hline( data_buffer, color, fill, y1, x1, x2 );
	 draw_hline( data_buffer, color, fill, y2, x1, x2 );
	 for( y = y1; y < y2; y++ )
	 {
	    ucil_set_pixel( data_buffer, color, x1, y );
	    ucil_set_pixel( data_buffer, color, x2, y );
	 }
      }
      break;

      case UCIL_COLORSPACE_RGB24:
      {
	 int x,y;
	 for( y = y1; y < y2; y++ )
	 {
	    ucil_set_pixel( data_buffer, color, x1, y );
	    ucil_set_pixel( data_buffer, color, x2, y );
	 }
	 for( x = x1; x < x2; x++ )
	 {
	    ucil_set_pixel( data_buffer, color, x, y1 );
	    ucil_set_pixel( data_buffer, color, x, y2 );
	 }
      }
      break;
      
      case UCIL_COLORSPACE_RGB32: 
      {
	 unsigned int fill;
	 int y;
	 fill = color->rgb32.r | ( color->rgb32.g << 8 ) | ( color->rgb32.b << 16 ) | ( color->rgb32.a << 24 );
	 draw_hline( data_buffer, color, fill, y1, x1, x2 );
	 draw_hline( data_buffer, color, fill, y2, x1, x2 );
	 for( y = y1; y < y2; y++ )
	 {
	    ucil_set_pixel( data_buffer, color, x1, y );
	    ucil_set_pixel( data_buffer, color, x2, y );
	 }
      }
      break;

      case UCIL_COLORSPACE_YUV:
      {
	 switch( data_buffer->format.fourcc )
	 {
	    case FOURCC('U','Y','V','Y'):
	    {
	       unsigned int fill;
	       int y;
	       fill = color->yuv.u + ( color->yuv.y << 8 ) + ( color->yuv.v << 16 ) + ( color->yuv.y << 24 );
	       draw_hline( data_buffer, color, fill, y1, x1, x2 );
	       draw_hline( data_buffer, color, fill, y2, x1, x2 );
	       for( y = y1; y < y2; y++ )
	       {
		  ucil_set_pixel( data_buffer, color, x1, y );
		  ucil_set_pixel( data_buffer, color, x2, y );
	       }
	    }
	    case FOURCC('Y','U','Y','V'):
	    case FOURCC('Y','U','Y','2'):
	    {
	       unsigned int fill;
	       int y;
	       fill = (color->yuv.u<<8) + ( color->yuv.y ) + ( color->yuv.v << 24 ) + ( color->yuv.y << 16 );
	       draw_hline( data_buffer, color, fill, y1, x1, x2 );
	       draw_hline( data_buffer, color, fill, y2, x1, x2 );
	       for( y = y1; y < y2; y++ )
	       {
		  ucil_set_pixel( data_buffer, color, x1, y );
		  ucil_set_pixel( data_buffer, color, x2, y );
	       }
	    }
	 }
      }
      break;
   }   
}

void ucil_draw_circle( unicap_data_buffer_t *dest, ucil_color_t *color, int cx, int cy, int r )
{
   double step = 1/(double)r;
   double i;

   for( i = 0; i < 2 * M_PI; i += step )
   {
      ucil_set_pixel( dest, color, cx + sin( i ) * r, cy + sin( i + M_PI_2 ) * r );
   }
}

