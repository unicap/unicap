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

#ifndef __NULL_H__
#define __NULL_H__

int cpi_enumerate_devices( unicap_device_t *device, int index );
int cpi_open( void **cpi_data, unicap_device_t *device );
int cpi_close( void *cpi_data );
unicap_status_t cpi_reenumerate_formats( void *cpi_data, int *count );
int cpi_enumerate_formats( void *cpi_data, unicap_format_t *format, int index );
int cpi_set_format( void *cpi_data, unicap_format_t *format );
unicap_status_t cpi_get_format( void *cpi_data, unicap_format_t *format );
unicap_status_t cpi_reenumerate_properties( void *cpi_data, int *count );
unicap_status_t cpi_enumerate_properties( void *cpi_data, unicap_property_t *property, int index );
unicap_status_t cpi_set_property( void *cpi_data, unicap_property_t *property );
unicap_status_t cpi_get_property( void *cpi_data, unicap_property_t *property );
unicap_status_t cpi_capture_start( void *cpi_data );
unicap_status_t cpi_capture_stop( void *cpi_data );
unicap_status_t cpi_queue_buffer( void *cpi_data, unicap_data_buffer_t *buffer );
unicap_status_t cpi_dequeue_buffer( void *cpi_data, unicap_data_buffer_t **buffer );
unicap_status_t cpi_wait_buffer( void *cpi_data, unicap_data_buffer_t **buffer );
unicap_status_t cpi_poll_buffer( void *cpi_data, int *count );


#define FOURCC(a,b,c,d) (unsigned int)((((unsigned int)d)<<24)+(((unsigned int)c)<<16)+(((unsigned int)b)<<8)+a)

#endif//__NULL_H__
