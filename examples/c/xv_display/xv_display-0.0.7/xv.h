#ifndef __XV_H__
#define __XV_H__

#include <linux/types.h>

#include <X11/Xlib.h>
#include <X11/extensions/Xvlib.h>
#include <X11/extensions/XShm.h>

#include <unicap.h>

struct _xv_handle
{
	XvAdaptorInfo *p_adaptor_info;
	int num_adaptors;
	
	unsigned int xv_mode_id;	
	
	XvImage *image;
	int use_shm;
	XShmSegmentInfo shminfo;

	int width;
	int height;

	int window_width;
	int window_height;

	Atom atom_colorkey;
	Atom atom_brightness;
	Atom atom_hue;
	Atom atom_contrast;

	int brightness_min;
	int brightness_max;
	int hue_min;
	int hue_max;
	int contrast_max;
	int contrast_min;
	
	unicap_handle_t unicap_handle;

	unicap_data_buffer_t data_buffers[5];

	Display *display;
	Drawable drawable;
	GC       gc;
	
} xv_handle;

typedef struct _xv_handle *xv_handle_t;

void xv_update_image( xv_handle_t handle, __u8 *image_data, size_t image_data_size, int w, int h );
int xv_init( xv_handle_t handle, Display *dpy, __u32 fourcc, int width, int height, int bpp );
int xv_test_fourcc( Display *dpy, __u32 fourcc );

#endif//__XV_H__
