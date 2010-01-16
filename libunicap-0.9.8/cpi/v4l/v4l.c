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
#include <pthread.h>

#include <errno.h>

#include <sys/time.h>
#include <time.h>

#if V4L_DEBUG
#define DEBUG
#endif
#include <debug.h>

#include <linux/videodev.h>
#include <linux/videodev2.h> // for v4l2 checks

#include "v4l.h"



unsigned int v4l_palette_array[] =
{
	VIDEO_PALETTE_GREY,
	VIDEO_PALETTE_HI240,
	VIDEO_PALETTE_RGB565,
	VIDEO_PALETTE_RGB555,
	VIDEO_PALETTE_RGB24,
	VIDEO_PALETTE_RGB32,
	VIDEO_PALETTE_YUV422,
	VIDEO_PALETTE_YUYV,
	VIDEO_PALETTE_UYVY,
	VIDEO_PALETTE_YUV420,
	VIDEO_PALETTE_YUV411,
	VIDEO_PALETTE_RAW,
	VIDEO_PALETTE_YUV422P,
	VIDEO_PALETTE_YUV411P,
};

static unicap_status_t v4l_enumerate_devices( unicap_device_t *device, int index );
static unicap_status_t v4l_open( void **cpi_data, unicap_device_t *device );
static unicap_status_t v4l_close( void *cpi_data );
static unicap_status_t v4l_reenumerate_formats( void *cpi_data, int *count );
static unicap_status_t v4l_enumerate_formats( void *cpi_data, unicap_format_t *format, int index );
static unicap_status_t v4l_set_format( void *cpi_data, unicap_format_t *format );
static unicap_status_t v4l_get_format( void *cpi_data, unicap_format_t *format );
static unicap_status_t v4l_reenumerate_properties( void *cpi_data, int *count );
static unicap_status_t v4l_enumerate_properties( void *cpi_data, unicap_property_t *property, int index );
static unicap_status_t v4l_set_property( void *cpi_data, unicap_property_t *property );
static unicap_status_t v4l_get_property( void *cpi_data, unicap_property_t *property );
static unicap_status_t v4l_capture_start( void *cpi_data );
static unicap_status_t v4l_capture_stop( void *cpi_data );
static unicap_status_t v4l_queue_buffer( void *cpi_data, unicap_data_buffer_t *buffer );
static unicap_status_t v4l_dequeue_buffer( void *cpi_data, unicap_data_buffer_t **buffer );
static unicap_status_t v4l_wait_buffer( void *cpi_data, unicap_data_buffer_t **buffer );
static unicap_status_t v4l_poll_buffer( void *cpi_data, int *count );
static unicap_status_t v4l_set_event_notify( void *cpi_data, 
					     unicap_event_callback_t func, 
					     unicap_handle_t unicap_handle );


static void v4l_capture_thread( v4l_handle_t handle );
static int queue_buffer( v4l_handle_t v4lhandle, int b );

static struct _unicap_cpi cpi_s = 
{
   cpi_version: 1<<16,
   cpi_capabilities: 0x3ffff,
   
   cpi_enumerate_devices: v4l_enumerate_devices,
   cpi_open: v4l_open, 
   cpi_close: v4l_close,
   
   cpi_reenumerate_formats: v4l_reenumerate_formats, 
   cpi_enumerate_formats: v4l_enumerate_formats,   
   cpi_set_format: v4l_set_format,	   
   cpi_get_format: v4l_get_format,          
   
   cpi_reenumerate_properties: v4l_reenumerate_properties,
   cpi_enumerate_properties: v4l_enumerate_properties,
   cpi_set_property: v4l_set_property,
   cpi_get_property: v4l_get_property,
   
   cpi_capture_start: v4l_capture_start,
   cpi_capture_stop: v4l_capture_stop,
   
   cpi_queue_buffer: v4l_queue_buffer,
   cpi_dequeue_buffer: v4l_dequeue_buffer,
   cpi_wait_buffer: v4l_wait_buffer,
   cpi_poll_buffer: v4l_poll_buffer,
   
   cpi_set_event_notify: v4l_set_event_notify,
};

#if ENABLE_STATIC_CPI
void unicap_v4l_register_static_cpi( struct _unicap_cpi **cpi )
{
   *cpi = &cpi_s;
}
#else
unicap_status_t cpi_register( struct _unicap_cpi *reg_data )
{
   memcpy( reg_data, &cpi_s, sizeof( struct _unicap_cpi ) );

/* 	reg_data->cpi_version = 1<<16; */
/* 	reg_data->cpi_capabilities = 0x3ffff; */

/* 	reg_data->cpi_enumerate_devices = v4l_enumerate_devices; */
/* 	reg_data->cpi_open = v4l_open;  */
/* 	reg_data->cpi_close = v4l_close; */

/* 	reg_data->cpi_reenumerate_formats = v4l_reenumerate_formats;  */
/* 	reg_data->cpi_enumerate_formats = v4l_enumerate_formats;    */
/* 	reg_data->cpi_set_format = v4l_set_format;	    */
/* 	reg_data->cpi_get_format = v4l_get_format;           */

/* 	reg_data->cpi_reenumerate_properties = v4l_reenumerate_properties; */
/* 	reg_data->cpi_enumerate_properties = v4l_enumerate_properties; */
/* 	reg_data->cpi_set_property = v4l_set_property; */
/* 	reg_data->cpi_get_property = v4l_get_property; */

/* 	reg_data->cpi_capture_start = v4l_capture_start; */
/* 	reg_data->cpi_capture_stop = v4l_capture_stop; */
	
/* 	reg_data->cpi_queue_buffer = v4l_queue_buffer; */
/* 	reg_data->cpi_dequeue_buffer = v4l_dequeue_buffer; */
/* 	reg_data->cpi_wait_buffer = v4l_wait_buffer; */
/* 	reg_data->cpi_poll_buffer = v4l_poll_buffer; */

/* 	reg_data->cpi_set_event_notify = v4l_set_event_notify; */

	return STATUS_SUCCESS;
}
#endif//ENABLE_STATIC_CPI

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

static unicap_status_t v4l_enumerate_devices( unicap_device_t *device, int index )
{
   int fd;
   char devname[256];
   struct dirent **namelist;
   int n;
   int found = -1;
   struct video_capability v4lcap;
   struct v4l2_capability v4l2caps;
   
   TRACE( "*** v4l enumerate devices\n" );

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
      TRACE( "v4l: open %s\n", devname );
      if( ( fd = open( devname, O_RDONLY | O_NONBLOCK ) ) == -1 )
      {
	 TRACE( "v4l_cpi: open(%s): %s\n", devname, strerror( errno ) );
	 continue;
      }
      
      if( ioctl( fd, VIDIOC_QUERYCAP, &v4l2caps ) == 0 && (v4l2caps.capabilities & V4L2_CAP_VIDEO_CAPTURE))
      {
	 
	 TRACE( "v4l2 ioctls succeeded\n" );
	 close( fd );
	 continue;
      }
      
/*       if( (v4l2caps.capabilities & V4L2_CAP_VIDEO_CAPTURE) ) */
/*       { */
/* 	 // v4l version two device */
/* 	 close( fd ); */
/* 	 continue; */
/*       } */

      if( ioctl( fd, VIDIOCGCAP, &v4lcap ) < 0 )
      {
	 TRACE( "VIDIOCGCAP ioctl failed!\n" );
	 close( fd );
	 continue;
      }

      if( !(v4lcap.type & VID_TYPE_CAPTURE ) )
      {
	 // no capture to memory capability
	 close( fd );
	 continue;
      }

      found++;
      close( fd );
   }
/*    free( namelist ); */
      
   if( found != index )
   {
      return STATUS_NO_DEVICE;
   }

	
   sprintf( device->identifier, "%s [%d]", v4lcap.name, index );
   strcpy( device->device, devname );
   strcpy( device->model_name, v4lcap.name );
   strcpy( device->vendor_name, "v4l" );
   device->model_id = 1;
   device->vendor_id = 0xffff0000;
   device->flags = UNICAP_CPI_SERIALIZED;


   return STATUS_SUCCESS;
}

static unicap_status_t v4l_open( void **cpi_data, unicap_device_t *device )
{
	v4l_handle_t v4lhandle;
	
	*cpi_data = malloc( sizeof( struct _v4l_handle ) );
	if( !cpi_data )
	{
		TRACE( "malloc failed\n" );
		return STATUS_FAILURE;
	}
	
	v4lhandle = *cpi_data;

	memset( v4lhandle, 0x0, sizeof( struct _v4l_handle ) );

	if( sem_init( &v4lhandle->sema, 0, 1 ) )
	{
	   TRACE( "sem_init failed\n" );
	   free( v4lhandle );
	   return STATUS_FAILURE;
	}
	
	if( sem_init( &v4lhandle->out_sema, 0, 0 ) )
	{
	   TRACE( "sem_init failed!\n" );
	   sem_destroy( &v4lhandle->sema );
	   free( v4lhandle );
	   return STATUS_FAILURE;
	}
	
	
	v4lhandle->fd = open( device->device, O_RDWR );
	if( v4lhandle->fd == -1 )
	{
		TRACE( "open failed, device = %s\n", device->device );
		TRACE( "error was: %s\n", strerror( errno ) );
		return STATUS_FAILURE;
	}

	if( ioctl( v4lhandle->fd, VIDIOCGCAP, &v4lhandle->v4lcap ) )
	{
		// failed v4l ioctl, look for next device
		close( v4lhandle->fd );
		return STATUS_FAILURE;
	}

	v4l_reenumerate_formats( v4lhandle, NULL );
	v4l_reenumerate_properties( v4lhandle, NULL );

	v4lhandle->in_queue = ucutil_queue_new();
	v4lhandle->out_queue = ucutil_queue_new();

	return STATUS_SUCCESS;
}

static unicap_status_t v4l_close( void *cpi_data )
{
	v4l_handle_t v4lhandle = ( v4l_handle_t ) cpi_data;

	close( v4lhandle->fd );
	sem_destroy( &v4lhandle->sema );
	sem_destroy( &v4lhandle->out_sema );
	
	if( v4lhandle->unicap_handle )
	{
	   // handle is a copy!!
	   free( v4lhandle->unicap_handle );
	}
	

	free( v4lhandle );
	
	return STATUS_SUCCESS;
}

static unicap_status_t v4l_reenumerate_formats( void *cpi_data, int *_pcount )
{
	v4l_handle_t v4lhandle = ( v4l_handle_t ) cpi_data;
	struct video_picture v4lpict;
	int count = 0;
	int p_count = sizeof( v4l_palette_array ) / sizeof( unsigned int );
	int i;

	if( ioctl( v4lhandle->fd, VIDIOCGPICT, &v4lhandle->v4lpict ) )
	{
		TRACE( "ioctl failed\n" );
		return STATUS_FAILURE;
	}

	memset( &v4lhandle->palette, 0x0, 32 * sizeof( unsigned int ) );
	memcpy( &v4lpict, &v4lhandle->v4lpict, sizeof( struct video_picture ) );

	for( i = 0; i < p_count; i++ )
	{
		v4lpict.palette = v4l_palette_array[i];
		TRACE( "try palette: %d\n", v4lpict.palette );
		
		if( !ioctl( v4lhandle->fd, VIDIOCSPICT, &v4lpict ) )
		{
			TRACE( "succeeded palette: %d\n", v4lpict.palette );
			v4lhandle->palette[i] = v4lpict.palette;
			count++;
		}
	}

	if( _pcount )
	{
		*_pcount = count;
	}

	return STATUS_SUCCESS;
}

static unicap_status_t v4l_enumerate_formats( void *cpi_data, unicap_format_t *format, int index )
{
	v4l_handle_t v4lhandle = ( v4l_handle_t ) cpi_data;
	unsigned int p = 0xffffffff;
	int p_count = sizeof( v4l_palette_array ) / sizeof( unsigned int );
	int i, j = -1;
	int tmp;
	
	if( v4lhandle->v4lpict.palette == 0 )
	{
		v4l_reenumerate_formats( cpi_data, &tmp );
	}		

	for( i = 0; i < p_count; i++ )
	{
		if( v4lhandle->palette[i] )
		{
			j++;
		}
		
		if( j == index )
		{
			p = v4lhandle->palette[i];
			TRACE( "p: %d\n", p );
			break;
		}
	}
	
	if( p == 0xffffffff )
	{
		return STATUS_NO_MATCH;
	}
	
	switch( p )
	{
	case VIDEO_PALETTE_GREY:
		strcpy( format->identifier, "Grey ( Mono 8pp )" );
		format->fourcc = FOURCC( 'G', 'R', 'E', 'Y' );
		format->bpp = 8;
		break;
		
	case VIDEO_PALETTE_HI240:
		strcpy( format->identifier, "HI240 Bt848 8Bit color cube" );
		format->fourcc = FOURCC( 'H', '2', '4', '0' );
		format->bpp = 8;
		break;

	case VIDEO_PALETTE_RGB565:
		strcpy( format->identifier, "RGB 16bpp" );
		format->fourcc = FOURCC( 'R', 'G', 'B', '6' );
		format->bpp = 16;
		break;
		
	case VIDEO_PALETTE_RGB555:
		strcpy( format->identifier, "RGB 15bpp" );
		format->fourcc = FOURCC( 'R', 'G', 'B', '5' );
		format->bpp = 15;
		break;
		
	case VIDEO_PALETTE_RGB24:
		strcpy( format->identifier, "BGR 24bpp" );
		format->fourcc = FOURCC( 'B', 'G', 'R', '3' );
		format->bpp = 24;
		break;
		
	case VIDEO_PALETTE_RGB32:
		strcpy( format->identifier, "RGB 32bpp" );
		format->fourcc = FOURCC( 'R', 'G', 'B', '4' );
		format->bpp = 32;
		break;
		
	case VIDEO_PALETTE_YUV422:
		strcpy( format->identifier, "YUV 4:2:2" );
		format->fourcc = FOURCC( 'Y', '4', '2', '2' );
		format->bpp = 16;
		break;
		
	case VIDEO_PALETTE_YUYV:
		strcpy( format->identifier, "YUYV" );
		format->fourcc = FOURCC( 'Y', 'U', 'Y', 'V' );
		format->bpp = 16;
		break;
		
	case VIDEO_PALETTE_UYVY:
		strcpy( format->identifier, "UYVY" );
		format->fourcc = FOURCC( 'U', 'Y', 'V', 'Y' );
		format->bpp = 16;
		break;
		
	case VIDEO_PALETTE_YUV420:
		strcpy( format->identifier, "Y 4:2:0" );
		format->fourcc = FOURCC( 'Y', '4', '2', '0' );
		format->bpp = 16;
		break;
		
	case VIDEO_PALETTE_YUV411:
		strcpy( format->identifier, "Y 4:1:1" );
		format->fourcc = FOURCC( 'Y', '4', '1', '1' );
		format->bpp = 12;
		break;
		
	case VIDEO_PALETTE_RAW:
		strcpy( format->identifier, "Bt848 raw format" );
		format->fourcc = FOURCC( 'R', 'A', 'W', ' ' );
		format->bpp = 8;
		break;
		
	case VIDEO_PALETTE_YUV422P:
		strcpy( format->identifier, "Y 4:2:2 planar" );
		format->fourcc = FOURCC( 'Y', '4', '2', 'P' );
		format->bpp = 16;
		break;
		
	case VIDEO_PALETTE_YUV411P:
		strcpy( format->identifier, "Y 4:1:1 planar" );
		format->fourcc = FOURCC( '4', '1', '1', 'P' );
		format->bpp = 12;
		break;
		
	default:
		TRACE( "unknown format / wrong index\n" );
		return STATUS_FAILURE;
	}

	format->size.width = v4lhandle->v4lcap.maxwidth;
	format->size.height = v4lhandle->v4lcap.maxheight;

	format->min_size.width = v4lhandle->v4lcap.minwidth;
	format->max_size.width = v4lhandle->v4lcap.maxwidth;
	
	format->min_size.height = v4lhandle->v4lcap.minheight;
	format->max_size.height = v4lhandle->v4lcap.maxheight;

	format->buffer_size = format->size.width * format->size.height * format->bpp / 8;

	format->sizes = 0;
	format->size_count = 0;

	format->h_stepping = 0;
	format->v_stepping = 0;

#ifdef DEBUG
	TRACE( "bsize: %d, wxhxb: %dx%dx%d\n", format->buffer_size, format->size.width, format->size.height, format->bpp );
	{
	   char str[1024];
	   int s = 1024;
	   unicap_describe_format( format, str, &s );
	   printf( str );
	}
#endif

	return STATUS_SUCCESS;
}

static unicap_status_t v4l_set_format( void *cpi_data, unicap_format_t *format )
{
	v4l_handle_t v4lhandle = ( v4l_handle_t ) cpi_data;
	unsigned int p;
	
	
	switch( format->fourcc )
	{
	case FOURCC( 'G', 'R', 'E', 'Y' ):
		p = VIDEO_PALETTE_GREY;
		break;
		
	case FOURCC( 'H', '2', '4', '0' ):
		p = VIDEO_PALETTE_HI240;
		break;

	case FOURCC( 'R', 'G', 'B', '6' ):
		p = VIDEO_PALETTE_RGB565;
		break;

	case FOURCC( 'R', 'G', 'B', '5' ):
		p = VIDEO_PALETTE_RGB555;
		break;
		
	case FOURCC( 'B', 'G', 'R', '3' ):
		p = VIDEO_PALETTE_RGB24;
		break;
		
	case FOURCC( 'R', 'G', 'B', '4' ):
		p = VIDEO_PALETTE_RGB32;
		break;
		
	case FOURCC( 'Y', '4', '2', '2' ):
		p = VIDEO_PALETTE_YUV422;
		break;
		
	case FOURCC( 'Y', 'U', 'Y', 'V' ):
		p = VIDEO_PALETTE_YUYV;
		break;
		
	case FOURCC( 'U', 'Y', 'V', 'Y' ):
		p = VIDEO_PALETTE_UYVY;
		break;
		
	case FOURCC( 'Y', '4', '2', '0' ):
		p = VIDEO_PALETTE_YUV420;
		break;
		
	case FOURCC( 'Y', '4', '1', '1' ):
		p = VIDEO_PALETTE_YUV411;
		break;
		
	case FOURCC( 'R', 'A', 'W', ' ' ):
		p = VIDEO_PALETTE_RAW;
		break;
		
	case FOURCC( 'Y', '4', '2', 'P' ):
		p = VIDEO_PALETTE_YUV422P;
		break;
		
	case FOURCC( '4', '1', '1', 'P' ):
		p = VIDEO_PALETTE_YUV411P;
		break;
		
	default:
		TRACE( "set format with unknown fourcc\n" );
		return STATUS_FAILURE;
		break;
	}
	
	// get current settings for brightness etc.
	if( ioctl( v4lhandle->fd, VIDIOCGPICT, &v4lhandle->v4lpict ) )
	{
		TRACE( "ioctl failed: %s\n", strerror( errno ) );
		return STATUS_FAILURE;
	}

	v4lhandle->v4lpict.palette = p;
	v4lhandle->v4lpict.depth = format->bpp;

	TRACE( "ioctl VIDIOCSPICT\n" );	
	
	if( ioctl( v4lhandle->fd, VIDIOCSPICT, &v4lhandle->v4lpict ) )
	{
		TRACE( "ioctl failed: %s\n", strerror( errno ) );
		TRACE( "p: %d\n", v4lhandle->v4lpict.palette );
		return STATUS_FAILURE;
	}
	if( ioctl( v4lhandle->fd, VIDIOCGPICT, &v4lhandle->v4lpict ) )
	{
		TRACE( "ioctl failed: %s\n", strerror( errno ) );
		TRACE( "p: %d\n", v4lhandle->v4lpict.palette );
		return STATUS_FAILURE;
	}
	
	
	TRACE( "result: depth: %d palette: %d\n", v4lhandle->v4lpict.depth, v4lhandle->v4lpict.palette );

	memset( &v4lhandle->v4lwindow, 0x0, sizeof( struct video_window ) );
	v4lhandle->v4lwindow.width = format->size.width;
	v4lhandle->v4lwindow.height = format->size.height;
	TRACE( "ioctl VIDIOCSWIN size: %dx%d\n", v4lhandle->v4lwindow.width, v4lhandle->v4lwindow.height );
	if( ioctl( v4lhandle->fd, VIDIOCSWIN, &v4lhandle->v4lwindow ) )
	{
		TRACE( "ioctl failed: %s\n", strerror( errno ) );
		return STATUS_FAILURE;
	}
	if( ioctl( v4lhandle->fd, VIDIOCGWIN, &v4lhandle->v4lwindow ) )
	{
		TRACE( "ioctl failed: %s\n", strerror( errno ) );
		return STATUS_FAILURE;
	}
	TRACE( "succeeded: size: %dx%d \n", v4lhandle->v4lwindow.x, v4lhandle->v4lwindow.y );


	unicap_copy_format( &v4lhandle->current_format, format );

	return STATUS_SUCCESS;
}

static unicap_status_t v4l_get_format( void *cpi_data, unicap_format_t *format )
{
	v4l_handle_t v4lhandle = ( v4l_handle_t ) cpi_data;
	memcpy( format, &v4lhandle->current_format, sizeof( unicap_format_t ) );
	return STATUS_SUCCESS;
}

static unicap_status_t v4l_reenumerate_properties( void *cpi_data, int *_pcount )
{
	v4l_handle_t v4lhandle = ( v4l_handle_t ) cpi_data;
	if( ioctl( v4lhandle->fd, VIDIOCGPICT, &v4lhandle->v4lpict_default ) )
	{
		TRACE( "ioctl failed: %s\n", strerror( errno ) );
		return STATUS_FAILURE;
	}
	if( _pcount )
	{
		*_pcount = 5;
	}

	return STATUS_SUCCESS;
}

static unicap_status_t v4l_enumerate_properties( void *cpi_data, unicap_property_t *property, int index )
{
	v4l_handle_t v4lhandle = ( v4l_handle_t ) cpi_data;

	memset( property, 0x0, sizeof( unicap_property_t ) );
	
	switch( index )
	{
	case 0:
		strcpy( property->identifier, "brightness" );
		property->value = v4lhandle->v4lpict_default.brightness;
		break;
		
	case 1:
		strcpy( property->identifier, "hue" );
		property->value = v4lhandle->v4lpict_default.hue;
		break;
		
	case 2:
		strcpy( property->identifier, "colour" );
		property->value = v4lhandle->v4lpict_default.colour;
		break;
		
	case 3:
		strcpy( property->identifier, "contrast" );
		property->value = v4lhandle->v4lpict_default.contrast;
		break;
		
	case 4:
		strcpy( property->identifier, "whiteness" );
		property->value = v4lhandle->v4lpict_default.whiteness;
		break;
		
	default:
		return STATUS_NO_MATCH;
	}
	
	
	property->range.min = 0.0f;
	property->range.max = 1.0f;
	property->value = property->value / 65535.0f;
	property->flags = UNICAP_FLAGS_MANUAL;
	property->flags_mask = UNICAP_FLAGS_MANUAL;
	property->stepping = 1.0f/256.0f;
	
	strcpy( property->category, "video" );

	return STATUS_SUCCESS;
}

static unicap_status_t v4l_set_property( void *cpi_data, unicap_property_t *property )
{
	v4l_handle_t v4lhandle = ( v4l_handle_t ) cpi_data;
	int value = property->value * 65535;
	
	if( !strcmp( property->identifier, "brightness" ) )
	{
		v4lhandle->v4lpict.brightness = value;
	}
	else if( !strcmp( property->identifier, "hue" ) )
	{
		v4lhandle->v4lpict.hue = value;
	}
	else if( !strcmp( property->identifier, "colour" ) )
	{
		v4lhandle->v4lpict.colour = value;
	}
	else if( !strcmp( property->identifier, "contrast" ) )
	{
		v4lhandle->v4lpict.contrast = value;
	}
	else if( !strcmp( property->identifier, "whiteness" ) )
	{
		v4lhandle->v4lpict.whiteness = value;
	} 
	else 
	{
		return STATUS_FAILURE;
	}
	
	if( ioctl( v4lhandle->fd, VIDIOCSPICT, &v4lhandle->v4lpict ) )
	{
		TRACE( "ioctl failed: %s\n", strerror( errno ) );
		return STATUS_FAILURE;
	}

	return STATUS_SUCCESS;
}

static unicap_status_t v4l_get_property( void *cpi_data, unicap_property_t *property )
{
	v4l_handle_t v4lhandle = ( v4l_handle_t ) cpi_data;
	double value;
	
	if( ioctl( v4lhandle->fd, VIDIOCGPICT, &v4lhandle->v4lpict ) )
	{
		TRACE( "ioctl failed: %s\n", strerror( errno ) );
		return STATUS_FAILURE;
	}

	if( !strcmp( property->identifier, "brightness" ) )
	{
		value = v4lhandle->v4lpict.brightness;
	}
	else if( !strcmp( property->identifier, "hue" ) )
	{
		value = v4lhandle->v4lpict.hue;
	}
	else if( !strcmp( property->identifier, "colour" ) )
	{
		value = v4lhandle->v4lpict.colour;
	}
	else if( !strcmp( property->identifier, "contrast" ) )
	{
		value = v4lhandle->v4lpict.contrast;
	}
	else if( !strcmp( property->identifier, "whiteness" ) )
	{
		value = v4lhandle->v4lpict.whiteness;
	} 
	else 
	{
		return STATUS_FAILURE;
	}

	property->range.min = 0.0f;
	property->range.max = 1.0f;
	property->value = value / 65535.0f;
	property->flags = UNICAP_FLAGS_MANUAL;
	property->flags_mask = UNICAP_FLAGS_MANUAL;
	property->stepping = 1.0f/256.0f;
	
	strcpy( property->category, "video" );

	return STATUS_SUCCESS;
}

static unicap_status_t v4l_capture_start( void *cpi_data )
{
	v4l_handle_t v4lhandle = ( v4l_handle_t ) cpi_data;
	int i;
	
	if( ioctl( v4lhandle->fd, VIDIOCGMBUF, &v4lhandle->v4lmbuf ) == -1 )
	{
	   TRACE( "ioctl failed: %s\n", strerror( errno ) );
	   return STATUS_FAILURE;
	}

	TRACE( "mbuf: size: %d, frames: %d\n", v4lhandle->v4lmbuf.size, v4lhandle->v4lmbuf.frames );
	for( i = 0; i < v4lhandle->v4lmbuf.frames; i++ )
	{
		TRACE( "offset %d: %x\n", i, v4lhandle->v4lmbuf.offsets[i] );
	}

	v4lhandle->map = mmap( 0, v4lhandle->v4lmbuf.size, PROT_WRITE | PROT_READ, MAP_SHARED, v4lhandle->fd, 0 );
	if( v4lhandle->map == MAP_FAILED )
	{
/* 		ERROR( "mmap failed: %s\nCapture can not start!\n", strerror( errno ) ); */
		return STATUS_FAILURE;
	}
	
	TRACE( "map: %p\n", v4lhandle->map );

	v4lhandle->ready_buffer = -1;

	
	for( i = 0; i < v4lhandle->v4lmbuf.frames; i++ )
	{
	   queue_buffer( v4lhandle, i );
	}
	
	v4lhandle->quit_capture_thread = 0;
	pthread_create( &v4lhandle->capture_thread, NULL, (void*(*)(void*))v4l_capture_thread, v4lhandle );
	
	v4lhandle->capture_running = 1;
	
	return STATUS_SUCCESS;
}

static unicap_status_t v4l_capture_stop( void *cpi_data )
{
   v4l_handle_t v4lhandle = ( v4l_handle_t ) cpi_data;

   if( v4lhandle->capture_running )
   {
      
      v4lhandle->quit_capture_thread = 1;
      sem_post( &v4lhandle->out_sema );
      pthread_join( v4lhandle->capture_thread, NULL );
      
      if( v4lhandle->map )
      {
	 munmap( v4lhandle->map, v4lhandle->v4lmbuf.size );
	 v4lhandle->map = 0;
      }
      
      v4lhandle->capture_running = 0;
   }
   
   return STATUS_SUCCESS;
}

static unicap_status_t v4l_queue_buffer( void *cpi_data, unicap_data_buffer_t *buffer )
{
   v4l_handle_t v4lhandle = ( v4l_handle_t ) cpi_data;
   struct _unicap_queue *entry;

   entry = malloc( sizeof( unicap_queue_t ) );
   entry->data = buffer;
   ucutil_insert_back_queue( v4lhandle->in_queue, entry );
   
   return STATUS_SUCCESS;
}

static unicap_status_t v4l_dequeue_buffer( void *cpi_data, unicap_data_buffer_t **buffer )
{
	v4l_handle_t handle = ( v4l_handle_t ) cpi_data;
	TRACE( "dequeue buffer\n" );

	if( handle->capture_running )
	{
	   TRACE( "dequeue while capturing\n" );
	   return STATUS_FAILURE;
	}

	return STATUS_NOT_IMPLEMENTED;
}

static unicap_status_t v4l_wait_buffer( void *cpi_data, unicap_data_buffer_t **buffer )
{
   v4l_handle_t handle = ( v4l_handle_t ) cpi_data;
   unicap_status_t status = STATUS_FAILURE;

   *buffer = NULL;

   if( !handle->out_queue->next &&! handle->capture_running )
   {
      TRACE( "Wait->capture stopped" );
      return STATUS_IS_STOPPED;
   }

   sem_wait( &handle->out_sema );
   
   if( handle->out_queue->next )
   {
      unicap_data_buffer_t *returned_buffer;
      struct _unicap_queue *entry;
      
      entry = ucutil_get_front_queue( handle->out_queue );
      returned_buffer = ( unicap_data_buffer_t * ) entry->data;
      free( entry );
      
      *buffer = returned_buffer;

      status = STATUS_SUCCESS;
   }

   return status;
}

static unicap_status_t v4l_poll_buffer( void *data, int *count )
{
	TRACE( "poll buffer\n" );
	return STATUS_NOT_IMPLEMENTED;
}

static unicap_status_t v4l_set_event_notify( void *cpi_data, unicap_event_callback_t func, unicap_handle_t unicap_handle )
{
   v4l_handle_t handle = ( v4l_handle_t )cpi_data;
   
   handle->event_callback = func;
   handle->unicap_handle = unicap_handle;
   
   return STATUS_SUCCESS;
}

static int queue_buffer( v4l_handle_t v4lhandle, int buffer )
{
   struct video_mmap v4lmmap;
   int frame;
   
   frame = buffer;
   
   TRACE( "queue buffer, frame: %d\n", frame );
   
   v4lmmap.frame = frame;
   v4lmmap.width = v4lhandle->current_format.size.width;
   v4lmmap.height = v4lhandle->current_format.size.height;
   v4lmmap.format = v4lhandle->v4lpict.palette;
      
   if( ioctl( v4lhandle->fd, VIDIOCMCAPTURE, &v4lmmap ) == -1 )
   {
      TRACE( "ioctl failed: %s\n", strerror( errno ) );
      return 0;
   }
      
   return 1;
}

static int wait_buffer( v4l_handle_t v4lhandle, unicap_data_buffer_t *data_buffer )
{
   int frame;
	
   v4lhandle->ready_buffer = ( v4lhandle->ready_buffer + 1 ) % v4lhandle->v4lmbuf.frames;
   frame = v4lhandle->ready_buffer;
	
   TRACE( "wait buffer, frame: %d\n", frame );

   if( ioctl( v4lhandle->fd, VIDIOCSYNC, &frame ) == -1 )
   {
      TRACE( "ioctl failed: %s\n", strerror( errno ) );
      return 0;
   }
	
   data_buffer->data = v4lhandle->map + v4lhandle->v4lmbuf.offsets[frame];   
   gettimeofday( &data_buffer->fill_time, NULL );

   return 1;
}

static void v4l_capture_thread( v4l_handle_t handle )
{
   unicap_data_buffer_t new_frame_buffer;

   unicap_copy_format( &new_frame_buffer.format, &handle->current_format );
   new_frame_buffer.buffer_size = handle->current_format.buffer_size;
   new_frame_buffer.type = UNICAP_BUFFER_TYPE_SYSTEM;

   while( !handle->quit_capture_thread )
   {
      unicap_queue_t *entry;
      

      if( sem_wait( &handle->sema ) )
      {
	 TRACE( "SEM_WAIT_FAILED" );
      }
      
      if( !wait_buffer( handle, &new_frame_buffer ) )
      {
	 sem_post( &handle->sema );
	 continue;
      }

      sem_post( &handle->sema );

      if( handle->event_callback )
      {
	 handle->event_callback( handle->unicap_handle, UNICAP_EVENT_NEW_FRAME, &new_frame_buffer );
      }
      
      entry = ucutil_get_front_queue( handle->in_queue );
      if( entry )
      {
	 unicap_data_buffer_t *data_buffer = ( unicap_data_buffer_t * ) entry->data;
	 struct _unicap_queue *outentry = malloc( sizeof( unicap_queue_t ) );
	 free( entry );

	 data_buffer->buffer_size = new_frame_buffer.buffer_size;
	 unicap_copy_format( &data_buffer->format, &new_frame_buffer.format );
	 if( data_buffer->type == UNICAP_BUFFER_TYPE_SYSTEM )
	 {
	    data_buffer->data = new_frame_buffer.data;
	 }
	 else
	 {
	    memcpy( data_buffer->data, new_frame_buffer.data, new_frame_buffer.buffer_size );
	 }
	 outentry->data = data_buffer;
	 memcpy( &data_buffer->fill_time, &new_frame_buffer.fill_time, sizeof( struct timeval ) );
	 ucutil_insert_back_queue( handle->out_queue, outentry );
	 sem_post( &handle->out_sema );
      }
      
      if( sem_wait( &handle->sema ) )
      {
	 TRACE( "SEM_WAIT_FAILED\n" );
      }
      
      queue_buffer( handle, handle->ready_buffer );
      
      sem_post( &handle->sema );
   }   
}
