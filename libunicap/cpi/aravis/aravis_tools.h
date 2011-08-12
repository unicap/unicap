#ifndef __ARAVIS_TOOLS_H__
#define __ARAVIS_TOOLS_H__

unsigned int aravis_tools_get_fourcc (ArvPixelFormat fmt);
const char *aravis_tools_get_pixel_format_string (ArvPixelFormat fmt);
int aravis_tools_get_bpp (ArvPixelFormat fmt);
ArvPixelFormat aravis_tools_get_pixel_format (unicap_format_t *format);


#endif//__ARAVIS_TOOLS_H__
