/* unicap
 *
 * Copyright (C) 2004 Arne Caspari ( arne@unicap-imaging.org )
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

#include "config.h"

#if HAVE_AVCODEC
#include <avcodec.h>
#include <avutil.h>
#endif //HAVE_AVCODEC

#include <sys/types.h>
#include <linux/types.h>
#include <glib.h>
#include <string.h>


#include "colorspace.h"
#include "ucil.h"

#if UNICAPGTK_DEBUG
#define DEBUG
#endif
#include "debug.h"

#define SAT(c) if( ( c & (~255) ) != 0 ) { if (c < 0) c = 0; else c = 255; }

#define FOURCC(a,b,c,d) (unsigned int)((((unsigned int)d)<<24)+ \
                        (((unsigned int)c)<<16)+(((unsigned int)b)<<8)+a)

#define SHIFT(x) (x>>shift)

//52474200-0000-0010-8000-00aa00389b71
#define GUID_RGB24 { 0x52, 0x47, 0x42, 0x00, 0x00, 0x00, 0x00, 0x10, \
		     0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 }
#define GUID_BGR24 { 0x42, 0x47, 0x52, 0x00, 0x00, 0x00, 0x00, 0x10, \
		     0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 }

//52474241-0000-0010-8000-00aa00389b71
#define GUID_RGB32 { 0x52, 0x47, 0x42, 0x41, 0x00, 0x00, 0x00, 0x10, \
                     0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } 
#define GUID_BGR32 { 0x41, 0x42, 0x47, 0x52, 0x00, 0x00, 0x00, 0x10, \
                     0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } 

//55595659-0000-0010-8000-00aa00389b71
#define GUID_UYVY  { 0x55, 0x59, 0x56, 0x59, 0x00, 0x00, 0x00, 0x10, \
		     0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 }

//59555932-0000-0010-8000-00aa00389b71
#define GUID_YUY2  { 0x59, 0x55, 0x59, 0x32, 0x00, 0x00, 0x00, 0x10, \
		     0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 }

//59555600-0000-0010-8000-00aa00389b71
#define GUID_YUV   { 0x59, 0x55, 0x56, 0x00, 0x00, 0x00, 0x00, 0x10, \
		     0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 }

//59383030-0000-0010-8000-00aa00389b71
#define GUID_Y800  { 0x59, 0x38, 0x32, 0x30, 0x00, 0x00, 0x00, 0x10, \
		     0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 }

//59563132-0000-0010-8000-00aa00389b71
#define GUID_YV21  { 0x59, 0x56, 0x31, 0x32, 0x00, 0x00, 0x00, 0x10, \
		     0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 }

//49343230-0000-0010-8000-00aa00389b71
#define GUID_I420  { 0x49, 0x34, 0x32, 0x30, 0x00, 0x00, 0x00, 0x10, \
		     0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 }

//49343230-0000-0010-8000-00aa00389b71
#define GUID_I422  { 0x49, 0x34, 0x32, 0x32, 0x00, 0x00, 0x00, 0x10, \
		     0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 }


static void grey2uyvy( __u8 *dest, __u8 *source, int width, int height );
static void uyvy2grey( __u8 *dest, __u8 *source, int width, int height );
static void grey2yuy2( __u8 *dest, __u8 *source, int width, int height );
static void yuy22grey( __u8 *dest, __u8 *source, int width, int height );
static void grey2yuv420p( __u8 *dest, __u8 *source, int width, int height );
static void y8002y420( __u8 *dest, __u8 *source, int width, int height );
static void y8002y422( __u8 *dest, __u8 *source, int width, int height );
static void y4202y800( __u8 *dest, __u8 *source, int width, int height );
static void uyvy2yuy2( __u8 *dest, __u8 *source, int width, int height );
static void uyvy2yuv( __u8 *dest, __u8 *source, int width, int height );
static void uyvy2rgb24( __u8 *dest, __u8 *source, int width, int height );
static void yuyv2rgb24( __u8 *dest, __u8 *source, int width, int height );
static void yuyv2bgr32( __u8 *dest, __u8 *source, int width, int height );
static void uyvy2rgb32( __u8 *dest, __u8 *source, int width, int height );
static void uyvy2bgr24( __u8 *dest, __u8 *source, int width, int height );
static void y4112rgb24( __u8 *dest, __u8 *source, int width, int height );
static void y4112rgb32( __u8 *dest, __u8 *source, int width, int height );
static void y4202rgb24( __u8 *dest, __u8 *source, int width, int height );
static void y4202rgb32( __u8 *dest, __u8 *source, int width, int height );
static void y8002rgb24( __u8 *dest, __u8 *source, int width, int height );
static void y8002rgb32( __u8 *dest, __u8 *source, int width, int height );
static void rgb242y800( __u8 *dest, __u8 *source, int width, int height );
static void rgb322y800( __u8 *dest, __u8 *source, int width, int height );
static void uyvytoyuv422p( __u8 *dest, __u8 *src, int width, int height );
static void uyvytoyuv420p( __u8 *dest, __u8 *src, int width, int height );
static void yuv420ptouyvy( __u8 *dest, __u8 *src, int width, int height );
static void yuyvtoyuv420p( __u8 *dest, __u8 *src, int width, int height );
static void yuv420ptoyuyv( __u8 *dest, __u8 *src, int width, int height );
static void yuv420ptorgb24( __u8 *dest, __u8 *source, int width, int height );
static void bgr242uyvy( __u8 *dest, __u8 *source, int width, int height );
static void rgb242uyvy( __u8 *dest, __u8 *source, int width, int height );
static void rgb322uyvy( __u8 *dest, __u8 *source, int width, int height );
static void rgb242yuyv( __u8 *dest, __u8 *source, int width, int height );
static void rgb24toyuv420p( __u8 *dest, __u8 *source, int width, int height );
static void bgr24toyuv420p( __u8 *dest, __u8 *source, int width, int height );

static void bgr24torgb24( __u8 *dest, __u8 *source, int width, int height );
static void rgb32torgb24( __u8 *dest, __u8 *source, int width, int height );
static void bgr24tobgr32( __u8 *dest, __u8 *source, int width, int height );

static void yuy2toyuyv( __u8 *dest, __u8 *source, int width, int height );
static void by8touyvy( __u8 *dest, __u8* source, int width, int height );
static void by8torgb24( __u8 *dest, __u8* source, int width, int height );
static void by8torgb32( __u8 *dest, __u8* source, int width, int height );
static void by8tobgr24( __u8 *dest, __u8* source, int width, int height );
static void by8touyvy( __u8 *dest, __u8* source, int width, int height );

static void by8toby8p( __u8 *dest, __u8* source, int width, int height );


static void y16touyvy( __u8 *dest, __u8* _source, int width, int height, int shift );
static void y16toyuy2( __u8 *dest, __u8* _source, int width, int height, int shift );
static void y16torgb24( __u8 *dest, __u8* _source, int width, int height, int shift );
static void rggbtouyvy_debayer( __u8 *dest, __u8 *_source, int width, int height, int shift );



static xfm_info_t xfminfo = 
{
   src_fourcc: 0xffffffff,
   dest_fourcc:   0xffffffff,
   func: NULL
};

static xfm_info_t conversions[] =
{
   { 
      src_fourcc: FOURCC('G','R','E','Y'), 
      src_guid: { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 
      src_bpp: 8, 

      dest_fourcc: FOURCC('Y','8','0','0'), 
      dest_guid: GUID_Y800, 
      dest_bpp: 8,
   
      priority: 20, 

      func: NULL,

      flags: XFM_PASSTHROUGH
   },

   { 
      src_fourcc: FOURCC('G','R','E','Y'), 
      src_guid: { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 
      src_bpp: 8, 
      
      dest_fourcc: FOURCC('U','Y','V','Y'), 
      dest_guid: GUID_UYVY, 
      dest_bpp: 16,
      
      priority: 5, 

      func: (xfm_func_t) grey2uyvy,

      flags: 0
   },
   { 
      src_fourcc: FOURCC('Y','8','0','0'), 
      src_guid: { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 
      src_bpp: 8, 
      
      dest_fourcc: FOURCC('U','Y','V','Y'), 
      dest_guid: GUID_UYVY, 
      dest_bpp: 16,
      
      priority: 5, 

      func: (xfm_func_t) grey2uyvy,

      flags: 0
   },
   { 
      src_fourcc: FOURCC('Y','8','0','0'), 
      src_guid: { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 
      src_bpp: 8, 
      
      dest_fourcc: FOURCC('I','4','2','0'), 
      dest_guid: GUID_UYVY, 
      dest_bpp: 12,
      
      priority: 5, 

      func: (xfm_func_t) y8002y420,

      flags: 0
   },
   { 
      src_fourcc: FOURCC('G','R','E','Y'), 
      src_guid: { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 
      src_bpp: 8, 
      
      dest_fourcc: FOURCC('I','4','2','0'), 
      dest_guid: GUID_I420, 
      dest_bpp: 12,
      
      priority: 5, 

      func: (xfm_func_t) y8002y420,

      flags: 0
   },
   { 
      src_fourcc: FOURCC('I','4','2','0'), 
      src_guid: { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 
      src_bpp: 12, 
      
      dest_fourcc: FOURCC('Y','8','0','0'), 
      dest_guid: { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 
      dest_bpp: 8,
      
      priority: 5, 

      func: (xfm_func_t) y4202y800,

      flags: 0
   },
   { 
      src_fourcc: FOURCC('Y','8','0','0'), 
      src_guid: { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 
      src_bpp: 8, 
      
      dest_fourcc: FOURCC('I','4','2','2'), 
      dest_guid: GUID_UYVY, 
      dest_bpp: 16,
      
      priority: 5, 

      func: (xfm_func_t) y8002y422,

      flags: 0
   },
   { 
      src_fourcc: FOURCC('G','R','E','Y'), 
      src_guid: { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 
      src_bpp: 8, 
      
      dest_fourcc: FOURCC('I','4','2','2'), 
      dest_guid: GUID_UYVY, 
      dest_bpp: 16,
      
      priority: 5, 

      func: (xfm_func_t) y8002y422,

      flags: 0
   },
   { 
      src_fourcc: FOURCC('G','R','E','Y'), 
      src_guid: { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 
      src_bpp: 8, 
      
      dest_fourcc: FOURCC('Y','U','Y','2'), 
      dest_guid: GUID_UYVY, 
      dest_bpp: 16,
      
      priority: 5, 

      func: (xfm_func_t) grey2yuy2,

      flags: 0
   },
   { 
      src_fourcc: FOURCC('Y','8','0','0'), 
      src_guid: { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 
      src_bpp: 8, 
      
      dest_fourcc: FOURCC('Y','U','Y','2'), 
      dest_guid: GUID_UYVY, 
      dest_bpp: 16,
      
      priority: 5, 

      func: (xfm_func_t) grey2yuy2,

      flags: 0
   },
   { 
      src_fourcc: FOURCC('Y','8','0','0'), 
      src_guid: { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 
      src_bpp: 8, 
      
      dest_fourcc: FOURCC('I','4','2','0'), 
      dest_guid: GUID_I420, 
      dest_bpp: 12,
      
      priority: 5, 

      func: (xfm_func_t) grey2yuv420p,

      flags: 0
   },
   { 
      src_fourcc: FOURCC('G','R','E','Y'), 
      src_guid: { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 
      src_bpp: 8, 
      
      dest_fourcc: FOURCC('I','4','2','0'), 
      dest_guid: GUID_I420, 
      dest_bpp: 12,
      
      priority: 5, 

      func: (xfm_func_t) grey2yuv420p,

      flags: 0
   },
   {
      src_fourcc: FOURCC('U','Y','V','Y'),
      src_guid: GUID_UYVY,
      src_bpp: 16, 
      
      dest_fourcc: FOURCC('I','4','2','2'),
      dest_guid: GUID_I422,
      dest_bpp: 16,

      priority: 15,
      func: (xfm_func_t) uyvytoyuv422p,
      flags: 0
   },

   {
      src_fourcc: FOURCC('U','Y','V','Y'),
      src_guid: GUID_UYVY,
      src_bpp: 16, 
      
      dest_fourcc: FOURCC('I','4','2','0'),
      dest_guid: GUID_I420,
      dest_bpp: 12,

      priority: 15,
      func: (xfm_func_t) uyvytoyuv420p,
      flags: 0
   },
   
   {
      src_fourcc: FOURCC('I','4','2','0'),
      src_guid: GUID_I420,
      src_bpp: 12,

      dest_fourcc: FOURCC('R','G','B','3'),
      dest_guid: GUID_RGB24,
      dest_bpp: 24, 
      
      priority: 15,
      func: (xfm_func_t) yuv420ptorgb24,
      flags: 0      
   },

   {
      src_fourcc: FOURCC('I','4','2','0'),
      src_guid: GUID_I420,
      src_bpp: 12,

      dest_fourcc: FOURCC('R','G','B',0),
      dest_guid: GUID_RGB24,
      dest_bpp: 24, 
      
      priority: 15,
      func: (xfm_func_t) yuv420ptorgb24,
      flags: 0      
   },

   {
      src_fourcc: FOURCC('I','4','2','0'),
      src_guid: GUID_I420,
      src_bpp: 12,

      dest_fourcc: FOURCC('U','Y','V','Y'),
      dest_guid: GUID_UYVY,
      dest_bpp: 16, 
      
      priority: 15,
      func: (xfm_func_t) yuv420ptouyvy,
      flags: 0      
   },

   {
      src_fourcc: FOURCC('Y','U','Y','V'),
      src_guid: GUID_YUY2,
      src_bpp: 16, 
      
      dest_fourcc: FOURCC('I','4','2','0'),
      dest_guid: GUID_I420,
      dest_bpp: 12,

      priority: 15,
      func: (xfm_func_t) yuyvtoyuv420p,
      flags: 0
   },

   {
      src_fourcc: FOURCC('Y','U','Y','2'),
      src_guid: GUID_YUY2,
      src_bpp: 16, 
      
      dest_fourcc: FOURCC('I','4','2','0'),
      dest_guid: GUID_I420,
      dest_bpp: 12,

      priority: 15,
      func: (xfm_func_t) yuyvtoyuv420p,
      flags: 0
   },
   
   {
      src_fourcc: FOURCC('I','4','2','0'),
      src_guid: GUID_I420,
      src_bpp: 12,

      dest_fourcc: FOURCC('Y','U','Y','V'),
      dest_guid: GUID_UYVY,
      dest_bpp: 16, 
      
      priority: 15,
      func: (xfm_func_t) yuv420ptoyuyv,
      flags: 0      
   },

   {
      src_fourcc: FOURCC('I','4','2','0'),
      src_guid: GUID_I420,
      src_bpp: 12,

      dest_fourcc: FOURCC('R','G','B','3'),
      dest_guid: GUID_RGB24,
      dest_bpp: 24, 
      
      priority: 15,
      func: (xfm_func_t) y4202rgb24,
      flags: 0      
   },

   {
      src_fourcc: FOURCC('I','4','2','0'),
      src_guid: GUID_I420,
      src_bpp: 12,

      dest_fourcc: FOURCC('R','G','B',0),
      dest_guid: GUID_RGB24,
      dest_bpp: 24, 
      
      priority: 15,
      func: (xfm_func_t) y4202rgb24,
      flags: 0      
   },

   {
      src_fourcc: FOURCC('I','4','2','0'),
      src_guid: GUID_I420,
      src_bpp: 12,

      dest_fourcc: FOURCC('R','G','B','4'),
      dest_guid: GUID_RGB32,
      dest_bpp: 32, 
      
      priority: 15,
      func: (xfm_func_t) y4202rgb32,
      flags: 0      
   },

   { 
      src_fourcc: FOURCC('Y','U','Y','V'), 
      src_guid: GUID_UYVY, 
      src_bpp: 16, 
      
      dest_fourcc: FOURCC('Y','U','Y','2'), 
      dest_guid: GUID_YUY2, 
      dest_bpp: 16,
      
      priority: 20, 

      func: NULL,

      flags: XFM_PASSTHROUGH
   },
   { 
      src_fourcc: FOURCC('Y','U','Y','V'), 
      src_guid: GUID_YUY2, 
      src_bpp: 16, 
      
      dest_fourcc: FOURCC('U','Y','V','Y'), 
      dest_guid: GUID_UYVY, 
      dest_bpp: 16,
      
      priority: 10, 

      func: (xfm_func_t) uyvy2yuy2,

      flags: XFM_IN_PLACE
   },
   { 
      src_fourcc: FOURCC('U','Y','V','Y'), 
      src_guid: GUID_UYVY, 
      src_bpp: 16, 
      
      dest_fourcc: FOURCC('Y','U','Y','2'), 
      dest_guid: GUID_YUY2, 
      dest_bpp: 16,
      
      priority: 10, 

      func: (xfm_func_t) uyvy2yuy2,

      flags: XFM_IN_PLACE
   },
   { 
      src_fourcc: FOURCC('Y','U','Y','2'), 
      src_guid: GUID_YUY2, 
      src_bpp: 16, 
      
      dest_fourcc: FOURCC('Y','U','Y','V'), 
      dest_guid: GUID_YUY2, 
      dest_bpp: 16,
      
      priority: 10, 

      func: (xfm_func_t) yuy2toyuyv,

      flags: 0
   },
   { 
      src_fourcc: FOURCC('U','Y','V','Y'), 
      src_guid: GUID_UYVY, 
      src_bpp: 16, 
      
      dest_fourcc: FOURCC('Y','U','V',0), 
      dest_guid: GUID_YUV, 
      dest_bpp: 24,
      
      priority: 10, 

      func: (xfm_func_t) uyvy2yuv,

      flags: 0,
   },
   // uyvy -> rgb24
   { 
      src_fourcc: FOURCC('U','Y','V','Y'), 
      src_guid: GUID_UYVY, 
      src_bpp: 16, 
      
      dest_fourcc: FOURCC('R','G','B',0), 
      dest_guid: GUID_RGB24, 
      dest_bpp: 24,
      
      priority: 5, 

      func: (xfm_func_t) uyvy2rgb24,

      flags: 0
   },
   // uyvy -> rgb24
   { 
      src_fourcc: FOURCC('U','Y','V','Y'), 
      src_guid: GUID_UYVY, 
      src_bpp: 16, 
      
      dest_fourcc: FOURCC('R','G','B','3'), 
      dest_guid: GUID_RGB24, 
      dest_bpp: 24,
      
      priority: 5, 

      func: (xfm_func_t) uyvy2rgb24,

      flags: 0
   },
   // uyvy -> bgr24
   { 
      src_fourcc: FOURCC('U','Y','V','Y'), 
      src_guid: GUID_UYVY, 
      src_bpp: 16, 
      
      dest_fourcc: FOURCC('B','G','R',0), 
      dest_guid: GUID_RGB24, 
      dest_bpp: 24,
      
      priority: 5, 

      func: (xfm_func_t) uyvy2bgr24,

      flags: 0
   },
   // uyvy -> bgr24
   { 
      src_fourcc: FOURCC('U','Y','V','Y'), 
      src_guid: GUID_UYVY, 
      src_bpp: 16, 
      
      dest_fourcc: FOURCC('B','G','R','3'), 
      dest_guid: GUID_RGB24, 
      dest_bpp: 24,
      
      priority: 5, 

      func: (xfm_func_t) uyvy2bgr24,

      flags: 0
   },

   // yuyv -> rgb24
   { 
      src_fourcc: FOURCC('Y','U','Y','V'), 
      src_guid: GUID_YUY2, 
      src_bpp: 16, 
      
      dest_fourcc: FOURCC('R','G','B',0), 
      dest_guid: GUID_RGB24, 
      dest_bpp: 24,
      
      priority: 5, 

      func: (xfm_func_t) yuyv2rgb24,

      flags: 0
   },
   // yuyv -> rgb24
   { 
      src_fourcc: FOURCC('Y','U','Y','2'), 
      src_guid: GUID_YUY2, 
      src_bpp: 16, 
      
      dest_fourcc: FOURCC('R','G','B',0), 
      dest_guid: GUID_RGB24, 
      dest_bpp: 24,
      
      priority: 5, 

      func: (xfm_func_t) yuyv2rgb24,

      flags: 0
   },
   // yuyv -> rgb24
   { 
      src_fourcc: FOURCC('Y','U','Y','2'), 
      src_guid: GUID_YUY2, 
      src_bpp: 16, 
      
      dest_fourcc: FOURCC('R','G','B','3'), 
      dest_guid: GUID_RGB24, 
      dest_bpp: 24,
      
      priority: 5, 

      func: (xfm_func_t) yuyv2rgb24,

      flags: 0
   },
   // yuyv -> rgb24
   { 
      src_fourcc: FOURCC('Y','U','Y','V'), 
      src_guid: GUID_UYVY, 
      src_bpp: 16, 
      
      dest_fourcc: FOURCC('R','G','B','3'), 
      dest_guid: GUID_RGB24, 
      dest_bpp: 24,
      
      priority: 5, 

      func: (xfm_func_t) yuyv2rgb24,

      flags: 0
   },


   // yuyv -> rgb24
   { 
      src_fourcc: FOURCC('Y','U','Y','V'), 
      src_guid: GUID_YUY2, 
      src_bpp: 16, 
      
      dest_fourcc: FOURCC('B','G','R','4'), 
      dest_guid: GUID_BGR32, 
      dest_bpp: 32,
      
      priority: 5, 

      func: (xfm_func_t) yuyv2bgr32,

      flags: 0
   },
   // yuyv -> rgb24
   { 
      src_fourcc: FOURCC('Y','U','Y','2'), 
      src_guid: GUID_YUY2, 
      src_bpp: 16, 
      
      dest_fourcc: FOURCC('B','G','R','4'), 
      dest_guid: GUID_BGR32, 
      dest_bpp: 32,
      
      priority: 5, 

      func: (xfm_func_t) yuyv2bgr32,

      flags: 0
   },


   // yuyv -> y800
   { 
      src_fourcc: FOURCC('Y','U','Y','V'), 
      src_guid: GUID_UYVY, 
      src_bpp: 16, 
      
      dest_fourcc: FOURCC('Y','8','0','0'), 
      dest_guid: GUID_Y800, 
      dest_bpp: 8,
      
      priority: 5, 

      func: (xfm_func_t) yuy22grey,

      flags: 0
   },
   // uyvy -> y800
   { 
      src_fourcc: FOURCC('U','Y','V','Y'), 
      src_guid: GUID_UYVY, 
      src_bpp: 16, 
      
      dest_fourcc: FOURCC('Y','8','0','0'), 
      dest_guid: GUID_Y800, 
      dest_bpp: 8,
      
      priority: 5, 

      func: (xfm_func_t) uyvy2grey,

      flags: 0
   },




   { 
      src_fourcc: FOURCC('U','Y','V','Y'), 
      src_guid: GUID_UYVY, 
      src_bpp: 16, 
      
      dest_fourcc: FOURCC('R','G','B','A'), 
      dest_guid: GUID_RGB32, 
      dest_bpp: 32,
      
      priority: 5, 

      func: (xfm_func_t) uyvy2rgb32,

      flags: 0
   },      
   { 
      src_fourcc: FOURCC('U','Y','V','Y'), 
      src_guid: GUID_UYVY, 
      src_bpp: 16, 
      
      dest_fourcc: FOURCC('R','G','B','4'), 
      dest_guid: GUID_RGB32, 
      dest_bpp: 32,
      
      priority: 5, 

      func: (xfm_func_t) uyvy2rgb32,

      flags: 0
   },      
   {
      src_fourcc: FOURCC('Y','4','1','1'),
      src_guid: GUID_UYVY,
      src_bpp: 12,
      
      dest_fourcc: FOURCC('R','G','B',0),
      dest_guid: GUID_RGB24,
      dest_bpp: 24,
      
      priority: 5,

      func: (xfm_func_t) y4112rgb24,

      flags: 0
   },
   { 
      src_fourcc: FOURCC('Y','4','1','1'), 
      src_guid: GUID_UYVY, 
      src_bpp: 12, 
      
      dest_fourcc: FOURCC('R','G','B','A'), 
      dest_guid: GUID_RGB32, 
      dest_bpp: 32,
      
      priority: 5, 

      func: (xfm_func_t) y4112rgb32,

      flags: 0
   },      
   {
      src_fourcc: FOURCC('Y','8','0','0'),
      src_guid: GUID_UYVY,
      src_bpp: 8,
      
      dest_fourcc: FOURCC('R','G','B',0),
      dest_guid: GUID_RGB24,
      dest_bpp: 24,
      
      priority: 5,

      func: (xfm_func_t) y8002rgb24,

      flags: 0
   },
   {
      src_fourcc: FOURCC('Y','8','0','0'),
      src_guid: GUID_UYVY,
      src_bpp: 8,
      
      dest_fourcc: FOURCC('R','G','B','3'),
      dest_guid: GUID_RGB24,
      dest_bpp: 24,
      
      priority: 5,

      func: (xfm_func_t) y8002rgb24,

      flags: 0
   },

   {
      src_fourcc: FOURCC('G','R','E','Y'),
      src_guid: GUID_Y800,
      src_bpp: 8,
      
      dest_fourcc: FOURCC('R','G','B',0),
      dest_guid: GUID_RGB24,
      dest_bpp: 24,
      
      priority: 5,

      func: (xfm_func_t) y8002rgb24,

      flags: 0
   },
   {
      src_fourcc: FOURCC('G','R','E','Y'),
      src_guid: GUID_UYVY,
      src_bpp: 8,
      
      dest_fourcc: FOURCC('R','G','B','3'),
      dest_guid: GUID_RGB24,
      dest_bpp: 24,
      
      priority: 5,

      func: (xfm_func_t) y8002rgb24,

      flags: 0
   },
   // Bt848 RAW, treat as Mono8
   {
      src_fourcc: FOURCC('R','A','W',' '),
      src_guid: GUID_UYVY,
      src_bpp: 8,
      
      dest_fourcc: FOURCC('R','G','B',0),
      dest_guid: GUID_RGB24,
      dest_bpp: 24,
      
      priority: 5,

      func: (xfm_func_t) y8002rgb24,

      flags: 0
   },
   
   {
      
      src_fourcc: FOURCC('R','G','B',0),
      src_guid: GUID_RGB24,
      src_bpp: 24,
      
      dest_fourcc: FOURCC('Y','8','0','0'),
      dest_guid: GUID_UYVY,
      dest_bpp: 8,

      priority: 5,

      func: (xfm_func_t) rgb242y800,

      flags: 0
   },
   {      
      src_fourcc: FOURCC('R','G','B','3'),
      src_guid: GUID_RGB24,
      src_bpp: 24,
      
      dest_fourcc: FOURCC('Y','8','0','0'),
      dest_guid: GUID_UYVY,
      dest_bpp: 8,

      priority: 5,

      func: (xfm_func_t) rgb242y800,

      flags: 0
   },

   {
      src_fourcc: FOURCC('R','G','B',0),
      src_guid: GUID_RGB24,
      src_bpp: 24,

      dest_fourcc: FOURCC('G','R','E','Y'),
      dest_guid: GUID_Y800,
      dest_bpp: 8,
      
      priority: 5,

      func: (xfm_func_t) rgb242y800,

      flags: 0
   },
   {     
      src_fourcc: FOURCC('R','G','B','3'),
      src_guid: GUID_RGB24,
      src_bpp: 24,
      
      dest_fourcc: FOURCC('G','R','E','Y'),
      dest_guid: GUID_UYVY,
      dest_bpp: 8,

      priority: 5,

      func: (xfm_func_t) rgb242y800,

      flags: 0
   },
   // Bt848 RAW, treat as Mono8
   {     
      src_fourcc: FOURCC('R','G','B',0),
      src_guid: GUID_RGB24,
      src_bpp: 24,
      
      dest_fourcc: FOURCC('R','A','W',' '),
      dest_guid: GUID_UYVY,
      dest_bpp: 8,

      priority: 5,

      func: (xfm_func_t) rgb242y800,

      flags: 0
   },
   {
      src_fourcc: FOURCC('Y','8','0','0'),
      src_guid: GUID_UYVY,
      src_bpp: 8,
      
      dest_fourcc: FOURCC('R','G','B','A'),
      dest_guid: GUID_RGB32,
      dest_bpp: 32,
      
      priority: 5,

      func: (xfm_func_t) y8002rgb32,

      flags: 0
   },
   { 
      src_fourcc: FOURCC('R','G','B','4'), 
      src_guid: GUID_RGB32, 
      src_bpp: 32, 
      
      dest_fourcc: FOURCC('R','G','B','A'), 
      dest_guid: GUID_RGB32, 
      dest_bpp: 32,
      
      priority: 20, 

      func: NULL,

      flags: XFM_PASSTHROUGH
   },
   // bgr24 -> uyvy
   { 
      src_fourcc: FOURCC('B','G','R','3'), 
      src_guid: GUID_RGB24, 
      src_bpp: 24,
      
      dest_fourcc: FOURCC('U','Y','V','Y'), 
      dest_guid: GUID_UYVY, 
      dest_bpp: 16, 
      
      priority: 5, 

      func: (xfm_func_t) bgr242uyvy,

      flags: 0
   },
   // bgr24 -> uyvy
   { 
      src_fourcc: FOURCC('B','G','R',0), 
      src_guid: GUID_RGB24, 
      src_bpp: 24,
      
      dest_fourcc: FOURCC('U','Y','V','Y'), 
      dest_guid: GUID_UYVY, 
      dest_bpp: 16, 
      
      priority: 5, 

      func: (xfm_func_t) bgr242uyvy,

      flags: 0
   },
   // rgb24 -> uyvy
   { 
      src_fourcc: FOURCC('R','G','B','3'), 
      src_guid: GUID_RGB24, 
      src_bpp: 24,
      
      dest_fourcc: FOURCC('U','Y','V','Y'), 
      dest_guid: GUID_UYVY, 
      dest_bpp: 16, 
      
      priority: 5, 

      func: (xfm_func_t) rgb242uyvy,

      flags: 0
   },
   // rgb24 -> uyvy
   { 
      src_fourcc: FOURCC('R','G','B',0), 
      src_guid: GUID_RGB24, 
      src_bpp: 24,
      
      dest_fourcc: FOURCC('U','Y','V','Y'), 
      dest_guid: GUID_UYVY, 
      dest_bpp: 16, 
      
      priority: 5, 

      func: (xfm_func_t) rgb242uyvy,

      flags: 0
   },      
   // rgb32 -> uyvy
   { 
      src_fourcc: FOURCC('R','G','B','A'), 
      src_guid: GUID_RGB32, 
      src_bpp: 32,
      
      dest_fourcc: FOURCC('U','Y','V','Y'), 
      dest_guid: GUID_UYVY, 
      dest_bpp: 16, 
      
      priority: 5, 

      func: (xfm_func_t) rgb322uyvy,

      flags: 0
   },      
   // rgb32 -> uyvy
   { 
      src_fourcc: FOURCC('R','G','B','4'), 
      src_guid: GUID_RGB32, 
      src_bpp: 32,
      
      dest_fourcc: FOURCC('U','Y','V','Y'), 
      dest_guid: GUID_UYVY, 
      dest_bpp: 16, 
      
      priority: 5, 

      func: (xfm_func_t) rgb322uyvy,

      flags: 0
   },      
   // rgb24 -> yuyv
   { 
      src_fourcc: FOURCC('R','G','B',0), 
      src_guid: GUID_RGB24, 
      src_bpp: 24,
      
      dest_fourcc: FOURCC('Y','U','Y','V'), 
      dest_guid: GUID_YUY2, 
      dest_bpp: 16, 
      
      priority: 5, 

      func: (xfm_func_t) rgb242yuyv,

      flags: 0
   },      
   // rgb24 -> yuyv
   { 
      src_fourcc: FOURCC('R','G','B','3'), 
      src_guid: GUID_RGB24, 
      src_bpp: 24,
      
      dest_fourcc: FOURCC('Y','U','Y','V'), 
      dest_guid: GUID_YUY2, 
      dest_bpp: 16, 
      
      priority: 5, 

      func: (xfm_func_t) rgb242yuyv,

      flags: 0
   },      
   // rgb24 -> yuyv
   { 
      src_fourcc: FOURCC('R','G','B',0), 
      src_guid: GUID_RGB24, 
      src_bpp: 24,
      
      dest_fourcc: FOURCC('Y','U','Y','2'), 
      dest_guid: GUID_YUY2, 
      dest_bpp: 16, 
      
      priority: 5, 

      func: (xfm_func_t) rgb242yuyv,

      flags: 0
   },      
   // rgb24 -> yuyv
   { 
      src_fourcc: FOURCC('R','G','B','3'), 
      src_guid: GUID_RGB24, 
      src_bpp: 24,
      
      dest_fourcc: FOURCC('Y','U','Y','2'), 
      dest_guid: GUID_YUY2, 
      dest_bpp: 16, 
      
      priority: 5, 

      func: (xfm_func_t) rgb242yuyv,

      flags: 0
   },      
   // rgb24 -> i420
   { 
      src_fourcc: FOURCC('R','G','B','3'), 
      src_guid: GUID_RGB24, 
      src_bpp: 24,
      
      dest_fourcc: FOURCC('I','4','2','0'), 
      dest_guid: GUID_UYVY, 
      dest_bpp: 12, 
      
      priority: 5, 

      func: (xfm_func_t) rgb24toyuv420p,

      flags: 0
   },
   // rgb24 -> i420
   { 
      src_fourcc: FOURCC('R','G','B',0), 
      src_guid: GUID_RGB24, 
      src_bpp: 24,
      
      dest_fourcc: FOURCC('I','4','2','0'), 
      dest_guid: GUID_I420, 
      dest_bpp: 12, 
      
      priority: 5, 

      func: (xfm_func_t) rgb24toyuv420p,

      flags: 0
   },      
   // bgr24 -> uyvy
   { 
      src_fourcc: FOURCC('B','G','R','3'), 
      src_guid: GUID_RGB24, 
      src_bpp: 24,
      
      dest_fourcc: FOURCC('I','4','2','0'), 
      dest_guid: GUID_I420, 
      dest_bpp: 12, 
      
      priority: 5, 

      func: (xfm_func_t) bgr24toyuv420p,

      flags: 0
   },
   // bgr24 -> uyvy
   { 
      src_fourcc: FOURCC('B','G','R',0), 
      src_guid: GUID_RGB24, 
      src_bpp: 24,
      
      dest_fourcc: FOURCC('I','4','2','0'), 
      dest_guid: GUID_I420, 
      dest_bpp: 12, 
      
      priority: 5, 

      func: (xfm_func_t) bgr24toyuv420p,

      flags: 0
   },      
   { 
      src_fourcc: FOURCC('B','G','R','3'), 
      src_guid: GUID_RGB24, 
      src_bpp: 24,
      
      dest_fourcc: FOURCC('R','G','B','3'), 
      dest_guid: GUID_RGB24, 
      dest_bpp: 24, 
      
      priority: 5, 

      func: (xfm_func_t) bgr24torgb24,

      flags: 0
   },
   { 
      src_fourcc: FOURCC('R','G','B','4'), 
      src_guid: GUID_RGB32, 
      src_bpp: 32,
      
      dest_fourcc: FOURCC('R','G','B','3'), 
      dest_guid: GUID_RGB24, 
      dest_bpp: 24, 
      
      priority: 5, 

      func: (xfm_func_t) rgb32torgb24,

      flags: 0
   },

   { 
      src_fourcc: FOURCC('B','G','R','3'), 
      src_guid: GUID_RGB24, 
      src_bpp: 24,
      
      dest_fourcc: FOURCC('B','G','R','4'), 
      dest_guid: GUID_RGB32, 
      dest_bpp: 32, 
      
      priority: 5, 

      func: (xfm_func_t) bgr24tobgr32,

      flags: 0
   },

   { 
      src_fourcc: FOURCC('R','G','B',0), 
      src_guid: GUID_RGB24, 
      src_bpp: 24,
      
      dest_fourcc: FOURCC('R','G','B','3'), 
      dest_guid: GUID_RGB24, 
      dest_bpp: 24, 
      
      priority: 20, 

      func: NULL,

      flags: XFM_PASSTHROUGH
   },
   { 
      src_fourcc: FOURCC('R','G','B','3'), 
      src_guid: GUID_RGB24, 
      src_bpp: 24,
      
      dest_fourcc: FOURCC('R','G','B',0), 
      dest_guid: GUID_RGB24, 
      dest_bpp: 24, 
      
      priority: 20, 

      func: NULL,

      flags: XFM_PASSTHROUGH
   },


   { 
      src_fourcc: FOURCC('B','Y','8',' '), 
      src_guid: {0}, 
      src_bpp: 8,
      
      dest_fourcc: FOURCC('U','Y','V','Y'), 
      dest_guid: GUID_UYVY, 
      dest_bpp: 16, 
      
      priority: 5, 

      func: (xfm_func_t) by8touyvy,

      flags: 0
   },
   { 
      src_fourcc: FOURCC('B','A','8','1'), 
      src_guid: {0}, 
      src_bpp: 8,
      
      dest_fourcc: FOURCC('U','Y','V','Y'), 
      dest_guid: GUID_UYVY, 
      dest_bpp: 16, 
      
      priority: 5, 

      func: (xfm_func_t) by8touyvy,

      flags: 0
   },
   { 
      src_fourcc: FOURCC('B','A','8','1'), 
      src_guid: {0}, 
      src_bpp: 8,
      
      dest_fourcc: FOURCC('R','G','B','3'), 
      dest_guid: GUID_RGB24, 
      dest_bpp: 24, 
      
      priority: 5, 

      func: (xfm_func_t) by8torgb24,

      flags: 0
   },
   { 
      src_fourcc: FOURCC('B','Y','8',' '), 
      src_guid: {0}, 
      src_bpp: 8,
      
      dest_fourcc: FOURCC('B','G','R','3'), 
      dest_guid: GUID_BGR24, 
      dest_bpp: 24, 
      
      priority: 5, 

      func: (xfm_func_t) by8tobgr24,

      flags: 0
   },
   { 
      src_fourcc: FOURCC('B','Y','8',' '), 
      src_guid: {0}, 
      src_bpp: 8,
      
      dest_fourcc: FOURCC('R','G','B','A'), 
      dest_guid: GUID_RGB32, 
      dest_bpp: 32, 
      
      priority: 5, 

      func: (xfm_func_t) by8torgb32,

      flags: 0
   },
   { 
      src_fourcc: FOURCC('B','A','8','1'), 
      src_guid: {0}, 
      src_bpp: 8,
      
      dest_fourcc: FOURCC('R','G','B','A'), 
      dest_guid: GUID_RGB32, 
      dest_bpp: 32, 
      
      priority: 5, 

      func: (xfm_func_t) by8torgb32,

      flags: 0
   },

   { 
      src_fourcc: FOURCC('B','Y','8',' '), 
      src_guid: {0}, 
      src_bpp: 8,
      
      dest_fourcc: FOURCC('B','Y','8','P'), 
      dest_guid: {0}, 
      dest_bpp: 8, 
      
      priority: 5, 

      func: (xfm_func_t) by8toby8p,

      flags: 0
   },
   { 
      src_fourcc: FOURCC('B','A','8','1'), 
      src_guid: {0}, 
      src_bpp: 8,
      
      dest_fourcc: FOURCC('B','Y','8','P'), 
      dest_guid: {0}, 
      dest_bpp: 8, 
      
      priority: 5, 

      func: (xfm_func_t) by8toby8p,

      flags: 0
   },


   // rgb32 -> uyvy
   {
      src_fourcc: FOURCC('R','G','G','B'),
      src_guid: {0},
      src_bpp: 16,
      
      dest_fourcc: FOURCC('U','Y','V','Y'),
      dest_guid: GUID_UYVY,
      dest_bpp: 16,
      
      priority: 5,

      func: (xfm_func_t) rggbtouyvy_debayer,

      flags: 0
   },

   { 
      src_fourcc: FOURCC('Y','1','6',' '), 
      src_guid: {0}, 
      src_bpp: 16,
      
      dest_fourcc: FOURCC('U','Y','V','Y'), 
      dest_guid: GUID_UYVY, 
      dest_bpp: 16, 
      
      priority: 5, 

      func: (xfm_func_t) y16touyvy,

      flags: 0
   },      

/*    {  */
/*       src_fourcc: FOURCC('R','G','G','B'),  */
/*       src_guid: {0},  */
/*       src_bpp: 16, */
      
/*       dest_fourcc: FOURCC('Y','U','Y','2'),  */
/*       dest_guid: GUID_UYVY,  */
/*       dest_bpp: 16,  */
      
/*       priority: 5,  */

/*       func: (xfm_func_t) rggbtoyuy2, */

/*       flags: 0 */
/*    },       */

   { 
      src_fourcc: FOURCC('Y','1','6',' '), 
      src_guid: {0}, 
      src_bpp: 16,
      
      dest_fourcc: FOURCC('Y','U','Y','2'), 
      dest_guid: GUID_UYVY, 
      dest_bpp: 16, 
      
      priority: 10, 

      func: (xfm_func_t) y16toyuy2,

      flags: 0
   },      

/*    {  */
/*       src_fourcc: FOURCC('R','G','G','B'),  */
/*       src_guid: {0},  */
/*       src_bpp: 16, */
      
/*       dest_fourcc: FOURCC('Y','U','Y','V'),  */
/*       dest_guid: GUID_UYVY,  */
/*       dest_bpp: 16,  */
      
/*       priority: 5,  */

/*       func: (xfm_func_t) y16toyuy2, */

/*       flags: 0 */
/*    },       */

   { 
      src_fourcc: FOURCC('Y','1','6',' '), 
      src_guid: {0}, 
      src_bpp: 16,
      
      dest_fourcc: FOURCC('Y','U','Y','V'), 
      dest_guid: GUID_UYVY, 
      dest_bpp: 16, 
      
      priority: 10, 

      func: (xfm_func_t) y16toyuy2,

      flags: 0
   },      

/*    {  */
/*       src_fourcc: FOURCC('R','G','G','B'),  */
/*       src_guid: {0},  */
/*       src_bpp: 16, */
      
/*       dest_fourcc: FOURCC('R','G','B','3'),  */
/*       dest_guid: GUID_RGB24,  */
/*       dest_bpp: 24,  */
      
/*       priority: 5,  */

/*       func: (xfm_func_t) rggbtorgb24, */

/*       flags: 0 */
/*    },       */

/*    {  */
/*       src_fourcc: FOURCC('R','G','G','B'),  */
/*       src_guid: {0},  */
/*       src_bpp: 16, */
      
/*       dest_fourcc: FOURCC('R','G','B',0),  */
/*       dest_guid: GUID_RGB24,  */
/*       dest_bpp: 24,  */
      
/*       priority: 5,  */

/*       func: (xfm_func_t) rggbtorgb24, */

/*       flags: 0 */
/*    },       */

   { 
      src_fourcc: FOURCC('Y','1','6',' '), 
      src_guid: {0}, 
      src_bpp: 16,
      
      dest_fourcc: FOURCC('R','G','B','3'), 
      dest_guid: GUID_RGB24, 
      dest_bpp: 24, 
      
      priority: 5, 

      func: (xfm_func_t) y16torgb24,

      flags: 0
   },      

   { 
      src_fourcc: FOURCC('Y','1','6',' '), 
      src_guid: {0}, 
      src_bpp: 16,
      
      dest_fourcc: FOURCC('R','G','B',0), 
      dest_guid: GUID_RGB24, 
      dest_bpp: 24, 
      
      priority: 5, 

      func: (xfm_func_t) y16torgb24,

      flags: 0
   },      
};

#if HAVE_AVCODEC


/* for compatibility with older versions of libavcodec */

#ifndef PIX_FMT_YUYV422
#define PIX_FMT_YUYV422 PIX_FMT_YUV422
#endif

#ifndef PIX_FMT_RGB32
#define PIX_FMT_RGB32 PIX_FMT_RGBA32
#endif


int PixelFormatBPP[] = {
   //PIX_FMT_NONE= -1,
   12,// PIX_FMT_YUV420P,   ///< Planar YUV 4:2:0, 12bpp, (1 Cr & Cb sample per 2x2 Y samples)
   16,// PIX_FMT_YUYV422,   ///< Packed YUV 4:2:2, 16bpp, Y0 Cb Y1 Cr
   24,// PIX_FMT_RGB24,     ///< Packed RGB 8:8:8, 24bpp, RGBRGB...
   24,// PIX_FMT_BGR24,     ///< Packed RGB 8:8:8, 24bpp, BGRBGR...
   16,// PIX_FMT_YUV422P,   ///< Planar YUV 4:2:2, 16bpp, (1 Cr & Cb sample per 2x1 Y samples)
   24,// PIX_FMT_YUV444P,   ///< Planar YUV 4:4:4, 24bpp, (1 Cr & Cb sample per 1x1 Y samples)
   32,// PIX_FMT_RGB32,     ///< Packed RGB 8:8:8, 32bpp, (msb)8A 8R 8G 8B(lsb), in cpu endianness
   9,// PIX_FMT_YUV410P,   ///< Planar YUV 4:1:0,  9bpp, (1 Cr & Cb sample per 4x4 Y samples)
   12,// PIX_FMT_YUV411P,   ///< Planar YUV 4:1:1, 12bpp, (1 Cr & Cb sample per 4x1 Y samples)
   16,// PIX_FMT_RGB565,    ///< Packed RGB 5:6:5, 16bpp, (msb)   5R 6G 5B(lsb), in cpu endianness
   16,// PIX_FMT_RGB555,    ///< Packed RGB 5:5:5, 16bpp, (msb)1A 5R 5G 5B(lsb), in cpu endianness most significant bit to 1
   8,// PIX_FMT_GRAY8,     ///<        Y        ,  8bpp
   1,// PIX_FMT_MONOWHITE, ///<        Y        ,  1bpp, 1 is white
   1,// PIX_FMT_MONOBLACK, ///<        Y        ,  1bpp, 0 is black
   8,// PIX_FMT_PAL8,      ///< 8 bit with PIX_FMT_RGB32 palette
   12,// PIX_FMT_YUVJ420P,  ///< Planar YUV 4:2:0, 12bpp, full scale (jpeg)
   16,// PIX_FMT_YUVJ422P,  ///< Planar YUV 4:2:2, 16bpp, full scale (jpeg)
   24,// PIX_FMT_YUVJ444P,  ///< Planar YUV 4:4:4, 24bpp, full scale (jpeg)
   8,// PIX_FMT_XVMC_MPEG2_MC,///< XVideo Motion Acceleration via common packet passing(xvmc_render.h)
   8,// PIX_FMT_XVMC_MPEG2_IDCT,
   16,// PIX_FMT_UYVY422,   ///< Packed YUV 4:2:2, 16bpp, Cb Y0 Cr Y1
   12,// PIX_FMT_UYYVYY411, ///< Packed YUV 4:1:1, 12bpp, Cb Y0 Y1 Cr Y2 Y3
   // PIX_FMT_NB,        ///< number of pixel formats, DO NOT USE THIS if you want to link with shared libav* because the number of formats might differ between versions
};

typedef struct PixelFormatFourccMap
{
      unsigned int fourcc;
      int pix_fmt;
} PixelFormatFourccMap;

PixelFormatFourccMap PixelFormatFourccs[] = 
{
   { UCIL_FOURCC( 'Y','U','Y','V' ), PIX_FMT_YUYV422 },
   { UCIL_FOURCC( 'R','G','B','3' ), PIX_FMT_RGB24 },
   { UCIL_FOURCC( 'B','G','R','3' ), PIX_FMT_BGR24 },
   { UCIL_FOURCC( 'R','G','B','4' ), PIX_FMT_RGB32 },
   { UCIL_FOURCC( 'U','Y','V','Y' ), PIX_FMT_UYVY422 },
   { UCIL_FOURCC( 'Y','8','0','0' ), PIX_FMT_GRAY8 },
};

const int ucil_NumPixelFormatFourccs = sizeof( PixelFormatFourccs ) / sizeof( PixelFormatFourccMap );

static int get_av_pix_fmt_from_fourcc( unsigned int fourcc )
{
   int i;
   

   for( i = 0; i < ucil_NumPixelFormatFourccs; i++ )
   {
      if( PixelFormatFourccs[i].fourcc == fourcc )
      {
	 return PixelFormatFourccs[i].pix_fmt;
      }
   }

   return -1;
}

#endif //HAVE_AVCODEC



void ucil_get_xfminfo_from_fourcc( unsigned int src_fourcc, 
				   unsigned int dest_fourcc, 
				   xfm_info_t *info )
{
   int nconvs = sizeof( conversions ) / sizeof( xfm_info_t );
   int priority = -1;
   int i, found = -1;

   for( i = 0; i < nconvs; i++ )
   {
      if( ( conversions[i].src_fourcc == src_fourcc ) && 
	  ( conversions[i].dest_fourcc == dest_fourcc ) && 
	  ( conversions[i].priority > priority ) )
      {
	 priority = conversions[i].priority;
	 found = i;
      }
   }
   
   if( found == -1 )
   {
      info->src_fourcc = src_fourcc;
      memset( info->src_guid, 0x0, sizeof( info->src_guid ) );
      info->dest_fourcc = dest_fourcc;
      memset( info->dest_guid, 0x0, sizeof( info->dest_guid ) );
      info->func = NULL;
      info->flags = XFM_PASSTHROUGH;

      if( src_fourcc == dest_fourcc )
      {
	 info->priority = 20;
      }
      else
      {
	 info->priority = 1;
	 info->flags = XFM_NO_CONVERSION;
      }
   }
   else
   {
      memcpy( info, &conversions[found], sizeof( xfm_info_t ) );
   }
}

/* unicap_status_t ucil_convert_buffer_with_xfm_info( xfm_info_t *info, unicap_data_buffer_t *dest, unicap_data_buffer_t *src ) */
/* { */
   
/* } */

unicap_status_t ucil_convert_buffer( unicap_data_buffer_t *dest, unicap_data_buffer_t *src )
{
   unicap_status_t status = STATUS_FAILURE;

   if( src->format.fourcc == dest->format.fourcc )
   {
      memcpy( dest->data, src->data, src->format.buffer_size );
      status = STATUS_SUCCESS;
   }
   else
   {
      if( ( xfminfo.src_fourcc != src->format.fourcc ) ||
	  ( xfminfo.dest_fourcc != src->format.fourcc ) || 
	  ( xfminfo.flags & XFM_NO_CONVERSION ) )
      {
	 ucil_get_xfminfo_from_fourcc( src->format.fourcc, dest->format.fourcc, &xfminfo );      
      }
      
      if( !(xfminfo.flags & XFM_NO_CONVERSION) )
      {
	 dest->format.bpp = xfminfo.dest_bpp;
	 if( xfminfo.flags & XFM_PASSTHROUGH )
	 {
	    memcpy( dest->data, src->data, src->format.buffer_size );
	    status = STATUS_SUCCESS;
	 }
	 else
	 {
            int src_bits  = (src->format.flags & UNICAP_FLAGS_SIGNIFICANT_BITS_MASK ?
                             src->format.flags & UNICAP_FLAGS_SIGNIFICANT_BITS_MASK : src->format.bpp);
            int dest_bits = (dest->format.flags & UNICAP_FLAGS_SIGNIFICANT_BITS_MASK ?
                             dest->format.flags & UNICAP_FLAGS_SIGNIFICANT_BITS_MASK : dest->format.bpp);

            xfminfo.func( dest->data, src->data,
                          src->format.size.width, src->format.size.height,
                          src_bits - dest_bits );
	    status = STATUS_SUCCESS;
	 }
      }
      else
      {
	 g_message( "Could not convert format: %c%c%c%c %08x to %c%c%c%c %08x\n", 
		    (src->format.fourcc >>  0) & 0xFF,
		    (src->format.fourcc >>  8) & 0xFF,
		    (src->format.fourcc >> 16) & 0xFF,
		    (src->format.fourcc >> 24) & 0xFF,
                    src->format.fourcc,
		    (dest->format.fourcc >>  0) & 0xFF,
		    (dest->format.fourcc >>  8) & 0xFF,
		    (dest->format.fourcc >> 16) & 0xFF,
		    (dest->format.fourcc >> 24) & 0xFF,
                    dest->format.fourcc );
      }
      
   }
   return status;
}

int ucil_conversion_supported( unsigned int dest_fourcc, unsigned int src_fourcc )
{
   int ret = 0;
   
   if( dest_fourcc == src_fourcc )
   {
      ret = 1;
   }
   else
   {
      xfm_info_t tmpinfo;
      
      ucil_get_xfminfo_from_fourcc( src_fourcc, dest_fourcc, &tmpinfo );
      if( !(tmpinfo.flags & XFM_NO_CONVERSION ) )
      {
	 ret = 1;
      }
   }
   
   return ret;
}


static void uyvytoyuv420p( __u8 *dest, __u8 *src, int width, int height )
{
   int size;
   int offy = 0;
   int offu = 0;
   int offv = 0;
   int x,y;
   int stride;

   __u8 *dest_u;
   __u8 *dest_v;
   __u8 *dest_y;

   size = width * height;
   stride = width * 2;
   dest_y = dest;
   dest_u = dest + size;
   dest_v = dest + size + size / 4;
   
   for( y = 0; y < height; y += 2 )
   {
      int hoff = y * stride;
      for( x = 0; x < stride; x+=2 )
      {
	 dest_y[offy++] = src[hoff + 1 + x ];
      }
      for( x = 0; x < stride; x+=2 )
      {
	 dest_y[offy++] = src[hoff + 1 + x + stride ];
      }
      for( x = 0; x < stride; x+=4 )
      {
	 int tmpoffset = hoff + x;
	 dest_u[offu++] = src[ tmpoffset ];
	 dest_v[offv++] = src[ tmpoffset + 2 ];
      }
   }
}

static void yuv420ptouyvy( __u8 *dest, __u8 *src, int width, int height )
{
   int ysize = width*height;
   int offy = 0;
   int offu = 0;
   int offv = 0;
   int y;
   int stride;

   stride = width * 2;

   __u8 *src_y = src;
   __u8 *src_u = src + ysize;
   __u8 *src_v = src + ysize + ysize / 4;

   for( y = 0; y < height; y += 2 )
   {
      int hoff = stride * y;
      int x;
      
      for( x = 0; x < stride; x+=4 )
      {
	 __u8 u = src_u[ offu++ ];
	 __u8 v = src_v[ offv++ ];

	 dest[ hoff + x ] = u;
	 dest[ hoff + stride + x ] = u;

	 dest[ hoff + x + 2 ] = v;
	 dest[ hoff + stride + x + 2 ] = v;
	 
      }
      

      for( x = 0; x < stride; x += 2 )
      {
	 dest[ hoff + x + 1 ] = src_y[ offy++ ];
      }

      for( x = 0; x < stride; x+= 2 )
      {
	 dest[ hoff + stride + x + 1 ] = src_y[ offy++ ];
      }
   }
}

static void uyvytoyuv422p( __u8 *dest, __u8 *src, int width, int height )
{
   int size;
   int offy = 0;
   int offu = 0;
   int offv = 0;
   int offsrc = 0;
   __u8 *dest_u;
   __u8 *dest_v;
   __u8 *dest_y;

   size = width * height;
   dest_y = dest;
   dest_u = dest + size;
   dest_v = dest + size + size / 2;
   
   while( offsrc < (size*2)  )
   {
      dest_u[offu++] = src[offsrc++];
      dest_y[offy++] = src[offsrc++];
      dest_v[offv++] = src[offsrc++];
      dest_y[offy++] = src[offsrc++];
   }
}


static void yuyvtoyuv420p( __u8 *dest, __u8 *src, int width, int height )
{
   int size;
   int offy = 0;
   int offu = 0;
   int offv = 0;
   int x,y;
   int stride;

   __u8 *dest_u;
   __u8 *dest_v;
   __u8 *dest_y;

   size = width * height;
   stride = width * 2;
   dest_y = dest;
   dest_u = dest + size;
   dest_v = dest + size + size / 4;
   
   for( y = 0; y < height; y += 2 )
   {
      int hoff = y * stride;
      for( x = 0; x < stride; x+=2 )
      {
	 dest_y[offy++] = src[hoff + x ];
      }
      for( x = 0; x < stride; x+=2 )
      {
	 dest_y[offy++] = src[hoff + x + stride ];
      }
      for( x = 0; x < stride; x+=4 )
      {
	 int tmpoffset = hoff + x;
	 dest_u[offu++] = src[ tmpoffset + 1 ];
	 dest_v[offv++] = src[ tmpoffset + 3 ];
      }
   }
}

static void yuv420ptoyuyv( __u8 *dest, __u8 *src, int width, int height )
{
   int ysize = width*height;
   int offy = 0;
   int offu = 0;
   int offv = 0;
   int y;
   int stride;

   stride = width * 2;

   __u8 *src_y = src;
   __u8 *src_u = src + ysize;
   __u8 *src_v = src + ysize + ysize / 4;

   for( y = 0; y < height; y += 2 )
   {
      int hoff = stride * y;
      int x;
      
      for( x = 0; x < stride; x+=4 )
      {
	 __u8 u = src_u[ offu++ ];
	 __u8 v = src_v[ offv++ ];

	 dest[ hoff + x ] = u;
	 dest[ hoff + stride + x ] = u;

	 dest[ hoff + x + 2 ] = v;
	 dest[ hoff + stride + x + 2 ] = v;
      }

      for( x = 0; x < stride; x += 2 )
      {
	 dest[ hoff + x + 1 ] = src_y[ offy++ ];
      }

      for( x = 0; x < stride; x+= 2 )
      {
	 dest[ hoff + stride + x + 1 ] = src_y[ offy++ ];
      }
   }
}

static void grey2yuv420p( __u8 *dest, __u8 *source, int width, int height )
{
   memcpy( dest, source, width * height );
   memset( dest + width * height, 0x7f, width * height / 2 );
}


static void grey2uyvy( __u8 *dest, __u8 *source, int width, int height )
{
   int i,j;

   int source_size = width * height;

   for( i = 0, j = 0; i < source_size; i+=2, j+=4 )
   {
      __u32 tmp = 0x7f007f00 | ( source[i]<<16 ) | source[i+1];
      *(int*)(dest+j) = GINT32_FROM_BE( tmp );
   }
}

static void uyvy2grey( __u8 *dest, __u8 *source, int width, int height )
{
   int i,j;

   int source_size = width * height * 2;

   for( i = 1, j = 0; i < source_size; i += 2, j++ )
   {
      dest[j] = source[i];
   }
}

static void grey2yuy2( __u8 *dest, __u8 *source, int width, int height )
{
   int i,j;

   int source_size = width * height;


   for( i = 0, j = 0; i < source_size; i+=2, j+=4 )
   {
      __u32 tmp = 0x007f007f + 
	 ( ((unsigned int)source[i])<<24 ) +
	 (((unsigned int)source[i+1])<<8);
      *(int*)(dest+j) = GINT32_FROM_BE( tmp );
   }
}

static void yuy22grey( __u8 *dest, __u8 *source, int width, int height )
{
   int i,j;

   int source_size = width * height * 2;

   for( i = 0, j = 0; i < source_size; i += 2, j++ )
   {
      dest[j] = source[i];
   }
}
		
static void y8002y420( __u8 *dest, __u8 *source, int width, int height )
{
   memcpy( dest, source, width * height );
   memset( dest + width * height, 0x7f, width * height / 2 );
}

static void y4202y800( __u8 *dest, __u8 *source, int width, int height )
{
   memcpy( dest, source, width * height );
}

static void y8002y422( __u8 *dest, __u8 *source, int width, int height )
{
   memcpy( dest, source, width * height );
   memset( dest + width * height, 0x7f, width *  height );
}

static void uyvy2yuy2( __u8 *dest, __u8 *source, int width, int height )
{
   __u8 *end;

   int source_size = width * height * 2;

   for( end = source + source_size; source < end; source+=4, dest += 4 )
   {
      register __u32 tmp = *((int*)source);
      tmp = ( ( tmp & 0xff00ff00 ) >> 8 ) | ( ( tmp & 0xff00ff ) << 8 );
      *((__u32*)dest) = tmp;
   }   
}

static void uyvy2yuv( __u8 *dest, __u8 *source, int width, int height )
{
   __u8 *end = source + width * height * 2;

   while( source < end )
   {
      __u8 y1, y2, u, v;
      
      u = *source++;
      y1 = *source++;
      v = *source++;
      y2 = *source++;
      
      *dest++ = y1;
      *dest++ = u;
      *dest++ = v;
      *dest++ = y2;
      *dest++ = u;
      *dest++ = v;
   }
}


static void uyvy2bgr24( __u8 *dest, __u8 *source, int width, int height )
{
   __u8 *source_end = source + (width * height * 2);

   while( source < source_end )
   {
      __u8 y1, y2, u, v;
      int c1, c2, d, dg, db, e, er, eg;
      int ir, ig, ib;
		
      u = *source++;
      y1 = *source++;
      v = *source++;
      y2 = *source++;
      
      c1 = (y1-16)*298;
      c2 = (y2-16)*298;
      d = u-128;
      dg = 100 * d;
      db = 516 * d;
      e = v-128;
      er = 409 * e;
      eg = 208 * e;

      ir = (c1 + er + 128)>>8;
      ig = (c1 - dg - eg + 128 )>>8;
      ib = (c1 + db)>>8;
      

      *dest++ = (__u8) ( ib > 255 ? 255 : ( ib < 0 ? 0 : ib ) );
      *dest++ = (__u8) ( ig > 255 ? 255 : ( ig < 0 ? 0 : ig ) );
      *dest++ = (__u8) ( ir > 255 ? 255 : ( ir < 0 ? 0 : ir ) );
		
      ir = (c2 + er + 128)>>8;
      ig = (c2 - dg - eg + 128 )>>8;
      ib = (c2 + db)>>8;

      *dest++ = (__u8) ( ib > 255 ? 255 : ( ib < 0 ? 0 : ib ) );
      *dest++ = (__u8) ( ig > 255 ? 255 : ( ig < 0 ? 0 : ig ) );
      *dest++ = (__u8) ( ir > 255 ? 255 : ( ir < 0 ? 0 : ir ) );
   }
}

static void uyvy2rgb24( __u8 *dest, __u8 *source, int width, int height )
{
   __u8 *source_end = source + (width * height * 2);

   while( source < source_end )
   {
      __u8 y1, y2, u, v;
      int c1, c2, d, dg, db, e, er, eg;
      int ir, ig, ib;
		
      u = *source++;
      y1 = *source++;
      v = *source++;
      y2 = *source++;
      
      c1 = (y1-16)*298;
      c2 = (y2-16)*298;
      d = u-128;
      dg = 100 * d;
      db = 516 * d;
      e = v-128;
      er = 409 * e;
      eg = 208 * e;

      ir = (c1 + er + 128)>>8;
      ig = (c1 - dg - eg + 128 )>>8;
      ib = (c1 + db)>>8;
      

      *dest++ = (__u8) ( ir > 255 ? 255 : ( ir < 0 ? 0 : ir ) );
      *dest++ = (__u8) ( ig > 255 ? 255 : ( ig < 0 ? 0 : ig ) );
      *dest++ = (__u8) ( ib > 255 ? 255 : ( ib < 0 ? 0 : ib ) );
		
      ir = (c2 + er + 128)>>8;
      ig = (c2 - dg - eg + 128 )>>8;
      ib = (c2 + db)>>8;

      *dest++ = (__u8) ( ir > 255 ? 255 : ( ir < 0 ? 0 : ir ) );
      *dest++ = (__u8) ( ig > 255 ? 255 : ( ig < 0 ? 0 : ig ) );
      *dest++ = (__u8) ( ib > 255 ? 255 : ( ib < 0 ? 0 : ib ) );
   }
}

static void yuyv2rgb24( __u8 *dest, __u8 *source, int width, int height )
{
   __u8 *source_end = source + (width * height * 2);

   while( source < source_end )
   {
      __u8 y1, y2, u, v;
      int c1, c2, d, dg, db, e, er, eg;
      int ir, ig, ib;
		
      y1 = *source++;
      u = *source++;
      y2 = *source++;
      v = *source++;
      
      c1 = (y1-16)*298;
      c2 = (y2-16)*298;
      d = u-128;
      dg = 100 * d;
      db = 516 * d;
      e = v-128;
      er = 409 * e;
      eg = 208 * e;

      ir = (c1 + er + 128)>>8;
      ig = (c1 - dg - eg + 128 )>>8;
      ib = (c1 + db)>>8;
      

      *dest++ = (__u8) ( ir > 255 ? 255 : ( ir < 0 ? 0 : ir ) );
      *dest++ = (__u8) ( ig > 255 ? 255 : ( ig < 0 ? 0 : ig ) );
      *dest++ = (__u8) ( ib > 255 ? 255 : ( ib < 0 ? 0 : ib ) );
		
      ir = (c2 + er + 128)>>8;
      ig = (c2 - dg - eg + 128 )>>8;
      ib = (c2 + db)>>8;

      *dest++ = (__u8) ( ir > 255 ? 255 : ( ir < 0 ? 0 : ir ) );
      *dest++ = (__u8) ( ig > 255 ? 255 : ( ig < 0 ? 0 : ig ) );
      *dest++ = (__u8) ( ib > 255 ? 255 : ( ib < 0 ? 0 : ib ) );
   }
}

static void yuyv2bgr32( __u8 *dest, __u8 *source, int width, int height )
{
   __u8 *source_end = source + (width * height * 2);

   while( source < source_end )
   {
      __u8 y1, y2, u, v;
      int c1, c2, d, dg, db, e, er, eg;
      int ir, ig, ib;
		
      y1 = *source++;
      u = *source++;
      y2 = *source++;
      v = *source++;
      
      c1 = (y1-16)*298;
      c2 = (y2-16)*298;
      d = u-128;
      dg = 100 * d;
      db = 516 * d;
      e = v-128;
      er = 409 * e;
      eg = 208 * e;

      ir = (c1 + er + 128)>>8;
      ig = (c1 - dg - eg + 128 )>>8;
      ib = (c1 + db)>>8;
      
      *dest++ = (__u8) ( ib > 255 ? 255 : ( ib < 0 ? 0 : ib ) );
      *dest++ = (__u8) ( ig > 255 ? 255 : ( ig < 0 ? 0 : ig ) );
      *dest++ = (__u8) ( ir > 255 ? 255 : ( ir < 0 ? 0 : ir ) );
      *dest++ = 0; // Alpha
		
      ir = (c2 + er + 128)>>8;
      ig = (c2 - dg - eg + 128 )>>8;
      ib = (c2 + db)>>8;

      *dest++ = (__u8) ( ib > 255 ? 255 : ( ib < 0 ? 0 : ib ) );
      *dest++ = (__u8) ( ig > 255 ? 255 : ( ig < 0 ? 0 : ig ) );
      *dest++ = (__u8) ( ir > 255 ? 255 : ( ir < 0 ? 0 : ir ) );
      *dest++ = 0; // Alpha
   }
}



/* static void uyvy2rgb24( __u8 *dest, __u8 *source, int width, int height ) */
/* { */
/*    int i; */
/*    int dest_offset = 0; */
	
/*    int source_size = width * height * 2; */

/*    for( i = 0; i < source_size; i+=4 ) */
/*    { */
/*       __u8 *r, *b, *g; */
/*       __u8 *y1, *y2, *u, *v; */
		
/*       float fr, fg, fb; */
/*       float fy1, fy2, fu, fur, fug, fub, fv, fvr, fvg, fvb; */

/*       r = dest + dest_offset; */
/*       g = r + 1; */
/*       b = g + 1; */
		
/*       u = source + i; */
/*       y1 = u + 1; */
/*       v = y1 + 1; */
/*       y2 = v + 1; */
		
/*       fu = *u - 128; */
/*       fur = fu * 0.0009267; */
/*       fug = fu * 0.3436954; */
/*       fub = fu * 1.7721604; */
/*       fv = *v - 128; */
/*       fvr = fv * 1.4016868; */
/*       fvg = fv * 0.7141690; */
/*       fvb = fv * 0.0009902; */
/*       fy1= *y1; */
/*       fy2= *y2; */

/*       fr = fy1 - fur + fvr; */
/*       fg = fy1 - fug - fvg; */
/*       fb = fy1 + fub + fvb; */

/*       *r = (__u8) ( fr > 255 ? 255 : ( fr < 0 ? 0 : fr ) ); */
/*       *g = (__u8) ( fg > 255 ? 255 : ( fg < 0 ? 0 : fg ) ); */
/*       *b = (__u8) ( fb > 255 ? 255 : ( fb < 0 ? 0 : fb ) ); */


		
/*       dest_offset += 3; */
		
/*       r = dest + dest_offset; */
/*       g = r + 1; */
/*       b = g + 1; */
		
/*       fr = fy2 - fur + fvr; */
/*       fg = fy2 - fug - fvg; */
/*       fb = fy2 + fub + fvb; */

/*       *r = (__u8) ( fr > 255 ? 255 : ( fr < 0 ? 0 : fr ) ); */
/*       *g = (__u8) ( fg > 255 ? 255 : ( fg < 0 ? 0 : fg ) ); */
/*       *b = (__u8) ( fb > 255 ? 255 : ( fb < 0 ? 0 : fb ) ); */

/*       dest_offset += 3; */
/*    } */
/* } */


size_t __y4112rgb24( __u8 *dest, __u8 *source, size_t dest_size, size_t source_size )
{
   int i;
   int dest_offset = 0;
	
   long co1, co2, co3, co4, co5, co6;
	
   co1 = 0.0009267 * 65536;
   co2 = 1.4016868 * 65536;
   co3 = 0.3436954 * 65536;
   co4 = 0.7141690 * 65536;
   co5 = 1.7721604 * 65536;
   co6 = 0.0009902 * 65536;

   if( dest_size < ( source_size + ( source_size >> 1 ) ) )
   {
      return 0;
   }

   for( i = 0; i < source_size;  )
   {
      __u8 *r, *b, *g;
      long y1, y2, y3, y4, u, v;

      r = dest + dest_offset;
      g = r + 1;
      b = g + 1;
		
      u =  ( ( *( source + i++ ) ) << 16 ) - ( 128 << 16 );
      y1 = ( *( source + i++ ) ) << 16;
      y2 = ( *( source + i++ ) ) << 16;
      v =  ( ( *( source + i++ ) ) << 16 ) - ( 128 << 16 );
      y3 = ( *( source + i++ ) ) << 16;
      y4 = ( *( source + i++ ) ) << 16;

      *r = ( ( y1 - co1 * u + co2 * v ) >> 16 );
      *g = ( ( y1 - co3 * u - co4 * v ) >> 16 );
      *b = ( ( y1 + co5 * u + co6 * v ) >> 16 );
		
      dest_offset += 3;

      r = dest + dest_offset;
      g = r + 1;
      b = g + 1;
		
      *r = ( ( y2 - co1 * u + co2 * v ) >> 16 );
      *g = ( ( y2 - co3 * u - co4 * v ) >> 16 );
      *b = ( ( y2 + co5 * u + co6 * v ) >> 16 );

      dest_offset += 3;

      r = dest + dest_offset;
      g = r + 1;
      b = g + 1;
		
      *r = ( y3 - co1 * u + co2 * v ) >> 16;
      *g = ( y3 - co3 * u - co4 * v ) >> 16;
      *b = ( y3 + co5 * u + co6 * v ) >> 16;

      dest_offset += 3;

      r = dest + dest_offset;
      g = r + 1;
      b = g + 1;

      *r = ( y4 - co1 * u + co2 * v ) >> 16;
      *g = ( y4 - co3 * u - co4 * v ) >> 16;
      *b = ( y4 + co5 * u + co6 * v ) >> 16;

      dest_offset += 3;
   }
	
   return source_size * 2;
}


static void y4112rgb24( __u8 *dest, __u8 *source, int width, int height )
{
   int i;
   int dest_offset = 0;
	
   int source_size = width * height * 1.5;

   for( i = 0; i < source_size; i+=6 )
   {
      __u8 *r, *b, *g;
      __u8 *y1, *y2, *y3, *y4, *u, *v;
		
      double fr, fg, fb;
      double fy1, fy2, fy3, fy4, fu, fur, fug, fub, fv, fvr, fvg, fvb;

      r = dest + dest_offset;
      g = r + 1;
      b = g + 1;
		
      u = source + i;
      y1 = u + 1;
      y2 = y1 + 1;
      v = y2 + 1;
      y3 = v + 1;
      y4 = y3 + 1;

      fu = *u - 128;
      fur = fu * 0.0009267;
      fug = fu * 0.3436954;
      fub = fu * 1.7721604;
      fv = *v - 128;
      fvr = fv * 1.4016868;
      fvg = fv * 0.7141690;
      fvb = fv * 0.0009902;
      fy1= *y1;
      fy2= *y2;
      fy3= *y3;
      fy4= *y4;

      fr = fy1 - fur + fvr;
      fg = fy1 - fug - fvg;
      fb = fy1 + fub + fvb;

      *r = (__u8) ( fr > 255 ? 255 : ( fr < 0 ? 0 : fr ) );
      *g = (__u8) ( fg > 255 ? 255 : ( fg < 0 ? 0 : fg ) );
      *b = (__u8) ( fb > 255 ? 255 : ( fb < 0 ? 0 : fb ) );
		
		
      dest_offset += 3;
		
      r = dest + dest_offset;
      g = r + 1;
      b = g + 1;
		
      fr = fy2 - fur + fvr;
      fg = fy2 - fug - fvg;
      fb = fy2 + fub + fvb;

      *r = (__u8) ( fr > 255 ? 255 : ( fr < 0 ? 0 : fr ) );
      *g = (__u8) ( fg > 255 ? 255 : ( fg < 0 ? 0 : fg ) );
      *b = (__u8) ( fb > 255 ? 255 : ( fb < 0 ? 0 : fb ) );

      dest_offset += 3;

      r = dest + dest_offset;
      g = r + 1;
      b = g + 1;
		
      fr = fy3 - fur + fvr;
      fg = fy3 - fug - fvg;
      fb = fy3 + fub + fvb;

      *r = (__u8) ( fr > 255 ? 255 : ( fr < 0 ? 0 : fr ) );
      *g = (__u8) ( fg > 255 ? 255 : ( fg < 0 ? 0 : fg ) );
      *b = (__u8) ( fb > 255 ? 255 : ( fb < 0 ? 0 : fb ) );

      dest_offset += 3;

      r = dest + dest_offset;
      g = r + 1;
      b = g + 1;
		
      fr = fy4 - fur + fvr;
      fg = fy4 - fug - fvg;
      fb = fy4 + fub + fvb;

      *r = (__u8) ( fr > 255 ? 255 : ( fr < 0 ? 0 : fr ) );
      *g = (__u8) ( fg > 255 ? 255 : ( fg < 0 ? 0 : fg ) );
      *b = (__u8) ( fb > 255 ? 255 : ( fb < 0 ? 0 : fb ) );

      dest_offset += 3;
   }
}

static void y4112rgb32( __u8 *dest, __u8 *source, int width, int height )
{
   int i;
   int dest_offset = 0;
	
   int source_size = width * height * 1.5;

   for( i = 0; i < source_size; i+=6 )
   {
      __u8 *r, *b, *g, *a;
      __u8 *y1, *y2, *y3, *y4, *u, *v;
		
      double fr, fg, fb;
      double fy1, fy2, fy3, fy4, fu, fur, fug, fub, fv, fvr, fvg, fvb;

      r = dest + dest_offset;
      g = r + 1;
      b = g + 1;
      a = b + 1;
		
      u = source + i;
      y1 = u + 1;
      y2 = y1 + 1;
      v = y2 + 1;
      y3 = v + 1;
      y4 = y3 + 1;

      fu = *u - 128;
      fur = fu * 0.0009267;
      fug = fu * 0.3436954;
      fub = fu * 1.7721604;
      fv = *v - 128;
      fvr = fv * 1.4016868;
      fvg = fv * 0.7141690;
      fvb = fv * 0.0009902;
      fy1= *y1;
      fy2= *y2;
      fy3= *y3;
      fy4= *y4;

      fr = fy1 - fur + fvr;
      fg = fy1 - fug - fvg;
      fb = fy1 + fub + fvb;

      *r = (__u8) ( fr > 255 ? 255 : ( fr < 0 ? 0 : fr ) );
      *g = (__u8) ( fg > 255 ? 255 : ( fg < 0 ? 0 : fg ) );
      *b = (__u8) ( fb > 255 ? 255 : ( fb < 0 ? 0 : fb ) );
      *a = 0xff;
		
		
      dest_offset += 4;
		
      r = dest + dest_offset;
      g = r + 1;
      b = g + 1;
      a = b + 1;
		
      fr = fy2 - fur + fvr;
      fg = fy2 - fug - fvg;
      fb = fy2 + fub + fvb;

      *r = (__u8) ( fr > 255 ? 255 : ( fr < 0 ? 0 : fr ) );
      *g = (__u8) ( fg > 255 ? 255 : ( fg < 0 ? 0 : fg ) );
      *b = (__u8) ( fb > 255 ? 255 : ( fb < 0 ? 0 : fb ) );
      *a = 0xff;

      dest_offset += 4;

      r = dest + dest_offset;
      g = r + 1;
      b = g + 1;
      a = b + 1;
		
      fr = fy3 - fur + fvr;
      fg = fy3 - fug - fvg;
      fb = fy3 + fub + fvb;

      *r = (__u8) ( fr > 255 ? 255 : ( fr < 0 ? 0 : fr ) );
      *g = (__u8) ( fg > 255 ? 255 : ( fg < 0 ? 0 : fg ) );
      *b = (__u8) ( fb > 255 ? 255 : ( fb < 0 ? 0 : fb ) );
      *a = 0xff;

      dest_offset += 4;

      r = dest + dest_offset;
      g = r + 1;
      b = g + 1;
      a = b + 1;
		
      fr = fy4 - fur + fvr;
      fg = fy4 - fug - fvg;
      fb = fy4 + fub + fvb;

      *r = (__u8) ( fr > 255 ? 255 : ( fr < 0 ? 0 : fr ) );
      *g = (__u8) ( fg > 255 ? 255 : ( fg < 0 ? 0 : fg ) );
      *b = (__u8) ( fb > 255 ? 255 : ( fb < 0 ? 0 : fb ) );
      *a = 0xff;

      dest_offset += 4;
   }
}

static void uyvy2rgb32( __u8 *dest, __u8 *source, int width, int height )
{
   __u8 *source_end = source + (width * height * 2);

   while( source < source_end )
   {
      __u8 y1, y2, u, v;
      int c1, c2, d, dg, db, e, er, eg;
      int ir, ig, ib;		
		
      u = *source++;
      y1 = *source++;
      v = *source++;
      y2 = *source++;

      c1 = (y1-16)*298;
      c2 = (y2-16)*298;
      d = u-128;
      dg = 100 * d;
      db = 516 * d;
      e = v-128;
      er = 409 * e;
      eg = 208 * e;

      ir = (c1 + er + 128)>>8;
      ig = (c1 - dg - eg + 128 )>>8;
      ib = (c1 + db)>>8;
      

      *dest++ = (__u8) ( ir > 255 ? 255 : ( ir < 0 ? 0 : ir ) );
      *dest++ = (__u8) ( ig > 255 ? 255 : ( ig < 0 ? 0 : ig ) );
      *dest++ = (__u8) ( ib > 255 ? 255 : ( ib < 0 ? 0 : ib ) );
      *dest++ = 0;		

      ir = (c2 + er + 128)>>8;
      ig = (c2 - dg - eg + 128 )>>8;
      ib = (c2 + db)>>8;
      
      *dest++ = (__u8) ( ir > 255 ? 255 : ( ir < 0 ? 0 : ir ) );
      *dest++ = (__u8) ( ig > 255 ? 255 : ( ig < 0 ? 0 : ig ) );
      *dest++ = (__u8) ( ib > 255 ? 255 : ( ib < 0 ? 0 : ib ) );
      *dest++ = 0;		

   }
	
}

static void y4202rgb24( __u8 *dest, __u8 *source, int width, int height )
{
   int i, j;
   __u8 *ybase = source;
   __u8 *ubase = source + width * height;
   __u8 *vbase = ubase + ( ( width * height ) / 4 );
	
   for( j = 0; j < height; j++ )
   {
      for( i = 0; i < width; i+=2 )
      {
	 __u8 y, u, v;
	 int c, d, dg, db, e, er, eg;
	 int ir, ig, ib;

	 int uvidx = j /2 * width /2 + i /2;
			
	 y = *(ybase + ( j * width ) + i);
	 u = *(ubase + uvidx );
	 v = *(vbase + uvidx );
			
	 c = (y-16)*298;
	 d = u-128;
	 dg = 100 * d;
	 db = 516 * d;
	 e = v-128;
	 er = 409 * e;
	 eg = 208 * e;

	 ir = (c + er + 128)>>8;
	 ig = (c - dg - eg + 128 )>>8;
	 ib = (c + db)>>8;

	 *dest++ = (__u8) ( ir > 255 ? 255 : ( ir < 0 ? 0 : ir ) );
	 *dest++ = (__u8) ( ig > 255 ? 255 : ( ig < 0 ? 0 : ig ) );
	 *dest++ = (__u8) ( ib > 255 ? 255 : ( ib < 0 ? 0 : ib ) );

	 y = *(ybase + ( j * width ) + i + 1);

	 c = (y-16)*298;

	 ir = (c + er + 128)>>8;
	 ig = (c - dg - eg + 128 )>>8;
	 ib = (c + db)>>8;

	 *dest++ = (__u8) ( ir > 255 ? 255 : ( ir < 0 ? 0 : ir ) );
	 *dest++ = (__u8) ( ig > 255 ? 255 : ( ig < 0 ? 0 : ig ) );
	 *dest++ = (__u8) ( ib > 255 ? 255 : ( ib < 0 ? 0 : ib ) );

      }
   }
}

static void y4202rgb32( __u8 *dest, __u8 *source, int width, int height )
{
   int i, j;
   __u8 *ybase = source;
   __u8 *ubase = source + width * height;
   __u8 *vbase = ubase + ( ( width * height ) / 4 );
	
   for( j = 0; j < height; j++ )
   {
      for( i = 0; i < width; i+=2 )
      {
	 __u8 y, u, v;
	 int c, d, dg, db, e, er, eg;
	 int ir, ig, ib;

	 int uvidx = j /2 * width /2 + i /2;
			
	 y = *(ybase + ( j * width ) + i);
	 u = *(ubase + uvidx );
	 v = *(vbase + uvidx );
			
	 c = (y-16)*298;
	 d = u-128;
	 dg = 100 * d;
	 db = 516 * d;
	 e = v-128;
	 er = 409 * e;
	 eg = 208 * e;

	 ir = (c + er + 128)>>8;
	 ig = (c - dg - eg + 128 )>>8;
	 ib = (c + db)>>8;

	 *dest++ = (__u8) ( ir > 255 ? 255 : ( ir < 0 ? 0 : ir ) );
	 *dest++ = (__u8) ( ig > 255 ? 255 : ( ig < 0 ? 0 : ig ) );
	 *dest++ = (__u8) ( ib > 255 ? 255 : ( ib < 0 ? 0 : ib ) );
	 *dest++ = 0;// a

	 y = *(ybase + ( j * width ) + i + 1);

	 c = (y-16)*298;

	 ir = (c + er + 128)>>8;
	 ig = (c - dg - eg + 128 )>>8;
	 ib = (c + db)>>8;

	 *dest++ = (__u8) ( ir > 255 ? 255 : ( ir < 0 ? 0 : ir ) );
	 *dest++ = (__u8) ( ig > 255 ? 255 : ( ig < 0 ? 0 : ig ) );
	 *dest++ = (__u8) ( ib > 255 ? 255 : ( ib < 0 ? 0 : ib ) );
	 *dest++ = 0;// a

      }
   }
}

static void y8002rgb24( __u8 *dest, __u8 *source, int width, int height )
{
   int i;
   int dest_offset = 0;	
   int source_size = width * height;
	
   for( i = 0; i < source_size; i++ )
   {
      __u8 *y;
      __u8 *r, *b, *g;
		
      r = dest + dest_offset;
      g = r + 1;
      b = g + 1;
		
      y = source + i;
		
      *r = *y;
      *g = *y;
      *b = *y;
		
      dest_offset +=3;
   }
	
}

static void rgb242y800( __u8 *dest, __u8 *source, int width, int height )
{
   int i;
   int dest_offset = 0;
   int source_size = width * height * 3;
   
   for( i = 0; i < source_size; i += 3 ){
      __u8 y;
      y = source[i];
      dest[dest_offset++] = y;
   }
}

static void rgb322y800( __u8 *dest, __u8 *source, int width, int height )
{
   int i;
   int dest_offset = 0;
   int source_size = width * height * 4;
   
   for( i = 1; i < source_size; i += 4 ){
      __u8 y;
      y = source[i];
      dest[dest_offset++] = y;
   }
}


static void y8002rgb32( __u8 *dest, __u8 *source, int width, int height )
{
   int i;
   int dest_offset = 0;
   int source_size = width * height;
	
   for( i = 0; i < source_size; i++ )
   {
      __u8 *y;
      __u8 *r, *b, *g, *a;
		
      r = dest + dest_offset;
      g = r + 1;
      b = g + 1;
      a = b + 1;
		
      y = source + i;
		
      *r = *y;
      *g = *y;
      *b = *y;
      *a = 0;
		
      dest_offset +=4;
   }
}

//y =  0.2990 * R + 0.5870 * G + 0.1140 * B
//U = -0.1687 * R - 0.3313 * G + 0.5000 * B
//V =  0.5000 * R - 0.4187 * G - 0.0813 * B	
static void rgb242uyvy( __u8 *dest, __u8 *source, int width, int height )
{
   int offset;
   int dest_offset = 0;
   int source_size = width * height * 3;
   
   for( offset = 0; offset < source_size; )
   {
      int r, b, g;
      
      r = source[offset++];
      g = source[offset++];
      b = source[offset++];
      
      dest[dest_offset++] = ( ( -38 * r -  74 * g + 112 * b + 128) >> 8) + 128;
      dest[dest_offset++] = ( (  66 * r + 129 * g +  25 * b + 128) >> 8) +  16;

      r = source[offset++];
      g = source[offset++];
      b = source[offset++];
      
      dest[dest_offset++] = ( ( 112 * r -  94 * g -  18 * b + 128) >> 8) + 128;
      dest[dest_offset++] = ( (  66 * r + 129 * g +  25 * b + 128) >> 8) +  16;
   }
}

static void bgr242uyvy( __u8 *dest, __u8 *source, int width, int height )
{
   int offset;
   int dest_offset = 0;
   int source_size = width * height * 3;
   
   for( offset = 0; offset < source_size; )
   {
      int r, b, g;
      
      b = source[offset++];
      g = source[offset++];
      r = source[offset++];
      
      dest[dest_offset++] = ( ( -38 * r -  74 * g + 112 * b + 128) >> 8) + 128;
      dest[dest_offset++] = ( (  66 * r + 129 * g +  25 * b + 128) >> 8) +  16;

      b = source[offset++];
      g = source[offset++];
      r = source[offset++];
      
      dest[dest_offset++] = ( ( 112 * r -  94 * g -  18 * b + 128) >> 8) + 128;
      dest[dest_offset++] = ( (  66 * r + 129 * g +  25 * b + 128) >> 8) +  16;
   }
}

static void rgb322uyvy( __u8 *dest, __u8 *source, int width, int height )
{
   int offset;
   int dest_offset = 0;
   int source_size = width * height * 4;
   
   for( offset = 0; offset < source_size; )
   {
      int r, b, g;
      
      r = source[offset++];
      g = source[offset++];
      b = source[offset++];
      offset++;
      
      dest[dest_offset++] = ( ( -38 * r -  74 * g + 112 * b + 128) >> 8) + 128;
      dest[dest_offset++] = ( (  66 * r + 129 * g +  25 * b + 128) >> 8) +  16;

      r = source[offset++];
      g = source[offset++];
      b = source[offset++];
      offset++;
      
      dest[dest_offset++] = ( ( 112 * r -  94 * g -  18 * b + 128) >> 8) + 128;
      dest[dest_offset++] = ( (  66 * r + 129 * g +  25 * b + 128) >> 8) +  16;
   }
}

static void rgb242yuyv( __u8 *dest, __u8 *source, int width, int height )
{
   int offset;
   int dest_offset = 0;
   int source_size = width * height * 3;
   
   for( offset = 0; offset < source_size; )
   {
      int r, b, g;
      
      r = source[offset++];
      g = source[offset++];
      b = source[offset++];
      
      dest[dest_offset++] = ( (  66 * r + 129 * g +  25 * b + 128) >> 8) +  16;
      dest[dest_offset++] = ( ( -38 * r -  74 * g + 112 * b + 128) >> 8) + 128;
 
      r = source[offset++];
      g = source[offset++];
      b = source[offset++];
      
      dest[dest_offset++] = ( (  66 * r + 129 * g +  25 * b + 128) >> 8) +  16;
      dest[dest_offset++] = ( ( 112 * r -  94 * g -  18 * b + 128) >> 8) + 128;
   }
}

static void rgb24toyuv420p( __u8 *dest, __u8 *source, int width, int height )
{
   int offset = 0;
   int dest_y_offset = 0;
   int dest_u_offset = 0;
   int dest_v_offset = 0;
   int x,y;

   __u8 *dest_y;
   __u8 *dest_u;
   __u8 *dest_v;
   
   dest_y = dest;
   dest_u = dest_y + ( width * height );
   dest_v = dest_u + ( width * height ) / 4;
   
   for( y = 0; y < height; y +=2 )
   {
      for( x = 0; x < width; x+= 2 )
      {
	 int r, b, g;
	 
	 r = source[offset++];
	 g = source[offset++];
	 b = source[offset++];
	 
	 dest_y[dest_y_offset++] = ( (  66 * r + 129 * g +  25 * b + 128) >> 8) +  16;
	 dest_u[dest_u_offset++] = ( ( -38 * r -  74 * g + 112 * b + 128) >> 8) + 128;
	 dest_v[dest_v_offset++] = ( ( 112 * r -  94 * g -  18 * b + 128) >> 8) + 128;

	 r = source[offset++];
	 g = source[offset++];
	 b = source[offset++];
	 
	 dest_y[dest_y_offset++] = ( (  66 * r + 129 * g +  25 * b + 128) >> 8) +  16;
      }
      
      for( x = 0; x < width; x+=2 )
      {
	 int r, b, g;
	 
	 r = source[offset++];
	 g = source[offset++];
	 b = source[offset++];
	 
	 dest_y[dest_y_offset++] = ( (  66 * r + 129 * g +  25 * b + 128) >> 8) +  16;
	 
	 r = source[offset++];
	 g = source[offset++];
	 b = source[offset++];
	 
	 dest_y[dest_y_offset++] = ( (  66 * r + 129 * g +  25 * b + 128) >> 8) +  16;
      }
   }
}

static void bgr24toyuv420p( __u8 *dest, __u8 *source, int width, int height )
{
   int offset = 0;
   int dest_y_offset = 0;
   int dest_u_offset = 0;
   int dest_v_offset = 0;
   int x,y;

   __u8 *dest_y;
   __u8 *dest_u;
   __u8 *dest_v;
   
   dest_y = dest;
   dest_u = dest_y + ( width * height );
   dest_v = dest_u + ( width * height ) / 4;
   
   for( y = 0; y < height; y +=2 )
   {
      for( x = 0; x < width; x+= 2 )
      {
	 int r, b, g;
	 
	 b = source[offset++];
	 g = source[offset++];
	 r = source[offset++];
	 
	 dest_y[dest_y_offset++] = ( (  66 * r + 129 * g +  25 * b + 128) >> 8) +  16;
	 dest_u[dest_u_offset++] = ( ( -38 * r -  74 * g + 112 * b + 128) >> 8) + 128;
	 dest_v[dest_v_offset++] = ( ( 112 * r -  94 * g -  18 * b + 128) >> 8) + 128;

	 b = source[offset++];
	 g = source[offset++];
	 r = source[offset++];
	 
	 dest_y[dest_y_offset++] = ( (  66 * r + 129 * g +  25 * b + 128) >> 8) +  16;
      }
      
      for( x = 0; x < width; x+=2 )
      {
	 int r, b, g;
	 
	 b = source[offset++];
	 g = source[offset++];
	 r = source[offset++];
	 
	 dest_y[dest_y_offset++] = ( (  66 * r + 129 * g +  25 * b + 128) >> 8) +  16;
	 
	 b = source[offset++];
	 g = source[offset++];
	 r = source[offset++];
	 
	 dest_y[dest_y_offset++] = ( (  66 * r + 129 * g +  25 * b + 128) >> 8) +  16;
      }
   }
}

static void yuv420ptorgb24( __u8 *dest, __u8 *source, int width, int height )
{
   int x,y;
   __u8 *src_y = source;
   __u8 *src_u = source + width * height;
   __u8 *src_v = src_u + ( width * height ) / 4;


   for( y = 0; y < height; y++ )
   {
      for( x = 0; x < width; x+= 2 )
      {
	 __u8 y1, y2, u, v;
	 int c1, c2, d, dg, db, e, er, eg;
	 int ir, ig, ib;
		
	 y1 = src_y[ y * width + x ];
	 y2 = src_y[ y * width + x + 1 ];
	 u = src_u[ ( ( y * width ) / 4 ) + x / 2 ];
	 v = src_v[ ( ( y * width ) / 4 ) + x / 2 ];
      
	 c1 = (y1-16)*298;
	 c2 = (y2-16)*298;
	 d = u-128;
	 dg = 100 * d;
	 db = 516 * d;
	 e = v-128;
	 er = 409 * e;
	 eg = 208 * e;

	 ir = (c1 + er + 128)>>8;
	 ig = (c1 - dg - eg + 128 )>>8;
	 ib = (c1 + db)>>8;

	 *dest++ = (__u8) ( ir > 255 ? 255 : ( ir < 0 ? 0 : ir ) );
	 *dest++ = (__u8) ( ig > 255 ? 255 : ( ig < 0 ? 0 : ig ) );
	 *dest++ = (__u8) ( ib > 255 ? 255 : ( ib < 0 ? 0 : ib ) );
		
	 ir = (c2 + er + 128)>>8;
	 ig = (c2 - dg - eg + 128 )>>8;
	 ib = (c2 + db)>>8;

	 *dest++ = (__u8) ( ir > 255 ? 255 : ( ir < 0 ? 0 : ir ) );
	 *dest++ = (__u8) ( ig > 255 ? 255 : ( ig < 0 ? 0 : ig ) );
	 *dest++ = (__u8) ( ib > 255 ? 255 : ( ib < 0 ? 0 : ib ) );
      }
   }
   
}

static void bgr24torgb24( __u8 *dest, __u8 *source, int width, int height )
{
   int size = width * height;
   int i;
   
   for( i = 0; i < size; i++ )
   {
      unsigned char r, g, b;
      b = *source++;
      g = *source++;
      r = *source++;
      
      *dest++ = r;
      *dest++ = g;
      *dest++ = b;
   }
}

static void rgb32torgb24( __u8 *dest, __u8 *source, int width, int height )
{
   int size = width * height;
   int i;
   

   for( i = 0; i < size; i++ )
   {
      unsigned char r,g,b;
      
      r = *source++;
      g = *source++;
      b = *source++;
      source++;
      
      *dest++ = r;
      *dest++ = g;
      *dest++ = b;
   }
}



static void bgr24tobgr32( __u8 *dest, __u8 *source, int width, int height )
{
   int size = width * height;
   int i;
   

   for( i = 0; i < size; i++ )
   {
      unsigned char r,g,b;
      
      b = *source++;
      g = *source++;
      r = *source++;
      
      *dest++ = b;
      *dest++ = g;
      *dest++ = r;
      *dest++ = 0;
   }
}


static void yuy2toyuyv( __u8 *dest, __u8 *source, int width, int height )
{
   memcpy( dest, source, width * height * 2 );
}




static void by8torgb24( __u8 *dest, __u8* source, int width, int height )
{
   int i,j;
   int dest_offset = 0;      
   
   for( j = 2; j < height - 2; j+=2 )
   {
      int lineoffset = j * width;
      dest_offset = j * width * 3 + 3;

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

	 dest[dest_offset++] = r;
	 dest[dest_offset++] = g;
	 dest[dest_offset++] = b;
	 
	 i++;
	 
	 g = g2;
	 b = ( (int)source[ ( lineoffset + i ) - 1 ] + (int)source[ ( lineoffset + i ) + 1 ] ) / 2;
	 r = ( (int)source[ ( lineoffset + i ) - width ] + (int)source[ lineoffset + i + width ] ) / 2;

	 dest[dest_offset++] = r;
	 dest[dest_offset++] = g;
	 dest[dest_offset++] = b;
	 
	 i++;
      }
    
      lineoffset += width;
      dest_offset = (j+1) * width * 3 + 6;
      
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
	 

	 dest[dest_offset++] = r;
	 dest[dest_offset++] = g;
	 dest[dest_offset++] = b;
	 
	 i++;
	 
	 g = g2;
	 r = ( (int)source[ ( lineoffset + i ) - 1 ] + (int)source[ ( lineoffset + i ) + 1 ] ) / 2;
	 b = ( (int)source[ ( lineoffset + i ) - width ] + (int)source[ lineoffset + i + width ] ) / 2;
	 
	 dest[dest_offset++] = r;
	 dest[dest_offset++] = g;
	 dest[dest_offset++] = b;

	 i++; 
      }
   }
   
}

static void by8toby8p( __u8 *dest, __u8* source, int width, int height )
{
   int i,j;

   __u8 *destr = dest;
   __u8 *destg = dest + width*height/4;
   __u8 *destb = destg + width*height/2;

   for( j = 0; j < height; j+=2 ){
      for( i = 0; i < width; i++ ){
	 *destg++ = *source++;
	 *destb++ = *source++;
      }
      for( i = 0; i < width; i++ ){
	 *destr++ = *source++;
	 *destg++ = *source++;
      }      
   }
}


static void by8tobgr24( __u8 *dest, __u8* source, int width, int height )
{
   int i,j;
   int dest_offset = 0;      
   
   for( j = 2; j < height - 2; j+=2 )
   {
      int lineoffset = j * width;
      dest_offset = j * width * 3 + 3;

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

	 dest[dest_offset++] = b;
	 dest[dest_offset++] = g;
	 dest[dest_offset++] = r;
	 
	 i++;
	 
	 g = g2;
	 b = ( (int)source[ ( lineoffset + i ) - 1 ] + (int)source[ ( lineoffset + i ) + 1 ] ) / 2;
	 r = ( (int)source[ ( lineoffset + i ) - width ] + (int)source[ lineoffset + i + width ] ) / 2;

	 dest[dest_offset++] = b;
	 dest[dest_offset++] = g;
	 dest[dest_offset++] = r;
	 
	 i++;
      }
    
      lineoffset += width;
      dest_offset = (j+1) * width * 3 + 6;
      
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
	 

	 dest[dest_offset++] = b;
	 dest[dest_offset++] = g;
	 dest[dest_offset++] = r;
	 
	 i++;
	 
	 g = g2;
	 r = ( (int)source[ ( lineoffset + i ) - 1 ] + (int)source[ ( lineoffset + i ) + 1 ] ) / 2;
	 b = ( (int)source[ ( lineoffset + i ) - width ] + (int)source[ lineoffset + i + width ] ) / 2;
	 
	 dest[dest_offset++] = b;
	 dest[dest_offset++] = g;
	 dest[dest_offset++] = r;

	 i++; 
      }
   }   
}

static void by8torgb32( __u8 *dest, __u8* source, int width, int height )
{
   int i,j;
   int dest_offset = 0;      
   
   for( j = 2; j < height - 2; j+=2 )
   {
      int lineoffset = j * width;
      dest_offset = j * width * 3 + 3;

      for( i = 1; i < width - 2; i += 2)
      {
	 int r, g, b;
	 int g1;
	 int g2;
	 int g3;
	 int g4;
	 int d1;
	 int d2;

	 int li = lineoffset + i;

	 g1 = source[ li - 1 ];
	 g2 = source[ li + 1 ];
	 g3 = source[ li - width ];
	 g4 = source[ li + width ];	 
	 d1 = ABS( g1 - g2 );
	 d2 = ABS( g3 - g4 );

	 b = source[ li ];
	 r = ( ( (int)source[ ( li - width ) + 1 ] + 
		 (int)source[ ( li + width ) + 1 ] + 
		 (int)source[ ( li - width ) - 1 ] + 
		 (int)source[ ( li + width ) - 1 ] ) / 4 );
	 
	 if( d1 < d2 )
	 {
	    g = ( g1 + g2 ) / 2;
	 }
	 else
	 {
	    g = ( g3 + g4 ) / 2;
	 }	    

	 dest[dest_offset++] = r;
	 dest[dest_offset++] = g;
	 dest[dest_offset++] = b;
	 dest[dest_offset++] = 0xff;
	 
	 li++;
	 
	 g = g2;
	 b = ( (int)source[ li - 1 ] + (int)source[ li + 1 ] ) / 2;
	 r = ( (int)source[ li - width ] + (int)source[ li + width ] ) / 2;

	 dest[dest_offset++] = r;
	 dest[dest_offset++] = g;
	 dest[dest_offset++] = b;
	 dest[dest_offset++] = 0xff;
	 
      }
    
      lineoffset += width;
      dest_offset = (j+1) * width * 3 + 6;
      
      for( i = 2; i < width - 3; i += 2 )
      {
	 int r, g, b;
	 int g1;
	 int g2;
	 int g3;
	 int g4;
	 int d1;
	 int d2;

	 int li = lineoffset + i;

	 g1 = source[ li - 1 ];
	 g2 = source[ li + 1 ];
	 g3 = source[ li - width ];
	 g4 = source[ li + width ];
	 d1 = ABS( g1 - g2 );
	 d2 = ABS( g3 - g4 );

	 r = source[ li ];

	 if( d1 < d2 )
	 {
	    g = ( g1 + g2 ) / 2;
	 }
	    
	 else
	 {
	    g = ( g3 + g4 ) / 2;
	 }

	 b = ( ( (int)source[ ( li - width ) + 1 ] + 
		 (int)source[ li + width + 1 ] + 
		 (int)source[ ( li - width ) - 1 ] + 
		 (int)source[ ( li + width ) - 1 ] ) / 4 );
	 

	 dest[dest_offset++] = r;
	 dest[dest_offset++] = g;
	 dest[dest_offset++] = b;
	 dest[dest_offset++] = 0xff;
	 
	 li++;
	 
	 g = g2;
	 r = ( (int)source[ li - 1 ] + (int)source[ li + 1 ] ) / 2;
	 b = ( (int)source[ li - width ] + (int)source[ li + width ] ) / 2;
	 
	 dest[dest_offset++] = r;
	 dest[dest_offset++] = g;
	 dest[dest_offset++] = b;
	 dest[dest_offset++] = 0xff;

      }
   }
   
}

// ROUNDED

__u8 rtab[256] = {0, 0, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 6, 6, 6, 7, 7, 7, 7, 8, 8, 8, 9, 9, 9, 10, 10, 10, 10, 11, 11, 11, 12, 12, 12, 13, 13, 13, 13, 14, 14, 14, 15, 15, 15, 16, 16, 16, 16, 17, 17, 17, 18, 18, 18, 19, 19, 19, 19, 20, 20, 20, 21, 21, 21, 22, 22, 22, 22, 23, 23, 23, 24, 24, 24, 25, 25, 25, 25, 26, 26, 26, 27, 27, 27, 28, 28, 28, 28, 29, 29, 29, 30, 30, 30, 30, 31, 31, 31, 32, 32, 32, 33, 33, 33, 33, 34, 34, 34, 35, 35, 35, 36, 36, 36, 36, 37, 37, 37, 38, 38, 38, 39, 39, 39, 39, 40, 40, 40, 41, 41, 41, 42, 42, 42, 42, 43, 43, 43, 44, 44, 44, 45, 45, 45, 45, 46, 46, 46, 47, 47, 47, 48, 48, 48, 48, 49, 49, 49, 50, 50, 50, 51, 51, 51, 51, 52, 52, 52, 53, 53, 53, 54, 54, 54, 54, 55, 55, 55, 56, 56, 56, 57, 57, 57, 57, 58, 58, 58, 59, 59, 59, 60, 60, 60, 60, 61, 61, 61, 62, 62, 62, 62, 63, 63, 63, 64, 64, 64, 65, 65, 65, 65, 66, 66, 66, 67, 67, 67, 68, 68, 68, 68, 69, 69, 69, 70, 70, 70, 71, 71, 71, 71, 72, 72, 72, 73, 73, 73, 74, 74, 74, 74, 75, 75, 75, 76, 76, 76 };

__u8 gtab[256] = {0, 1, 1, 2, 2, 3, 4, 4, 5, 5, 6, 6, 7, 8, 8, 9, 9, 10, 11, 11, 12, 12, 13, 14, 14, 15, 15, 16, 16, 17, 18, 18, 19, 19, 20, 21, 21, 22, 22, 23, 23, 24, 25, 25, 26, 26, 27, 28, 28, 29, 29, 30, 31, 31, 32, 32, 33, 33, 34, 35, 35, 36, 36, 37, 38, 38, 39, 39, 40, 41, 41, 42, 42, 43, 43, 44, 45, 45, 46, 46, 47, 48, 48, 49, 49, 50, 50, 51, 52, 52, 53, 53, 54, 55, 55, 56, 56, 57, 58, 58, 59, 59, 60, 60, 61, 62, 62, 63, 63, 64, 65, 65, 66, 66, 67, 68, 68, 69, 69, 70, 70, 71, 72, 72, 73, 73, 74, 75, 75, 76, 76, 77, 77, 78, 79, 79, 80, 80, 81, 82, 82, 83, 83, 84, 85, 85, 86, 86, 87, 87, 88, 89, 89, 90, 90, 91, 92, 92, 93, 93, 94, 95, 95, 96, 96, 97, 97, 98, 99, 99, 100, 100, 101, 102, 102, 103, 103, 104, 104, 105, 106, 106, 107, 107, 108, 109, 109, 110, 110, 111, 112, 112, 113, 113, 114, 114, 115, 116, 116, 117, 117, 118, 119, 119, 120, 120, 121, 122, 122, 123, 123, 124, 124, 125, 126, 126, 127, 127, 128, 129, 129, 130, 130, 131, 131, 132, 133, 133, 134, 134, 135, 136, 136, 137, 137, 138, 139, 139, 140, 140, 141, 141, 142, 143, 143, 144, 144, 145, 146, 146, 147, 147, 148, 149, 149, 149 };

__u8 btab[256] = {0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20, 21, 21, 21, 21, 21, 21, 21, 21, 21, 22, 22, 22, 22, 22, 22, 22, 22, 22, 23, 23, 23, 23, 23, 23, 23, 23, 23, 24, 24, 24, 24, 24, 24, 24, 24, 25, 25, 25, 25, 25, 25, 25, 25, 25, 26, 26, 26, 26, 26, 26, 26, 26, 26, 27, 27, 27, 27, 27, 27, 27, 27, 27, 28, 28, 28, 28, 28, 28, 28, 28, 29, 29, 29, 29, 29, 29 };

__s8 rytab[256] = {-128, -127, -126, -125, -124, -124, -123, -122, -121, -120, -119, -118, -117, -117, -116, -115, -114, -113, -112, -111, -110, -110, -109, -108, -107, -106, -105, -104, -103, -103, -102, -101, -100, -99, -98, -97, -96, -96, -95, -94, -93, -92, -91, -90, -89, -89, -88, -87, -86, -85, -84, -83, -82, -82, -81, -80, -79, -78, -77, -76, -75, -75, -74, -73, -72, -71, -70, -69, -68, -67, -67, -66, -65, -64, -63, -62, -61, -60, -60, -59, -58, -57, -56, -55, -54, -53, -53, -52, -51, -50, -49, -48, -47, -46, -46, -45, -44, -43, -42, -41, -40, -39, -39, -38, -37, -36, -35, -34, -33, -32, -32, -31, -30, -29, -28, -27, -26, -25, -25, -24, -23, -22, -21, -20, -19, -18, -17, -17, -16, -15, -14, -13, -12, -11, -10, -10, -9, -8, -7, -6, -5, -4, -3, -3, -2, -1, 0, 1, 2, 3, 4, 4, 5, 6, 7, 8, 9, 10, 11, 11, 12, 13, 14, 15, 16, 17, 18, 18, 19, 20, 21, 22, 23, 24, 25, 25, 26, 27, 28, 29, 30, 31, 32, 32, 33, 34, 35, 36, 37, 38, 39, 40, 40, 41, 42, 43, 44, 45, 46, 47, 47, 48, 49, 50, 51, 52, 53, 54, 54, 55, 56, 57, 58, 59, 60, 61, 61, 62, 63, 64, 65, 66, 67, 68, 68, 69, 70, 71, 72, 73, 74, 75, 75, 76, 77, 78, 79, 80, 81, 82, 82, 83, 84, 85, 86, 87, 88, 89, 89, 90, 91, 92, 93, 94, 95, 96};

__s8 bytab[256] = { -128, -128, -127, -127, -126, -126, -125, -125, -124, -124, -123, -123, -122, -122, -121, -121, -120, -120, -119, -119, -118, -118, -117, -117, -116, -116, -115, -115, -114, -114, -113, -113, -112, -112, -111, -111, -110, -110, -109, -109, -108, -108, -107, -107, -106, -106, -105, -105, -104, -104, -103, -103, -102, -102, -101, -101, -100, -100, -99, -99, -98, -98, -97, -97, -97, -96, -96, -95, -95, -94, -94, -93, -93, -92, -92, -91, -91, -90, -90, -89, -89, -88, -88, -87, -87, -86, -86, -85, -85, -84, -84, -83, -83, -82, -82, -81, -81, -80, -80, -79, -79, -78, -78, -77, -77, -76, -76, -75, -75, -74, -74, -73, -73, -72, -72, -71, -71, -70, -70, -69, -69, -68, -68, -67, -67, -66, -66, -66, -65, -65, -64, -64, -63, -63, -62, -62, -61, -61, -60, -60, -59, -59, -58, -58, -57, -57, -56, -56, -55, -55, -54, -54, -53, -53, -52, -52, -51, -51, -50, -50, -49, -49, -48, -48, -47, -47, -46, -46, -45, -45, -44, -44, -43, -43, -42, -42, -41, -41, -40, -40, -39, -39, -38, -38, -37, -37, -36, -36, -36, -35, -35, -34, -34, -33, -33, -32, -32, -31, -31, -30, -30, -29, -29, -28, -28, -27, -27, -26, -26, -25, -25, -24, -24, -23, -23, -22, -22, -21, -21, -20, -20, -19, -19, -18, -18, -17, -17, -16, -16, -15, -15, -14, -14, -13, -13, -12, -12, -11, -11, -10, -10, -9, -9, -8, -8, -7, -7, -6, -6, -5, -5, -5, -4, -4, -3, -3 };

   



   
/* // not rounded */
/* __u8 rtab[256] = */

static void by8touyvy( __u8 *dest, __u8* source, int width, int height )
{
   int i,j;
   int dest_offset = 0;   
   int x = 1;
   int y = 2;
   int imgwidth = width;
   
   

   
   for( j = y; j < (y+height); j += 2 )
   {
      int lineoffset = j * width;
      dest_offset = (j-y) * width * 2 + 2;

      for( i = x; i < (width + x); )
      {
	 int r, g, b;
	 int li = lineoffset + i;
	 int g1;
	 int g2;
	 int g3;
	 int g4;
	 
	 int d1;
	 int d2;

	 __u8 y,u,v;

	 g1 = source[ li - 1 ];		  	 
	 g2 = source[ li + 1 ];		  
	 g3 = source[ li - imgwidth ];
	 g4 = source[ li + imgwidth ];    

	 d1 = ABS( g1 - g2 );
	 d2 = ABS( g3 - g4 );


	 b = source[ li ];
	 r = ( ( (int)source[ ( li - imgwidth ) + 1 ] + 
		 (int)source[ ( li + imgwidth ) + 1 ] + 
		 (int)source[ ( li - imgwidth ) - 1 ] + 
		 (int)source[ ( li + imgwidth ) - 1 ] ) / 4 );
	 
	 if( d1 < d2 )
	 {
	    g = ( g1 + g2 ) / 2;
	 }
	 else
	 {
	    g = ( g3 + g4 ) / 2;
	 }	    

	 y = rtab[r] + gtab[g] + btab[b];
	 v = ( 0.877 * (float)(r-y) - 127 );
	 dest[dest_offset++] = v;
	 dest[dest_offset++] = y;

	 i++;

	 li = lineoffset + i;
	 
	 g = g2;
	 b = ( (int)source[ li - 1 ] + (int)source[ li + 1 ] ) / 2;
	 r = ( (int)source[ li - imgwidth ] + (int)source[ li + imgwidth ] ) / 2;

	 y = rtab[r] + gtab[g] + btab[b];
	 u = ( 0.492 * (float)(b-y) -127 );
	 
	 dest[dest_offset++] = u;
	 dest[dest_offset++] = y;
	 
	 i++;
      }
    
      lineoffset += imgwidth;
      dest_offset = ( (j-y) + 1 ) * width * 2 + 4;
      
      for( i = x+1; i < (width+x) - 3; )
      {
	 int r, g, b;
	 int g1;
	 int g2;
	 int g3;
	 int g4;
	 
	 int d1;
	 int d2;

	 __u8 y,u,v;

	 int li = lineoffset + i;

	 g1 = source[ li - 1 ];
	 g2 = source[ li + 1 ];
	 g3 = source[ li - imgwidth ];
	 g4 = source[ li + imgwidth ];
	 
	 d1 = ABS( g1 - g2 );
	 d2 = ABS( g3 - g4 );

	 r = source[ li ];

	 if( d1 < d2 )
	 {
	    g = ( g1 + g2 ) / 2;
	 }
	    
	 else
	 {
	    g = ( g3 + g4 ) / 2;
	 }

	 b = ( ( (int)source[ ( li - imgwidth ) + 1 ] + 
		 (int)source[ ( li + imgwidth ) + 1 ] + 
		 (int)source[ ( li - imgwidth ) - 1 ] + 
		 (int)source[ ( li + imgwidth ) - 1 ] ) / 4 );
	 

/* 	 y = 0.299 * (float)r + 0.587 * (float)g + 0.114 * (float)b; */
	 y = rtab[r] + gtab[g] + btab[b];
	 u = (0.492 * (float)(b-y)-128);

	 dest[dest_offset++] = u;
	 dest[dest_offset++] = y;
	 
	 i++;
	 li = lineoffset + i;
	 
	 g = g2;
	 r = ( (int)source[ li - 1 ] + (int)source[ li + 1 ] ) / 2;
	 b = ( (int)source[ li - imgwidth ] + (int)source[ li + imgwidth ] ) / 2;
	 
/* 	 y = 0.299 * (float)r + 0.587 * (float)g + 0.114 * (float)b; */
	 y = rtab[r] + gtab[g] + btab[b];
	 v = (0.877 * (float)(r-y)-128);
	 

	 dest[dest_offset++] = v;
	 dest[dest_offset++] = y;

	 i++; 
      }
   }   
   
}


static void y16touyvy( __u8 *dest, __u8* _source, int width, int height, int shift )
{
   int dest_size = width * height * 2;
   int offset;
   __u16 *source = (__u16*)_source;

   for( offset = 0; offset < dest_size; offset += 2 )
   {
      dest[offset] = 0x7f;
      dest[offset+1] = (source[offset/2]>>shift );
   }   
}

static void y16toyuy2( __u8 *dest, __u8* _source, int width, int height, int shift )
{
   int dest_size = width * height * 2;
   int offset;
   __u16 *source = (__u16*)_source;

   for( offset = 0; offset < dest_size; offset += 2 )
   {
      dest[offset+1] = (source[offset/2]>>6 );
      dest[offset] = 0x7f;
   }   
}

static void y16torgb24( __u8 *dest, __u8* _source, int width, int height, int shift )
{
   int dest_size = width * height * 3;
   int offset;
   
   __u16 *source = (__u16*)_source;

   for( offset = 0; offset < dest_size; offset += 3 )
   {
      char c = (source[offset/3]>>shift );

      dest[offset] = c;
      dest[offset+1] = c;
      dest[offset+2] = c;
   }
}

static void rggbtouyvy_debayer( __u8 *dest, __u8 *_source, int width, int height, int shift )
{
   int i,j;
   int dest_offset = 0;   
   int x = 1;
   int y = 2;
   int imgwidth = width;
   __u16 *source = (__u16*)_source;
   
   for( j = y; j < (y+height); j += 2 )
   {
      int lineoffset = j * width;
      dest_offset = (j-y) * width * 2 + 4;

      for( i = x+1; i < (width+x) - 3; )
      {
	 unsigned int r, g, b;
	 int g1;
	 int g2;
	 int g3;
	 int g4;
	 
	 int d1;
	 int d2;

	 __u8 y,u,v;

	 int li = lineoffset + i;

	 g1 = SHIFT(source[ li - 1 ]);
	 g2 = SHIFT(source[ li + 1 ]);
	 g3 = SHIFT(source[ li - imgwidth ]);
	 g4 = SHIFT(source[ li + imgwidth ]);
	 
	 d1 = ABS( g1 - g2 );
	 d2 = ABS( g3 - g4 );

	 r = SHIFT(source[ li ]);

	 if( d1 < d2 )
	 {
	    g = ( g1 + g2 ) / 2;
	 }	    
	 else
	 {
	    g = ( g3 + g4 ) / 2;
	 }

	 b = ( ( (int)SHIFT(source[ ( li - imgwidth ) + 1 ]) +
		 (int)SHIFT(source[ ( li + imgwidth ) + 1 ]) +
		 (int)SHIFT(source[ ( li - imgwidth ) - 1 ]) +
		 (int)SHIFT(source[ ( li + imgwidth ) - 1 ]) ) / 4 );

	 y = ( (  66 * r + 129 * g +  25 * b + 128) >> 8) +  16;
	 u = ( ( -38 * r -  74 * g + 112 * b + 128) >> 8) + 128;

	 dest[dest_offset++] = u;
	 dest[dest_offset++] = y;
	 
	 i++;
	 li = lineoffset + i;
	 
	 g = g2;
	 r = ( (int)SHIFT(source[ li - 1 ]) + (int)SHIFT(source[ li + 1 ]) ) / 2;
	 b = ( (int)SHIFT(source[ li - imgwidth ]) + (int)SHIFT(source[ li + imgwidth ]) ) / 2;	 
	 
	 y = ( (  66 * r + 129 * g +  25 * b + 128) >> 8) +  16;
	 v = ( ( 112 * r -  94 * g -  18 * b + 128) >> 8) + 128;
	 

	 dest[dest_offset++] = v;
	 dest[dest_offset++] = y;

	 i++; 
      }
    
      lineoffset += imgwidth;
      dest_offset = ( (j-y) + 1 ) * width * 2 + 2;
      
      for( i = x; i < (width + x); )
      {
	 int r, g, b;
	 int li = lineoffset + i;
	 int g1;
	 int g2;
	 int g3;
	 int g4;
	 
	 int d1;
	 int d2;

	 __u8 y,u,v;

	 g1 = SHIFT(source[ li - 1 ]);		  	 
	 g2 = SHIFT(source[ li + 1 ]);		  
	 g3 = SHIFT(source[ li - imgwidth ]);
	 g4 = SHIFT(source[ li + imgwidth ]);

	 d1 = ABS( g1 - g2 );
	 d2 = ABS( g3 - g4 );


	 b = SHIFT(source[ li ]);
	 r = ( ( (int)SHIFT(source[ ( li - imgwidth ) + 1 ]) +
		 (int)SHIFT(source[ ( li + imgwidth ) + 1 ]) +
		 (int)SHIFT(source[ ( li - imgwidth ) - 1 ]) +
		 (int)SHIFT(source[ ( li + imgwidth ) - 1 ]) ) / 4 );
	 
	 if( d1 < d2 )
	 {
	    g = ( g1 + g2 ) / 2;
	 }
	 else
	 {
	    g = ( g3 + g4 ) / 2;
	 }
	 

	 y = ( (  66 * r + 129 * g +  25 * b + 128) >> 8) +  16;
	 v = ( ( 112 * r -  94 * g -  18 * b + 128) >> 8) + 128;
	 dest[dest_offset++] = v;
	 dest[dest_offset++] = y;

	 i++;

	 li = lineoffset + i;
	 
	 g = g2;
	 b = ( (int)SHIFT(source[ li - 1 ]) + (int)SHIFT(source[ li + 1 ]) ) / 2;
	 r = ( (int)SHIFT(source[ li - imgwidth ]) + (int)SHIFT(source[ li + imgwidth ] )) / 2;


	 y = ( (  66 * r + 129 * g +  25 * b + 128) >> 8) +  16;
	 u = ( ( -38 * r -  74 * g + 112 * b + 128) >> 8) + 128;
	 
	 dest[dest_offset++] = u;
	 dest[dest_offset++] = y;
	 
	 i++;
      }
   }      
}
