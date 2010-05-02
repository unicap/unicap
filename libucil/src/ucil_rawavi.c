/* unicap
 *
 * Copyright (C) 2004-2008 Arne Caspari ( arne@unicap-imaging.org )
 *
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

#include "config.h"

#include <unicap.h>
#include "ucil.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <linux/types.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdarg.h>
#include <glib.h>
#include "avi.h"
#include "ucil_rawavi.h"
#include "colorspace.h"

#if UCIL_DEBUG
#define DEBUG
#endif
#include <debug.h>

#define ENCODEBUFFER_SIZE 4
#define STRUCTOFFSET(s,m) (((char *)(&(((s *)0)->m)))-((char *)0))


struct _ucil_rawavi_video_file_object
{
   // The destination format for the video output
   // contains the bit depth, size, etc.
   unicap_format_t    format;
   
   unsigned int file_size;
   
   FILE *outfile;
   int row_stride;
   int movi_size;
   int movi_size_pos;
   int movi_frames;
   int stream_length_pos;

   avi_buffer_t *avibuf;

   GQueue *full_queue;
   GQueue *empty_queue;

   sem_t lock;

   int quit_thread;
   pthread_t encoder_thread;
};


    

#define AVI_PAD_SIZE 4096

static void avi_add_chunk( avi_buffer_t *buffer, __u32 fourcc, __u32 size, __u8 *data );
static __u32 avi_add_list_hdr( avi_buffer_t *buffer, __u32 fourcc, __u32 size );
static __u32 avi_add_chunk_hdr( avi_buffer_t *buffer, __u32 fourcc, __u32 size );
static void avi_list_pad( avi_buffer_t *list, int offset, int padding );
static int avi_write_buffer( FILE *f, avi_buffer_t *buffer );



static void *ucil_rawavi_encode_thread( ucil_rawavi_video_file_object_t *vobj )
{
   while( !vobj->quit_thread )
   {
      __u32 chunkptr;
      __u32 *tmp;
      avi_buffer_t *avibuf;
      __u32 size;
      unicap_data_buffer_t *buffer;

      sem_wait( &vobj->lock );
      buffer = g_queue_pop_head( vobj->full_queue );
      sem_post( &vobj->lock );
      if( !buffer )
      {
	 usleep( 1000 ); // ToDo: wait on object
	 continue;
      }
      TRACE( "encode frame\n" );
      
      avibuf = vobj->avibuf;
      
      chunkptr = avi_add_chunk_hdr( avibuf, UCIL_FOURCC( '0', '0', 'd', 'c' ), 0 );

      memcpy( avibuf->bData + avibuf->dwPtr, buffer->data, buffer->format.buffer_size );
      avibuf->dwPtr += buffer->format.buffer_size;

      size = buffer->format.buffer_size;
   
      tmp = (__u32*)( avibuf->bData + chunkptr + 4 ); // pointer to
						      // chunk size
      *tmp = size;

      vobj->movi_size += size + 0 + sizeof( avi_chunk_t )/*  + sizeof( avi_list_t ) */;
      vobj->movi_frames++;
      avi_write_buffer( vobj->outfile, avibuf );
      vobj->file_size += avibuf->dwPtr;

      avibuf->dwPtr = 0;

      sem_wait( &vobj->lock );
      g_queue_push_head( vobj->empty_queue, buffer );
      sem_post( &vobj->lock );
   }

   TRACE( "rawavi quit thread\n" );

   return NULL;
}

static int write_avi_header( FILE *f )
{
   __u32 hdr[3];
   int res = 0;

   hdr[0] = UCIL_FOURCC( 'R', 'I', 'F', 'F');
   hdr[1] = 0;
   hdr[2] = UCIL_FOURCC( 'A', 'V', 'I', ' ');
   

   return( fwrite( hdr, sizeof(hdr), 1, f ) < 1 );
}

static avi_buffer_t *avi_create_list( __u32 fourcc )
{
   avi_buffer_t *buf;
   avi_list_t list;
   
   buf = malloc( sizeof( avi_buffer_t ) );
   buf->dwBufferSize = 0;
   buf->dwPtr = 0;

   buf->bData = malloc( sizeof( avi_list_t ) );
   buf->dwBufferSize = sizeof( avi_list_t );

   list.dwList = UCIL_FOURCC( 'L', 'I', 'S', 'T' );
   list.dwSize = 4;
   list.dwFourCC = fourcc;
   
   memcpy( buf->bData, &list, sizeof( list ) );
   buf->dwPtr += sizeof( list );
   
   return buf;
}

static avi_buffer_t *avi_create_chunk( __u32 fourcc, void *data, __u32 size )
{
   avi_buffer_t *buf;
   avi_chunk_t chunk;
   
   buf = malloc( sizeof( avi_buffer_t ) );
   buf->dwBufferSize = size;
   buf->dwPtr = 0;

   buf->bData = malloc( sizeof( avi_chunk_t ) + size );
   buf->dwBufferSize = sizeof( avi_chunk_t ) + size;

   chunk.dwSize = size;
   chunk.dwFourCC = fourcc;
   
   memcpy( buf->bData, &chunk, sizeof( chunk ) );
   buf->dwPtr += sizeof( chunk );
   memcpy( buf->bData + buf->dwPtr, data, size );
   buf->dwPtr += size;
   
   return buf;
}

static void avi_list_add( avi_buffer_t *list, avi_buffer_t *buf )
{
   avi_list_t *listh;
   
   if( list->dwBufferSize < ( list->dwPtr + buf->dwBufferSize ) )
   {
      __u8 *tmp;
      
      tmp = malloc( list->dwBufferSize + buf->dwBufferSize );
      memcpy( tmp, list->bData, list->dwPtr );
      free( list->bData );
      list->bData = tmp;
      list->dwBufferSize += buf->dwBufferSize;
   }

   memcpy( list->bData + list->dwPtr, buf->bData, buf->dwBufferSize );
   list->dwPtr += buf->dwBufferSize;
   
   listh = ( avi_list_t * )list->bData;
   listh->dwSize = listh->dwSize + buf->dwBufferSize;
}

static void avi_list_pad( avi_buffer_t *list, int offset, int padding )
{
   __u8 *chunk_buffer;
   int padsize;
   
   chunk_buffer = malloc( padding );
   memset( chunk_buffer, 0x0, padding );

   padsize = padding - ( ( offset + list->dwPtr + sizeof( avi_list_t ) - 4 ) % padding );
   
   avi_add_chunk( list, UCIL_FOURCC( 'J', 'U', 'N', 'K' ), padsize, chunk_buffer );
}


static __u32 avi_add_list_hdr( avi_buffer_t *buffer, __u32 fourcc, __u32 size )
{
   avi_list_t list;
   __u32 ret;
   
   list.dwList = UCIL_FOURCC( 'L', 'I', 'S', 'T' );
   list.dwSize = size + 4;
   list.dwFourCC = fourcc;
   
   if( buffer->dwBufferSize < ( buffer->dwPtr + sizeof( list ) ) )
   {
      __u8 *tmp;
      
      tmp = malloc( buffer->dwPtr + sizeof( list ) );
      memcpy( tmp, buffer->bData, buffer->dwPtr );
      free( buffer->bData );
      buffer->bData = tmp;
      buffer->dwBufferSize = buffer->dwPtr + sizeof( list );
   }

   memcpy( buffer->bData + buffer->dwPtr, &list, sizeof( list ) );
   ret = buffer->dwPtr;
   buffer->dwPtr += sizeof( list );
   
   return ret;
}

static int avi_write_list_hdr( FILE *f, __u32 fourcc, __u32 size )
{
   avi_list_t list;
   
   list.dwList = UCIL_FOURCC( 'L', 'I', 'S', 'T' );
   list.dwSize = size + 4;
   list.dwFourCC = fourcc;
   
   return fwrite( &list, sizeof( list ), 1, f );
}

static void avi_add_chunk( avi_buffer_t *buffer, __u32 fourcc, __u32 size, __u8 *data )
{
   avi_chunk_t chunk;
   
   chunk.dwFourCC = fourcc;
   chunk.dwSize = size;
   
   if( buffer->dwBufferSize < ( buffer->dwPtr + sizeof( chunk ) + size ) )
   {
      __u8 *tmp;
      
      tmp = malloc( buffer->dwPtr + sizeof( chunk ) + size );
      memcpy( tmp, buffer->bData, buffer->dwPtr );
      buffer->dwBufferSize = buffer->dwPtr + sizeof( chunk ) + size;
      free( buffer->bData );
      buffer->bData = tmp;
   }

   memcpy( buffer->bData + buffer->dwPtr, &chunk, sizeof( chunk ) );
   buffer->dwPtr += sizeof( chunk );
   memcpy( buffer->bData + buffer->dwPtr, data, size );
   buffer->dwPtr += size;
}

static __u32 avi_add_chunk_hdr( avi_buffer_t *buffer, __u32 fourcc, __u32 size )
{
   avi_chunk_t chunk;
   __u32 ret;
   
   chunk.dwFourCC = fourcc;
   chunk.dwSize = size;
   
   if( buffer->dwBufferSize < ( buffer->dwPtr + sizeof( chunk ) ) )
   {
      __u8 *tmp;
      
      tmp = malloc( buffer->dwPtr + sizeof( chunk ) );
      memcpy( tmp, buffer->bData, buffer->dwPtr );
      buffer->dwBufferSize = buffer->dwPtr + sizeof( chunk );
      free( buffer->bData );
      buffer->bData = tmp;
   }

   memcpy( buffer->bData + buffer->dwPtr, &chunk, sizeof( chunk ) );
   ret = buffer->dwPtr;
   buffer->dwPtr += sizeof( chunk );

   return ret;
}

static int avi_write_buffer( FILE *f, avi_buffer_t *buffer )
{
   if( fwrite( buffer->bData, buffer->dwPtr, 1, f ) < 1 )
   {
      return -1;
   }
   
   return 0;
}

static void avi_free_buffer( avi_buffer_t *buf )
{
   free( buf->bData );
   free( buf );
}

static void parse_args( ucil_rawavi_video_file_object_t *vobj, guint n_parameters, GParameter *parameters )
{
   int i;
   
   for( i = 0; i < n_parameters; i++ )
   {
      if( !strcmp( parameters[i].name, "fourcc" ) )
      {
	 vobj->format.fourcc = g_value_get_int( &parameters[i].value );
      }
   }
}

static void encode_va_list_to_parameters( va_list ap, guint *n_parameters, GParameter **parameters )
{
   char *arg;
   const int MAX_PARAMETERS = 128;
   GParameter *tmp;
   guint n = 0;

   tmp = g_new0( GParameter, MAX_PARAMETERS );

   while( ( arg = va_arg( ap, char * ) ) )
   {
      tmp[n].name = g_strdup( arg );
      
      if( !strcmp( arg, "fourcc" ) )
      {
	 g_value_init( &tmp[n].value, G_TYPE_INT );
	 g_value_set_int( &tmp[n].value, va_arg( ap, int ) );
      }
      else
      {
	 g_message( "Unknown/Illegal argument for encoder: '%s'", arg );
	 break;
      }
      n++;
   }
   *parameters = g_new( GParameter, n );
   *n_parameters = n;
   memcpy( *parameters, tmp, sizeof( GParameter ) * n );
   g_free( tmp );
}

//
//
//
ucil_rawavi_video_file_object_t *ucil_rawavi_create_video_file( const char *path, unicap_format_t *format, const char *codec, va_list ap )
{
   guint n_parameters;
   GParameter *parameters;
   ucil_rawavi_video_file_object_t *vobj;
   
   encode_va_list_to_parameters( ap, &n_parameters, &parameters );
   vobj = ucil_rawavi_create_video_filev( path, format, codec, n_parameters, parameters );
   g_free( parameters );
   return vobj;
}

ucil_rawavi_video_file_object_t *ucil_rawavi_create_video_filev( const char *path, unicap_format_t *format, const char *codec, guint n_parameters, GParameter *parameters )
{
   avi_main_header_t avih;
   avi_buffer_t *avibuf;
   avi_buffer_t *hdrl;
   avi_buffer_t *vstrl;
   avi_buffer_t *streambuf;
   avi_stream_header_t strh;
   avi_bitmapinfohdr_t strf;

   ucil_rawavi_video_file_object_t *vobj;

   int i;

   vobj = malloc( sizeof( ucil_rawavi_video_file_object_t ) );
   if( !vobj )
   {
      return NULL;
   }  
   vobj->outfile = fopen( path, "wb" );
   if( !vobj->outfile )
   {
      free( vobj );
      return NULL;
   }

   unicap_copy_format( &vobj->format, format );

   vobj->movi_frames = 0;
   
   parse_args( vobj, n_parameters, parameters );

   if( vobj->format.fourcc != format->fourcc )
   {
      xfm_info_t xfminfo;
      
      ucil_get_xfminfo_from_fourcc( format->fourcc, 
				    vobj->format.fourcc, 
				    &xfminfo );
      
      vobj->format.bpp = xfminfo.dest_bpp;
   }

   
   write_avi_header( vobj->outfile );
   vobj->file_size = 4;
   avih.dwMicroSecPerFrame = 33333;
   avih.dwMaxBytesPerSec = vobj->format.size.width * vobj->format.size.height * vobj->format.bpp/8*30;
   avih.dwPaddingGranularity = AVI_PAD_SIZE;
   avih.dwFlags = 0;
   avih.dwInitialFrames = 0;
   avih.dwStreams = 1;
   avih.dwSuggestedBufferSize = 4096;
   avih.dwWidth = vobj->format.size.width;
   avih.dwHeight = vobj->format.size.height;
      
   vstrl = avi_create_list( UCIL_FOURCC( 's', 't', 'r', 'l' ) );
   strh.fccType = UCIL_FOURCC( 'v', 'i', 'd', 's' );
   strh.fccHandler = vobj->format.fourcc;
   strh.dwFlags = 0;
   strh.dwInitialFrames = 0;
   strh.wPriority = 0;
   strh.wLanguage = 0;
   strh.dwScale = 10000;
   strh.dwRate = 300000;
   strh.dwStart = 0;
   strh.dwLength = 0;
   strh.dwSuggestedBufferSize = format->buffer_size; 
   strh.dwQuality = 0;
   strh.dwSampleSize = 0;
   strh.rcFrame.left = 0;
   strh.rcFrame.top = 0;
   strh.rcFrame.right = vobj->format.size.width;
   strh.rcFrame.bottom = vobj->format.size.height;
      
   avibuf = avi_create_chunk( UCIL_FOURCC( 's', 't', 'r', 'h' ), &strh, sizeof( strh ) );
   avi_list_add( vstrl, avibuf );
   avi_free_buffer( avibuf );

   strf.biSize = sizeof(strf);
   strf.biWidth = vobj->format.size.width;
   strf.biHeight = vobj->format.size.height;
   strf.biPlanes = 1;
   strf.biBitCount = vobj->format.bpp;
   strf.biCompression = vobj->format.fourcc;
   strf.biSizeImage = vobj->format.size.width * vobj->format.size.height * vobj->format.bpp / 8;
   strf.biXPelsPerMeter = 0;
   strf.biYPelsPerMeter = 0;
   strf.biClrUsed = 0;
   strf.biClrImportant = 0;
      
   avibuf = avi_create_chunk( UCIL_FOURCC( 's', 't', 'r', 'f' ), &strf, sizeof( strf ) );
   avi_list_add( vstrl, avibuf );
   avi_free_buffer( avibuf );
      
   hdrl = avi_create_list( UCIL_FOURCC( 'h', 'd', 'r', 'l' ) );
   avibuf = avi_create_chunk( UCIL_FOURCC( 'a', 'v', 'i', 'h' ), &avih, sizeof( avih ) );
   avi_list_add( hdrl, avibuf );
   avi_free_buffer( avibuf );
   avi_list_add( hdrl, vstrl );
   
   avi_list_pad( hdrl, 3 * sizeof( __u32 ), AVI_PAD_SIZE );
   
   vobj->stream_length_pos = ftell( vobj->outfile ) + sizeof( avi_list_t ) * 2 + sizeof( avih ) + STRUCTOFFSET( avi_stream_header_t, dwLength ) + 16;   
   
   avi_write_buffer( vobj->outfile, hdrl );
   vobj->file_size += hdrl->dwPtr;
   vobj->movi_size_pos = ftell( vobj->outfile ) + 4;
   avi_write_list_hdr( vobj->outfile, UCIL_FOURCC( 'm', 'o', 'v', 'i' ), 0 );
   vobj->file_size += sizeof( avi_list_t );

   streambuf = malloc( sizeof( avi_buffer_t ) );
   streambuf->dwBufferSize = format->buffer_size + 2 * sizeof( avi_list_t );
   streambuf->bData = malloc( streambuf->dwBufferSize );
   streambuf->dwPtr = 0;

   vobj->avibuf = streambuf;
   vobj->movi_size = 0;

   vobj->full_queue = g_queue_new();
   vobj->empty_queue = g_queue_new();
   sem_init( &vobj->lock, 0, 1 );

   vobj->format.buffer_size = vobj->format.size.width * vobj->format.size.height * vobj->format.bpp/8;
   for( i = 0; i < ENCODEBUFFER_SIZE; i++ )
   {
      unicap_data_buffer_t *data_buffer;
      
      data_buffer = malloc( sizeof( unicap_data_buffer_t ) );
      unicap_copy_format( &data_buffer->format, &vobj->format );
      
      data_buffer->data = malloc( vobj->format.buffer_size );
      data_buffer->buffer_size = vobj->format.buffer_size;
      
      g_queue_push_head( vobj->empty_queue, data_buffer );
   }
   

   vobj->quit_thread = 0;
   if( pthread_create( &vobj->encoder_thread, NULL, (void*(*)(void*))ucil_rawavi_encode_thread, vobj ) )
   {
      TRACE( "Failed to create encoder thread!\n " );
   }


   return vobj;
}

unicap_status_t ucil_rawavi_encode_frame( ucil_rawavi_video_file_object_t *vobj, unicap_data_buffer_t *buffer )
{
   unicap_status_t status = STATUS_SUCCESS;
   unicap_data_buffer_t *dst_buffer;
   
   TRACE( "encode frame\n" );
   
   sem_wait( &vobj->lock );
   dst_buffer = g_queue_pop_head( vobj->empty_queue );
   sem_post( &vobj->lock );
   if( !dst_buffer )
   {
      TRACE( "no entries in empty queue\n" );
      status = STATUS_FAILURE;
   }
   else
   {
      buffer->flags |= UNICAP_FLAGS_BUFFER_LOCKED;
      ucil_convert_buffer( dst_buffer, buffer );
      memcpy( &dst_buffer->fill_time, &buffer->fill_time, sizeof( struct timeval ) );
      buffer->flags &= ~UNICAP_FLAGS_BUFFER_LOCKED;
      sem_wait( &vobj->lock );
      g_queue_push_tail( vobj->full_queue, dst_buffer );
      sem_post( &vobj->lock );
   }
   
   return status;
}

static avi_buffer_t *ucil_rawavi_create_index( ucil_rawavi_video_file_object_t *vobj )
{
   avi_index_entry_t *idx;
   int i;
   int offset = 4;
   
   idx = malloc( vobj->movi_frames * sizeof( avi_index_entry_t ) );

   for( i = 0; i < vobj->movi_frames; i++ )
   {
      idx[i].dwFourCC = UCIL_FOURCC( '0', '0', 'd', 'c' );
      idx[i].dwFlags = 0x10;
      idx[i].dwChunkOffset = offset;
      idx[i].dwChunkLength = vobj->format.buffer_size;
      
      offset += vobj->format.buffer_size + 8;
   }
   
   return avi_create_chunk( UCIL_FOURCC( 'i', 'd', 'x', '1' ), idx, vobj->movi_frames * sizeof( avi_index_entry_t ) );   
}

unicap_status_t ucil_rawavi_close_video_file( ucil_rawavi_video_file_object_t *vobj )
{
   avi_buffer_t *buf;
   unicap_status_t status = STATUS_SUCCESS;
   

   while( g_queue_get_length( vobj->full_queue ) )
   {
      usleep( 100 );
   }
   vobj->quit_thread = 1;
   pthread_join( vobj->encoder_thread, NULL );   


   buf = ucil_rawavi_create_index( vobj );
   avi_write_buffer( vobj->outfile, buf );
   vobj->file_size += buf->dwPtr;

   fseek( vobj->outfile, 4, SEEK_SET );
   if( fwrite( &vobj->file_size, 4, 1, vobj->outfile ) < 1 )
   {
      status = STATUS_FAILURE;
      goto out;
   }

   vobj->movi_size += 4;
   if( fseek( vobj->outfile, vobj->movi_size_pos, SEEK_SET ) < 0 )
   {
      status = STATUS_FAILURE;
      goto out;
   }
   
   if( fwrite( &vobj->movi_size, 4, 1, vobj->outfile ) < 1 )
   {
      status = STATUS_FAILURE;
      goto out;
   }

   if( fseek( vobj->outfile, vobj->stream_length_pos, SEEK_SET ) < 0 )
   {
      status = STATUS_FAILURE;
      goto out;
   }
   
   if( fwrite( &vobj->movi_frames, 4, 1, vobj->outfile ) < 1 )
   {
      status = STATUS_FAILURE;
      goto out;
   }

  out:
   fclose( vobj->outfile );
   
   unicap_data_buffer_t *data_buffer;
   for( data_buffer = g_queue_pop_head( vobj->empty_queue ); data_buffer; data_buffer = g_queue_pop_head( vobj->empty_queue ) )
   {
      free( data_buffer->data );
      free( data_buffer );
   }
   g_queue_free( vobj->empty_queue );
   g_queue_free( vobj->full_queue );
   free( vobj );
   
   return STATUS_SUCCESS;
}
