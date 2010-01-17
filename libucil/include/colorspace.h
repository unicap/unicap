/* unicap
 *
 * Copyright (C) 2004 Arne Caspari ( arne_caspari@users.sourceforge.net )
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

#ifndef __COLORSPACE_H__
#define __COLORSPACE_H__

#include <sys/types.h>
#include <linux/types.h>

#include "unicap.h"

#define XFM_IN_PLACE      0x1
#define XFM_PASSTHROUGH   0x2
#define XFM_NO_CONVERSION 0x4

#ifndef FOURCC
#define FOURCC(a,b,c,d) (unsigned int)((((unsigned int)d)<<24)+ \
                        (((unsigned int)c)<<16)+(((unsigned int)b)<<8)+a)
#endif

typedef void (*xfm_func_t)( unsigned char *dest_buffer, 
			    unsigned char *src_buffer, 
			    int width, int height, int shift );

typedef struct _xfm_info
{
      unsigned int src_fourcc;
      char src_guid[16];
      int src_bpp;
      
      unsigned int dest_fourcc;
      char dest_guid[16];
      int dest_bpp;
      
      int priority;

      xfm_func_t func;

      unsigned int flags;
      
} xfm_info_t, *pxfm_info_t;


void ucil_get_xfminfo_from_fourcc( unsigned int src_fourcc, 
				   unsigned int dest_fourcc, 
				   xfm_info_t *info );

#endif//__COLORSPACE_H__
