#include "config.h"

#include <linux/types.h>
#include <linux/videodev2.h>
#include <semaphore.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>

#if V4L2_DEBUG
#define DEBUG
#endif
#include "debug.h"

#include "buffermanager.h"

#define MAX_BUFFERS 16

#if USE_LIBV4L
 #define OPEN   v4l2_open
 #define CLOSE  v4l2_close
 #define IOCTL  v4l2_ioctl
 #define MMAP   v4l2_mmap
 #define MUNMAP v4l2_munmap
#else
 #define OPEN   open
 #define CLOSE  close
 #define IOCTL  ioctl
 #define MMAP   mmap
 #define MUNMAP munmap
#endif

struct v4l2cpi_buffer
{
   struct v4l2_buffer v4l2_buffer;
   unicap_data_buffer_t data_buffer;
   int refcount;
   int queued;
   void *start;
   size_t length;
};

typedef struct v4l2cpi_buffer v4l2cpi_buffer_t;

struct buffer_mgr
{
   v4l2cpi_buffer_t buffers[ MAX_BUFFERS ];
   int free_buffers;
   int num_buffers;
   sem_t lock;
   int fd;
};


#define BUFFER_MGR_LOCK(mgr) { sem_wait( &mgr->lock ); }

#define BUFFER_MGR_UNLOCK(mgr) { sem_post( &mgr->lock); }

static v4l2cpi_buffer_t *buffer_mgr_get_cpi_buffer( buffer_mgr_t mgr, unicap_data_buffer_t *buffer );


static void v4l2_data_buffer_unref( unicap_data_buffer_t *buffer, buffer_mgr_t mgr )
{
   if (unicap_data_buffer_get_refcount( buffer ) == 1 ){
      buffer_mgr_queue (mgr, buffer);
   }
}

buffer_mgr_t buffer_mgr_create( int fd, unicap_format_t *format )
{
   buffer_mgr_t mgr = malloc( sizeof( struct buffer_mgr ) );
   int i;
   
   unicap_data_buffer_init_data_t init_data = { NULL, NULL, NULL, NULL, (unicap_data_buffer_func_t)v4l2_data_buffer_unref, mgr };
   
   memset( mgr, 0x0, sizeof( buffer_mgr_t ) );
   
   if( sem_init( &mgr->lock, 0, 1 ) ){
      TRACE( "sem_init failed\n" );
      free( mgr );
      return NULL;
   }

   mgr->fd = fd;
   
   struct v4l2_requestbuffers v4l2_reqbuf;
   memset( &v4l2_reqbuf, 0x0, sizeof( struct v4l2_requestbuffers ) );
   v4l2_reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
   v4l2_reqbuf.memory = V4L2_MEMORY_MMAP;
   v4l2_reqbuf.count = MAX_BUFFERS;

   if( IOCTL( fd, VIDIOC_REQBUFS, &v4l2_reqbuf ) < 0 )
   {
      TRACE( "VIDIOC_REQBUFS failed: %s\n", strerror( errno ) );
      return NULL;
   }

   mgr->num_buffers = v4l2_reqbuf.count;
   
   for( i = 0; i < v4l2_reqbuf.count; i++ ){
      memset( &mgr->buffers[i].v4l2_buffer, 0x0, sizeof( struct v4l2_buffer ) );
      unicap_data_buffer_init (&mgr->buffers[i].data_buffer, format, &init_data);
      unicap_data_buffer_ref (&mgr->buffers[i].data_buffer);

      mgr->buffers[i].v4l2_buffer.type = v4l2_reqbuf.type;
      mgr->buffers[i].v4l2_buffer.memory = V4L2_MEMORY_MMAP;
      mgr->buffers[i].v4l2_buffer.index = i;
      
      if( IOCTL( mgr->fd, VIDIOC_QUERYBUF, &mgr->buffers[i].v4l2_buffer ) < 0 ){
	 TRACE( "VIDIOC_QUERYBUF ioctl failed: %s, index = %d\n", strerror( errno ), i );
	 // TODO: Cleanup
	 return NULL;
      }
      
      
      mgr->buffers[i].length = mgr->buffers[i].v4l2_buffer.length;
      mgr->buffers[i].start = MMAP( NULL, 
				    mgr->buffers[i].length, 
				    PROT_READ | PROT_WRITE, 
				    MAP_SHARED, 
				    fd, 
				    mgr->buffers[i].v4l2_buffer.m.offset );
      if( mgr->buffers[i].start == MAP_FAILED ){
	 TRACE( "MMAP Failed: %s, index = %d\n", strerror( errno ), i );
	 // TODO: Cleanup
	 return NULL;
      }

      mgr->buffers[i].data_buffer.buffer_size = mgr->buffers[i].v4l2_buffer.length;
      mgr->buffers[i].data_buffer.data = mgr->buffers[i].start;

      mgr->free_buffers++;
   }

   return mgr;
}


void buffer_mgr_destroy( buffer_mgr_t mgr ) 
{
   int i;

   BUFFER_MGR_LOCK( mgr );
   
   for (i = 0; i < mgr->num_buffers; i++ ){
      MUNMAP( mgr->buffers[i].start, mgr->buffers[i].length );
   }
   
   struct v4l2_requestbuffers v4l2_reqbuf;
   memset( &v4l2_reqbuf, 0x0, sizeof( struct v4l2_requestbuffers ) );
   v4l2_reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
   v4l2_reqbuf.memory = V4L2_MEMORY_MMAP;
   v4l2_reqbuf.count = 0;

   if( IOCTL( mgr->fd, VIDIOC_REQBUFS, &v4l2_reqbuf ) < 0 )
   {
      TRACE( "VIDIOC_REQBUFS failed: %s\n", strerror( errno ) );
   }

   sem_destroy (&mgr->lock); 
   
   free (mgr);
}


static v4l2cpi_buffer_t *buffer_mgr_get_cpi_buffer( buffer_mgr_t mgr, unicap_data_buffer_t *buffer )
{
   int i;
   
   for( i = 0; i < mgr->num_buffers; i++ ){
      if (&mgr->buffers[i].data_buffer == buffer){
	 return &mgr->buffers[i];
      }
   }

   return NULL;
}

   

unicap_status_t buffer_mgr_queue_all( buffer_mgr_t mgr )
{
   int i;
   unicap_status_t status = STATUS_SUCCESS;
   
   for( i = 0; i < mgr->num_buffers; i++ ){
      unicap_status_t tmp;
      if( !SUCCESS( tmp = buffer_mgr_queue( mgr, &mgr->buffers[i].data_buffer ) ) ){
	 status = tmp;
      }
   }

   return status;
}

unicap_status_t buffer_mgr_queue( buffer_mgr_t mgr, unicap_data_buffer_t *buffer )
{
   int i;
   unicap_status_t status = STATUS_INVALID_PARAMETER;

   BUFFER_MGR_LOCK( mgr );
   for( i = 0; i < mgr->num_buffers; i++ ){
      if (&mgr->buffers[i].data_buffer == buffer){
	 int ret;
	 if( ( ret = IOCTL( mgr->fd, VIDIOC_QBUF, &mgr->buffers[i].v4l2_buffer ) ) < 0 ){
	    if( ret == -ENODEV ){
	       status = STATUS_NO_DEVICE;
	    }
	    TRACE( "VIDIOC_QBUF ioctl failed: %s\n", strerror( errno ) );
	 } else {
	    status = STATUS_SUCCESS;
	    mgr->buffers[i].queued = 1;
	 }
	 
	 break;
      }
   }
   
   BUFFER_MGR_UNLOCK( mgr );

   return status;
}

unicap_status_t buffer_mgr_dequeue( buffer_mgr_t mgr, unicap_data_buffer_t **buffer )
{
   struct v4l2_buffer v4l2_buffer;
   unicap_status_t status = STATUS_SUCCESS;
   int i;

   *buffer = NULL;

   memset (&v4l2_buffer, 0x0, sizeof (v4l2_buffer));
   v4l2_buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
   v4l2_buffer.memory = V4L2_MEMORY_MMAP;
   
   BUFFER_MGR_LOCK (mgr);
   if( IOCTL( mgr->fd, VIDIOC_DQBUF, &v4l2_buffer ) < 0 ){
      TRACE( "VIDIOC_DQBUF ioctl failed: %s\n", strerror( errno ) );
      status = STATUS_FAILURE;
   } else {
      for (i = 0; i < mgr->num_buffers; i++ ){
	 if (mgr->buffers[i].v4l2_buffer.index == v4l2_buffer.index ){
	    mgr->buffers[i].queued = 0;
	    mgr->buffers[i].data_buffer.buffer_size = v4l2_buffer.bytesused;
	    memcpy( &mgr->buffers[i].data_buffer.fill_time, &v4l2_buffer.timestamp, sizeof( struct timeval ) );
	    *buffer = &mgr->buffers[i].data_buffer;
	    break;
	 }
      }
   }

   if (!*buffer){
      TRACE ("VIDIOC_DQBUF returned a buffer that is not in the pool (%d) !?!?!?", v4l2_buffer.index );
      status = STATUS_FAILURE;
   }
   
   BUFFER_MGR_UNLOCK (mgr);
   
   return status;
}

unicap_status_t buffer_mgr_dequeue_all (buffer_mgr_t mgr )
{
   int i;
   unicap_status_t status = STATUS_SUCCESS;

   BUFFER_MGR_LOCK (mgr);
   for (i=0; i < mgr->num_buffers; i++){
      if (mgr->buffers[i].queued){
	 if( IOCTL( mgr->fd, VIDIOC_DQBUF, &mgr->buffers[i].v4l2_buffer ) < 0 ){
	    TRACE( "VIDIOC_DQBUF ioctl failed: %s\n", strerror( errno ) );
	    status = STATUS_FAILURE;
	 } else {
	    mgr->buffers[i].queued = 0;
	 }
      }
   }
   
   BUFFER_MGR_UNLOCK (mgr);
   
   return status;
}
