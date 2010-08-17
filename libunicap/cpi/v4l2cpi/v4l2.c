/*
  unicap
  Copyright (C) 2004  Arne Caspari

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

#define N_(x) x

#include "config.h"
#include <unicap.h>
#include <unicap_status.h>
#include <unicap_cpi.h>
#include <queue.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <dirent.h>


#include <semaphore.h>

#include <errno.h>
#include <pthread.h>

#include <linux/types.h>
#include <linux/videodev2.h>
#include "v4l2.h"
#include "buffermanager.h"

#include "tisuvccam.h"
#include "tiseuvccam.h"

#if USE_LIBV4L
#include <libv4l2.h>
#endif


#if V4L2_DEBUG
#define DEBUG
#endif
#include "debug.h"

#define V4L2_VIDEO_IN_PPTY_NAME   N_("video source")
#define V4L2_VIDEO_NORM_PPTY_NAME N_("video norm")
#define V4L2_VIDEO_FRAMERATE_PPTY_NAME N_("frame rate")


#ifndef V4L2_CTRL_CLASS_CAMERA
#define V4L2_CTRL_CLASS_CAMERA 0x009A0000	/* Camera class controls */
#define V4L2_CID_CAMERA_CLASS_BASE 		(V4L2_CTRL_CLASS_CAMERA | 0x900)
#endif


#if USE_LIBV4L
 #define OPEN   v4l2_open
 #define CLOSE  v4l2_close
 #define IOCTL  v4l2_ioctl
 #define MMAP   v4l2_mmap
 #define MUNMAP v4l2_munmap
#else
 #define OPEN   open
 #define CLOSE  close
 #define IOCTL  ioctl
 #define MMAP   mmap
 #define MUNMAP munmap
#endif




struct prop_category
{
      char *property_id;
      char *category;
};


struct fourcc_bpp
{
      __u32 fourcc;
      int bpp;
};



struct prop_category category_list[] =
{
   { N_("shutter"), N_("exposure") },
   { N_("gain"), "exposure" }, 
   { N_("brightness"), "exposure" },
   { N_("white balance red component"), N_("color") },
   { N_("white balance blue component"), "color" },
   { N_("white balance component, auto"), "color" },
   { N_("white balance temperature"), "color" },
   { N_("saturation"), "color" },
};



static struct v4l2_uc_compat v4l2_uc_compat_list[] = 
{
   { 
      driver: "uvcvideo", 
      probe_func: tisuvccam_probe,
      count_ext_property_func: tisuvccam_count_ext_property, 
      enumerate_property_func: tisuvccam_enumerate_properties,
      override_property_func: tisuvccam_override_property,
      set_property_func: tisuvccam_set_property,
      get_property_func: tisuvccam_get_property,
      fmt_get_func: tisuvccam_fmt_get,
      override_framesize_func: NULL, 
      tov4l2format_func: NULL, 
   },

   { 
      driver: "uvcvideo", 
      probe_func: tiseuvccam_probe, 
      count_ext_property_func: tiseuvccam_count_ext_property, 
      enumerate_property_func: tiseuvccam_enumerate_properties,
      override_property_func: tiseuvccam_override_property,
      set_property_func: tiseuvccam_set_property,
      get_property_func: tiseuvccam_get_property,
      fmt_get_func: tiseuvccam_fmt_get,
#ifdef VIDIOC_ENUM_FRAMESIZES
      override_framesize_func: tiseuvccam_override_framesize, 
#else
      override_framesize_func: NULL,
#endif
      tov4l2format_func: tiseuvccam_tov4l2format,
   },   
};



static struct fourcc_bpp fourcc_bpp_map[] =
{
   { V4L2_PIX_FMT_GREY,     8 },
   { V4L2_PIX_FMT_YUYV,    16 },
   { V4L2_PIX_FMT_UYVY,    16 },
   { V4L2_PIX_FMT_Y41P,    12 },
   { V4L2_PIX_FMT_YVU420,  12 },
   { V4L2_PIX_FMT_YUV420,  12 },
   { V4L2_PIX_FMT_YVU410,   9 },
   { V4L2_PIX_FMT_YUV410,   9 },
   { V4L2_PIX_FMT_YUV422P, 16 },
   { V4L2_PIX_FMT_YUV411P, 12 },
   { V4L2_PIX_FMT_NV12,    12 },
   { V4L2_PIX_FMT_NV21,    12 },
   { V4L2_PIX_FMT_YYUV,    16 },
   { V4L2_PIX_FMT_HI240,    8 },
   { V4L2_PIX_FMT_RGB332,   8 },
   { V4L2_PIX_FMT_RGB555,  16 },
   { V4L2_PIX_FMT_RGB565,  16 },
   { V4L2_PIX_FMT_RGB555X, 16 },
   { V4L2_PIX_FMT_RGB565X, 16 },
   { V4L2_PIX_FMT_BGR24,   24 },
   { V4L2_PIX_FMT_RGB24,   24 },
   { V4L2_PIX_FMT_BGR32,   32 },
   { V4L2_PIX_FMT_RGB32,   32 },
   { FOURCC( 'Y', '8', '0', '0' ), 8 }, 
   { FOURCC( 'B', 'Y', '8', ' ' ), 8 },
   { FOURCC( 'B', 'A', '8', '1' ), 8 },
};

struct size_map_s
{
      char *card_name;
      unicap_rect_t *sizes;
      int size_count;
};

static unicap_rect_t try_sizes[] =
{
   { 0,0, 160, 120 }, 
   { 0,0, 176, 144 }, 
   { 0,0, 320, 240 }, 
   { 0,0, 384, 288 }, 
   { 0,0, 640, 480 }, 
   { 0,0, 720, 480 },
   { 0,0, 720, 576 },
   { 0,0, 768, 576 },
   { 0,0, 800, 600 },
   { 0,0, 1024, 768 },
   { 0,0, 1280, 960 },
};

static unicap_rect_t webcam_sizes[] =
{
   { 0,0, 160, 120 }, 
   { 0,0, 320, 240 },
   { 0,0, 352, 288 }, 
   { 0,0, 640, 480 }, 
   { 0,0, 748, 480 },
};

static unicap_rect_t dxx41_sizes[] =
{
   { 0,0, 1280, 960 }, 
};
  
static struct size_map_s format_size_map[] = 
{
   { N_("STK-1135 USB2 Camera Controller"), webcam_sizes, sizeof( webcam_sizes ) / sizeof( unicap_rect_t ) },
   { N_("DFx 41"), dxx41_sizes, sizeof( dxx41_sizes ) / sizeof( unicap_rect_t ) },
};


static unicap_status_t v4l2_enum_frameintervals( v4l2_handle_t handle, unicap_property_t *property );

static unicap_status_t v4l2_enumerate_devices( unicap_device_t *device, int index );
static unicap_status_t v4l2_cpi_open( void **cpi_data, unicap_device_t *device );
static unicap_status_t v4l2_cpi_close( void *cpi_data );
static unicap_status_t v4l2_reenumerate_formats( void *cpi_data, int *_pcount );
static unicap_status_t v4l2_enumerate_formats( void *cpi_data, 
					       unicap_format_t *format, int index );
static unicap_status_t v4l2_set_format( void *cpi_data, unicap_format_t *format );
static unicap_status_t v4l2_get_format( void *cpi_data, unicap_format_t *format );
static unicap_status_t v4l2_reenumerate_properties( void *cpi_data, int *_pcount );
static unicap_status_t v4l2_enumerate_properties( void *cpi_data, 
						  unicap_property_t *property, int index );
static unicap_status_t v4l2_set_property( void *cpi_data, unicap_property_t *property );
static unicap_status_t v4l2_get_property( void *cpi_data, unicap_property_t *property );
static unicap_status_t v4l2_capture_start( void *cpi_data );
static unicap_status_t v4l2_capture_stop( void *cpi_data );
static unicap_status_t v4l2_queue_buffer( void *cpi_data, unicap_data_buffer_t *buffer );
static unicap_status_t v4l2_dequeue_buffer( void *cpi_data, unicap_data_buffer_t **buffer );
static unicap_status_t v4l2_wait_buffer( void *cpi_data, unicap_data_buffer_t **buffer );
static unicap_status_t v4l2_poll_buffer( void *cpi_data, int *count );
static unicap_status_t v4l2_set_event_notify( void *cpi_data, 
					      unicap_event_callback_t func, 
					      unicap_handle_t unicap_handle );

static unicap_status_t queue_buffer( v4l2_handle_t handle, unicap_data_buffer_t *buffer );
static unicap_status_t queue_system_buffers( v4l2_handle_t handle );
static void v4l2_capture_thread( v4l2_handle_t handle );

static struct _unicap_cpi cpi_s = 
{
   cpi_version: 1<<16,
   cpi_capabilities: 0x3ffff,

   cpi_enumerate_devices: v4l2_enumerate_devices,
   cpi_open: v4l2_cpi_open, 
   cpi_close: v4l2_cpi_close,

   cpi_reenumerate_formats: v4l2_reenumerate_formats, 
   cpi_enumerate_formats: v4l2_enumerate_formats,   
   cpi_set_format: v4l2_set_format,	   
   cpi_get_format: v4l2_get_format,          

   cpi_reenumerate_properties: v4l2_reenumerate_properties,
   cpi_enumerate_properties: v4l2_enumerate_properties,
   cpi_set_property: v4l2_set_property,
   cpi_get_property: v4l2_get_property,

   cpi_capture_start: v4l2_capture_start,
   cpi_capture_stop: v4l2_capture_stop,
   
   cpi_queue_buffer: v4l2_queue_buffer,
   cpi_dequeue_buffer: v4l2_dequeue_buffer,
   cpi_wait_buffer: v4l2_wait_buffer,
   cpi_poll_buffer: v4l2_poll_buffer,

   cpi_set_event_notify: v4l2_set_event_notify,
};


#if ENABLE_STATIC_CPI
void unicap_v4l2_register_static_cpi( struct _unicap_cpi **cpi )
{
   *cpi = &cpi_s;
}
#else
unicap_status_t cpi_register( struct _unicap_cpi *reg_data )
{
   memcpy( reg_data, &cpi_s, sizeof( struct _unicap_cpi ) );

   return STATUS_SUCCESS;
}
#endif//ENABLE_STATIC_CPI


static unicap_status_t get_current_format( v4l2_handle_t handle, unicap_format_t *format )
{
   struct v4l2_cropcap v4l2_crop;
   struct v4l2_format v4l2_fmt;
   struct v4l2_fmtdesc v4l2_fmtdesc;
   int i;
   int index = -1;
   unsigned int pixelformat;

   v4l2_crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

   if( IOCTL( handle->fd, VIDIOC_CROPCAP, &v4l2_crop ) < 0 )
   {
      v4l2_crop.defrect.width = v4l2_crop.bounds.width = 640;
      v4l2_crop.defrect.height = v4l2_crop.bounds.height = 480;
   }

   v4l2_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
   if( IOCTL( handle->fd, VIDIOC_G_FMT, &v4l2_fmt ) )
   {
      TRACE( "VIDIOC_G_FMT ioctl failed: %s\n", strerror( errno ) );
      return STATUS_FAILURE;
   }

   pixelformat = v4l2_fmt.fmt.pix.pixelformat;

   if( handle->compat )
   {
      memset( &v4l2_fmtdesc, 0x0, sizeof( v4l2_fmtdesc ) );
      v4l2_fmtdesc.pixelformat = v4l2_fmt.fmt.pix.pixelformat;
      handle->compat->fmt_get_func( &v4l2_fmtdesc, &v4l2_crop, NULL, &pixelformat, NULL  );
   }

   

   for( i = 0; i < handle->format_count; i++ )
   {
      if( pixelformat == handle->unicap_formats[ i ].fourcc )
      {
	 index = i;
	 break;
      }
   }
	
   if( index == -1 )
   {
      TRACE( "v4l2 get format: format not in the list of currently known formats; pixelformat = %08x\n", pixelformat );
      return STATUS_FAILURE;
   }

   unicap_copy_format( format, &handle->unicap_formats[index] );
   return STATUS_SUCCESS;
}

static int file_filter( const struct dirent *a )
{
   int match = 0;
   
   // match: 'videoXY' where X = {0..9} and Y = {0..9}
   if( !strncmp( a->d_name, "video", 5 ) )
   {
      if( strlen( a->d_name ) > 5 )
      {
	 if( ( a->d_name[5] >= '0' ) && ( a->d_name[5] <= '9' ) ) // match
							      // the 'X'
	 {
	    match = 1;
	 }
	 
	 if( strlen( a->d_name ) > 6 )
	 {
	    match = 0;
	    
	    if( ( a->d_name[6] >= '0' ) && ( a->d_name[6] <= '9' ) )
	    {
	       match = 1;
	    }
	 }
	 
	 if( strlen( a->d_name ) > 7 )
	 {
	    match = 0;
	 }
      }
   }
   
   return match;
}

static unicap_status_t v4l2_enumerate_devices( unicap_device_t *device, int index )
{
   int fd;
   struct v4l2_capability v4l2caps;
   struct dirent **namelist;
   int n;
   int found = -1;
   char devname[512];
      
   TRACE( "v4l2_enumerate_devices[%d]\n", index );
   
   n = scandir( "/dev", &namelist, file_filter, alphasort );
   if( n < 0 )
   {
      TRACE( "Failed to scan directory '/dev' \n" );
      return STATUS_NO_DEVICE;
   }

   while( ( found != index ) && n-- )
   {      
      sprintf( devname, "/dev/%s", namelist[n]->d_name );
      
/*       free( namelist[n]->d_name ); */
      TRACE( "v4l2: open %s\n", devname );
      if( ( fd = open( devname, O_RDONLY | O_NONBLOCK ) ) == -1 )
      {
	 TRACE( "v4l2_cpi: open(%s): %s\n", devname, strerror( errno ) );
	 continue;
      }

#if USE_LIBV4L
      v4l2_fd_open( fd, V4L2_ENABLE_ENUM_FMT_EMULATION );
#endif

      
      if( IOCTL( fd, VIDIOC_QUERYCAP, &v4l2caps ) < 0 )
      {
	 TRACE( "ioctl failed\n" );
	 close( fd );
	 continue;
      }
      
      if( !(v4l2caps.capabilities & V4L2_CAP_VIDEO_CAPTURE) )
      {
	 // v4l version one device
	 CLOSE( fd );
	 continue;
      }

      found++;
      CLOSE( fd );
   }
/*    free( namelist ); */

   if( found != index )
   {
      return STATUS_NO_DEVICE;
   }


   sprintf( device->identifier, "%s (%s)", v4l2caps.card, devname );
   strcpy( device->model_name, (char*)v4l2caps.card );
   strcpy( device->vendor_name, "");
   device->model_id = 0x1;
   device->vendor_id = 0xffff0000;
   device->flags = UNICAP_CPI_SERIALIZED;
   strcpy( device->device, devname ) ;  

   return STATUS_SUCCESS;
}

static unicap_status_t v4l2_cpi_open( void **cpi_data, unicap_device_t *device )
{
   v4l2_handle_t handle = NULL;
   struct v4l2_capability v4l2caps;
   char identifier[128];
	
   int i;

   TRACE( "v4l2_cpi_open\n" );

   *cpi_data = malloc( sizeof( struct _v4l2_handle ) );
   memset( *cpi_data, 0x0, sizeof( struct _v4l2_handle ) );

   handle = (v4l2_handle_t) *cpi_data;   

   if( sem_init( &handle->sema, 0, 1 ) )
   {
      TRACE( "sem_init failed!\n" );
      free( handle );
      return STATUS_FAILURE;
   }

   handle->removed = 0;
   
   handle->io_method = CPI_V4L2_IO_METHOD_MMAP;
   handle->buffer_count = V4L2_NUM_BUFFERS;

   handle->fd = open( device->device, O_RDWR );  
   if( handle->fd == -1 )
   {
      TRACE( "v4l2 open failed\n" );
      free( handle );
      return STATUS_FAILURE;
   }
	
#if USE_LIBV4L
      v4l2_fd_open( handle->fd, V4L2_ENABLE_ENUM_FMT_EMULATION );
#endif

   for( i = 0; i < V4L2_MAX_VIDEO_INPUTS; i++ )
   {
      handle->video_inputs[i] = malloc( 32 );
   }
   
   for( i = 0; i < V4L2_MAX_VIDEO_NORMS; i++ )
   {
      handle->video_norms[i] = malloc( 32 );
   }

   memset( &v4l2caps, 0x0, sizeof( v4l2caps ) );
   if( IOCTL( handle->fd, VIDIOC_QUERYCAP, &v4l2caps ) < 0 )
   {
      TRACE( "ioctl failed\n" );
      for( i = 0; i < V4L2_MAX_VIDEO_INPUTS; i++ )
      {
	 free( handle->video_inputs[i] );
      }
      for( i = 0; i < V4L2_MAX_VIDEO_NORMS; i++ )
      {
	 free( handle->video_norms[i] );
      }
      CLOSE( handle->fd );
      free( handle );
      return STATUS_FAILURE;
   }
	
   sprintf( identifier, "%s (%s)", v4l2caps.card, device->device );
   
   if( strcmp( identifier, device->identifier ) )
   {
      for( i = 0; i < V4L2_MAX_VIDEO_INPUTS; i++ )
      {
	 free( handle->video_inputs[i] );
      }
      for( i = 0; i < V4L2_MAX_VIDEO_NORMS; i++ )
      {
	 free( handle->video_norms[i] );
      }
      CLOSE( handle->fd );
      free( handle );
      return STATUS_NO_MATCH;
   }

   strcpy( handle->card_name, (char*) v4l2caps.card );

   for( i = 0; i < ( sizeof( v4l2_uc_compat_list ) / sizeof( struct v4l2_uc_compat ) ); i++ )
   {
      if( !strcmp( (char*)v4l2_uc_compat_list[i].driver, (char*)v4l2caps.driver ) )
      {
	 if( v4l2_uc_compat_list[i].probe_func( handle, device->device ) )
	 {
	    handle->compat = &v4l2_uc_compat_list[i];
	    break;
	 }
      }
   }

   v4l2_reenumerate_formats( handle, NULL );
   
   get_current_format( handle, &handle->current_format );
   v4l2_reenumerate_properties( handle, NULL );


   handle->in_queue = ucutil_queue_new();
   handle->out_queue = ucutil_queue_new();

   return STATUS_SUCCESS;
}

static unicap_status_t v4l2_cpi_close( void *cpi_data )
{
   v4l2_handle_t handle = (v4l2_handle_t) cpi_data;

   TRACE( "v4l2_cpi_close\n" );
   
   if( handle->capture_running )
   {
      v4l2_capture_stop( cpi_data );
   }

   if( handle->unicap_formats )
   {
      int i;
      
      if( handle->sizes_allocated )
      {
	 for( i = 0; i < handle->format_count; i++ )
	 {
	    if( handle->unicap_formats[i].sizes )
	    {
	       free( handle->unicap_formats[i].sizes );
	    }
	 }
      }

      free( handle->unicap_formats );
      handle->unicap_formats = 0;
   }
	
   if( handle->unicap_properties )
   {
      free( handle->unicap_properties );
      handle->unicap_properties = 0;
   }
	
   if( handle->control_ids )
   {
      free( handle->control_ids );
      handle->control_ids = 0;
   }

   if( handle->unicap_handle )
   {
      free( handle->unicap_handle );
   }
   

   CLOSE( handle->fd );

   sem_destroy( &handle->sema );

   free( handle );

   return STATUS_SUCCESS;
}

static int v4l2_get_bpp( struct v4l2_fmtdesc *fmt )
{
   int bpp = 0;
   int i;
	
   for( i = 0; i < ( sizeof( fourcc_bpp_map ) / sizeof( struct fourcc_bpp ) ); i++ )
   {
      if( fourcc_bpp_map[i].fourcc == fmt->pixelformat )
      {
	 bpp = fourcc_bpp_map[i].bpp;
	 break;
      }
   }
	

   return bpp;
}

static unicap_rect_t *try_enum_framesizes( v4l2_handle_t handle, __u32 fourcc, int *pcount )
{
#ifndef VIDIOC_ENUM_FRAMESIZES
   TRACE( "ENUM_FRAMESIZES not supported\n" );
   return NULL;
#else
   int nfound = 0;
   struct v4l2_frmsizeenum frms;
   unicap_rect_t *sizes;
   
   frms.pixel_format = fourcc;
   
   for( frms.index = 0; IOCTL( handle->fd, VIDIOC_ENUM_FRAMESIZES, &frms ) == 0; frms.index++ )
   {
      if( frms.type == V4L2_FRMSIZE_TYPE_DISCRETE )
      {
	 nfound++;
      }
      else
      {
	 TRACE( "VIDIOC_ENUM_FRAME_SIZES returned unsupported type\n" );
	 return NULL;
      }
   }
   
   /* If the IOCTL fails the first time this means the cam doesn't support
      VIDIOC_ENUM_FRAME_SIZES ! */
   if (!nfound)
      return NULL;
   
   sizes = malloc( sizeof( unicap_rect_t ) * nfound );
   
   for( frms.index = 0; ( frms.index < nfound ) && ( IOCTL( handle->fd, VIDIOC_ENUM_FRAMESIZES, &frms ) == 0 ); frms.index++ )
   {
      if( handle->compat && handle->compat->override_framesize_func )
      {
	 handle->compat->override_framesize_func( handle, &frms );
      }
      
      if( frms.type == V4L2_FRMSIZE_TYPE_DISCRETE )
      {
	 sizes[frms.index].x = 0;
	 sizes[frms.index].y = 0;
	 sizes[frms.index].width = frms.discrete.width;
	 sizes[frms.index].height = frms.discrete.height;
      }
      else
      {
	 TRACE( "VIDIOC_ENUM_FRAME_SIZES returned unsupported type\n" );
	 return NULL;
      }
   }

   TRACE( "found %d framesizes for fourcc: %08x\n", nfound, fourcc );
   
   *pcount = nfound;
   return sizes;
#endif
}
   

static unicap_rect_t *build_format_size_table( v4l2_handle_t handle, __u32 fourcc, int *pcount )
{
   int nfound = 0;
   int i;
   unicap_rect_t sfound[sizeof(try_sizes)/sizeof(unicap_rect_t)];
   struct v4l2_format v4l2_fmt;
   unicap_rect_t *sizes = NULL;
   int n;
   
   handle->sizes_allocated = 1;
   memset( sfound, 0x0, sizeof( sfound ) );

   sizes = try_enum_framesizes( handle, fourcc, pcount );
   if( sizes )
   {
      return sizes;
   }

   // First check whether the sizes for the device are already known
   n = sizeof( format_size_map ) / sizeof( struct size_map_s );
   for( i = 0; i < n; i++ )
   {
      if( !strncmp( format_size_map[i].card_name, handle->card_name, sizeof( format_size_map[i].card_name ) ) )
      {
	 handle->sizes_allocated = 0;
	 *pcount = format_size_map[i].size_count;
	 return format_size_map[i].sizes;
      }
   } 

   for( i = 0; i < sizeof(try_sizes)/sizeof(unicap_rect_t); i++ )
   {
      v4l2_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
      v4l2_fmt.fmt.pix.width = try_sizes[i].width;
      v4l2_fmt.fmt.pix.height = try_sizes[i].height;
      v4l2_fmt.fmt.pix.pixelformat = fourcc;
      v4l2_fmt.fmt.pix.field = V4L2_FIELD_ANY;
      if( IOCTL( handle->fd, VIDIOC_S_FMT, &v4l2_fmt ) )
      {
	 TRACE( "VIDIOC_S_FMT ioctl failed: %s\n", strerror( errno ) );
      }
      else
      {
	 int j;
	 int found_fmt = 0;
	 for( j = 0; j < nfound; j++ )
	 {
	    if( ( sfound[j].width == v4l2_fmt.fmt.pix.width ) &&
		( sfound[j].height == v4l2_fmt.fmt.pix.height ) )
	    {
	       found_fmt = 1;
	       break;
	    }
	 }
	 
	 if( !found_fmt )
	 {
	    sfound[nfound].width = v4l2_fmt.fmt.pix.width;
	    sfound[nfound].height = v4l2_fmt.fmt.pix.height;
	    TRACE( "Found new size: %dx%d\n", sfound[nfound].width, sfound[nfound].height );
	    nfound++;
	 }	 
      }
   }

   if( nfound )
   {
      sizes = malloc( sizeof( unicap_rect_t ) * nfound );
      memcpy( sizes, sfound, nfound * sizeof( unicap_rect_t ) );
   }

   *pcount = nfound;

   return sizes;
}

static unicap_status_t v4l2_reenumerate_formats( void *cpi_data, int *_pcount )
{
   v4l2_handle_t handle = (v4l2_handle_t) cpi_data;

   int count;
   struct v4l2_fmtdesc v4l2_fmt;
   struct v4l2_cropcap v4l2_crop;
   struct v4l2_cropcap v4l2_crop_orig;
	
   if( handle->unicap_formats && handle->sizes_allocated )
   {
      int i;
      
      for( i = 0; i < handle->format_count; i++ )
      {
	 if( handle->unicap_formats[i].sizes )
	 {
	    free( handle->unicap_formats[i].sizes );
	 }
      }

      free( handle->unicap_formats );
      handle->unicap_formats = 0;
   }

   memset( handle->format_mask, 0x0, sizeof( handle->format_mask ) );

   count = 0;

   handle->unicap_formats = (unicap_format_t *) calloc( MAX_V4L2_FORMATS, sizeof( unicap_format_t ) );

   v4l2_crop_orig.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

   if( IOCTL( handle->fd, VIDIOC_CROPCAP, &v4l2_crop_orig ) < 0 )
   {
      v4l2_crop_orig.defrect.width = v4l2_crop_orig.bounds.width = 768;
      v4l2_crop_orig.defrect.height = v4l2_crop_orig.bounds.height = 576;
   }
	
   v4l2_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
   for( v4l2_fmt.index = 0; !IOCTL( handle->fd, VIDIOC_ENUM_FMT, &v4l2_fmt ); v4l2_fmt.index++ )
   {
      int size_count;
      unicap_rect_t *sizes;
      char *identifier;
      unsigned int fourcc;
      int bpp;
      char tmp_identifier[128];

      memcpy( &v4l2_crop, &v4l2_crop_orig, sizeof( v4l2_crop ) );
      
      sprintf( tmp_identifier, "%s ( %c%c%c%c )", 
	       v4l2_fmt.description, 
	       ( v4l2_fmt.pixelformat & 0xff ), 
	       ( v4l2_fmt.pixelformat >> 8 & 0xff ), 
	       ( v4l2_fmt.pixelformat >> 16 & 0xff ), 
	       ( v4l2_fmt.pixelformat >> 24 & 0xff ) );
      identifier = tmp_identifier;
      bpp = v4l2_get_bpp( &v4l2_fmt );
      fourcc = v4l2_fmt.pixelformat;

      if( handle->compat && ( handle->compat->fmt_get_func( &v4l2_fmt, &v4l2_crop, &identifier, &fourcc, &bpp  ) == STATUS_SKIP_CTRL ) )
      {
	 handle->format_mask[v4l2_fmt.index] = 1;
	 handle->unicap_formats[v4l2_fmt.index].size_count = 0;
	 continue;
      }

      count++;

      strcpy( handle->unicap_formats[v4l2_fmt.index].identifier, identifier );
      handle->unicap_formats[v4l2_fmt.index].bpp = bpp;
      handle->unicap_formats[v4l2_fmt.index].fourcc = fourcc;

      TRACE( "enum format: index %d desc %s\n", v4l2_fmt.index, v4l2_fmt. description );
		
      handle->unicap_formats[v4l2_fmt.index].size.width = v4l2_crop.defrect.width;
      handle->unicap_formats[v4l2_fmt.index].size.height = v4l2_crop.defrect.height;
      handle->unicap_formats[v4l2_fmt.index].buffer_type = UNICAP_BUFFER_TYPE_USER;

      sizes = build_format_size_table( handle, v4l2_fmt.pixelformat, &size_count );
		
      if( size_count == 0 )
      {
	 handle->unicap_formats[v4l2_fmt.index].min_size.width = 
	    handle->unicap_formats[v4l2_fmt.index].min_size.height = 1;
	 handle->unicap_formats[v4l2_fmt.index].max_size.width = v4l2_crop.bounds.width;
	 handle->unicap_formats[v4l2_fmt.index].max_size.height = v4l2_crop.bounds.height;
	 if( v4l2_crop.defrect.width && v4l2_crop.defrect.height )
	 {
	    handle->unicap_formats[v4l2_fmt.index].buffer_size = 
	       v4l2_crop.defrect.width * v4l2_crop.defrect.height * handle->unicap_formats[v4l2_fmt.index].bpp / 8;
	 }
	 else
	 {
	    handle->unicap_formats[v4l2_fmt.index].buffer_size =
	       v4l2_crop.bounds.width * v4l2_crop.bounds.height * handle->unicap_formats[v4l2_fmt.index].bpp / 8;
	 }
	 
	 handle->unicap_formats[v4l2_fmt.index].h_stepping = 16;
	 handle->unicap_formats[v4l2_fmt.index].v_stepping = 16;
	 handle->unicap_formats[v4l2_fmt.index].sizes = 0;
	 handle->unicap_formats[v4l2_fmt.index].size_count = 0;
      }
      else
      {
	 // TODO : fix: currently assuming smallest format is first;
	 // largest is last
	 int i; 
	 int min_width = 0x7fffffff;
	 int max_width = 0;
	 int min_height = 0x7fffffff;
	 int max_height = 0;
	 for( i = 0; i < size_count; i++ )
	 {
	    if( sizes[i].width < min_width )
	    {
	       min_width = sizes[i].width;
	    }
	    
	    if( sizes[i].width > max_width )
	    {
	       max_width = sizes[i].width;   
	    }
	    
	    if( sizes[i].height < min_height )
	    {
	       min_height = sizes[i].height;   
	    }
	    
	    if( sizes[i].height > max_height )
	    {
	       max_height = sizes[i].height;
	    }
	 }
	 

	 handle->unicap_formats[v4l2_fmt.index].min_size.width = min_width;
	 handle->unicap_formats[v4l2_fmt.index].min_size.height = min_height;
	 handle->unicap_formats[v4l2_fmt.index].max_size.width = max_width;
	 handle->unicap_formats[v4l2_fmt.index].max_size.height = max_height;
	 handle->unicap_formats[v4l2_fmt.index].sizes = sizes;
	 handle->unicap_formats[v4l2_fmt.index].size_count = size_count;
	 handle->unicap_formats[v4l2_fmt.index].buffer_size = 
	    ( sizes[size_count-1].width * sizes[size_count-1].height * handle->unicap_formats[v4l2_fmt.index].bpp / 8 );
      } 

      v4l2_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
   }
	
   handle->format_count = v4l2_fmt.index;
   handle->supported_formats = count;
   if( _pcount )
   {
      *_pcount = count;
   }

   return STATUS_SUCCESS;
}

static unicap_status_t v4l2_enumerate_formats( void *cpi_data, unicap_format_t *format, int index )
{
   v4l2_handle_t handle = (v4l2_handle_t) cpi_data;
   int tmp_index, i, v4l2index = 0;
   unicap_status_t status = STATUS_NO_MATCH;

/* 	TRACE( "enumerate formats: %d (%d)\n", index, handle->format_count ); */

   if( index >= handle->format_count )
   {
      return STATUS_NO_MATCH;
   }

   for( i = 0, tmp_index = -1; ( i < handle->format_count ) && ( tmp_index != index ); i++ )
   {      
      if( handle->format_mask[i] == 0 )
      {
	 tmp_index++;
	 v4l2index = i;
      }
   }
   
   if( tmp_index == index )
   {
      unicap_copy_format( format, &handle->unicap_formats[v4l2index] );
      status = STATUS_SUCCESS;
   }
	
   return status;
}

static unicap_status_t v4l2_set_format( void *cpi_data, unicap_format_t *_format )
{
   v4l2_handle_t handle = (v4l2_handle_t) cpi_data;

   struct v4l2_format v4l2_fmt;
   int i;
   int index = -1;

   __u32 fourcc;

   unicap_format_t format;

   int old_running = handle->capture_running;
   
   if( handle->capture_running ){
      v4l2_capture_stop( handle );
   }

   unicap_copy_format( &format, _format );

   TRACE( "v4l2_set_format\n" );
	
   for( i = 0; i < handle->format_count; i++ )
   {
      if( !strcmp( format.identifier, handle->unicap_formats[ i ].identifier ) )
      {
	 index = i;
	 break;
      }
   }
	
   if( index == -1 )
   {
      TRACE( "v4l2_set_format failed: NO_MATCH\n" );
      return STATUS_NO_MATCH;
   }
	
   unicap_copy_format( &handle->current_format, &format );
   handle->current_format.buffer_size = format.size.width * format.size.height * format.bpp / 8;

   if( handle->compat && handle->compat->tov4l2format_func )
   {
      handle->compat->tov4l2format_func( handle, &format );
   }
   else
   {
      fourcc = handle->unicap_formats[ index ].fourcc;
   }



   v4l2_fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
   v4l2_fmt.fmt.pix.width = format.size.width;
   v4l2_fmt.fmt.pix.height = format.size.height;
   v4l2_fmt.fmt.pix.pixelformat = format.fourcc;/* handle->unicap_formats[ index ].fourcc; */
   v4l2_fmt.fmt.pix.field = V4L2_FIELD_ANY;
	
   if( IOCTL( handle->fd, VIDIOC_S_FMT, &v4l2_fmt ) < 0 )
   {
      TRACE( "VIDIOC_S_FMT ioctl failed: %s\n", strerror( errno ) );
      return STATUS_FAILURE;
   }

   if( old_running ){
      v4l2_capture_start( handle );
   }
   


   return STATUS_SUCCESS;
}

static unicap_status_t v4l2_get_format( void *cpi_data, unicap_format_t *format )
{
   v4l2_handle_t handle = (v4l2_handle_t) cpi_data;
   unicap_copy_format( format, &handle->current_format );

   return STATUS_SUCCESS;
}

static unicap_status_t v4l2_enum_frameintervals( v4l2_handle_t handle, unicap_property_t *property )
{
#ifdef VIDIOC_ENUM_FRAMEINTERVALS
   unicap_format_t format;
   struct v4l2_frmivalenum frmival;
   
   
   TRACE( "v4l2_enum_frameintervals\n" );

   v4l2_get_format( handle, &format );
   if( !format.fourcc )
   {
      return STATUS_FAILURE;
   }
   
   if( handle->compat && handle->compat->tov4l2format_func )
   {
      handle->compat->tov4l2format_func( handle, &format );
   }

   frmival.pixel_format = format.fourcc;
   frmival.width = format.size.width;
   frmival.height = format.size.height;

   handle->frame_rate_count = 0;
                                                                                            
   for( frmival.index = 0; 
	( frmival.index < V4L2_MAX_FRAME_RATES ) && ( IOCTL( handle->fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival ) >= 0 ); 
	frmival.index++ )
   {
      int i;
      int is_dublette = 0;

      if( frmival.type == V4L2_FRMIVAL_TYPE_DISCRETE )
      {
	 handle->frame_rates[ handle->frame_rate_count ] = 1/((double)frmival.discrete.numerator / (double)frmival.discrete.denominator);
	 TRACE( "num: %d denom: %d\n", frmival.discrete.numerator, frmival.discrete.denominator );
      }
      else
      {
	 handle->frame_rates[ handle->frame_rate_count ] = 1/((double)frmival.stepwise.max.numerator / (double)frmival.stepwise.max.denominator);
      }

      // Some devices enumerate the same frame interval over and over - filter out
      // those
      for( i = 0; i < handle->frame_rate_count; i++ )
      {
	 if( handle->frame_rates[ handle->frame_rate_count ] == handle->frame_rates[ i ] )
	 {
	    is_dublette = 1;
	    break;
	 }
      }
      if( !is_dublette )
      {
	 handle->frame_rate_count++;
      }
   }
   
   if( frmival.index == 0 )
   {
      return STATUS_FAILURE;
   }
   
   property->value_list.values = handle->frame_rates;
   property->value_list.value_count = handle->frame_rate_count;
   strcpy( property->identifier, V4L2_VIDEO_FRAMERATE_PPTY_NAME );
   strcpy( property->category, "video" );
   strcpy( property->unit, "" );
   property->relations = 0;
   property->relations_count = 0;
   property->value = property->value_list.values[0];
   property->stepping = 0;
   property->type = UNICAP_PROPERTY_TYPE_VALUE_LIST;
   property->flags = UNICAP_FLAGS_MANUAL;
   property->flags_mask = UNICAP_FLAGS_MANUAL;
   property->property_data = 0;
   property->property_data_size = 0;
	
   return STATUS_SUCCESS;
#else // ndef VIDIOC_ENUM_FRAMEINTERVALS
   return STATUS_FAILURE;
#endif
}

static unicap_status_t v4l2_set_frame_interval( v4l2_handle_t handle, unicap_property_t *property )
{
#ifdef VIDIOC_S_PARM
   struct v4l2_streamparm parm;
   int running = handle->capture_running;
   
   v4l2_capture_stop( handle );

   parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
   parm.parm.capture.timeperframe.numerator = 100;
   parm.parm.capture.timeperframe.denominator = property->value * 100;
   
   if( IOCTL( handle->fd, VIDIOC_S_PARM, &parm ) < 0 )
   {
      TRACE( "Failed to set frame interval: %s\n", strerror( errno ) );
      return STATUS_FAILURE;
   }

   if( running )
   {
      v4l2_capture_start( handle );
   }
   

   return STATUS_SUCCESS;   
#else
   return STATUS_FAILURE;
#endif
}

static unicap_status_t v4l2_get_frame_interval( v4l2_handle_t handle, unicap_property_t *property )
{
#ifdef VIDIOC_G_PARM
   struct v4l2_streamparm parm;
   
   parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
   
   if( IOCTL( handle->fd, VIDIOC_G_PARM, &parm ) < 0 )
   {
      TRACE( "Failed to get frame interval\n" );
      return STATUS_FAILURE;
   }
   

   property->value = 1/( (double)parm.parm.capture.timeperframe.numerator / (double) parm.parm.capture.timeperframe.denominator );
   
   return STATUS_SUCCESS;
#else

   return STATUS_FAILURE;
#endif
}

static unicap_status_t v4l2_enum_inputs( v4l2_handle_t handle, unicap_property_t *property )
{
   struct v4l2_input input;

   for( input.index = 0; 
	(input.index < V4L2_MAX_VIDEO_INPUTS ) && 
	   ( IOCTL( handle->fd, VIDIOC_ENUMINPUT, &input ) == 0 ); 
	input.index++ )
   {
      strncpy( handle->video_inputs[input.index], (char*)input.name, 32 );
   }

   // Only present inputs if there are more than one
   if( input.index <= 1 )
   {
      return STATUS_FAILURE;
   }
	
   property->menu.menu_items = handle->video_inputs;
   property->menu.menu_item_count = input.index;
   strcpy( property->identifier, V4L2_VIDEO_IN_PPTY_NAME );
   strcpy( property->category, N_("source") );
   strcpy( property->unit, "" );
   property->relations = 0;
   property->relations_count = 0;
   strcpy( property->menu_item, property->menu.menu_items[0] );
   property->stepping = 0;
   property->type = UNICAP_PROPERTY_TYPE_MENU;
   property->flags = UNICAP_FLAGS_MANUAL;
   property->flags_mask = UNICAP_FLAGS_MANUAL;
   property->property_data = 0;
   property->property_data_size = 0;
	
   handle->video_in_count = input.index;

   return STATUS_SUCCESS;
}

static unicap_status_t v4l2_enum_norms( v4l2_handle_t handle, unicap_property_t *property )
{
   struct v4l2_input input;
   struct v4l2_standard standard;
   
   if( IOCTL( handle->fd, VIDIOC_G_INPUT, &input.index ) < 0 ) 
   {
      TRACE( "Failed to get input\n" );
      return STATUS_FAILURE;
   }

   input.index = 0;
   
   if( IOCTL( handle->fd, VIDIOC_ENUMINPUT, &input ) < 0 ) 
   {
      TRACE( "Failed to enumerate input\n" );
      return STATUS_FAILURE;
   }
   
   standard.index = 0;
   
   while( IOCTL( handle->fd, VIDIOC_ENUMSTD, &standard ) == 0 ) 
   {
      if( standard.id && input.std )
      {
	 TRACE ("Found norm: %s\n", standard.name);
	 strcpy( handle->video_norms[standard.index], (char*)standard.name );
      }
      standard.index++;
   }


   /* From v4l2-draft:
      EINVAL indicates the end of the enumeration, which cannot be
      empty unless this device falls under the USB exception. */
   if( errno != EINVAL || standard.index == 0 ) 
   {
      TRACE( "Failed to enumerate norms\n" );
      return STATUS_FAILURE;
   }

   property->menu.menu_items = (char**)handle->video_norms;
   property->menu.menu_item_count = standard.index;
   strcpy( property->identifier, V4L2_VIDEO_NORM_PPTY_NAME );
   strcpy( property->category, "source" );
   strcpy( property->unit, "" );
   property->relations = 0;
   property->relations_count = 0;
   strcpy( property->menu_item, property->menu.menu_items[0] );
   property->stepping = 0;
   property->type = UNICAP_PROPERTY_TYPE_MENU;
   property->flags = UNICAP_FLAGS_MANUAL;
   property->flags_mask = UNICAP_FLAGS_MANUAL;
   property->property_data = 0;
   property->property_data_size = 0;
   
   TRACE( "-enum_norms\n" );

   return STATUS_SUCCESS;

}

static char *get_category( char *identifier )
{
   char *cat = "video";
   int i;
   
   for( i = 0; i < ( sizeof( category_list ) / sizeof( struct prop_category ) ); i++ )
   {
      if( !strcasecmp( category_list[i].property_id, identifier ) )
      {
	 cat = category_list[i].category;
	 break;
      }
   }

   return cat;
   
}



static unicap_status_t add_properties( v4l2_handle_t handle, int index_start, int index_end, int *ppty_index )
{
   int index;
   struct v4l2_queryctrl v4l2ctrl;
   int tmp_ppty_index = *ppty_index;

   memset( &v4l2ctrl, 0x0, sizeof( v4l2ctrl ) );
   
   for( index = index_start; index < index_end; index++ )
   {
      v4l2ctrl.id = index;
      if( IOCTL( handle->fd, VIDIOC_QUERYCTRL, &v4l2ctrl ) )
      {
	 if( errno == EINVAL )
	 {
	    continue;
	 }

	 TRACE( "ioctl failed\n" );
	 perror( "error" );
	 return STATUS_FAILURE;
      }

      if( handle->compat && !v4l2ctrl.flags & V4L2_CTRL_FLAG_DISABLED )
      {
	 if( handle->compat->override_property_func )
	 {
	    unicap_status_t status = handle->compat->override_property_func( handle, &v4l2ctrl, &handle->unicap_properties[ tmp_ppty_index ] );
	    if( status == STATUS_SUCCESS )
	    {
	       tmp_ppty_index++;
	       continue;
	    }
	    if( status == STATUS_SKIP_CTRL )
	    {
	       continue;
	    }
	 }
      }      

      if( !v4l2ctrl.flags & V4L2_CTRL_FLAG_DISABLED )
      {
	 TRACE( "add property: %s\n", v4l2ctrl.name );
	 strcpy( handle->unicap_properties[ tmp_ppty_index ].identifier, 
		 (char*)v4l2ctrl.name );
	 handle->unicap_properties[ tmp_ppty_index ].value = v4l2ctrl.default_value;
	 if( v4l2ctrl.type != V4L2_CTRL_TYPE_BOOLEAN )
	 {
	    handle->unicap_properties[ tmp_ppty_index ].range.min = v4l2ctrl.minimum;
	    handle->unicap_properties[ tmp_ppty_index ].range.max = v4l2ctrl.maximum;
	    handle->unicap_properties[ tmp_ppty_index ].stepping = v4l2ctrl.step;
	 }
	 else
	 {
	    handle->unicap_properties[ tmp_ppty_index ].range.min = 0;
	    handle->unicap_properties[ tmp_ppty_index ].range.max = 1;
	    handle->unicap_properties[ tmp_ppty_index ].stepping = 1;
	 }
	 handle->unicap_properties[ tmp_ppty_index ].type = UNICAP_PROPERTY_TYPE_RANGE;
	 strcpy( handle->unicap_properties[ tmp_ppty_index ].category, get_category( handle->unicap_properties[ tmp_ppty_index ].identifier ) );
	 strcpy( handle->unicap_properties[ tmp_ppty_index ].unit, "" );
	 handle->unicap_properties[ tmp_ppty_index ].relations = 0;
	 handle->unicap_properties[ tmp_ppty_index ].relations_count = 0;
	 handle->unicap_properties[ tmp_ppty_index ].flags = UNICAP_FLAGS_MANUAL;
	 handle->unicap_properties[ tmp_ppty_index ].flags_mask = UNICAP_FLAGS_MANUAL;
	 handle->unicap_properties[ tmp_ppty_index ].property_data = 0;
	 handle->unicap_properties[ tmp_ppty_index ].property_data_size = 0;
		   
	 handle->control_ids[ tmp_ppty_index ] = index;
		   
	 tmp_ppty_index++;
      }
   }

   *ppty_index = tmp_ppty_index;

   return STATUS_SUCCESS;
}


static int count_properties( v4l2_handle_t handle, int index_start, int index_end )
{
   struct v4l2_queryctrl v4l2ctrl;
   int index;
   int count = 0;

   for( index = index_start; index < index_end; index++ )
   {
      v4l2ctrl.id = index;
      if( IOCTL( handle->fd, VIDIOC_QUERYCTRL, &v4l2ctrl ) )
      {
	 if( errno == EINVAL )
	 {
	    continue;
	 }

	 TRACE( "ioctl failed at index: %d\n", index );
	 perror( "error" );
	 continue;
      }

      if( (handle->compat) && (!v4l2ctrl.flags & V4L2_CTRL_FLAG_DISABLED)  && ( handle->compat->override_property_func ) )
      {
	 unicap_status_t status = handle->compat->override_property_func( handle, &v4l2ctrl, NULL );
	 if( status == STATUS_SKIP_CTRL )
	 {
	    continue;
	 }
      }      

      if( !v4l2ctrl.flags & V4L2_CTRL_FLAG_DISABLED )
      {
	 count++;
      }
   }

   return count;
}

static unicap_status_t add_properties_ext( v4l2_handle_t handle, int *ppty_index )
{
#ifdef V4L2_CTRL_FLAG_NEXT_CTRL
   struct v4l2_queryctrl v4l2ctrl;
   int tmp_ppty_index = *ppty_index;
   int ret;
   
   v4l2ctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL;
   while( ret = IOCTL( handle->fd, VIDIOC_QUERYCTRL, &v4l2ctrl ) == 0 )
   {
	 TRACE( "++%s++\n", v4l2ctrl.name );

      if( handle->compat && ( !v4l2ctrl.flags & V4L2_CTRL_FLAG_DISABLED ) && handle->compat->override_property_func )
      {
	 unicap_status_t status = handle->compat->override_property_func( handle, &v4l2ctrl, &handle->unicap_properties[ tmp_ppty_index ] );
	 if( status == STATUS_SUCCESS )
	 {
	    v4l2ctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
	    TRACE( "Added from compat: %s\n", v4l2ctrl.name );
	    tmp_ppty_index++;
	    continue;
	 }
	 if( status == STATUS_SKIP_CTRL )
	 {
	    v4l2ctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
	    TRACE( "Skip: %s\n", v4l2ctrl.name );
	    continue;
	 }
      }      

      v4l2ctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
	    
      if( !v4l2ctrl.flags & V4L2_CTRL_FLAG_DISABLED )
      {
	 TRACE( "add property: %s\n", v4l2ctrl.name );
	 strcpy( handle->unicap_properties[ tmp_ppty_index ].identifier, 
		 (char*)v4l2ctrl.name );

	 handle->unicap_properties[ tmp_ppty_index ].value = v4l2ctrl.default_value;
	 if( v4l2ctrl.type != V4L2_CTRL_TYPE_BOOLEAN )
	 {
	    handle->unicap_properties[ tmp_ppty_index ].range.min = v4l2ctrl.minimum;
	    handle->unicap_properties[ tmp_ppty_index ].range.max = v4l2ctrl.maximum;
	    handle->unicap_properties[ tmp_ppty_index ].stepping = v4l2ctrl.step;
	 }
	 else
	 {
	    handle->unicap_properties[ tmp_ppty_index ].range.min = 0;
	    handle->unicap_properties[ tmp_ppty_index ].range.max = 1;
	    handle->unicap_properties[ tmp_ppty_index ].stepping = 1;
	 }
	 handle->unicap_properties[ tmp_ppty_index ].type = UNICAP_PROPERTY_TYPE_RANGE;
	 strcpy( handle->unicap_properties[ tmp_ppty_index ].category, get_category( handle->unicap_properties[ tmp_ppty_index ].identifier ) );
	 strcpy( handle->unicap_properties[ tmp_ppty_index ].unit, "" );
	 handle->unicap_properties[ tmp_ppty_index ].relations = 0;
	 handle->unicap_properties[ tmp_ppty_index ].relations_count = 0;
	 handle->unicap_properties[ tmp_ppty_index ].flags = UNICAP_FLAGS_MANUAL;
	 handle->unicap_properties[ tmp_ppty_index ].flags_mask = UNICAP_FLAGS_MANUAL;
	 handle->unicap_properties[ tmp_ppty_index ].property_data = 0;
	 handle->unicap_properties[ tmp_ppty_index ].property_data_size = 0;

	 if( v4l2ctrl.flags & V4L2_CTRL_FLAG_READ_ONLY )
	 {
	    handle->unicap_properties[ tmp_ppty_index ].flags |= UNICAP_FLAGS_READ_ONLY;
	 }
		   
	 handle->control_ids[ tmp_ppty_index ] = v4l2ctrl.id & V4L2_CTRL_ID_MASK;
		   
	 tmp_ppty_index++;
      } else{
	 TRACE( "disabled property: %s\n", v4l2ctrl.name );
      }
   }


   *ppty_index = tmp_ppty_index;

   return STATUS_SUCCESS;
#else
   return STATUS_NOT_IMPLEMENTED;
#endif
}

static int count_properties_ext( v4l2_handle_t handle )
{
#ifdef V4L2_CTRL_FLAG_NEXT_CTRL
   struct v4l2_queryctrl v4l2ctrl;
   int count = 0;

   v4l2ctrl.id = V4L2_CTRL_FLAG_NEXT_CTRL;
   while( IOCTL( handle->fd, VIDIOC_QUERYCTRL, &v4l2ctrl ) == 0 )
   {
      if( (handle->compat) && (!v4l2ctrl.flags & V4L2_CTRL_FLAG_DISABLED)  && ( handle->compat->override_property_func ) )
      {
	 unicap_status_t status = handle->compat->override_property_func( handle, &v4l2ctrl, NULL );
	 if( status == STATUS_SKIP_CTRL )
	 {
	    v4l2ctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
	    continue;
	 }
      }      
      if( !v4l2ctrl.flags & V4L2_CTRL_FLAG_DISABLED )
      {
	 count++;
      }
      v4l2ctrl.id |= V4L2_CTRL_FLAG_NEXT_CTRL;
   }

   return count;      
#else
   return 0;
#endif
}



static unicap_status_t v4l2_reenumerate_properties( void *cpi_data, int *_pcount )
{
   v4l2_handle_t handle = (v4l2_handle_t) cpi_data;
   int count = 0;
   int compat_count = 0;
   int index = 0;
   int ppty_index = 0;
   int use_extended_ctrl = 0;

   TRACE( "v4l2_reenumerate_properties\n" );

   if( handle->unicap_properties )
   {
      free( handle->unicap_properties );
      handle->unicap_properties = 0;
   }
	
   if( handle->control_ids )
   {
      free( handle->control_ids );
      handle->control_ids = 0;
   }
   
   count = count_properties_ext( handle );
   
   if( count > 1 )
   {
      TRACE( "Use extended control interface\n" );
      use_extended_ctrl = 1;
   }
   else
   {
      count = count_properties( handle, V4L2_CID_BASE, V4L2_CID_BASE + 0x1000 );
      count += count_properties( handle, V4L2_CID_PRIVATE_BASE, V4L2_CID_PRIVATE_BASE + 0x1000 );
      count += count_properties( handle, V4L2_CID_CAMERA_CLASS_BASE, V4L2_CID_CAMERA_CLASS_BASE + 0x1000 );
/* #ifdef V4L2_CID_USER_BASE */
/*    count += count_properties( handle, V4L2_CID_USER_BASE, V4L2_CID_USER_BASE + 0x1000 ); */
/* #endif */
   }
   
   count++;// video input source property
   count++;// video norm property
   count++;// frame rate property

   if( handle->compat )
   {
      compat_count = handle->compat->count_ext_property_func( handle );
      count += compat_count;
   }

   handle->unicap_properties = (unicap_property_t *) malloc( count * sizeof( unicap_property_t ) );
   handle->control_ids = (__u32*) malloc( count * sizeof( __u32 ) );
   handle->property_count = count;	

   if( use_extended_ctrl )
   {
      if( !SUCCESS( add_properties_ext( handle, &ppty_index ) ) )
      {
	 TRACE( "add_properties_ext failed\n" );
	 return STATUS_FAILURE;
      }
   }
   else
   {
      if( !SUCCESS( add_properties( handle, V4L2_CID_BASE, V4L2_CID_BASE + 0x1000, &ppty_index ) ) ||
	  !SUCCESS( add_properties( handle, V4L2_CID_PRIVATE_BASE, V4L2_CID_PRIVATE_BASE + 0x1000, &ppty_index ) ) ||
	  !SUCCESS( add_properties( handle, V4L2_CID_CAMERA_CLASS_BASE, V4L2_CID_CAMERA_CLASS_BASE + 0x1000, &ppty_index ) ) )
      {
	 return STATUS_FAILURE;
      }
   }
/* #ifdef V4L2_CID_USER_BASE */
/*    if( !SUCCESS( add_properties( handle, V4L2_CID_USER_BASE, V4L2_CID_USER_BASE + 0x1000, &ppty_index ) ) ) */
/*    { */
/*       return STATUS_FAILURE; */
/*    } */
/* #endif */


   if( !SUCCESS( v4l2_enum_inputs( handle, &handle->unicap_properties[ ppty_index++ ] ) ) )
   {
      handle->property_count--;
      ppty_index--;
   }
   
   if( !SUCCESS( v4l2_enum_norms( handle, &handle->unicap_properties[ ppty_index++ ] ) ) )
   {
      handle->property_count--;
      ppty_index--;
   }    

   if( !SUCCESS( v4l2_enum_frameintervals( handle, &handle->unicap_properties[ ppty_index ] ) ) )
   {
      handle->property_count--;
      ppty_index--;
   }
   else
   {
      if( handle->compat && handle->compat->override_property_func )
      {
	 handle->compat->override_property_func( handle, NULL, &handle->unicap_properties[ ppty_index ] );
      }
   }
   ppty_index++;

   if( handle->compat )
   {
      for( index = 0; index < compat_count; index++ )
      {
	 if( !SUCCESS( handle->compat->enumerate_property_func( handle, index, &handle->unicap_properties[ ppty_index++ ] ) ) )
	 {
	    handle->property_count--;
	    ppty_index--;
	 }
      }  
   }
   
      
   if( _pcount )
   {
      *_pcount = ppty_index;
   }

   return STATUS_SUCCESS;
}

static unicap_status_t v4l2_enumerate_properties( void *cpi_data, 
						  unicap_property_t *property, int index )
{
   v4l2_handle_t handle = (v4l2_handle_t) cpi_data;

   if( index >= handle->property_count )
   {
      return STATUS_NO_MATCH;
   }

   unicap_copy_property( property, &handle->unicap_properties[index] );
	
   return STATUS_SUCCESS;
}

static unicap_status_t v4l2_set_input( v4l2_handle_t handle, unicap_property_t *property )
{
   struct v4l2_input input;
	
   for( input.index = 0; input.index < handle->video_in_count; input.index++ )
   {
      if( !strcmp( property->menu_item, handle->video_inputs[input.index] ) )
      {
	 if( IOCTL( handle->fd, VIDIOC_S_INPUT, &input ) == 0 )
	 {
	    return STATUS_SUCCESS;
	 }
	 else
	 {
	    return STATUS_FAILURE;
	 }
      }
   }

   return STATUS_NO_MATCH;
}

static unicap_status_t v4l2_set_norm( v4l2_handle_t handle, unicap_property_t *property )
{
   struct v4l2_input input;
   struct v4l2_standard standard;
   v4l2_std_id id=0;
   
   if( IOCTL( handle->fd, VIDIOC_G_INPUT, &input.index ) < 0 ) 
   {
      TRACE( "Failed to get input\n" );
      return STATUS_FAILURE;
   }

   input.index = 0;
   
   if( IOCTL( handle->fd, VIDIOC_ENUMINPUT, &input ) < 0 ) 
   {
      TRACE( "Failed to enumerate input\n" );
      return STATUS_FAILURE;
   }
   
   standard.index = 0;
   
   while( IOCTL( handle->fd, VIDIOC_ENUMSTD, &standard ) == 0 ) 
   {
      if( standard.id & input.std )
      {
	 TRACE ("Found norm: %s [%016llx], looking for: %s\n", 
		standard.name, standard.id, property->menu_item);
	 if( !strcmp( property->menu_item, (char*)standard.name ) )
	 {
	    id = standard.id;
	    break;
	 }
      }
      standard.index++;
   }

   TRACE( "Set norm: %016llx\n", id );

   if( IOCTL( handle->fd, VIDIOC_S_STD, &id ) < 0 )
   {
      TRACE( "Failed to set norm: %016llx %016llx\n", id, input.std );
      return STATUS_FAILURE;
   }

   return STATUS_SUCCESS;
}

static unicap_status_t v4l2_set_property( void *cpi_data, unicap_property_t *property )
{
   v4l2_handle_t handle = (v4l2_handle_t) cpi_data;
   unicap_status_t status;
   int index;
	
   TRACE( "v4l2_set_property\n" );

   if( handle->compat )
   {
      status = handle->compat->set_property_func( handle, property );
      if( status != STATUS_NO_MATCH )
      {
	 return status;
      }
   }

   if( !strcmp( property->identifier, V4L2_VIDEO_IN_PPTY_NAME ) )
   {
      return v4l2_set_input( handle, property );
   }
   
   if( !strcmp( property->identifier, V4L2_VIDEO_NORM_PPTY_NAME ) )
   {
      return v4l2_set_norm( handle, property );
   }

   if( !strcmp( property->identifier, V4L2_VIDEO_FRAMERATE_PPTY_NAME ) )
   {
      return v4l2_set_frame_interval( handle, property );
   }

   for( index = 0; index < handle->property_count; index++ )
   {
      
      if( !strcmp( property->identifier, handle->unicap_properties[index].identifier ) )
      {
	 struct v4l2_control v4l2ctrl;
	 v4l2ctrl.id = handle->control_ids[ index ];
	 v4l2ctrl.value = property->value;
	 if( IOCTL( handle->fd, VIDIOC_S_CTRL, &v4l2ctrl ) < 0 )
	 {
	    TRACE( "VIDIOC_S_CTRL failed\n" );
	    return STATUS_FAILURE;
	 }
			
	 return STATUS_SUCCESS;
      }
   }

   return STATUS_NO_MATCH;
}

static unicap_status_t v4l2_get_input( v4l2_handle_t handle, unicap_property_t *property )
{
   struct v4l2_input input;
	
   if( !IOCTL( handle->fd, VIDIOC_G_INPUT, &input ) )
   {
      strcpy( property->menu_item, handle->video_inputs[input.index] );
      return STATUS_SUCCESS;
   }
	
   return STATUS_FAILURE;
}

static unicap_status_t v4l2_get_norm( v4l2_handle_t handle, unicap_property_t *property )
{
   struct v4l2_standard standard;
   v4l2_std_id id=0;
   unicap_status_t status = STATUS_FAILURE;
   
   if( IOCTL( handle->fd, VIDIOC_G_STD, &id ) < 0 ) 
   {
      TRACE( "Failed to get norm\n" );
      return STATUS_FAILURE;
   }

   standard.index = 0;
   
   while( IOCTL( handle->fd, VIDIOC_ENUMSTD, &standard ) == 0 ) 
   {
      if( standard.id & id )
      {
	 TRACE ("Found norm: %s [%016llx]", 
		standard.name, standard.id );
	 strcpy( property->menu_item, (char*)standard.name );
	 status = STATUS_SUCCESS;
	 break;
      }
      standard.index++;
   }

   return status;
}

static unicap_status_t v4l2_get_property( void *cpi_data, unicap_property_t *property )
{
   v4l2_handle_t handle = (v4l2_handle_t) cpi_data;
   unicap_status_t status;
   int index;
	
/*    TRACE( "v4l2_get_property\n" ); */

   for( index = 0; index < handle->property_count; index++ )
   {
      if( !strcmp( property->identifier, handle->unicap_properties[index].identifier ) )
      {
	 struct v4l2_control v4l2ctrl;

	 unicap_copy_property( property, &handle->unicap_properties[ index ] );
			
	 if( handle->compat )
	 {
	    status = handle->compat->get_property_func( handle, property );
	    if( status != STATUS_NO_MATCH )
	    {
	       return status;
	    }   
	 }

	 if( !strcmp( property->identifier, V4L2_VIDEO_IN_PPTY_NAME ) )
	 {
	    return v4l2_get_input( handle, property );
	 }

	 if( !strcmp( property->identifier, V4L2_VIDEO_NORM_PPTY_NAME ) )
	 {
	    return v4l2_get_norm( handle, property );
	 }

	 if( !strcmp( property->identifier, V4L2_VIDEO_FRAMERATE_PPTY_NAME ) )
	 {
	    return v4l2_get_frame_interval( handle, property );
	 }
	 
	 if( property->flags & UNICAP_FLAGS_WRITE_ONLY )
	 {
	    property->value = 0.0;
	    return STATUS_SUCCESS;
	 }
	 
	 v4l2ctrl.id = handle->control_ids[ index ];
	 if( IOCTL( handle->fd, VIDIOC_G_CTRL, &v4l2ctrl ) < 0 )
	 {
	    TRACE( "VIDIOC_G_CTRL failed\n" );
	    return STATUS_FAILURE;
	 }
	 property->value = v4l2ctrl.value;
	 return STATUS_SUCCESS;
      }
   }

   return STATUS_NO_MATCH;
}

static unicap_status_t v4l2_capture_start( void *cpi_data )
{
   v4l2_handle_t handle = (v4l2_handle_t) cpi_data;
   struct v4l2_requestbuffers v4l2_reqbuf;

   unicap_status_t status = STATUS_SUCCESS;

   TRACE( "v4l2_start_capture\n" );

   if( handle->capture_running )
   {
      return STATUS_CAPTURE_ALREADY_STARTED;
   }
   
   handle->buffer_mgr = buffer_mgr_create( handle->fd, &handle->current_format );

   handle->capture_running = 1;

   status = buffer_mgr_queue_all( handle->buffer_mgr );

   handle->quit_capture_thread = 0;
   pthread_create( &handle->capture_thread, NULL, (void*(*)(void*))v4l2_capture_thread, handle );

   int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
   if( IOCTL( handle->fd, VIDIOC_STREAMON, &type ) < 0  ){
      TRACE( "VIDIOC_STREAMON ioctl failed: %s\n", strerror( errno ) );
      return STATUS_FAILURE;
   }
	 
   return status;
}

static unicap_status_t v4l2_capture_stop( void *cpi_data )
{
   v4l2_handle_t handle = (v4l2_handle_t) cpi_data;
   
   int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
   int i;

   TRACE( "v4l2_capture_stop\n" );


   if( handle->capture_running )
   {
      handle->capture_running = 0;
      handle->quit_capture_thread = 1;
      pthread_join( handle->capture_thread, NULL );

      if( IOCTL( handle->fd, VIDIOC_STREAMOFF, &type ) < 0  )
      {
	 TRACE( "VIDIOC_STREAMOFF ioctl failed: %s\n", strerror( errno ) );
	 return STATUS_FAILURE;
      }

      buffer_mgr_dequeue_all (handle->buffer_mgr);
      buffer_mgr_destroy (handle->buffer_mgr);
      
      while( ucutil_get_front_queue( handle->in_queue ) )
      {
	 TRACE( "!!possible memleak\n" );
      }
   }
   

   return STATUS_SUCCESS;
}

static unicap_status_t queue_buffer( v4l2_handle_t handle, unicap_data_buffer_t *buffer )
{
   struct v4l2_buffer v4l2_buffer;
   memset( &v4l2_buffer, 0x0, sizeof( v4l2_buffer ) );
   
   v4l2_buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
   v4l2_buffer.length = buffer->buffer_size;
   switch( handle->io_method )
   {
      case CPI_V4L2_IO_METHOD_MMAP:
      {
	 int ret = 0;
	 
	 v4l2_buffer.index = 0;
	 v4l2_buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	 v4l2_buffer.memory = V4L2_MEMORY_MMAP;

	 if( sem_wait( &handle->sema ) )
	 {
	    TRACE( "SEM_WAIT FAILED!\n" );
	    return STATUS_FAILURE;
	 }
		
	 if( ( ( handle->qindex + 1 ) % handle->buffer_count ) == handle->dqindex )
	 {
	    TRACE( "NO BUFFERS\n" );
	    sem_post( &handle->sema );
	    return STATUS_NO_BUFFERS;
	 }
		
	 v4l2_buffer.index = handle->qindex;
	 TRACE( "Q: index = %d type = %u, memory = %u\n", handle->qindex, v4l2_buffer.type, v4l2_buffer.memory );
	 handle->qindex = ( handle->qindex + 1 ) % handle->buffer_count;
	 v4l2_buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	 if( ( ret = IOCTL( handle->fd, VIDIOC_QBUF, &v4l2_buffer ) ) < 0 )
	 {
	    if( ( ret == -ENODEV ) && !handle->removed && handle->event_callback )
	    {
	       handle->event_callback( handle->unicap_handle, UNICAP_EVENT_DEVICE_REMOVED );
	       handle->removed = 1;
	    }

	    TRACE( "VIDIOC_QBUF ioctl failed: %s\n", strerror( errno ) );
	    sem_post( &handle->sema );
	    return STATUS_FAILURE;
	 }

	 if( sem_post( &handle->sema ) )
	 {
	    TRACE( "SEM_POST FAILED\n" );
	    return STATUS_FAILURE;
	 }  
      }
      break;
		
      case CPI_V4L2_IO_METHOD_USERPOINTER:
      {
	 int ret;
	 v4l2_buffer.m.userptr = ( unsigned long ) buffer->data;
	 v4l2_buffer.memory = V4L2_MEMORY_USERPTR;
	 v4l2_buffer.index = 0;

	 if( ( ret = IOCTL( handle->fd, VIDIOC_QBUF, &v4l2_buffer ) ) < 0 )
	 {
	    if( ( ret == -ENODEV ) && !handle->removed && handle->event_callback )
	    {
	       handle->event_callback( handle->unicap_handle, UNICAP_EVENT_DEVICE_REMOVED );
	       handle->removed = 1;
	    }
	    TRACE( "VIDIOC_QBUF ioctl failed: %s\n", strerror( errno ) );
	    return STATUS_FAILURE;
	 }
		
	 if( ucutil_queue_get_size( handle->in_queue ) == 2 )
	 {
	 }
      }
      break;
	
      default:
	 return STATUS_FAILURE;
   }

   return STATUS_SUCCESS;
}

static unicap_status_t queue_system_buffers( v4l2_handle_t handle )
{
   struct v4l2_buffer v4l2_buffer;
   memset( &v4l2_buffer, 0x0, sizeof( v4l2_buffer ) );
   v4l2_buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
   v4l2_buffer.length = handle->current_format.buffer_size;
   switch( handle->io_method )
   {
      case CPI_V4L2_IO_METHOD_MMAP:
      {
	 int i;
	 v4l2_buffer.index = 0;
	 v4l2_buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	 v4l2_buffer.memory = V4L2_MEMORY_MMAP;

	 if( sem_wait( &handle->sema ) )
	 {
	    TRACE( "SEM_WAIT FAILED!\n" );
	    return STATUS_FAILURE;
	 }
		
	 for( i = 0; i < handle->buffer_count; i++ )
	 {
	    int ret;
	    if( ( ( handle->qindex + 1 ) % handle->buffer_count ) == handle->dqindex )
	    {
	       TRACE( "NO BUFFERS\n" );
	       sem_post( &handle->sema );
	       return STATUS_NO_BUFFERS;
	    }

	    v4l2_buffer.index = handle->qindex;
	    TRACE( "Q: index = %d type = %u, memory = %u  dqindex = %d\n", handle->qindex, v4l2_buffer.type, v4l2_buffer.memory, handle->dqindex );
	    handle->qindex = ( handle->qindex + 1 ) % handle->buffer_count;
	    v4l2_buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	    if( ( ret = IOCTL( handle->fd, VIDIOC_QBUF, &v4l2_buffer ) ) < 0 )
	    {
	       if( ( ret == -ENODEV ) && !handle->removed && handle->event_callback )
	       {
		  handle->event_callback( handle->unicap_handle, UNICAP_EVENT_DEVICE_REMOVED );
		  handle->removed = 1;
	       }
	       TRACE( "VIDIOC_QBUF ioctl failed: %s\n", strerror( errno ) );
	       sem_post( &handle->sema );
	       return STATUS_FAILURE;
	    }

	    if( sem_post( &handle->sema ) )
	    {
	       TRACE( "SEM_POST FAILED\n" );
	       return STATUS_FAILURE;
	    }  
	 }
      }
      break;
      default:
	 return STATUS_FAILURE;
   }

   return STATUS_SUCCESS;
}

static unicap_status_t v4l2_queue_buffer( void *cpi_data, unicap_data_buffer_t *buffer )
{
   v4l2_handle_t handle = (v4l2_handle_t) cpi_data;
   struct _unicap_queue *queue = malloc( sizeof( struct _unicap_queue ) );

   TRACE( "QUEUE\n" );

   if( handle->capture_running )
   {
      unicap_status_t status = queue_buffer( handle, buffer );
      if( SUCCESS( status ) )
      {
	 queue->data = buffer;
	 ucutil_insert_back_queue( handle->in_queue, queue );
      }
      else
      {
	 TRACE( "queue buffer failed\n" );
      }

      if( ( status == STATUS_NO_BUFFERS ) && ( buffer->type == UNICAP_BUFFER_TYPE_SYSTEM ) )
      {
	 status = STATUS_SUCCESS;
      }
   }

   
   return STATUS_SUCCESS;
}

static unicap_status_t v4l2_dequeue_buffer( void *cpi_data, unicap_data_buffer_t **buffer )
{
/* 	v4l2_handle_t handle = (v4l2_handle_t) cpi_data; */

   return STATUS_NOT_IMPLEMENTED;
}

static unicap_status_t v4l2_wait_buffer( void *cpi_data, unicap_data_buffer_t **buffer )
{
   v4l2_handle_t handle = (v4l2_handle_t) cpi_data;
   unicap_data_buffer_t *returned_buffer;
   unicap_queue_t *entry;

   TRACE( "WAIT\n" );

   if( !handle->out_queue->next &&! handle->capture_running )
   {
      TRACE( "Wait->capture stopped" );
      return STATUS_IS_STOPPED;
   }
   
   while( !handle->out_queue->next )
   {
      usleep( 1000 );
   }
   

   if( handle->out_queue->next )
   {
      entry = ucutil_get_front_queue( handle->out_queue );
      returned_buffer = ( unicap_data_buffer_t * ) entry->data;
      free( entry );

      *buffer = returned_buffer;
   }
   

   TRACE( "-WAIT\n" );

   return STATUS_SUCCESS;
}

static unicap_status_t v4l2_poll_buffer( void *cpi_data, int *count )
{
   v4l2_handle_t handle = (v4l2_handle_t) cpi_data;
/*    TRACE( "POLL\n" ); */
   *count = ucutil_queue_get_size( handle->out_queue );

   return STATUS_SUCCESS;
}

static unicap_status_t v4l2_set_event_notify( void *cpi_data, unicap_event_callback_t func, unicap_handle_t unicap_handle )
{
   v4l2_handle_t handle = ( v4l2_handle_t )cpi_data;
   
   handle->event_callback = func;
   handle->unicap_handle = unicap_handle;
   
   return STATUS_SUCCESS;
}

static void v4l2_capture_thread( v4l2_handle_t handle )
{
   unicap_data_buffer_t new_frame_buffer;

   handle->dqindex = -1;

   while( !handle->quit_capture_thread )
   {
      unicap_queue_t *entry;
      struct timeval  ctime;
      int old_index;
      int drop = 0;
      int ret = 0;

      unicap_data_buffer_t *data_buffer;
      
      if( !SUCCESS( buffer_mgr_dequeue( handle->buffer_mgr, &data_buffer ) ) ){
	 TRACE( "buffer_mgr_dequeue failed!\n" );
	 usleep( 1000 );
	 continue;
      }

      unicap_data_buffer_ref (data_buffer);

      if( data_buffer->buffer_size < handle->current_format.buffer_size )
      {
	 TRACE( "Corrupt frame!\n" );
	 drop = 1;
      }
      
      if( !drop && handle->event_callback )
      {
	 handle->event_callback( handle->unicap_handle, UNICAP_EVENT_NEW_FRAME, data_buffer );
      }

      unicap_data_buffer_unref( data_buffer );
   }   
}
