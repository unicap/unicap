#ifndef __UCIL_PPM_H__
#define __UCIL_PPM_H__

#include "ucil_private.h"
#include <unicap.h>

__HIDDEN__ unicap_data_buffer_t *ucil_ppm_read_file( const char *path );
__HIDDEN__ unicap_status_t ucil_ppm_write_file( unicap_data_buffer_t *databuffer, const char *path );


#endif//__UCIL_PPM_H__
