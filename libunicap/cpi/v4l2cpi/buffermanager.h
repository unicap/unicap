#ifndef __BUFERMGR_H__
#define __BUFERMGR_H__

#include <linux/types.h>
#include <linux/videodev2.h>
#include <semaphore.h>

#include <unicap.h>

typedef struct buffer_mgr *buffer_mgr_t;

buffer_mgr_t buffer_mgr_create( int fd, unicap_format_t *format );
void buffer_mgr_destroy( buffer_mgr_t mgr );
unicap_status_t buffer_mgr_queue_all( buffer_mgr_t mgr );
unicap_status_t buffer_mgr_queue( buffer_mgr_t mgr, unicap_data_buffer_t *buffer );
unicap_status_t buffer_mgr_dequeue( buffer_mgr_t mgr, unicap_data_buffer_t **buffer );
unicap_status_t buffer_mgr_dequeue_all (buffer_mgr_t mgr );



#endif//__BUFERMGR_H__
