/* unicap
 *
 * Copyright (C) 2004-2008 Arne Caspari ( arne@unicap-imaging.org )
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
#include "yuvops.h"
#include "rgbops.h"

#include <glib.h>
#include <stdlib.h>

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

