/*
  unicap euvccam plugin

  Copyright (C) 2009  Arne Caspari

  Compilation, installation or use of this program requires a written license. 

  By compilling, installing or using this software you agree to accept the terms 
  of the license as specified in the COPYING file in the root folder of this 
  software package.
 */

#ifndef __EUVCCAM_VIDEO_FORMATS_H__
#define __EUVCCAM_VIDEO_FORMATS_H__

#include <unicap.h>


#define EUVCCAM_HAS_AUTO_EXPOSURE   ( 1 << 0 )
#define EUVCCAM_HAS_AUTO_GAIN       ( 1 << 1 )

#define EUVCCAM_FORMAT_IS_PARTIAL_SCAN ( 1 << 0 )



typedef unicap_status_t(*euvccam_property_func_t)(euvccam_handle_t handle,unicap_property_t *property);
typedef void(*euvccam_convert_func_t)(euvccam_handle_t handle, unicap_data_buffer_t *dest, unicap_data_buffer_t *src );

struct euvccam_video_format_description
{
   int format_index;
   int frame_index;
   unicap_format_t format;
   int frame_rate_count;
   double *frame_rates;
   int    *frame_rate_map;
   int usb_buffer_size;
   euvccam_convert_func_t convert_func;
   int flags;
};

typedef struct euvccam_video_format_description euvccam_video_format_description_t;

struct euvccam_property
{
   unicap_property_t property;
   euvccam_property_func_t get_func;
   euvccam_property_func_t set_func;
   euvccam_property_func_t enumerate_func;
};

typedef struct euvccam_property euvccam_property_t;

struct euvccam_devspec 
{
   unsigned short pid;
   unsigned char type_flag;
   unsigned int flags;
   int format_count;
   struct euvccam_video_format_description *format_list;
   int property_count;
   struct euvccam_property *property_list;
};

extern struct euvccam_devspec euvccam_devspec[];


#endif//__EUVCCAM_VIDEO_FORMATS_H__
