/*
** tiseuvccam.h
** 
** Made by Arne Caspari
*/

#ifndef   	TISEUVCCAM_H_
# define   	TISEUVCCAM_H_

#include <unicap.h>
#include <unicap_cpi.h>
#include <linux/videodev2.h>
#include "v4l2.h"

int tiseuvccam_probe( v4l2_handle_t handle, const char *path );
int tiseuvccam_count_ext_property( v4l2_handle_t handle );
unicap_status_t tiseuvccam_enumerate_properties( v4l2_handle_t handle, int index, unicap_property_t *property );
unicap_status_t tiseuvccam_override_property( v4l2_handle_t handle, struct v4l2_queryctrl *ctrl, unicap_property_t *property );
unicap_status_t tiseuvccam_set_property( v4l2_handle_t handle, unicap_property_t *property );
unicap_status_t tiseuvccam_get_property( v4l2_handle_t handle, unicap_property_t *property );
unicap_status_t tiseuvccam_fmt_get( struct v4l2_fmtdesc *v4l2fmt, struct v4l2_cropcap *cropcap, char **identifier, unsigned int *fourcc, int *bpp );
unicap_status_t tiseuvccam_tov4l2format( v4l2_handle_t handle, unicap_format_t *format );

#ifdef VIDIOC_ENUM_FRAMESIZES
unicap_status_t tiseuvccam_override_framesize( v4l2_handle_t handle, struct v4l2_frmsizeenum *frms );
#endif

#endif 	    /* !TISEUVCCAM_H_ */
