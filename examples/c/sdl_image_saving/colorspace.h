#ifndef __COLORSPACE_H__
#define __COLORSPACE_H__

#include <sys/types.h>
#include <linux/types.h>

size_t uyvy2rgb24( __u8 *dest, __u8 *source, size_t dest_size, size_t source_size );
size_t uyvy2rgb32( __u8 *dest, __u8 *source, size_t dest_size, size_t source_size );
size_t y4202rgb32( __u8 *dest, __u8 *source, size_t dest_size, size_t source_size, int w, int h );
size_t y8002rgb32( __u8 *dest, __u8 *source, size_t dest_size, size_t source_size );
size_t y8002rgb24( __u8 *dest, __u8 *source, size_t dest_size, size_t source_size );


#endif//__COLORSPACE_H__
