/*
** tisuvccam.h
** 
** Made by Arne Caspari
** Login   <arne@localhost>
** 
** Started on  Tue Jan  8 07:23:05 2008 Arne Caspari
** Last update Tue Jan  8 07:23:05 2008 Arne Caspari
*/

#ifndef   	TISUVCCAM_H_
# define   	TISUVCCAM_H_

#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/videodev2.h>
#include <unicap.h>
#include "v4l2.h"

int tisuvccam_probe( v4l2_handle_t handle, const char *path );
int tisuvccam_count_ext_property( v4l2_handle_t handle );
unicap_status_t tisuvccam_override_property( v4l2_handle_t handle, struct v4l2_queryctrl *ctrl, unicap_property_t *property );
unicap_status_t tisuvccam_enumerate_properties( v4l2_handle_t handle, int index, unicap_property_t *property );
unicap_status_t tisuvccam_set_property( v4l2_handle_t handle, unicap_property_t *property );
unicap_status_t tisuvccam_get_property( v4l2_handle_t handle, unicap_property_t *property );
unicap_status_t tisuvccam_fmt_get( struct v4l2_fmtdesc *v4l2fmt, struct v4l2_cropcap *cropcap, char **identifier, unsigned int *fourcc, int *bpp );

#endif 	    /* !TISUVCCAM_H_ */
