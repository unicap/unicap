/* unicap
 *
 * Copyright (C) 2004-2010 Arne Caspari ( arne@unicap-imaging.org )
 *
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

#include "ucil_version.h"
#include "ucil.h"
#include "ucil_private.h"
#include "ucil_ppm.h"
#include "yuvops.h"
#include "rgbops.h"

#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include "debug.h"

const unsigned int ucil_major_version=UCIL_MAJOR_VERSION;
const unsigned int ucil_minor_version=UCIL_MINOR_VERSION;
const unsigned int ucil_micro_version=UCIL_MICRO_VERSION;

unicap_status_t ucil_check_version( unsigned int major, unsigned int minor, unsigned int micro )
{
   unicap_status_t status = STATUS_SUCCESS;
   
   if( UCIL_MAJOR_VERSION < major )
   {
      status = STATUS_INCOMPATIBLE_MAJOR_VERSION;
   }
   else if( major == UCIL_MAJOR_VERSION )
   {
      if( UCIL_MINOR_VERSION < minor )
      {
	 status = STATUS_INCOMPATIBLE_MINOR_VERSION;
      }
      else if( minor == UCIL_MINOR_VERSION )
      {
	 if( UCIL_MICRO_VERSION < micro )
	 {
	    status = STATUS_INCOMPATIBLE_MICRO_VERSION;
	 }
      }
   }
   
   return status;
}

void ucil_blend_alpha( unicap_data_buffer_t *dest, unicap_data_buffer_t *bg, unicap_data_buffer_t *fg, int alpha )
{
   switch( bg->format.fourcc )
   {
      case UCIL_FOURCC( 'U', 'Y', 'V', 'Y' ):
      {
	 ucil_blend_uyvy_a( dest, bg, fg, alpha );
      }
      break;

      case UCIL_FOURCC( 'R', 'G', 'B', '3' ):
      {
	 ucil_blend_rgb24_a( dest, bg, fg, alpha );
      }
      break;

      case UCIL_FOURCC( 'R', 'G', 'B', '4' ):
      {
	 ucil_blend_rgb32_a( dest, bg, fg, alpha );
      }
      break;

      default:
      {
	 g_warning( "Operation not defined for color format\n" );
      }
      break;
   }
}

void ucil_composite( unicap_data_buffer_t *dest, 
		     unicap_data_buffer_t *img, 
		     int xpos, 
		     int ypos, 
		     double scalex, 
		     double scaley, 
		     ucil_interpolation_type_t interp )
{
   switch( dest->format.fourcc )
   {
      case UCIL_FOURCC( 'U', 'Y', 'V', 'Y' ):
	 ucil_composite_UYVY_YUVA( dest, img, xpos, ypos, scalex, scaley, interp );
	 break; 
      case UCIL_FOURCC( 'Y', 'U', 'Y', 'V' ):
      case UCIL_FOURCC( 'Y', 'U', 'Y', '2' ):
	 ucil_composite_YUYV_YUVA( dest, img, xpos, ypos, scalex, scaley, interp );
	 break; 
      default:
	 g_warning( "Operation not defined for color format\n" );
	 break;
   }
   
}

void ucil_fill( unicap_data_buffer_t *buffer, ucil_color_t *color )
{
   switch( buffer->format.fourcc )
   {
      case UCIL_FOURCC( 'U', 'Y', 'V', 'Y' ):
      {
	 ucil_fill_uyvy( buffer, color );
      }
      break;

      case UCIL_FOURCC( 'R', 'G', 'B', '3' ):
      {
	 ucil_fill_rgb24( buffer, color );
      }
      break;

      case UCIL_FOURCC( 'R', 'G', 'B', '4' ):
      {
	 ucil_fill_rgb32( buffer, color );
      }
      break;

      default:
      {
	 g_warning( "Operation not defined for color format\n" );
      }
      break;
   }
}
      
void ucil_convolution_mask( unicap_data_buffer_t *dest, unicap_data_buffer_t *src, ucil_convolution_mask_t *mask )
{
   switch( src->format.fourcc )
   {
      case UCIL_FOURCC( 'U', 'Y', 'V', 'Y' ):
      {
	 ucil_convolution_mask_uyvy( dest, src, mask );
      }
      break;
      
      default:
      {
	 g_warning( "Operation not defined for color format\n" );
      }
      break;
   }
}

ucil_convolution_mask_t *ucil_create_convolution_mask( unsigned char *array, int size, ucil_colorspace_t cs, int mode )
{
   ucil_convolution_mask_t *mask;
   
   mask = malloc( sizeof( ucil_convolution_mask_t ) );
   mask->mask = malloc( size * size * sizeof( ucil_color_t ) );
   mask->size = size;
   mask->colorspace = cs;

   switch( cs )
   {
      case UCIL_COLORSPACE_YUV:
      {
	 int i, j;
	 
	 for( i = 0; i < size; i++ )
	 {
	    for( j = 0; j < size; j++ )
	    {
	       mask->mask[i * size + j].colorspace = cs;
	       mask->mask[i * size + j].yuv.y = array[ i * size + j ];
	       mask->mask[i * size + j].yuv.u = 0;
	       mask->mask[i * size + j].yuv.v = 0;
	    }
	 }
      }

      default:
	 TRACE( "color format not supported for operation\n" );
	 break;
   }
   
   return mask;
}

ucil_colorspace_t ucil_get_colorspace_from_fourcc( unsigned int fourcc )
{
   ucil_colorspace_t cs = UCIL_COLORSPACE_UNKNOWN;
   
   switch( fourcc )
   {
      case UCIL_FOURCC( 'Y', '8', '0', '0' ):
      case UCIL_FOURCC( 'G', 'R', 'E', 'Y' ):
	 cs = UCIL_COLORSPACE_Y8;
	 break;
	 
      case UCIL_FOURCC( 'R', 'G', 'B', ' ' ):
      case UCIL_FOURCC( 'R', 'G', 'B', '3' ):
	 cs = UCIL_COLORSPACE_RGB24;
	 break;
	 
      case UCIL_FOURCC( 'R', 'G', 'B', 'A' ):
      case UCIL_FOURCC( 'R', 'G', 'B', '4' ):
	 cs = UCIL_COLORSPACE_RGB32;
	 break;
	 
      case UCIL_FOURCC( 'U', 'Y', 'V', 'Y' ):
      case UCIL_FOURCC( 'Y', 'U', 'Y', 'V' ):
      case UCIL_FOURCC( 'Y', 'U', 'Y', '2' ):
      case UCIL_FOURCC( 'Y', '4', '2', '2' ):
      case UCIL_FOURCC( 'Y', '4', '1', '1' ):
      case UCIL_FOURCC( 'Y', '4', '2', '0' ):
	 cs = UCIL_COLORSPACE_YUV;
	 break;
	 
      default:
	 cs = UCIL_COLORSPACE_UNKNOWN;
	 break;
   }
   
   return cs;
}

__HIDDEN__ unicap_data_buffer_t *ucil_allocate_buffer( int width, int height, unsigned int fourcc, int bpp )
{
   unicap_data_buffer_t *buffer;
   buffer = malloc( sizeof( unicap_data_buffer_t) );
   unicap_void_format(&buffer->format);
   buffer->format.size.width = width;
   buffer->format.size.height = height;
   buffer->format.fourcc = fourcc;
   buffer->format.bpp = bpp;
   buffer->format.buffer_size = buffer->format.size.width * buffer->format.size.height * buffer->format.bpp / 8;
   buffer->buffer_size = buffer->format.buffer_size;
   buffer->data = malloc( buffer->buffer_size );
   
   return buffer;
}


unicap_status_t ucil_copy_region( unicap_data_buffer_t *destbuf, unicap_data_buffer_t *srcbuf, unicap_rect_t *rect )
{
   int sy, dy;

   if ( (destbuf->format.fourcc != srcbuf->format.fourcc) ||
	((rect->y + rect->height) > srcbuf->format.size.height) ||
	((rect->x + rect->width) > srcbuf->format.size.width) )
      return STATUS_INVALID_PARAMETER;

   memcpy( &destbuf->capture_start_time, &srcbuf->capture_start_time, sizeof( struct timeval ) );
   memcpy( &destbuf->duration, &srcbuf->duration, sizeof( struct timeval ) );
   memcpy( &destbuf->fill_time, &srcbuf->fill_time, sizeof( struct timeval ) );
   
   for (sy = rect->y, dy=0; sy < (rect->y + rect->height); sy++, dy++ ){
      memcpy (destbuf->data + (dy * destbuf->format.size.width * destbuf->format.bpp / 8), 
	      srcbuf->data + ( (sy * srcbuf->format.size.width * srcbuf->format.bpp / 8) + rect->x * srcbuf->format.bpp / 8), 
	      rect->width * srcbuf->format.bpp / 8 );
   }
   
   return STATUS_SUCCESS;
}


unicap_data_buffer_t *ucil_copy_region_alloc( unicap_data_buffer_t *srcbuf, unicap_rect_t *rect )
{
   unicap_data_buffer_t *destbuf;
   
   destbuf = ucil_allocate_buffer( rect->width, rect->height, srcbuf->format.fourcc, srcbuf->format.bpp );
   ucil_copy_region(destbuf, srcbuf, rect);
   
   return destbuf;
}

unicap_status_t ucil_copy_field( unicap_data_buffer_t *destbuf, unicap_data_buffer_t *srcbuf, ucil_field_type_t type )
{
   if( (destbuf->format.fourcc != srcbuf->format.fourcc)||
       (destbuf->format.size.width != srcbuf->format.size.width)||
       (destbuf->format.size.height != (srcbuf->format.size.height/2))){
      return STATUS_INVALID_PARAMETER;
   }
   
   memcpy( &destbuf->capture_start_time, &srcbuf->capture_start_time, sizeof( struct timeval ) );
   memcpy( &destbuf->duration, &srcbuf->duration, sizeof( struct timeval ) );
   memcpy( &destbuf->fill_time, &srcbuf->fill_time, sizeof( struct timeval ) );

   int offset = 0;
   int rowstride = srcbuf->format.size.width * srcbuf->format.bpp / 8;
   
   if( type == UCIL_FIELD_ODD )
      offset = rowstride;
   
   int y;
   for( y = 0; y < ((srcbuf->format.size.height/2)); y++ ){
      memcpy(destbuf->data + y*rowstride, srcbuf->data + y*2*rowstride + offset, rowstride);
   }
   
   return STATUS_SUCCESS;
}

unicap_data_buffer_t *ucil_copy_field_alloc( unicap_data_buffer_t *srcbuf, ucil_field_type_t type )
{
   unicap_data_buffer_t *destbuf;
   
   destbuf = ucil_allocate_buffer( srcbuf->format.size.width, srcbuf->format.size.height/2, srcbuf->format.fourcc, srcbuf->format.bpp );
   ucil_copy_field( destbuf, srcbuf, type );
   
   return destbuf;
}

unicap_status_t ucil_copy_color_plane( unicap_data_buffer_t *destbuf, unicap_data_buffer_t *srcbuf, ucil_color_plane_t plane )
{
   unicap_status_t status = STATUS_SUCCESS;

   memcpy( &destbuf->capture_start_time, &srcbuf->capture_start_time, sizeof( struct timeval ) );
   memcpy( &destbuf->duration, &srcbuf->duration, sizeof( struct timeval ) );
   memcpy( &destbuf->fill_time, &srcbuf->fill_time, sizeof( struct timeval ) );

   switch( srcbuf->format.fourcc ){
   case UCIL_FOURCC( 'B', 'Y', '8', ' '):
   case UCIL_FOURCC( 'B', 'A', '8', '1'):
      ucil_copy_color_plane_by8( destbuf, srcbuf, plane );
      break;
   default:
      status = STATUS_NOT_IMPLEMENTED;
      break;
   }
   
   return status;
}
   
unicap_data_buffer_t *ucil_copy_color_plane_alloc( unicap_data_buffer_t *srcbuf, ucil_color_plane_t plane )
{
   int width, height;
   int bpp = 8;
   
   switch( srcbuf->format.fourcc ){
   case UCIL_FOURCC( 'B', 'Y', '8', ' '):
   case UCIL_FOURCC( 'B', 'A', '8', '1'):
      switch( plane ){
      case UCIL_COLOR_PLANE_RED:
      case UCIL_COLOR_PLANE_BLUE:
	 width = srcbuf->format.size.width / 2;
	 height = srcbuf->format.size.height / 2;
	 break;
      case UCIL_COLOR_PLANE_GREEN:
	 width = srcbuf->format.size.width / 2;
	 height = srcbuf->format.size.height;
	 break;
      default:
	 return NULL;
      }
      break;
   default:
      return NULL;
   }

   unicap_data_buffer_t *destbuf = ucil_allocate_buffer( width, height, UCIL_FOURCC( 'Y', '8', '0', '0' ), bpp );
   ucil_copy_color_plane( destbuf, srcbuf, plane );
   
   return destbuf;   
}

unicap_status_t *ucil_weave( unicap_data_buffer_t *destbuf, unicap_data_buffer_t *odd, unicap_data_buffer_t *even )
{
   /* if( (destbuf->format.fourcc != srcbuf->format.fourcc)|| */
   /*     (destbuf->format.size.width != srcbuf->format.size.width)|| */
   /*     (destbuf->format.size.height != (srcbuf->format.size.height/2))){ */
   /*    return STATUS_INVALID_PARAMETER; */
   /* } */
   
   int rowstride = destbuf->format.size.width * destbuf->format.bpp / 8;   
   int y;

   memcpy( &destbuf->capture_start_time, &even->capture_start_time, sizeof( struct timeval ) );
   memcpy( &destbuf->duration, &even->duration, sizeof( struct timeval ) );
   memcpy( &destbuf->fill_time, &even->fill_time, sizeof( struct timeval ) );

   for( y = 0; y < destbuf->format.size.height/2; y++ ){
      memcpy(destbuf->data + y*2*rowstride, even->data + y*rowstride, rowstride);
      memcpy(destbuf->data + (y*2+1)*rowstride, odd->data + y*rowstride, rowstride);
   }
   
   return STATUS_SUCCESS;
}

unicap_data_buffer_t *ucil_weave_alloc( unicap_data_buffer_t *odd, unicap_data_buffer_t *even )
{
   unicap_data_buffer_t *destbuf = ucil_allocate_buffer( odd->format.size.width, odd->format.size.height * 2, 
							 odd->format.fourcc, odd->format.bpp );
   
   ucil_weave( destbuf, odd, even );
   
   return destbuf;
}

unicap_data_buffer_t *ucil_read_file_alloc( const char* path )
{
   return ucil_ppm_read_file( path );
}

unicap_status_t ucil_write_file( const char* path, const char *format, unicap_data_buffer_t *buffer )
{
   return ucil_ppm_write_file( buffer, path );
}
