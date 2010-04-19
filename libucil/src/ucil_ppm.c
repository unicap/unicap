#include "ucil.h"
#include "ucil_private.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>


int nextval( int fd )
{
   char c;
   
   while( 1 ){
      if( read( fd, &c, 1 ) <= 0 )
	 return -1;
      while( isblank( (int)c ) ){
	 if( read( fd, &c, 1 )  <=0 )
	    return -1;
      }
      if( c == '\n' ){
	 if( read( fd, &c, 1 ) <= 0 )
	    return -1;
	 if (c=='#'){
	    while (c != '\n' ){
	       if( read( fd, &c, 1 ) <= 0 )
		  return -1;
	    }
	 }
      }
      if (isdigit((int)c)){
	 break;
      }
   }
   
   char buf[16];
   int i = 0;
   while ((i<15) && isdigit(c)){
      buf[i] = c;
      if( read( fd, &c, 1 ) <= 0 )
	 return -1;
      i++;
   }
   buf[i] = 0;

   if (!isspace(c)){
      return -1;
   }
   
   return atoi(buf);
}	 

__HIDDEN__ unicap_data_buffer_t *ucil_ppm_read_file( const char *path )
{
   int fd;
   char buffer[16];
   
   fd = open( path, O_RDONLY );
   if (fd < 0)
      return NULL;
   
   if (read (fd, buffer, 2) < 2){
      close(fd);
      return NULL;
   }
   
   int width;
   int height;
   int maxval;
   int size;

   width = nextval( fd );
   height = nextval( fd );
   maxval = nextval( fd );

   if ((width == -1) || (height == -1) || (maxval == -1) ){
      close(fd);
      return NULL;
   }

   size = width * height * 3 * ( maxval < 256 ? 1 : 2 );
   
   unicap_data_buffer_t *destbuf;
   destbuf = ucil_allocate_buffer( width, height, UCIL_FOURCC( 'R', 'G', 'B', '3' ), maxval < 256 ? 24 : 48 );
   
   int bytes_read = 0;
   
   while (bytes_read < size){
      ssize_t rc;
      rc = read( fd, destbuf->data + bytes_read, size - bytes_read );
      if (rc<0){
	 if (errno != EINTR){
	    free(destbuf->data);
	    free(destbuf);
	    close(fd);
	    return NULL;
	 }
      } else {
	 bytes_read += rc;
      }
   }   
   
   return destbuf;
}

__HIDDEN__ unicap_status_t ucil_ppm_write_file( unicap_data_buffer_t *databuffer, const char *path  )
{
   char hdr[128];
   int fd;
   
   sprintf( hdr, "P6\n%d %d %d\n", databuffer->format.size.width, databuffer->format.size.height, databuffer->format.bpp == 24 ? 255 : 65535 );
   
   fd = open( path, O_RDWR | O_CREAT, 00777 );
   if (fd < 0 ){
      return STATUS_FAILURE;
   }
   
   ssize_t s = strlen( hdr );
   ssize_t wc;
   wc = write( fd, hdr, s );
   if (wc != s){
      close( fd );
      return STATUS_FAILURE;
   }
   
   ssize_t bytes_written = 0;
   while (bytes_written < databuffer->buffer_size ){
      wc = write( fd, databuffer->data + bytes_written, databuffer->buffer_size - bytes_written );
      if( wc < 0 ){
	 if (errno != EINTR){
	    close (fd);
	    return STATUS_FAILURE;
	 }
      }
      bytes_written += wc;
   }

   close (fd);

   return STATUS_SUCCESS;
}
