/*
    unicap
    Copyright (C) 2004  Arne Caspari

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef __DCAM_CAPTURE_H__
#define __DCAM_CAPTURE_H__


#include "dcam.h"


void *dcam_dma_capture_thread( void *arg );

void *dcam_capture_thread( void * );
enum raw1394_iso_disposition dcam_iso_handler( raw1394handle_t raw1394handle, 
					       unsigned char *data, 
					       unsigned int len, 
					       unsigned char channel,
					       unsigned char tag, 
					       unsigned char sy, 
					       unsigned int cycle, 
					       unsigned int dropped );

unicap_status_t _dcam_dma_setup( dcam_handle_t dcamhandle );
unicap_status_t _dcam_dma_free( dcam_handle_t dcamhandle );
unicap_status_t _dcam_dma_unlisten( dcam_handle_t dcamhandle );

unicap_status_t dcam_dma_wait_buffer( dcam_handle_t dcamhandle );
#endif//__DCAM_CAPTURE_H__
