/*
** tiseuvccam.c
** 
** Made by (Arne Caspari)
** Login   <arne@localhost>
** 
** Started on  Thu Jun  5 14:33:13 2008 Arne Caspari
*/

#include "config.h"
#include "tiseuvccam.h"

#include <limits.h>
#include <stdlib.h>
#include <linux/types.h>
#include <string.h>
#include <libgen.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/videodev2.h>

#include "uvc_compat.h"

#if V4L2_DEBUG
#define DEBUG
#endif

#define TRUE 1

#include "debug.h"

#define N_(x) x

#define FOURCC(a,b,c,d) (unsigned int)((((unsigned int)d)<<24)+(((unsigned int)c)<<16)+(((unsigned int)b)<<8)+a)

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


#define V4L2_CID_EUVC_REGISTER 0x00980926


static double frame_rates[] = 
{
   60.0,
   30.0, 
   25.0, 
   15.0, 
   7.5
};

static int frame_rate_register_values[] = 
{
   6, 
   0, 
   1, 
   2, 
   3
};



static int get_usb_ids( const char *path, unsigned short *vid, unsigned short *pid )
{
   char respath[PATH_MAX];
   char *basen;
   char prodidpath[PATH_MAX];
   char vendidpath[PATH_MAX];
   char devicepath[PATH_MAX];
   char tmp[PATH_MAX];
   FILE *f;

   strcpy( tmp, path );
   basen = basename( tmp );
   sprintf( devicepath, "/sys/class/video4linux/%s/device", basen );
   
   if( !realpath( devicepath, respath ) )
   {
      TRACE( "could not resolve path '%s'\n", devicepath );
      return 0;
   }
   
   strcpy( vendidpath, devicepath );
   strcat( vendidpath, "/../idVendor" );

   f = fopen( vendidpath, "r" );
   if( !f )
   {
      TRACE( "could not open '%s'\n", vendidpath );
      return 0;
   }
   else
   {
      char buf[5];
      memset( buf, 0x0, sizeof( buf ) );
      if( fscanf( f, "%hx", vid ) < 1 )
      {
	 vid = 0;
      }
      fclose( f );
   }
   

   strcpy( prodidpath, devicepath );
   strcat( prodidpath, "/../idProduct" );
   
   f = fopen( prodidpath, "r" );
   if( !f )
   {
      TRACE( "could not open '%s'\n", prodidpath );
      return 0;
   }
   else
   {
      char buf[5];
      memset( buf, 0x0, sizeof( buf ) );
      if( fscanf( f, "%hx", pid ) < 1 )
      {
	 pid = 0;
      }
      fclose( f );
   }
   
   return 1;
}

int tiseuvccam_probe( v4l2_handle_t handle, const char *path )
{
   int ret = 0;
   unsigned short vid, pid;

   get_usb_ids( path, &vid, &pid );

   handle->pid = pid;

   if( ( vid == 0x199e ) && ( ( pid == 0x8201 ) || ( pid == 0x8202 ) || ( pid == 0x8203 ) || ( pid == 0x8204 ) ) )
   {
      TRACE( "Found TIS EUVCCAM\n" );
      ret = 1;
   }
   
   return ret;
}


int tiseuvccam_count_ext_property( v4l2_handle_t handle )
{
   return 1;
}


unicap_status_t tiseuvccam_enumerate_properties( v4l2_handle_t handle, int index, unicap_property_t *property )
{
   unicap_status_t status = STATUS_NO_MATCH;

   switch( index )
   {
      case 0:
      {
	 struct v4l2_control ctrl;
	 strcpy( property->identifier, "sharpness register" );
	 strcpy( property->category, "debug" );
	 memset( &ctrl, 0x0, sizeof( ctrl ) );
	 ctrl.id = V4L2_CID_EUVC_REGISTER;
	 
	 if( IOCTL( handle->fd, VIDIOC_G_CTRL, &ctrl ) < 0 )
	 {
	    // XU control not added to the driver
	    return STATUS_NO_MATCH;
	 }
	 
	 property->type = UNICAP_PROPERTY_TYPE_RANGE;
	 property->flags_mask = UNICAP_FLAGS_MANUAL;
	 property->flags = UNICAP_FLAGS_MANUAL;
	 property->value = 0;
	 property->range.min = 0;
	 property->range.max = 0xff;
	 property->stepping = 1.0;
	 
	 return STATUS_SUCCESS;
      }
      break;

      default: 
	 break;
   }	 

   
   return status;
}

unicap_status_t tiseuvccam_override_property( v4l2_handle_t handle, struct v4l2_queryctrl *ctrl, unicap_property_t *property )
{
   unicap_status_t status = STATUS_NO_MATCH;

   if( !ctrl )
   {
      if( !strcmp( property->identifier, "frame rate" ) )
      {
	 struct v4l2_control ctrl;
	 memset( &ctrl, 0x0, sizeof( ctrl ) );
	 ctrl.id = V4L2_CID_EUVC_REGISTER;

	 if( IOCTL( handle->fd, VIDIOC_G_CTRL, &ctrl ) < 0 )
	 {
	    // XU control not added to the driver
	    return STATUS_NO_MATCH;
	 }
	 
	 handle->frame_rate = 30.0;

	 property->type = UNICAP_PROPERTY_TYPE_VALUE_LIST;
	 property->value = 30.0;
	 property->value_list.values = frame_rates;
	 property->value_list.value_count = sizeof( frame_rates ) / sizeof( double );
	 return STATUS_SUCCESS;
      }
      else
      {
	 return STATUS_NO_MATCH;
      }
   }
   
   
   switch( ctrl->id )
   {
#ifdef V4L2_CID_PRIVACY_CONTROL
      case V4L2_CID_PRIVACY_CONTROL:
	 if( property )
	 {
	    strcpy( property->identifier, N_("trigger") );
	    strcpy( property->category, N_("device" ) );
	    strcpy( property->unit, "" );
	    property->flags_mask = UNICAP_FLAGS_MANUAL | UNICAP_FLAGS_ON_OFF;
	    property->flags = UNICAP_FLAGS_MANUAL;
	    property->type = UNICAP_PROPERTY_TYPE_FLAGS;
	 }
	 status = STATUS_SUCCESS;
	 
	 break;
#endif
      case V4L2_CID_EXPOSURE_AUTO:
	 status = STATUS_SKIP_CTRL;
	 break;

      case V4L2_CID_EXPOSURE_ABSOLUTE:
	 if( property )
	 {
	    strcpy( property->identifier, N_("shutter") );
	    strcpy( property->category, N_("exposure") );
	    strcpy( property->unit, "s" );
	    if( TRUE /* handle->pid == 0x8203 */ )
	    {
	       property->flags_mask = UNICAP_FLAGS_MANUAL | UNICAP_FLAGS_AUTO;
	       property->flags = UNICAP_FLAGS_AUTO;
	    }
	    else
	    {
	       property->flags_mask = UNICAP_FLAGS_MANUAL;
	       property->flags = UNICAP_FLAGS_MANUAL;
	    }
	    property->type = UNICAP_PROPERTY_TYPE_RANGE;
	    property->range.min = (double)ctrl->minimum / 10000.0;
	    property->range.max = (double)ctrl->maximum / 10000.0;
	    property->value = (double)ctrl->default_value / 10000.0;
	 }
	 status = STATUS_SUCCESS;
	 break;

      case V4L2_CID_GAIN:
	 if( property )
	 {
	    strcpy( property->identifier, N_("gain") );
	    strcpy( property->category, N_("exposure" ) );
	    if( TRUE /* handle->pid != 0x8203 */ )
	    {
	       property->flags_mask = UNICAP_FLAGS_MANUAL | UNICAP_FLAGS_AUTO;
	       property->flags = UNICAP_FLAGS_AUTO;
	    }
	    else
	    {
	       property->flags_mask = UNICAP_FLAGS_MANUAL;
	       property->flags = UNICAP_FLAGS_MANUAL;
	    }
	    property->type = UNICAP_PROPERTY_TYPE_RANGE;
	    property->range.min = (double)ctrl->minimum;
	    property->range.max = (double)ctrl->maximum;
	    property->stepping = 1.0;
	    property->value = (double)ctrl->default_value;
	 }
	 status = STATUS_SUCCESS;
	 break;

/* 	 case V4L2_CID_WHITE */
	 

      default:
	 break;
	 
   }
   
	    
   return status;
}

unicap_status_t tiseuvccam_set_property( v4l2_handle_t handle, unicap_property_t *property )
{
   unicap_status_t status = STATUS_NO_MATCH;

   if( !strcmp( property->identifier, "trigger" ) )
   {
#ifdef V4L2_CID_PRIVACY_CONTROL
      struct v4l2_control ctrl;
      memset( &ctrl, 0x0, sizeof( ctrl ) );
      
      ctrl.id = V4L2_CID_PRIVACY_CONTROL;
      ctrl.value = ( ( property->flags & UNICAP_FLAGS_ON_OFF ) == UNICAP_FLAGS_ON_OFF ) ? 1 : 0;
      if( IOCTL( handle->fd, VIDIOC_S_CTRL, &ctrl ) < 0 )
      {
	 TRACE( "VIDIOC_S_CTRL failed\n" );
	 return STATUS_FAILURE;
      }
      else
      {
	 status = STATUS_SUCCESS;
      }
#else
      return STATUS_FAILURE;
#endif
   } 
   else if( !strcmp( property->identifier, "shutter" ) )
   {
      struct v4l2_control ctrl;
      int shift = 1;

      if( handle->pid == 0x8201 )
      {
	 shift = 2;
      }

      memset( &ctrl, 0x0, sizeof( ctrl ) );
      if( (handle->pid != 0x8203 ) && ( handle->pid != 0x8204 ) )
      {
	 ctrl.id = V4L2_CID_EXPOSURE_AUTO;
	 
	 if( IOCTL( handle->fd, VIDIOC_G_CTRL, &ctrl ) < 0 )
	 {
	    TRACE( "VIDIOC_G_CTRL failed\n" );
	    return STATUS_FAILURE;
	 }
	 ctrl.value &= ~(1<<shift);
	 if( property->flags & UNICAP_FLAGS_AUTO )
	 {
	    ctrl.value |= 1<<shift;
	 }

	 if( IOCTL( handle->fd, VIDIOC_S_CTRL, &ctrl ) < 0 )
	 {
	    TRACE( "VIDIOC_S_CTRL failed\n" );
	    return STATUS_FAILURE;
	 }
      }
	  
      if( property->flags & UNICAP_FLAGS_MANUAL )
      {
	 memset( &ctrl, 0x0, sizeof( ctrl ) );
	 ctrl.id = V4L2_CID_EXPOSURE_ABSOLUTE;
	 ctrl.value = property->value * 10000;
	 if( IOCTL( handle->fd, VIDIOC_S_CTRL, &ctrl ) < 0 )
	 {
	    TRACE( "VIDIOC_S_CTRL failed\n" );
	    return STATUS_FAILURE;
	 }

	 status = STATUS_SUCCESS;
      }
   }
   else if( !strcmp( property->identifier, "gain" ) )
   {
      struct v4l2_control ctrl;
      int shift = 1;
      if( handle->pid == 0x8201 )
      {
	 shift = 2;
      }
      
      memset( &ctrl, 0x0, sizeof( ctrl ) );

      if( (handle->pid != 0x8203 ) && ( handle->pid != 0x8204 ) )
      {
	 ctrl.id = V4L2_CID_EXPOSURE_AUTO;
	 
	 if( IOCTL( handle->fd, VIDIOC_G_CTRL, &ctrl ) < 0 )
	 {
	    TRACE( "VIDIOC_G_CTRL failed\n" );
	    return STATUS_FAILURE;
	 }
	 
	 ctrl.value &= ~(2<<shift);
	 if( property->flags & UNICAP_FLAGS_AUTO )
	 {
	    ctrl.value |= 2<<shift;
	 }
	 if( IOCTL( handle->fd, VIDIOC_S_CTRL, &ctrl ) < 0 )
	 {
	    TRACE( "VIDIOC_S_CTRL failed\n" );
	    return STATUS_FAILURE;
	 }
      }
      
      if( property->flags & UNICAP_FLAGS_MANUAL )
      {
	 memset( &ctrl, 0x0, sizeof( ctrl ) );
	 ctrl.id = V4L2_CID_GAIN;
	 ctrl.value = property->value;
	 if( IOCTL( handle->fd, VIDIOC_S_CTRL, &ctrl ) < 0 )
	 {
	    TRACE( "VIDIOC_S_CTRL failed\n" );
	    return STATUS_FAILURE;
	 }
      }
      status = STATUS_SUCCESS;
   }
   else if( !strcmp( property->identifier, "frame rate" ) )
   {
      struct v4l2_control ctrl;
      double d = 9999999;
      int sel = 0, i;
      
      memset( &ctrl, 0x0, sizeof( ctrl ) );
      ctrl.id = V4L2_CID_EUVC_REGISTER;
      for( i = 0; i < ( sizeof( frame_rates ) / sizeof( double ) ); i ++ )
      {
	 if( abs( property->value - frame_rates[i] < d ) )
	 {
	    d = abs( property->value - frame_rates[i] < d );
	    sel = i;
	 }
      }
      ctrl.value =  ( (unsigned int )frame_rate_register_values[sel] << 16) | (0x01 <<8) | 0x3a;

      if( IOCTL( handle->fd, VIDIOC_S_CTRL, &ctrl ) < 0 )
      {
	 TRACE( "failed to set frame rate control\n" );
	 status = STATUS_FAILURE;
      }      

      handle->frame_rate = frame_rates[sel];
      status = STATUS_SUCCESS;
   }
   else if( !strcmp( property->identifier, "sharpness register" ) )
   {
      struct v4l2_control ctrl;
      int val;
      val = property->value;
      val &= 0xff;
      memset( &ctrl, 0x0, sizeof( ctrl ) );
      ctrl.id = V4L2_CID_EUVC_REGISTER;
      ctrl.value =  ( val << 16) | (0x01 <<8) | 0x23;
      if( IOCTL( handle->fd, VIDIOC_S_CTRL, &ctrl ) < 0 )
      {
	 TRACE( "sharpness register\n" );
	 status = STATUS_FAILURE;
      }      
      ctrl.value =  ( val << 16) | (0x01 <<8) | 0x24;
      if( IOCTL( handle->fd, VIDIOC_S_CTRL, &ctrl ) < 0 )
      {
	 TRACE( "sharpness register\n" );
	 status = STATUS_FAILURE;
      }      

/*       handle->sharpness_register = val; */
      status = STATUS_SUCCESS;
   }
      
      
   

   return status;
}

unicap_status_t tiseuvccam_get_property( v4l2_handle_t handle, unicap_property_t *property )
{
   unicap_status_t status = STATUS_NO_MATCH;

   if( !strcmp( property->identifier, "trigger" ) )
   {
#ifdef V4L2_CID_PRIVACY_CONTROL
      struct v4l2_control ctrl;
      memset( &ctrl, 0x0, sizeof( ctrl ) );
      
      ctrl.id = V4L2_CID_PRIVACY_CONTROL;
      if( IOCTL( handle->fd, VIDIOC_G_CTRL, &ctrl ) < 0 )
      {
	 TRACE( "VIDIOC_G_CTRL failed\n" );
	 return STATUS_FAILURE;
      }
      else
      {
	 status = STATUS_SUCCESS;
      }

      if( ctrl.value )
      {
	 property->flags = UNICAP_FLAGS_MANUAL | UNICAP_FLAGS_ON_OFF;
      }
      else
      {
	 property->flags = UNICAP_FLAGS_MANUAL;
      }      
#else
      return STATUS_FAILURE;
#endif
   }
   else if( !strcmp( property->identifier, "shutter" ) )
   {
      struct v4l2_control ctrl;
      int shift = 1;
      
      if( handle->pid == 0x8201 )
      {
	 shift = 2;
      }
      memset( &ctrl, 0x0, sizeof( ctrl ) );      
      property->flags = UNICAP_FLAGS_MANUAL;
      if( (handle->pid != 0x8203 ) && ( handle->pid != 0x8204 ) )
      {
	 ctrl.id = V4L2_CID_EXPOSURE_AUTO;
	 if( IOCTL( handle->fd, VIDIOC_G_CTRL, &ctrl ) < 0 )
	 {
	    TRACE( "VIDIOC_G_CTRL ( V4L2_CID_EXPOSURE_AUTO ) failed\n" );
	    return STATUS_FAILURE;
	 }
	 else
	 {
	    status = STATUS_SUCCESS;
	 }
	 
	 if( !(ctrl.value & (1<<shift) ) )
	 {
	    property->flags = UNICAP_FLAGS_AUTO;
	 }
	 else
	 {
	    property->flags = UNICAP_FLAGS_MANUAL;
	 }
      }
      
      memset( &ctrl, 0x0, sizeof( ctrl ) );
      ctrl.id = V4L2_CID_EXPOSURE_ABSOLUTE;
      if( IOCTL( handle->fd, VIDIOC_G_CTRL, &ctrl ) < 0 )
      {
	 TRACE( "VIDIOC_G_CTRL failed!\n" );
	 return STATUS_FAILURE;
      }
      else
      {
	 property->value = ctrl.value / 10000.0;
	 status = STATUS_SUCCESS;
      }
   }   
   else if( !strcmp( property->identifier, "gain" ) )
   {
      struct v4l2_control ctrl;
      int shift = 1;

      if( handle->pid == 0x8201 )
      {
	 shift = 2;
      }
      
      memset( &ctrl, 0x0, sizeof( ctrl ) ); 
      property->flags = UNICAP_FLAGS_MANUAL;
      if( (handle->pid != 0x8203 ) && ( handle->pid != 0x8204 ) )
      {
	 ctrl.id = V4L2_CID_EXPOSURE_AUTO;
	 if( IOCTL( handle->fd, VIDIOC_G_CTRL, &ctrl ) < 0 )
	 {
	    TRACE( "VIDIOC_G_CTRL ( V4L2_CID_EXPOSURE_AUTO ) failed\n" );
	    return STATUS_FAILURE;
	 }
	 else
	 {
	    status = STATUS_SUCCESS;
	 }
      
      
	 if( !(ctrl.value & (2<<shift) ) )
	 {
	    property->flags = UNICAP_FLAGS_AUTO;
	 }
	 else
	 {
	    property->flags = UNICAP_FLAGS_MANUAL;
	 }
      }
      
      memset( &ctrl, 0x0, sizeof( ctrl ) );
      ctrl.id = V4L2_CID_GAIN;
      if( IOCTL( handle->fd, VIDIOC_G_CTRL, &ctrl ) < 0 )
      {
	 TRACE( "VIDIOC_G_CTRL failed!\n" );
	 return STATUS_FAILURE;
      }
      else
      {
	 property->value = ctrl.value;
	 status = STATUS_SUCCESS;
      }
   } 
   else if( !strcmp( property->identifier, "frame rate" ) )
   {
      property->value = handle->frame_rate;
      status = STATUS_SUCCESS;
   }
   else if( !strcmp( property->identifier, "Register" ) )
   {
      struct v4l2_control ctrl;
      int val;
      val = property->value;
      val &= 0xff;
      memset( &ctrl, 0x0, sizeof( ctrl ) );
      ctrl.id = V4L2_CID_EUVC_REGISTER;
      ctrl.value =  val;
      if( IOCTL( handle->fd, VIDIOC_S_CTRL, &ctrl ) < 0 )
      {
	 TRACE( "register\n" );
	 status = STATUS_FAILURE;
      }      
      ctrl.value = val;
      if( IOCTL( handle->fd, VIDIOC_S_CTRL, &ctrl ) < 0 )
      {
	 TRACE( "register\n" );
	 status = STATUS_FAILURE;
      }      

      if( IOCTL( handle->fd, VIDIOC_G_CTRL, &ctrl ) < 0 )
      {
	 TRACE( "register\n" );
	 status = STATUS_FAILURE;
      }      
      
      property->value = ctrl.value;

      status = STATUS_SUCCESS;
      
   }
   else if( !strcmp( property->identifier, "sharpness register" ) )
   {
/*       property->value = handle->sharpness_register; */
      property->flags = UNICAP_FLAGS_MANUAL;
      status = STATUS_SUCCESS;
   }
   
   
   return status;
}

unicap_status_t tiseuvccam_fmt_get( struct v4l2_fmtdesc *v4l2fmt, struct v4l2_cropcap *cropcap, char **identifier, unsigned int *fourcc, int *bpp )
{
   unicap_status_t status = STATUS_NO_MATCH;

   TRACE( "fmt_get desc='%s', defrect.width=%d\n", v4l2fmt->description, cropcap->defrect.width );

   if( ( (float)cropcap->defrect.width / (float)cropcap->defrect.height ) < 1.0 )
   {
      if( ( v4l2fmt->pixelformat == FOURCC( 'Y', 'U', 'Y', 'V' ) ) /* &&  */
/*        ( ( cropcap->defrect.width == 372 ) || ( cropcap->defrect.width == 320 ) || ( cropcap->defrect.width == 1024 )  */
/* 	  || ( cropcap->defrect.width == 512 ) ) */ )
      {
	 if( identifier )
	 {
	    *identifier = "Y800";
	 }
	 if( fourcc )
	 {
	    *fourcc = FOURCC( 'Y', '8', '0', '0' );
	 }
	 if( bpp )
	 {
	    *bpp = 8;
	 }
	 
	 cropcap->defrect.width *= 2;
	 cropcap->bounds.width *= 2;
      
	 status = STATUS_SUCCESS;
      }
      else if( ( v4l2fmt->pixelformat == FOURCC( 'U', 'Y', 'V', 'Y' ) ) /* &&  */
/* 	    ( ( cropcap->defrect.width == 372 ) || ( cropcap->defrect.width == 320 ) || ( cropcap->defrect.width == 1024 ) */
      /* 	       || ( cropcap->defrect.width == 512 ) ) */ )
      {
	 TRACE( "skip format: %s\n", v4l2fmt->description );
	 status = STATUS_SKIP_CTRL;
      }
   }
   
   TRACE( "fmt get: id = %s width= %d\n", identifier ? *identifier : "", cropcap->defrect.width );
   return status;
}

#ifdef VIDIOC_ENUM_FRAMESIZES
unicap_status_t tiseuvccam_override_framesize( v4l2_handle_t handle, struct v4l2_frmsizeenum *frms )
{
   unicap_status_t status = STATUS_NO_MATCH;
   
/*    if( ( frms->discrete.height == 480 && ( ( frms->discrete.width == 372 ) || ( frms->discrete.width == 320 ) ) ) || */
/*        ( frms->discrete.height == 1532 && ( ( frms->discrete.width == 1024 ) ) ) || */
/*        ( frms->discrete.height == 768 && ( frms->discrete.width == 512 ) ) */
/*       ) */
/*    { */
   if( handle->pid != 0x8201 )
   {
      frms->discrete.width *= 2;
      status = STATUS_SUCCESS;
   }
   
/*    } */
   
   return status;
}
#endif

unicap_status_t tiseuvccam_tov4l2format( v4l2_handle_t handle, unicap_format_t *format )
{
   unicap_status_t status = STATUS_NO_MATCH;

   usleep(100000);
   
   
   if( format->fourcc == FOURCC( 'Y', '8', '0', '0' ) )
   {
      format->fourcc = FOURCC( 'Y', 'U', 'Y', 'V' );
      format->size.width /= 2;
      format->bpp = 16;
      status = STATUS_SUCCESS;
   }
   
   return status;
}
