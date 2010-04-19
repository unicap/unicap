#ifndef __UCIL_PRIVATE_H__
#define __UCIL_PRIVATE_H__

#ifndef __HIDDEN__
#define __HIDDEN__ __attribute__((visibility("hidden")))
#endif


__HIDDEN__ unicap_data_buffer_t *ucil_allocate_buffer( int width, int height, unsigned int fourcc, int bpp );




#endif//__UCIL_PRIVATE_H__
