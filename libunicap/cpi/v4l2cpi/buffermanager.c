struct v4l2cpi_buffer
{
   struct v4l2_buffer v4l2_buffer;
   unicap_data_buffer_t data_buffer;
   int refcount;
   int queued;
};


struct buffer_mgr
{
   v4l2cpi_buffer_t buffers[ MAX_BUFFERS ];
   int free_buffers;
   int num_buffers;
   sem_t lock
   int fd;
};

#define BUFFER_MGR_LOCK(mgr) { sem_wait( mgr->lock ); }

#define BUFFER_MGR_UNLOCK(mgr) { sem_post( mgr->lock); }


buffer_mgr_t *buffer_mgr_create( int fd, unicap_format_t *format )
{
   buffer_mgr_t mgr = malloc( sizeof( struct buffer_mgr ) );
   int i;

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
   v4l2_reqbuf.count = MAX_V4L2_BUFFERS;

   if( IOCTL( fd, VIDIOC_REQBUFS, &v4l2_reqbuf ) < 0 )
   {
      TRACE( "VIDIOC_REQBUFS failed: %s\n", strerror( errno ) );
      return NULL;
   }

   mgr->num_buffers = v4l2_reqbuf.count;
   
   for( i = 0; i < v4l2_reqbuf.count; i++ ){
      unicap_copy_format( &mgr->data_buffers[i].format, format );

      struct v4l2_buffer v4l2_buffer;
      memset( &v4l2_buffer, 0x0, sizeof( v4l2_buffer ) );

      v4l2_buffer.type = v4l2_reqbuf.type;
      v4l2_buffer.memory = V4L2_MEMORY_MMAP;
      v4l2_buffer.index = i;
      
      if( IOCTL( handle->fd, VIDIOC_QUERYBUF, &v4l2_buffer ) < 0 ){
	 TRACE( "VIDIOC_QUERYBUF ioctl failed: %s, index = %d\n", strerror( errno ), i );
	 // TODO: Cleanup
	 return NULL;
      }
      
      
      mgr->v4l2_buffers[i].length = length;
      mgr->v4l2_buffers[i].start = MMAP( NULL, 
					  v4l2_buffer.length, 
					  PROT_READ | PROT_WRITE, 
					  MAP_SHARED, 
					  fd, 
					  v4l2_buffer.m.offset );
      if( handle->buffers[i].start == MAP_FAILED ){
	 TRACE( "MMAP Failed: %s, index = %d\n", strerror( errno ), i );
	 // TODO: Cleanup
	 return NULL;
      }
      mgr->free_buffers++;
   }

   return mgr;
}

unicap_status_t buffer_mgr_queue_all( buffer_mgr_t mgr )
{
   int i;
   unicap_status_t status = STATUS_SUCCESS;
   
   BUFFER_MGR_LOCK( mgr );

   for( i = 0; i < mgr->num_buffers; i++ ){
      unicap_status_t tmp;
      if( !SUCCESS( tmp = buffer_mgr_queue( mgr, &mgr->buffers[i].data_buffer ) ) ){
	 status = tmp;
      }
   }

   BUFFER_MGR_UNLOCK( mgr );

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
	 if( ( ret = IOCTL( mgr->fd, VIDIOC_QBUF, &v4l2_buffer ) ) < 0 ){
	    if( ret == -ENODEV ){
	       status = STATUS_NO_DEVICE;
	    }
	    TRACE( "VIDIOC_QBUF ioctl failed: %s\n", strerror( errno ) );
	 } else {
	    status = STATUS_SUCCESS;
	    mgr->buffers[i].queued = TRUE;
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
   
   BUFFER_MGR_LOCK (mgr);
   if( IOCTL( mgr->fd, VIDIOC_DQBUF, &v4l2_buffer ) < 0 ){
      TRACE( "VIDIOC_DQBUF ioctl failed: %s\n", strerror( errno ) );
      status = STATUS_FAILURE;
   } else {
      for (i = 0; i < mgr->num_buffers; i++ ){
	 if (mgr->buffers[i].v4l2_buffer.index == v4l2_buffer.index ){
	    *buffer = &mgr->buffers[i].data_buffer;
	    *buffer->buffer_size = v4l2_buffer.bytesused;
	    memcpy( &*buffer->fill_time, &v4l2_buffer.timestamp, sizeof( struct timeval ) );
	 }
      }
   }

   if (!*buffer){
      TRACE ("VIDIOC_DQBUF returned a buffer that is not in the pool!?!?!?");
      status = STATUS_FAILURE;
   }
   
   BUFFER_MGR_UNLOCK (mgr);
   
   return status;
}
