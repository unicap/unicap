/*
** tisuvccam.c
** 
** Made by (Arne Caspari)
** Login   <arne@localhost>
** 
** Started on  Tue Jan  8 07:22:47 2008 Arne Caspari
*/

#include "config.h"
#include "tisuvccam.h"

#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <string.h>

#include "uvcvideo.h"

#if V4L2_DEBUG
#define DEBUG
#endif
#include "debug.h"

#define N_(x) x

#define FOURCC(a,b,c,d) (unsigned int)((((unsigned int)d)<<24)+(((unsigned int)c)<<16)+(((unsigned int)b)<<8)+a)
      
#define UVC_GUID_TISUVC_XU	{0x0a, 0xba, 0x49, 0xde, 0x5c, 0x0b, 0x49, 0xd5, \
				 0x8f, 0x71, 0x0b, 0xe4, 0x0f, 0x94, 0xa6, 0x7a}


#define V4L2_CID_AUTO_GAIN              0x00980920
#define V4L2_CID_AUTO_SHUTTER           0x00980921
#define V4L2_CID_WHITE_BALANCE_ONE_PUSH 0x00980922
#define V4L2_CID_AUTO_SHUTTER_MAX       0x00980923
#define V4L2_CID_AUTO_SHUTTER_MAX_AUTO  0x00980924
#define V4L2_CID_TRIGGER                0x00980925


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

typedef enum
{
   XU_AUTO_SHUTTER          = 0x1,
   XU_AUTO_GAIN             = 0x2, 
   XU_ONE_PUSH_WB           = 0x3, 
   XU_AUTO_EXPOSURE_REF     = 0x4,
   XU_TRIGGER               = 0x5, 
   XU_STROBE_MODE           = 0x6, 
   XU_STROBE_PARAMS         = 0x7, 
   XU_GPIO                  = 0x8, 
   XU_AUTO_SHUTTER_MAX      = 0x9, 
   XU_AUTO_SHUTTER_MAX_AUTO = 0xa,
   XU_AVG_FRAMES            = 0xe,
} TISUVCCtrlIdx;

typedef enum
{
   CTRL_TYPE_BITMAP = 0, 
   CTRL_TYPE_RANGE, 
} TISUVCCtrlType;

struct ctrlrange
{
      int min;
      int max;
};


struct ppty_info
{
      char identifier[128];
      unicap_status_t (*set_func)(int fd, unicap_property_t *property);
      unicap_status_t (*get_func)(int fd, unicap_property_t *property);
};


struct ctrlinfo
{
      struct uvc_xu_control_info xu_ctrl_info;
      TISUVCCtrlType type;
      union
      {
	    unsigned int bitmap_mask;
	    struct ctrlrange range;
      };    
      int def;
      int has_property;

      unicap_property_t property;
};

struct uvc_format {
	__u8 type;
	__u8 index;
	__u8 bpp;
	__u8 colorspace;
	__u32 fcc;
	__u32 flags;

	char name[32];

	unsigned int nframes;
	struct uvc_frame *frame;
};

static unicap_status_t tisuvccam_set_shutter( int fd, unicap_property_t *property );
static unicap_status_t tisuvccam_get_shutter( int fd, unicap_property_t *property );
static unicap_status_t tisuvccam_set_gain( int fd, unicap_property_t *property );
static unicap_status_t tisuvccam_get_gain( int fd, unicap_property_t *property );
static unicap_status_t tisuvccam_set_wb_auto( int fd, unicap_property_t *property );
static unicap_status_t tisuvccam_get_wb_auto( int fd, unicap_property_t *property );


#define TRIGGER_FREE_RUNNING "free running"
#define TRIGGER_FALLING_EDGE "trigger on falling edge"
#define TRIGGER_RISING_EDGE  "trigger on rising edge"

static char* trigger_menu[] = 
{
   N_(TRIGGER_FREE_RUNNING), 
   N_(TRIGGER_FALLING_EDGE), 
   N_(TRIGGER_RISING_EDGE)
};



static struct ctrlinfo TISUVCCtrlInfo[] = 
{
   {
      xu_ctrl_info: {
	       UVC_GUID_TISUVC_XU,
	       index: 0,
	       selector: XU_AUTO_SHUTTER,
	       size: 1,
	       flags: UVC_CONTROL_GET_CUR | UVC_CONTROL_SET_CUR
      },
      CTRL_TYPE_BITMAP,
      {bitmap_mask: 0x1},
      def: 0x1,
      has_property: 0,
      property:{
	 identifier: N_("auto shutter"),
	 category: N_("exposure"),
	 unit: "",
	 relations: NULL,
	 relations_count: 0,
	 {value: 0},
	 {range: { min: 0, max: 0 } },
	 stepping: 0.0,
	 type: UNICAP_PROPERTY_TYPE_FLAGS,
	 flags: UNICAP_FLAGS_AUTO,
	 flags_mask: UNICAP_FLAGS_AUTO,
	 property_data: NULL,
	 property_data_size: 0
      }
   },
   {
      xu_ctrl_info: {
	       UVC_GUID_TISUVC_XU,
	       index: 1,
	       selector: XU_AUTO_GAIN,
	       size: 1,
	       flags: UVC_CONTROL_GET_CUR | UVC_CONTROL_SET_CUR
      },
      CTRL_TYPE_BITMAP,
      {bitmap_mask: 0x1},
      def: 0x1,
      has_property: 0,
      property:{
	 identifier: N_("auto gain"),
	 category: N_("exposure"),
	 unit: "",
	 relations: NULL,
	 relations_count: 0,
	 {value: 0},
	 {range: { min: 0, max: 0 } },
	 stepping: 0.0,
	 type: UNICAP_PROPERTY_TYPE_FLAGS,
	 flags: UNICAP_FLAGS_AUTO,
	 flags_mask: UNICAP_FLAGS_AUTO,
	 property_data: NULL,
	 property_data_size: 0
      }
   },
   {
      xu_ctrl_info: {
	       UVC_GUID_TISUVC_XU, 
	       index: 2, 
	       selector: XU_ONE_PUSH_WB,
	       size: 1,
	       flags: UVC_CONTROL_GET_CUR | UVC_CONTROL_SET_CUR 
      },       
      CTRL_TYPE_BITMAP, 
      {bitmap_mask: 0x1}, 
      def: 0x1,
      has_property: 0,
      property:{
	 identifier: N_("one push wb"), 
	 category: N_("color"), 
	 unit: "", 
	 relations: NULL, 
	 relations_count: 0, 
	 {value: 0}, 
	 {range: { min: 0, max: 0 } }, 
	 stepping: 0.0, 
	 type: UNICAP_PROPERTY_TYPE_FLAGS, 
	 flags: UNICAP_FLAGS_ONE_PUSH, 
	 flags_mask: UNICAP_FLAGS_ONE_PUSH, 
	 property_data: NULL, 
	 property_data_size: 0 
      }
   },
   {
      xu_ctrl_info: {
	       UVC_GUID_TISUVC_XU, 
	       index: 3, 
	       selector: XU_AUTO_EXPOSURE_REF,
	       size: 4,
	       flags: UVC_CONTROL_GET_CUR | UVC_CONTROL_SET_CUR 
      },       
      CTRL_TYPE_RANGE, 
      {range: { 0, 255 }},
      def: 128,
      has_property: 1,
      property:{
	 identifier: N_("auto exposure reference"), 
	 category: N_("exposure"), 
	 unit: "", 
	 relations: NULL, 
	 relations_count: 0, 
	 {value: 128}, 
	 {range: { min: 0, max: 255 } }, 
	 stepping: 1.0, 
	 type: UNICAP_PROPERTY_TYPE_RANGE, 
	 flags: UNICAP_FLAGS_MANUAL, 
	 flags_mask: UNICAP_FLAGS_MANUAL, 
	 property_data: NULL, 
	 property_data_size: 0 
      }
   }, 
   {
      xu_ctrl_info: {
	       UVC_GUID_TISUVC_XU, 
	       index: 4, 
	       selector: XU_AUTO_SHUTTER_MAX,
	       size: 4,
	       flags: UVC_CONTROL_GET_CUR | UVC_CONTROL_SET_CUR 
      },       
      CTRL_TYPE_RANGE, 
      {range: { 1, 5000 }}, 
      def: 333,
      has_property: 1,
      property:{
	 identifier: N_("auto shutter maximum"), 
	 category: N_("exposure"), 
	 unit: N_("s"), 
	 relations: NULL, 
	 relations_count: 0, 
	 {value: 0.03333}, 
	 {range: { min: 0.0001, max: 0.5 } }, 
	 stepping: 0.0001, 
	 type: UNICAP_PROPERTY_TYPE_RANGE, 
	 flags: UNICAP_FLAGS_MANUAL | UNICAP_FLAGS_AUTO, 
	 flags_mask: UNICAP_FLAGS_MANUAL | UNICAP_FLAGS_AUTO, 
	 property_data: NULL, 
	 property_data_size: 0 
      }
   },
   {
      xu_ctrl_info: {
	       UVC_GUID_TISUVC_XU,
	       index: 5,
	       selector: XU_TRIGGER,
	       size: 1,
	       flags: UVC_CONTROL_GET_CUR | UVC_CONTROL_SET_CUR
      },
      CTRL_TYPE_BITMAP,
      {bitmap_mask: 0x3},
      def: 0x0,
      has_property: 1,
      property:{
	 identifier: N_("trigger"),
	 category: N_("device"),
	 unit: "",
	 relations: NULL,
	 relations_count: 0,
	 {menu_item: TRIGGER_FREE_RUNNING},
	 {menu: { menu_items: trigger_menu, 
		  menu_item_count: sizeof( trigger_menu ) / sizeof( char * ) } },
	 stepping: 0.0,
	 type: UNICAP_PROPERTY_TYPE_MENU,
	 flags: UNICAP_FLAGS_MANUAL,
	 flags_mask: UNICAP_FLAGS_MANUAL,
	 property_data: NULL,
	 property_data_size: 0
      }
   },
   {
      xu_ctrl_info: {
	       UVC_GUID_TISUVC_XU, 
	       index: 6, 
	       selector: XU_AVG_FRAMES,
	       size: 4,
	       flags: UVC_CONTROL_GET_CUR | UVC_CONTROL_SET_CUR 
      },       
      CTRL_TYPE_RANGE, 
      {range: { 1, 255 }},
      def: 1,
      has_property: 1,
      property:{
	 identifier: N_("auto exposure average frames"), 
	 category: N_("exposure"), 
	 unit: "", 
	 relations: NULL, 
	 relations_count: 0, 
	 {value: 1.0}, 
	 {range: { min: 1.0, max: 255 } }, 
	 stepping: 1.0, 
	 type: UNICAP_PROPERTY_TYPE_RANGE, 
	 flags: UNICAP_FLAGS_MANUAL, 
	 flags_mask: UNICAP_FLAGS_MANUAL, 
	 property_data: NULL, 
	 property_data_size: 0 
      }
   }, 
};

static struct ppty_info TISUVCPropertyOverrides[] =
{
   {
      "shutter", 
      tisuvccam_set_shutter, 
      tisuvccam_get_shutter, 
   },
   {
      "gain", 
      tisuvccam_set_gain, 
      tisuvccam_get_gain, 
   }, 
   { 
      "white balance mode", 
      tisuvccam_set_wb_auto, 
      tisuvccam_get_wb_auto
   }
};

static struct uvc_format TISUVCFormats[] =
{
   {
      type: V4L2_BUF_TYPE_VIDEO_CAPTURE, 
      bpp: 8, 
      colorspace: 0, 
      fcc: FOURCC( 'Y', '8', '0', '0' ), 
      flags: 0,
      name: "30303859-0000-0010-8000-00aa003", 
   }
};
      
static void tisuvccam_add_controls( int fd )
{
   int i; 
   int n = sizeof( TISUVCCtrlInfo ) / sizeof( struct ctrlinfo );
   struct uvc_xu_control_info extinfos[] =
      {
	 {
	    UVC_GUID_TISUVC_XU, 
	    index: 7, 
	    selector: XU_AUTO_SHUTTER_MAX_AUTO, 
	    size: 1, 
	    flags: UVC_CONTROL_GET_CUR | UVC_CONTROL_SET_CUR
	 }
	 
      };

   for( i = 0; i < n; i++ )
   {
      if( IOCTL( fd, UVCIOC_CTRL_ADD, &TISUVCCtrlInfo[i].xu_ctrl_info ) < 0 )
      {
	 TRACE( "Failed to add info for control: %d\n", i );
      }
   }

   n = sizeof( extinfos ) / sizeof( struct uvc_xu_control_info );
   for( i = 0; i < n; i++ )
   {
      if( IOCTL( fd, UVCIOC_CTRL_ADD, &extinfos[i] ) < 0 )
      {
	 TRACE( "Failed to add info for control: %d\n", i );
      }
   }
}

static void tisuvccam_add_formats( int fd )
{
/*    int i; */
/*    int n = sizeof( TISUVCFormats ) / sizeof( struct uvc_format ); */
   
/*    for( i = 0; i < n; i++ ) */
/*    { */
/*       if( ioctl( fd, UVCIOC_FMT_SET, &TISUVCFormats[i] ) < 0 ) */
/*       { */
/* 	 TRACE( "Failed to set format info for %d\n", i ); */
/*       } */
/*    } */
}



int tisuvccam_probe( v4l2_handle_t handle, const char *path )
{
   __u8 data;
   int ret = 0;

   struct uvc_xu_control xuctrl = 
      {
	 unit: 6,
	 selector: XU_AUTO_SHUTTER,
	 size: 1,
	 data: &data,
      };   
	
   tisuvccam_add_controls( handle->fd );
 
   if( IOCTL( handle->fd, UVCIOC_CTRL_GET, &xuctrl ) < 0 )
   {
      TRACE( "tisuvccam_probe: not a TIS UVC device\n" );
   }
   else
   {
      TRACE( "tisuvccam_probe: found TIS UVC device\n" );
      ret = 1;
   }
   
   return ret;
}

int tisuvccam_count_ext_property( v4l2_handle_t handle )
{
   int i;
   int cnt = 0;
   int n = sizeof( TISUVCCtrlInfo ) / sizeof( struct ctrlinfo );
   
   for( i = 0; i < n; ++i )
   {
      __u32 data;
      
      if( TISUVCCtrlInfo[i].has_property )
      {
	 struct uvc_xu_control xuctrl;
	 xuctrl.unit = 6;
	 xuctrl.selector = TISUVCCtrlInfo[i].xu_ctrl_info.selector;
	 xuctrl.size = TISUVCCtrlInfo[i].xu_ctrl_info.size;
	 xuctrl.data = (__u8*)&data;
	 
	 if( IOCTL( handle->fd, UVCIOC_CTRL_GET, &xuctrl ) >= 0 )
	 {
	    TRACE( "tisuvccam_count: found property - selector = %d\n", TISUVCCtrlInfo[i].xu_ctrl_info.selector );
	    cnt++;
	 }
      }
   }

   return cnt;
}

unicap_status_t tisuvccam_override_property( v4l2_handle_t handle, struct v4l2_queryctrl *ctrl, unicap_property_t *property )
{
   unicap_status_t status = STATUS_NO_MATCH;

   if( !ctrl )
   {
      return STATUS_NO_MATCH;
   }
   
   switch( ctrl->id )
   {
      case V4L2_CID_EXPOSURE_ABSOLUTE:
      {
	 if( property )
	 {
	    strcpy( property->identifier, N_("shutter") );
	    strcpy( property->category, N_("exposure") );
	    strcpy( property->unit, N_("s") );
	    property->flags_mask = UNICAP_FLAGS_MANUAL | UNICAP_FLAGS_AUTO | UNICAP_FLAGS_READ_OUT;
	    property->flags = UNICAP_FLAGS_AUTO;
	    property->type = UNICAP_PROPERTY_TYPE_RANGE;
	    property->range.min = (double)ctrl->minimum / 10000.0;
	    property->range.max = (double)ctrl->maximum / 10000.0;
	    property->value = (double)ctrl->default_value / 10000.0;
	 }


	 status = STATUS_SUCCESS;
      }
      break;

      case V4L2_CID_GAIN:
      {
	 if( property )
	 {
	    strcpy( property->identifier, N_("gain") );
	    strcpy( property->category, N_("exposure") );
	    strcpy( property->unit, "" );

	    property->flags_mask = UNICAP_FLAGS_MANUAL | UNICAP_FLAGS_AUTO | UNICAP_FLAGS_READ_OUT;
	    property->flags = UNICAP_FLAGS_AUTO;
	    property->type = UNICAP_PROPERTY_TYPE_RANGE;
	    property->range.min = (double)ctrl->minimum;
	    property->range.max = (double)ctrl->maximum;
	    property->value = (double)ctrl->default_value;
	 }
	 status = STATUS_SUCCESS;
      }
      break;

      case V4L2_CID_AUTO_WHITE_BALANCE:
      {
	 if( property )
	 {
	    unicap_void_property( property );
	    strcpy( property->identifier, N_("white balance mode") );
	    strcpy( property->category, N_("color") );
	    
	    property->flags_mask = UNICAP_FLAGS_MANUAL | UNICAP_FLAGS_AUTO | UNICAP_FLAGS_READ_OUT;
	    property->flags = UNICAP_FLAGS_AUTO;
	    property->type = UNICAP_PROPERTY_TYPE_FLAGS;
	 }
	 status = STATUS_SUCCESS;
      }
      break;
      
      case V4L2_CID_EXPOSURE_AUTO:
      case V4L2_CID_AUTO_GAIN:
      case V4L2_CID_AUTO_SHUTTER:
      case V4L2_CID_WHITE_BALANCE_ONE_PUSH:
      case V4L2_CID_AUTO_SHUTTER_MAX:
      case V4L2_CID_AUTO_SHUTTER_MAX_AUTO:
      case V4L2_CID_TRIGGER:
      {
	 TRACE( "Skip control %x: '%s'\n", ctrl->id, ctrl->name );
	 status = STATUS_SKIP_CTRL;
      }
      break;

      default:
	 break;
   }
   
   if( SUCCESS( status ) )
   {
      TRACE( "Using override for property '%s'\n", property->identifier );
   }


   return status;
}

unicap_status_t tisuvccam_enumerate_properties( v4l2_handle_t handle, int index, unicap_property_t *property )
{
   int i;
   int n;
   int tmp_index = -1;
   unicap_status_t status = STATUS_NO_MATCH;
   
   n = sizeof( TISUVCCtrlInfo ) / sizeof( struct ctrlinfo );

   for( i = 0; ( i < n ) && ( tmp_index < index ); ++i )
   {
      __u32 data;
      
      struct uvc_xu_control xuctrl;
      xuctrl.unit = 6;
      xuctrl.selector = TISUVCCtrlInfo[i].xu_ctrl_info.selector;
      xuctrl.size = TISUVCCtrlInfo[i].xu_ctrl_info.size;
      xuctrl.data = (__u8*)&data;

      if( TISUVCCtrlInfo[i].has_property )
      {
	 if( IOCTL( handle->fd, UVCIOC_CTRL_GET, &xuctrl ) >= 0 )
	 {
	    tmp_index++;
	    if( tmp_index == index )
	    {
	       TRACE( "UVCIOC_CTRL_GET: Found control %d\n", xuctrl.selector );
	       unicap_copy_property( property, &TISUVCCtrlInfo[i].property );
	       status = STATUS_SUCCESS;
	       break;
	    }
	 }
	 else
	 {
	    TRACE( "UVCIOC_CTRL_GET failed for selector: %d\n", xuctrl.selector );
	 }
      }
   }
   
   return status;
}

unicap_status_t tisuvccam_set_property( v4l2_handle_t handle, unicap_property_t *property )
{
   int n;
   int i;
   int index = -1;
   unicap_status_t status = STATUS_FAILURE;
   struct uvc_xu_control xuctrl;
   __u32 data;
   
   n = sizeof( TISUVCPropertyOverrides ) / sizeof( struct ppty_info );
   
   for( i = 0; i < n; ++i )
   {
      if( !strcmp( property->identifier, TISUVCPropertyOverrides[i].identifier ) )
      {
	 return TISUVCPropertyOverrides[i].set_func( handle->fd, property );
      }
   }

   n = sizeof( TISUVCCtrlInfo)  / sizeof( struct ctrlinfo );

   for( i = 0; i < n; ++i )
   {
      if( !strcmp( property->identifier, TISUVCCtrlInfo[i].property.identifier ) )
      {
	 index = i;
	 break;
      }
   }
   
   if( index < 0 ) 
   {
      return STATUS_NO_MATCH;
   }
   
   xuctrl.unit = 6;
   xuctrl.selector = TISUVCCtrlInfo[index].xu_ctrl_info.selector;
   xuctrl.size = TISUVCCtrlInfo[index].xu_ctrl_info.size;
   xuctrl.data = (__u8*)&data;


   if( IOCTL( handle->fd, UVCIOC_CTRL_GET, &xuctrl ) < 0 )
   {
      return STATUS_NO_MATCH;
   }

   switch( TISUVCCtrlInfo[index].xu_ctrl_info.selector )
   {
      case XU_AUTO_SHUTTER_MAX:
      {
	 struct uvc_xu_control autoctrl;
	 __u8 u8data;
	 
	 /* u8data = ( property->flags & UNICAP_FLAGS_AUTO ) ? 1 : 0; */
	    
	 /* autoctrl.unit = 6; */
	 /* autoctrl.selector = XU_AUTO_SHUTTER_MAX_AUTO; */
	 /* autoctrl.size = 1; */
	 /* autoctrl.data = &u8data; */
	    
	 /* if( IOCTL( handle->fd, UVCIOC_CTRL_SET, &autoctrl ) < 0 ) */
	 /* { */
	 /*    TRACE( "failed to set XU_AUTO_SHUTTER_MAX_AUTO!\n" ); */
	 /*    status = STATUS_FAILURE; */
	 /*    break; */
	 /* } */
	 
	 /* data = property->value * 10000; */
	 data = property->value;
	 
	 if( IOCTL( handle->fd, UVCIOC_CTRL_SET, &xuctrl ) < 0 )
	 {
	    status = STATUS_FAILURE;
	    break;
	 }
	 status = STATUS_SUCCESS;
      }
      break;
      
      case XU_AUTO_SHUTTER:
      case XU_AUTO_GAIN:
      {
	 __u8 u8data;
	 
	 u8data = ( property->flags & UNICAP_FLAGS_AUTO ) ? 1 : 0;
	 xuctrl.data = &u8data;
	 
	 if( IOCTL( handle->fd, UVCIOC_CTRL_SET, &xuctrl ) < 0 )
	 {
	    status = STATUS_FAILURE;
	    break;
	 }
	 
	 status = STATUS_SUCCESS;
      }
      break;
      
      case XU_ONE_PUSH_WB:
      {
	 __u8 u8data;
	 
	 u8data = ( property->flags & UNICAP_FLAGS_ONE_PUSH ) ? 1 : 0;
	 xuctrl.data = &u8data;
	 
	 if( IOCTL( handle->fd, UVCIOC_CTRL_SET, &xuctrl ) < 0 )
	 {
	    status = STATUS_FAILURE;
	    break;
	 }
	 
	 status = STATUS_SUCCESS;
      }
      break;

      case XU_AUTO_EXPOSURE_REF:
      case XU_AVG_FRAMES:
      {
	 data = property->value;
	 
	 if( IOCTL( handle->fd, UVCIOC_CTRL_SET, &xuctrl ) < 0 )
	 {
	    status = STATUS_FAILURE;
	    break;
	 }
	 
	 status = STATUS_SUCCESS;
      }
      break;

      case XU_TRIGGER:
      {
	 __u8 u8data;
	 
	 if( !strcmp( property->menu_item, TRIGGER_FREE_RUNNING ) )
	 {
	    u8data = 0;
	 }
	 else if( !strcmp( property->menu_item, TRIGGER_FALLING_EDGE ) )
	 {
	    u8data = 0x1;
	 }
	 else if( !strcmp( property->menu_item, TRIGGER_RISING_EDGE ) )
	 {
	    u8data = 0x3;
	 }
	 
	 xuctrl.data = &u8data;
	 
	 if( IOCTL( handle->fd, UVCIOC_CTRL_SET, &xuctrl ) < 0 )
	 {
	    status = STATUS_FAILURE;
	    break;
	 }
	 
	 status = STATUS_SUCCESS;
      }
      break;
      
      default:
      {
	 TRACE( "Unknown/Unhandled unit!\n");
	 status = STATUS_NO_MATCH;
      }
      break;
   }
   
   if( !SUCCESS( status ) )
   {
      TRACE( "Failed to set property: %s\n", property->identifier );
   }
   
   return status;
}

unicap_status_t tisuvccam_get_property( v4l2_handle_t handle, unicap_property_t *property )
{
   int n;
   int i;
   int index = -1;
   unicap_status_t status = STATUS_FAILURE;
   struct uvc_xu_control xuctrl;
   __u32 data;

   n = sizeof( TISUVCPropertyOverrides ) / sizeof( struct ppty_info );
   
   for( i = 0; i < n; ++i )
   {
      if( !strcmp( property->identifier, TISUVCPropertyOverrides[i].identifier ) )
      {
	 return TISUVCPropertyOverrides[i].get_func( handle->fd, property );
      }
   }

   n = sizeof( TISUVCCtrlInfo ) / sizeof( struct ctrlinfo );
   
   for( i = 0; i < n; ++i )
   {
      if( !strcmp( property->identifier, TISUVCCtrlInfo[i].property.identifier ) )
      {
	 index = i;
	 break;
      }
   }
   
   if( index < 0 ) 
   {
      return STATUS_NO_MATCH;
   }
   
   xuctrl.unit = 6;
   xuctrl.selector = TISUVCCtrlInfo[index].xu_ctrl_info.selector;
   xuctrl.size = TISUVCCtrlInfo[index].xu_ctrl_info.size;
   xuctrl.data = (__u8*)&data;

   if( IOCTL( handle->fd, UVCIOC_CTRL_GET, &xuctrl ) < 0 )
   {
      return STATUS_NO_MATCH;
   }

   unicap_copy_property( property, &TISUVCCtrlInfo[index].property );

   switch( TISUVCCtrlInfo[index].xu_ctrl_info.selector )
   {
      case XU_AUTO_SHUTTER_MAX:
      {
	 struct uvc_xu_control autoctrl;
	 __u8 u8data;
	 
	 autoctrl.unit = 6;
	 autoctrl.selector = XU_AUTO_SHUTTER_MAX_AUTO;
	 autoctrl.size = 1;
	 autoctrl.data = &u8data;
	    
	 if( IOCTL( handle->fd, UVCIOC_CTRL_GET, &autoctrl ) < 0 )
	 {
	    TRACE( "failed to get XU_AUTO_SHUTTER_MAX_AUTO!\n" );
	    status = STATUS_FAILURE;
	    break;
	 }

	 property->flags = u8data ? UNICAP_FLAGS_AUTO : UNICAP_FLAGS_MANUAL;
	 
	 if( IOCTL( handle->fd, UVCIOC_CTRL_GET, &xuctrl ) < 0 )
	 {
	    status = STATUS_FAILURE;
	    break;
	 }
	 
	 property->value = (double)data / 10000.0;
	 status = STATUS_SUCCESS;
      }
      break;
      
      case XU_AUTO_SHUTTER:
      case XU_AUTO_GAIN:
      {
	 __u8 u8data;
	 xuctrl.data = &u8data;
	 
	 if( IOCTL( handle->fd, UVCIOC_CTRL_GET, &xuctrl ) < 0 )
	 {
	    status = STATUS_FAILURE;
	    break;
	 }
	 
	 property->flags = u8data ? UNICAP_FLAGS_AUTO : UNICAP_FLAGS_MANUAL;
	 status = STATUS_SUCCESS;
      }
      break;
      
      case XU_ONE_PUSH_WB:
      {
	 __u8 u8data;
	 xuctrl.data = &u8data;
	 
	 if( IOCTL( handle->fd, UVCIOC_CTRL_GET, &xuctrl ) < 0 )
	 {
	    status = STATUS_FAILURE;
	    break;
	 }
	 
	 property->flags = u8data ? UNICAP_FLAGS_ONE_PUSH : UNICAP_FLAGS_MANUAL;
	 status = STATUS_SUCCESS;
      }
      break;
      
      case XU_AUTO_EXPOSURE_REF:
      case XU_AVG_FRAMES:
      {
	 if( IOCTL( handle->fd, UVCIOC_CTRL_GET, &xuctrl ) < 0 )
	 {
	    status = STATUS_FAILURE;
	    break;
	 }
	 
	 property->value = data;
	 status = STATUS_SUCCESS;
      }
      break;

      case XU_TRIGGER:
      {
	 __u8 u8data;
	 xuctrl.data = &u8data;
	 if( IOCTL( handle->fd, UVCIOC_CTRL_GET, &xuctrl ) < 0 )
	 {
	    status = STATUS_FAILURE;
	    break;
	 }
	 
	 status = STATUS_SUCCESS;

	 switch( data & 0x3 )
	 {
	    case 0x0:
	    case 0x2:
	       strcpy( property->menu_item, TRIGGER_FREE_RUNNING );
	       break;
	       
	    case 0x1:
	       strcpy( property->menu_item, TRIGGER_FALLING_EDGE );
	       break;
	       
	    case 0x3:
	       strcpy( property->menu_item, TRIGGER_RISING_EDGE );
	       break;

	    default:
	       break;
	 }
      }
      break;       

      default:
      {
	 TRACE( "Unknown/Unhandled unit!\n");
	 status = STATUS_NO_MATCH;
      }
      break;
   }
   
      
   if( !SUCCESS( status ) )
   {
      TRACE( "Failed to get property: %s\n", property->identifier );
   }
   
   return status;
}

unicap_status_t tisuvccam_fmt_get( struct v4l2_fmtdesc *v4l2fmt, struct v4l2_cropcap *cropcap, char **identifier, unsigned int *fourcc, int *bpp )
{
   unicap_status_t status = STATUS_NO_MATCH;

   if( !strcmp( (char*)v4l2fmt->description, "30303859-0000-0010-8000-00aa003" ) )
   {
      if( identifier )
      {
	 *identifier = "Y800";
      }
/*       if( fourcc ) */
/*       { */
/* 	 *fourcc = FOURCC( 'Y', '8', '0', '0' ); */
/*       } */
      if( bpp )
      {
	 *bpp = 8;
      }
      
      status = STATUS_SUCCESS;
   }
   else if( !strcmp( (char*)v4l2fmt->description, "20385942-0000-0010-8000-00aa003" ) )
   {
      // BY8 
      if( identifier )
      {
	 *identifier = "8-Bit Bayer RAW";
      }
/*       if( fourcc ) */
/*       { */
/* 	 *fourcc = FOURCC( 'B', 'Y', '8', ' ' ); */
/*       } */
      if( bpp )
      {
	 *bpp = 8;
      }
      
      status = STATUS_SUCCESS;
   }
      
   return status;
}


static unicap_status_t tisuvccam_set_shutter( int fd, unicap_property_t *property )
{
   struct uvc_xu_control xuctrl;
   __u8 u8data;
   
   xuctrl.unit = 6;
   xuctrl.selector = XU_AUTO_SHUTTER;
   xuctrl.size = 1;
   u8data = ( property->flags & UNICAP_FLAGS_AUTO ) ? 1 : 0;
   xuctrl.data = &u8data;
   
   if( IOCTL( fd, UVCIOC_CTRL_SET, &xuctrl ) < 0 )
   {
      TRACE( "UVCIOC_CTRL_SET failed!\n" );
      return STATUS_FAILURE;
   }
   
   if( property->flags & UNICAP_FLAGS_MANUAL )
   {
      struct v4l2_control ctrl;

      ctrl.id = V4L2_CID_EXPOSURE_ABSOLUTE;
      ctrl.value = property->value * 10000;

      if( IOCTL( fd, VIDIOC_S_CTRL, &ctrl ) < 0 )
      {
	 TRACE( "VIDIOC_S_CTRL failed\n" );
	 return STATUS_FAILURE;
      }
   }
      
   return STATUS_SUCCESS;
}

static unicap_status_t tisuvccam_get_shutter( int fd, unicap_property_t *property )
{
   struct uvc_xu_control xuctrl;
   struct v4l2_control ctrl;
   __u8 u8data;
   
   xuctrl.unit = 6;
   xuctrl.selector = XU_AUTO_SHUTTER;
   xuctrl.size = 1;
   xuctrl.data = &u8data;

   if( IOCTL( fd, UVCIOC_CTRL_GET, &xuctrl ) < 0 )
   {
      TRACE( "UVCIOC_CTRL_GET failed!\n" );
      return STATUS_FAILURE;
   }
   
   property->flags = u8data ? UNICAP_FLAGS_AUTO : UNICAP_FLAGS_MANUAL;
   
   ctrl.id = V4L2_CID_EXPOSURE_ABSOLUTE;
   
   if( IOCTL( fd, VIDIOC_G_CTRL, &ctrl ) < 0 )
   {
      TRACE( "VIDIOC_G_CTRL failed!\n" );
      return STATUS_FAILURE;
   }
   
   property->value = (double)ctrl.value / 10000.0;
   
   return STATUS_SUCCESS;
}

static unicap_status_t tisuvccam_set_gain( int fd, unicap_property_t *property )
{
   struct uvc_xu_control xuctrl;
   __u8 u8data;
   
   xuctrl.unit = 6;
   xuctrl.selector = XU_AUTO_GAIN;
   xuctrl.size = 1;
   u8data = ( property->flags & UNICAP_FLAGS_AUTO ) ? 1 : 0;
   xuctrl.data = &u8data;
   
   if( IOCTL( fd, UVCIOC_CTRL_SET, &xuctrl ) < 0 )
   {
      TRACE( "UVCIOC_CTRL_SET failed!\n" );
      return STATUS_FAILURE;
   }
   
   if( property->flags & UNICAP_FLAGS_MANUAL )
   {
      struct v4l2_control ctrl;

      ctrl.id = V4L2_CID_GAIN;
      ctrl.value = property->value;

      if( IOCTL( fd, VIDIOC_S_CTRL, &ctrl ) < 0 )
      {
	 TRACE( "VIDIOC_S_CTRL failed\n" );
	 return STATUS_FAILURE;
      }
   }
      
   return STATUS_SUCCESS;
}

static unicap_status_t tisuvccam_get_gain( int fd, unicap_property_t *property )
{
   struct uvc_xu_control xuctrl;
   struct v4l2_control ctrl;
   __u8 u8data;
   
   xuctrl.unit = 6;
   xuctrl.selector = XU_AUTO_GAIN;
   xuctrl.size = 1;
   xuctrl.data = &u8data;

   if( IOCTL( fd, UVCIOC_CTRL_GET, &xuctrl ) < 0 )
   {
      TRACE( "UVCIOC_CTRL_GET failed!\n" );
      return STATUS_FAILURE;
   }
   
   property->flags = u8data ? UNICAP_FLAGS_AUTO : UNICAP_FLAGS_MANUAL;
   
   ctrl.id = V4L2_CID_GAIN;
   
   if( IOCTL( fd, VIDIOC_G_CTRL, &ctrl ) < 0 )
   {
      TRACE( "VIDIOC_G_CTRL failed!\n" );
      return STATUS_FAILURE;
   }
   
   property->value = (double)ctrl.value;
   
   return STATUS_SUCCESS;
}

static unicap_status_t tisuvccam_set_wb_auto( int fd, unicap_property_t *property )
{
   struct v4l2_control ctrl;
   unicap_status_t status = STATUS_SUCCESS;

   ctrl.id = V4L2_CID_AUTO_WHITE_BALANCE;   
   ctrl.value = ( property->flags & UNICAP_FLAGS_AUTO ) ? 1 : 0;
   
   if( IOCTL( fd, VIDIOC_S_CTRL, &ctrl ) < 0 )
   {
      TRACE( "VIDIOC_S_CTRL failed!\n" );
      status = STATUS_FAILURE;
   }
   
   return status;
}

static unicap_status_t tisuvccam_get_wb_auto( int fd, unicap_property_t *property )
{
   struct v4l2_control ctrl;
   unicap_status_t status = STATUS_SUCCESS;
   
   ctrl.id = V4L2_CID_AUTO_WHITE_BALANCE;
   
   if( IOCTL( fd, VIDIOC_G_CTRL, &ctrl ) < 0 )
   {
      TRACE( "VIDIOC_G_CTRL failed!\n" );
      status = STATUS_FAILURE;
   }

   if( ctrl.value ){
      property->flags = UNICAP_FLAGS_AUTO;
   }else{
      property->flags = UNICAP_FLAGS_MANUAL;
   }

   return status;
}
