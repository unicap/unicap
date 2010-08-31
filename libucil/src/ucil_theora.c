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
#include <string.h>
#include <ogg/ogg.h>
#include <theora/theora.h>
#if HAVE_ALSA
#include <vorbis/vorbisenc.h>
#include "ucil_alsa.h"
#endif
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <pthread.h>

#include "ucil_theora.h"
#include "ucil.h"
#include "unicap.h"
#if UCIL_DEBUG
#define DEBUG
#endif
#include "debug.h"
#if 0
#include "../libunicap/include/unicap_private.h"
#endif

#define INBUF_SIZE 4096

#define ENCODEBUFFER_SIZE 8

#define UCIL_ERROR 101
#define UCIL_ERROR_INVALID_STREAM 109

static double encode_vorbis( FILE *f, vorbis_dsp_state *vd, ogg_stream_state *vo, vorbis_block *vb, 
			     unsigned int audio_rate, double audiopos, signed char *buf, int n_samples );


static unsigned long long timevaltousec( struct timeval *tv )
{
   unsigned long long t = 0;
   
   t += tv->tv_sec * 1000000;
   t += tv->tv_usec;
   
   return t;
}

static int buffer_data( FILE *f, ogg_sync_state *oy )
{
   char * buffer;
   int bytes;
   buffer = ogg_sync_buffer( oy, 4096 );
   bytes = fread( buffer, 1, 4096, f );	    
   ogg_sync_wrote( oy, bytes );

   return bytes;
}

gboolean ucil_theora_combine_av_file( const char *path, const char *codec, gboolean remove, ucil_processing_info_func_t procfunc, void *func_data, GError **error )
{
   FILE *f;
   FILE *af;
   FILE *vf;
   
   gchar *apath;
   gchar *vpath;

   long len;
   long filepos = 0;

   ogg_sync_state   oy;
   ogg_stream_state tins;
   theora_comment   tc;
   theora_info      ti;
   theora_state     ts;

   ogg_stream_state vs;
   vorbis_info      vi;
   vorbis_comment   vc;
   vorbis_dsp_state vd;
   vorbis_block     vb;
   
   ogg_page og;
   ogg_packet op;
   ogg_packet header_comm;
   ogg_packet header_code;
   

   int pkt;
   double videopos, audiopos;

   f = fopen( path, "w" );
   if( !f )
   {
      gchar *name = g_filename_display_name( path );
      g_set_error( error, G_FILE_ERROR, g_file_error_from_errno( errno ),
		   _("Could not open '%s' for writing: %s"),
                   name, g_strerror( errno ) );
      g_free( name );
      return FALSE;
   }
   
   apath = g_strconcat( path, ".atmp", NULL );

   af = fopen( apath, "r" );
   if( !af )
   {
      gchar *name = g_filename_display_name( apath );
      g_set_error( error, G_FILE_ERROR, g_file_error_from_errno( errno ),
		   _("Could not open '%s' for reading: %s"),
                   name, g_strerror( errno ) );
      g_free( name );
      g_free( apath );
      fclose( f );
      return FALSE;
   }
   
   vpath = g_strconcat( path, ".vtmp", NULL );
   
   vf = fopen( vpath, "r" );
   if( !vf )
   {
      gchar *name = g_filename_display_name( vpath );
      g_set_error( error, G_FILE_ERROR, g_file_error_from_errno( errno ),
		   _("Could not open '%s' for reading: %s"),
                   name, g_strerror( errno ) );
      g_free( name );
      g_free( apath );
      g_free( vpath );
      fclose( f );
      fclose( af );
      return FALSE;
   }
   
   fseek( af, 0L, SEEK_END );
   len = ftell( af );
   fseek( af, 0L, SEEK_SET );

   ogg_sync_init( &oy );
   theora_comment_init( &tc );
   theora_info_init( &ti );

   if( ogg_stream_init( &vs, 2 ) != 0 )
   {
      g_free( apath );
      g_free( vpath );
      fclose( f ); fclose( af ); fclose( vf );
      g_set_error( error, UCIL_ERROR, UCIL_ERROR_INVALID_STREAM, 
		   _("Failed to initialize audio stream") );
      return FALSE;
   }
   vorbis_info_init( &vi );
   if( vorbis_encode_init( &vi, 2, 44100, -1, 128000, -1 ) )
   {
      g_free( apath );
      g_free( vpath );
      fclose( f ); fclose( af ); fclose( vf );
      g_set_error( error, UCIL_ERROR, UCIL_ERROR_INVALID_STREAM, 
		   _("Failed to initialize audio encoder") );
      return FALSE;
   }
   vorbis_comment_init( &vc );
   vorbis_analysis_init( &vd, &vi );
   vorbis_block_init( &vd, &vb );

   //
   // write theora header page
   //
   {
      ogg_page og;
      
      while( ogg_sync_pageout( &oy, &og ) == 0 )
      {
	 int bytes;
	 
	 bytes = buffer_data( vf, &oy );
	 if( bytes == 0 )
	 {
	    g_free( apath );
	    g_free( vpath );
	    fclose( f ); fclose( af ); fclose( vf );
	    g_set_error( error, UCIL_ERROR, UCIL_ERROR_INVALID_STREAM, 
			 _("Invalid video stream") );
	    return FALSE;
	 }
      }
      if( !ogg_page_bos( &og ) )
      {
	 g_free( apath );
	 g_free( vpath );
	 fclose( f ); fclose( af ); fclose( vf );
	 g_set_error( error, UCIL_ERROR, UCIL_ERROR_INVALID_STREAM, 
		      _("Invalid video stream") );
	 return FALSE;
      }
      ogg_stream_init( &tins, ogg_page_serialno( &og ) );

      fwrite( og.header, 1, og.header_len, f );
      fwrite( og.body, 1, og.body_len, f );

      if( ogg_stream_pagein( &tins, &og ) < 0 )
      {
	 g_free( apath );
	 g_free( vpath );
	 fclose( f ); fclose( af ); fclose( vf );
	 g_set_error( error, UCIL_ERROR, UCIL_ERROR_INVALID_STREAM, 
		      _("Invalid video stream") );
	 return FALSE;
      }
      pkt = 0;
      while( ( pkt < 3 ) && ( ogg_stream_packetout( &tins, &op ) > 0 ) )
      {
	 if( theora_decode_header( &ti, &tc, &op ) != 0 )
	 {
	    g_free( apath );
	    g_free( vpath );
	    fclose( f ); fclose( af ); fclose( vf );
	    g_set_error( error, UCIL_ERROR, UCIL_ERROR_INVALID_STREAM, 
			 _("Invalid video stream") );
	    return FALSE;
	 }
	 pkt++;
      }
   }
   
   //
   // Write vorbis header page
   //
   vorbis_analysis_headerout( &vd, &vc, &op, &header_comm, &header_code );
   ogg_stream_packetin( &vs, &op );
   if( ogg_stream_pageout( &vs, &og ) != 1 )
   {
      g_free( apath );
      g_free( vpath );
      fclose( f ); fclose( af ); fclose( vf );
      g_set_error( error, UCIL_ERROR, UCIL_ERROR_INVALID_STREAM, 
		   _("Could not create audio stream") );
      return FALSE;
   }
   fwrite( og.header, 1, og.header_len, f );
   fwrite( og.body,   1, og.body_len, f );
   
   ogg_stream_packetin( &vs, &header_comm );
   ogg_stream_packetin( &vs, &header_code );

   // write remaining theora headers
   for( pkt = 1; pkt < 3; )
   {
      ogg_page og;
      
      if( ogg_sync_pageout( &oy, &og ) > 0 )
      {
	 
	 fwrite( og.header, 1, og.header_len, f );
	 fwrite( og.body, 1, og.body_len, f );

	 if( ogg_stream_pagein( &tins, &og ) < 0 )
	 {
	    g_free( apath );
	    g_free( vpath );
	       fclose( f ); fclose( af ); fclose( vf );
	       g_set_error( error, UCIL_ERROR, UCIL_ERROR_INVALID_STREAM, 
			    _("Invalid video stream") );
	       return FALSE;
	 }
	 
	 while( ( pkt < 3 ) && ( ogg_stream_packetout( &tins, &op ) > 0 ) )
	 {
	    if( theora_decode_header( &ti, &tc, &op ) != 0 )
	    {
	       g_free( apath );
	       g_free( vpath );
	       fclose( f ); fclose( af ); fclose( vf );
	       g_set_error( error, UCIL_ERROR, UCIL_ERROR_INVALID_STREAM, 
			    _("Invalid video stream") );
	       return FALSE;
	    }
	    pkt++;
	 }
      }
      else
      {
	 int bytes;
	 
	 bytes = buffer_data( vf, &oy );
	 if( bytes == 0 )
	 {
	    g_free( apath );
	    g_free( vpath );
	    fclose( f ); fclose( af ); fclose( vf );
	    g_set_error( error, UCIL_ERROR, UCIL_ERROR_INVALID_STREAM, 
			 _("Invalid video stream") );
	    return FALSE;
	 }
      }
   }

   theora_decode_init( &ts, &ti );
   

   //
   // Write remaining vorbis headers
   //
   while( ogg_stream_flush( &vs, &og ) > 0 )
   {
      fwrite( og.header, 1, og.header_len, f );
      fwrite( og.body,   1, og.body_len, f );
   }

   audiopos = 0.0;
   videopos = 0.0;
   
   while( !feof( af ) )
   {
      signed char abuf[512 * 4];
      int n_samples;
      
      if( feof( vf ) || ( audiopos <= videopos ) )
      {
	 n_samples = fread( abuf, 4, 512, af );
	 audiopos = encode_vorbis( f, &vd, &vs, &vb, 44100, audiopos, abuf, n_samples );
	 filepos += n_samples * 4;
      }
      else
      {
	 buffer_data( vf, &oy );
	 
	 if( ogg_sync_pageout( &oy, &og ) > 0 )
	 {
	    double tmppos;
	    fwrite( og.header, 1, og.header_len, f );
	    fwrite( og.body, 1, og.body_len, f );
	    if( ogg_stream_pagein( &tins, &og ) < 0 )
	    {
	       TRACE( "failed to decode theora\n" );
	    }
	    ogg_stream_packetout( &tins, &op );
	    tmppos = theora_granule_time( &ts, ogg_page_granulepos( &og ) );
	    if( tmppos > 0 )
	    {
	       videopos = tmppos;
	    }	    
	 }
      }

      if( procfunc )
      {
	 procfunc( func_data, (double)filepos/(double)len );
      }
   }

   while( !feof( vf ) && buffer_data( vf, &oy ) )
   {
      if( ogg_sync_pageout( &oy, &og ) > 0 )
      {
	 fwrite( og.header, 1, og.header_len, f );
	 fwrite( og.body, 1, og.body_len, f );
      }
   }

   if( remove )
   {
      unlink( vpath );
      unlink( apath );
   }

   g_free( vpath );
   g_free( apath );
   
   fclose( f );
   fclose( af );
   fclose( vf );
   
   return TRUE;
}



static double encode_vorbis( FILE *f, vorbis_dsp_state *vd, ogg_stream_state *vo, vorbis_block *vb, 
			     unsigned int audio_rate, double audiopos, signed char *buf, int n_samples )
{
   float **vorbis_buffer;
   const int audio_ch = 2;
   int i,j;
   int count = 0;
   ogg_page og;
   int got_page = 0;

   vorbis_buffer = vorbis_analysis_buffer( vd, n_samples );
   
   for( i = 0; i < n_samples; i++ )
   {
      for( j = 0; j < audio_ch; j++ )
      {
	 vorbis_buffer[j][i] = ( ( buf[count+1] << 8 ) | ( buf[count] & 0xff ) ) / 32768.0f;
	 count += 2;
      }
   }
   
   vorbis_analysis_wrote( vd, n_samples );

   while( vorbis_analysis_blockout( vd, vb ) == 1 )
   {
      ogg_packet op;
      
      vorbis_analysis( vb, NULL );
      vorbis_bitrate_addblock( vb );
      
      while( vorbis_bitrate_flushpacket( vd, &op ) )
      {
	 ogg_stream_packetin( vo, &op );
      }
   }
   
   while( ogg_stream_pageout( vo, &og ) )
   {
      got_page = 1;
      audiopos = vorbis_granule_time( vd, ogg_page_granulepos( &og ) );
/*       printf( "VORBIS: %f\n", audiopos ); */
      fwrite( og.header, og.header_len, 1, f );
      fwrite( og.body, og.body_len, 1, f );
   }

   if( !got_page )
   {
      double t;
      
      t = ( ( 1.0 * n_samples ) / audio_rate );
      
      audiopos += t;
   }

   return audiopos;
}


static double write_pcm( ucil_theora_video_file_object_t *vobj, double audiopos, signed char * buf, int n_samples )
{
   double t;

   fwrite( buf, n_samples * 2 * 2, 1, vobj->audiof );
   vobj->total_samples += n_samples;
   
   t = (double)vobj->total_samples / vobj->audio_rate;
   
   return t;
}

static double fetch_and_process_audio( ucil_theora_video_file_object_t *vobj, double audiopos )
{
#if HAVE_ALSA
   int n_samples;
   signed char *buf;

   
   if( !vobj->audio )
   {
      return audiopos;
   }

   while( ucil_alsa_fill_audio_buffer( vobj->audio_data ) )
      ;

   n_samples = ucil_alsa_get_audio_buffer( vobj->audio_data, (short**)&buf );
   
   if( !n_samples )
   {
      return audiopos;
   }
   
   if( vobj->async_audio_encoding )
   {
      audiopos = write_pcm( vobj, audiopos, buf, n_samples );
   }
   else
   {
      audiopos = encode_vorbis( vobj->f, &vobj->vd, &vobj->vo, &vobj->vb, vobj->audio_rate, audiopos, buf, n_samples );
   }

#endif
   return audiopos;
}

static void fill_frames( ucil_theora_video_file_object_t *vobj, unicap_data_buffer_t *data_buffer, yuv_buffer *yuv, 
			 unsigned char *ds_y_buffer, unsigned char *ds_u_buffer, unsigned char *ds_v_buffer )
{
   unsigned long long ctu;
   unsigned long long rtu;
   ogg_page og;

   ctu = timevaltousec( &data_buffer->fill_time );
   TRACE( "ctu: %lld\n", ctu );
   
   if( ctu )
   {
      unsigned long long etu;
      
      if( vobj->last_frame )
      {
	 unicap_data_buffer_t *last_data_buffer;
	 
	 last_data_buffer = vobj->last_frame;
	 if( vobj->downsize > 1 || vobj->requires_resizing_frames )
	 {
	    yuv->y = ds_y_buffer;
	    yuv->u = ds_u_buffer;
	    yuv->v = ds_v_buffer;
	 }
	 else
	 {
	    yuv->y = last_data_buffer->data;
	    yuv->u = last_data_buffer->data + ( yuv->y_stride * yuv->y_height );
	    yuv->v = yuv->u + ( yuv->uv_stride * yuv->uv_height );
	 }
	    
	 rtu = timevaltousec( &vobj->recording_start_time );
	 ctu -= rtu;
	 
	 etu = vobj->frame_interval * (unsigned long long)vobj->frame_count + (unsigned long long)vobj->frame_interval / 2;
	 
/* 		  TRACE( "ctu: %lld etu: %lld\n", ctu, etu ); */
	    
	 while( ctu > etu )
	 {
/* 		     TRACE( "Frame fill\n" );       */
	    if( theora_encode_YUVin( &vobj->th, yuv ) )
	    {
	       TRACE( "theora_encode_YUVin FAILED!\n" );
	    }
	    theora_encode_packetout( &vobj->th, 0, &vobj->op );
	    ogg_stream_packetin( &vobj->os, &vobj->op );
	    while( ogg_stream_pageout( &vobj->os, &og ) )
	    {
/* 		  printf( "THEORA: %f\n", theora_granule_time( &vobj->th, ogg_page_granulepos( &og ) ) ); */
	       fwrite( og.header, og.header_len, 1, vobj->f );
	       fwrite( og.body, og.body_len, 1, vobj->f );
	    }
	    etu += vobj->frame_interval;
	    vobj->frame_count++;
	    
	 }
	 
	 last_data_buffer->flags &= ~UNICAP_FLAGS_BUFFER_LOCKED;
	 sem_wait( &vobj->lock );
	 g_queue_push_head( vobj->empty_queue, vobj->last_frame );
	 sem_post( &vobj->lock );
      } 
   }
   else// (!ctu)
   {
      vobj->fill_frames = 0;
      if(  vobj->frame_count == 0 )
      {
	 TRACE( "Frame time information not available; can not fill frames\n" );
      }
   } 
}

static void downsize_yuv420plane( const gint srcw, const gint srch, 
				  const gint f, const gint destw, 
                                  const gint desth, 
                                  guint8 *dest, guint8 *src )
{
   int x, y;
   int i;
   int rows;

   rows = 0;
   for( y = 0; (y < srch) && (rows < desth); y += f )
   {
      gint dstentries = 0;
      for( x = 0; x < srcw; x += f )
      {
         if (dstentries < destw)
         {
	    *dest = *src;
	 
	    dest++;
            dstentries++;
         }
	 src += f;
      }
      for ( i = 1; i < f; i++ ) 
      {
         src += srcw;
      }
      rows ++;
   }
}

static void downsize_yuv420p( gint srcw, gint srch, gint f, 
			      gint destw, gint desth, 
			      guint8 *ydest, guint8 *udest, guint8 *vdest, 
			      guint8 *ysrc, guint8 *usrc, guint8 *vsrc )
{

   downsize_yuv420plane( srcw,   srch,   f, destw,   desth,   ydest, ysrc );
   downsize_yuv420plane( srcw/2, srch/2, f, destw/2, desth/2, udest, usrc );
   downsize_yuv420plane( srcw/2, srch/2, f, destw/2, desth/2, vdest, vsrc );
}


static void *ucil_theora_encode_thread( ucil_theora_video_file_object_t *vobj )
{
   yuv_buffer yuv;
   double videopos = 0;
   double audiopos = 0;
   int gotpage = 0;
   unsigned char *ds_y_buffer = NULL;
   unsigned char *ds_u_buffer = NULL;
   unsigned char *ds_v_buffer = NULL;
   
   
   yuv.y_width = vobj->ti.width;
   yuv.y_height = vobj->ti.height;
   yuv.y_stride = vobj->ti.width;
   yuv.uv_width = vobj->ti.width / 2;
   yuv.uv_height = vobj->ti.height / 2;
   yuv.uv_stride = vobj->ti.width / 2;

   if( vobj->downsize > 1 || vobj->requires_resizing_frames )
   {
      ds_y_buffer = malloc( yuv.y_width * yuv.y_height );
      ds_u_buffer = malloc( yuv.uv_width * yuv.uv_height );
      ds_v_buffer = malloc( yuv.uv_width * yuv.uv_height );
   }

   vobj->last_frame = NULL;

   while( !vobj->quit_thread )
   {
      unicap_data_buffer_t *data_buffer;
      ogg_page og;

      sem_wait( &vobj->lock );
      data_buffer = ( unicap_data_buffer_t *)g_queue_pop_head( vobj->full_queue );
      sem_post( &vobj->lock );
      if( !data_buffer )
      {
	 audiopos = fetch_and_process_audio( vobj, audiopos );	 usleep( 1000 );
	 continue;
      }

      if( vobj->frame_count == 0 )
      {
	 memcpy( &vobj->recording_start_time, &data_buffer->fill_time, sizeof( struct timeval ) );
      }

      audiopos = fetch_and_process_audio( vobj, audiopos );

/*       printf( "v: %f   a: %f\n", videopos, audiopos ); */

      if( vobj->audio && ( videopos > audiopos ) )
      {
	 data_buffer->flags &= ~UNICAP_FLAGS_BUFFER_LOCKED;
	 sem_wait( &vobj->lock );
	 g_queue_push_head( vobj->empty_queue, data_buffer );
	 sem_post( &vobj->lock );
	 continue;
      }


      if( vobj->fill_frames )
      {

	 if( vobj->audio )
	 {
	    if( vobj->last_frame )
	    {
	       unicap_data_buffer_t *last_data_buffer;
	       double streampos;
	       struct timeval streamtime;
	       
	       last_data_buffer = vobj->last_frame;
	       if( vobj->downsize > 1 || vobj->requires_resizing_frames )
	       {
		  yuv.y = ds_y_buffer;
		  yuv.u = ds_u_buffer;
		  yuv.v = ds_v_buffer;
	       }
	       else
	       {
		  yuv.y = last_data_buffer->data;
		  yuv.u = last_data_buffer->data + ( yuv.y_stride * yuv.y_height );
		  yuv.v = yuv.u + ( yuv.uv_stride * yuv.uv_height );
	       }

	       streamtime.tv_sec = data_buffer->fill_time.tv_sec - vobj->recording_start_time.tv_sec;
	       streamtime.tv_usec = data_buffer->fill_time.tv_usec;
	       if( data_buffer->fill_time.tv_usec < vobj->recording_start_time.tv_usec )
	       {
		  streamtime.tv_sec--;
		  streamtime.tv_usec += 1000000;
	       }
	       streamtime.tv_usec -= vobj->recording_start_time.tv_usec;
	       streampos = streamtime.tv_sec + ( (double)streamtime.tv_usec / 1000000.0f );
	       
		  
	       // If the streampos is ahead, this means that we get
	       // less than 30 frames per seconds.
	       // --> Fill up the stream 
	       while( streampos > videopos )
	       {
/* 		  printf( "s v: %f   a: %f\n", videopos, audiopos ); */
		  gotpage = 0;
		  if( theora_encode_YUVin( &vobj->th, &yuv ) )
		  {
		     TRACE( "theora_encode_YUVin FAILED!\n" );
		  }
		  theora_encode_packetout( &vobj->th, 0, &vobj->op );
		  ogg_stream_packetin( &vobj->os, &vobj->op );
		  while( ogg_stream_pageout( &vobj->os, &og ) )
		  {
		     double gt;
		     fwrite( og.header, og.header_len, 1, vobj->f );
		     fwrite( og.body, og.body_len, 1, vobj->f );
		     
		     gt = theora_granule_time( &vobj->th, ogg_page_granulepos( &og ) );
		     if( gt < 0 )
		     {
			continue;
		     }

		     gotpage = 1;
		     videopos = gt;
/* 		     printf( "THEORA: %f\n", videopos ); */
		  }
		  if( !gotpage )
		  {
		     videopos += vobj->frame_interval / 1000000.0f;
		  }
		  
		  vobj->frame_count++;
		  audiopos = fetch_and_process_audio( vobj, audiopos );
	       }

	       // 
	       while( ( videopos + ( vobj->frame_interval / 1000000.0f ) ) < audiopos ) 
	       {
/* 		  printf( "a v: %f   a: %f\n", videopos, audiopos ); */
		  gotpage = 0;
		  if( theora_encode_YUVin( &vobj->th, &yuv ) )
		  {
		     TRACE( "theora_encode_YUVin FAILED!\n" );
		  }
		  theora_encode_packetout( &vobj->th, 0, &vobj->op );
		  ogg_stream_packetin( &vobj->os, &vobj->op );
		  while( ogg_stream_pageout( &vobj->os, &og ) )
		  {
		     double gt;
		     fwrite( og.header, og.header_len, 1, vobj->f );
		     fwrite( og.body, og.body_len, 1, vobj->f );

		     gt = theora_granule_time( &vobj->th, ogg_page_granulepos( &og ) );
		     if( gt < 0 )
		     {
			continue;
		     }
		     
		     gotpage = 1;
		     videopos = gt;
/* 		     printf( "THEORA: %f\n", videopos ); */
		  }
		  if( !gotpage )
		  {
		     videopos += vobj->frame_interval / 1000000.0f;
		  }
		  
		  vobj->frame_count++;
		  audiopos = fetch_and_process_audio( vobj, audiopos );
	       }
	       last_data_buffer->flags &= ~UNICAP_FLAGS_BUFFER_LOCKED;
	       sem_wait( &vobj->lock );
	       g_queue_push_head( vobj->empty_queue, vobj->last_frame );
	       sem_post( &vobj->lock );
	       vobj->last_frame = NULL;
	    }
	 }
	 else
	 {
	    fill_frames( vobj, data_buffer, &yuv, ds_y_buffer, ds_u_buffer, ds_v_buffer );
	 }
	 
      }
      else// ( !vobj->fill_frames )
      {
	 if( vobj->last_frame )
	 {
	    unicap_data_buffer_t *last_data_buffer;
	    
	    last_data_buffer = vobj->last_frame;
	    last_data_buffer->flags &= ~UNICAP_FLAGS_BUFFER_LOCKED;
	    sem_wait( &vobj->lock );
	    g_queue_push_head( vobj->empty_queue, vobj->last_frame );
	    sem_post( &vobj->lock );
	 }
	 vobj->last_frame = NULL;
      }
      
      //
      // Encode the new buffer
      //
      if( vobj->encode_frame_cb )
      {
	 vobj->encode_frame_cb( UNICAP_EVENT_NEW_FRAME, NULL, data_buffer, vobj->encode_frame_cb_data );
      }
      vobj->frame_count++;

      if( vobj->downsize > 1 || vobj->requires_resizing_frames )
      {
	 downsize_yuv420p( vobj->format.size.width, vobj->format.size.height, vobj->downsize, 
			   vobj->ti.width, vobj->ti.height, 
			   ds_y_buffer, ds_u_buffer, ds_v_buffer, 
			   data_buffer->data, data_buffer->data + ( vobj->format.size.width * vobj->format.size.height ), 
			   data_buffer->data + ( vobj->format.size.width * vobj->format.size.height ) + 
			   ( ( vobj->format.size.width * vobj->format.size.height ) / 4 ) );
	 yuv.y = ds_y_buffer;
	 yuv.u = ds_u_buffer;
	 yuv.v = ds_v_buffer;
      }
      else
      {
	 yuv.y = data_buffer->data;
	 yuv.u = data_buffer->data + ( yuv.y_stride * yuv.y_height );
	 yuv.v = yuv.u + ( yuv.uv_stride * yuv.uv_height );
      }
      
      if( theora_encode_YUVin( &vobj->th, &yuv ) )
      {
	 TRACE( "theora_encode_YUVin FAILED!\n" );
      }
      memcpy( &vobj->last_frame_time, &data_buffer->fill_time, sizeof( struct timeval ) );

      vobj->last_frame = data_buffer;
      
      theora_encode_packetout( &vobj->th, 0, &vobj->op );
      ogg_stream_packetin( &vobj->os, &vobj->op );
/*       printf( "= v: %f   a: %f\n", videopos, audiopos ); */
      gotpage = 0;
      while( ogg_stream_pageout( &vobj->os, &og ) )
      {
	 double gt;
	 fwrite( og.header, og.header_len, 1, vobj->f );
	 fwrite( og.body, og.body_len, 1, vobj->f );

	 gt = theora_granule_time( &vobj->th, ogg_page_granulepos( &og ) );
	 if( gt < 0 )
	 {
	    continue;
	 }
	 
	 gotpage = 1;
	 videopos = gt;
/* 	 printf( "THEORA: %f\n", videopos ); */
      }
      if( !gotpage )
      {
	 videopos += vobj->frame_interval / 1000000.0f;
      }
   }

   if( vobj->last_frame )
   {
      // encode again to set eos
      unicap_data_buffer_t *last_data_buffer;
      ogg_page og;
      ogg_packet op;

#if HAVE_ALSA      
      if( vobj->audio && !vobj->async_audio_encoding )
      {
	 audiopos = fetch_and_process_audio( vobj, audiopos );
	 vorbis_analysis_wrote( &vobj->vd, 0 );
	 while( vorbis_analysis_blockout( &vobj->vd, &vobj->vb ) == 1 )
	 {
	    vorbis_analysis( &vobj->vb, NULL );
	    vorbis_bitrate_addblock( &vobj->vb );
	    while( vorbis_bitrate_flushpacket( &vobj->vd, &op ) )
	    {
	       ogg_stream_packetin( &vobj->vo, &op );
	    }
	 }
	 while( ogg_stream_pageout( &vobj->vo, &og ) )
	 {
	    fwrite( og.header, og.header_len, 1, vobj->f );
	    fwrite( og.body, og.body_len, 1, vobj->f );
	 }
      }
      else if( vobj->audio )
      {
	 audiopos = fetch_and_process_audio( vobj, audiopos );
      }
#endif

      last_data_buffer = vobj->last_frame;
      if( vobj->downsize > 1 || vobj->requires_resizing_frames )
      {
	 yuv.y = ds_y_buffer;
	 yuv.u = ds_u_buffer;
	 yuv.v = ds_v_buffer;
      }
      else
      {
         yuv.y = last_data_buffer->data;
         yuv.u = last_data_buffer->data + ( yuv.y_stride * yuv.y_height );
         yuv.v = yuv.u + ( yuv.uv_stride * yuv.uv_height );
      }
      if( theora_encode_YUVin( &vobj->th, &yuv ) )
      {
	 TRACE( "theora_encode_YUVin FAILED!\n" );
      }
      theora_encode_packetout( &vobj->th, 1, &vobj->op );
      ogg_stream_packetin( &vobj->os, &vobj->op );
      while( ogg_stream_pageout( &vobj->os, &og ) )
      {
/* 	 printf( "THEORA: %f\n", theora_granule_time( &vobj->th, ogg_page_granulepos( &og ) ) ); */
	 fwrite( og.header, og.header_len, 1, vobj->f );
	 fwrite( og.body, og.body_len, 1, vobj->f );
      }
      last_data_buffer->flags &= ~UNICAP_FLAGS_BUFFER_LOCKED;
      sem_wait( &vobj->lock );
      g_queue_push_head( vobj->empty_queue, vobj->last_frame );
      sem_post( &vobj->lock );

      vobj->last_frame = NULL;
   }

   return NULL;
}

static void copy_yuv( unsigned char *dst, yuv_buffer *yuv, theora_info *ti )
{
   int y;
   unsigned char *yoff;
   unsigned char *uvoff;
   unsigned char *dstoff;
   int crop_offset;
   
   dstoff = dst;
   crop_offset = ti->offset_x + yuv->y_stride * ti->offset_y;
   yoff = yuv->y + crop_offset;
   
   for( y = 0; y < yuv->y_height; y++ )
   {
      memcpy( dstoff, yoff, yuv->y_width );
      dstoff += yuv->y_width;
      yoff += yuv->y_stride;
   }

   crop_offset = ( ti->offset_x / 2 ) + ( yuv->uv_stride ) * ( ti->offset_y / 2 );
   uvoff = yuv->u + crop_offset;

   for( y = 0; y < yuv->uv_height; y++ )
   {
      memcpy( dstoff, uvoff, yuv->uv_width );
      dstoff += yuv->uv_width;
      uvoff += yuv->uv_stride;
   }
   
   uvoff = yuv->v;
   
   for( y = 0; y < yuv->uv_height; y++ )
   {
      memcpy( dstoff, uvoff, yuv->uv_width );
      dstoff += yuv->uv_width;
      uvoff += yuv->uv_stride;
   }
}

// Video Playback disabled - see ucview_videoplay_plugin on how to play back video files
#if 0

static void *ucil_theora_worker_thread( ucil_theora_input_file_object_t *vobj )
{
   unicap_data_buffer_t new_frame_buffer;

   struct timeval ltime;
   int eos = 0;

   unicap_copy_format( &new_frame_buffer.format, &vobj->format );
   new_frame_buffer.type = UNICAP_BUFFER_TYPE_SYSTEM;
   new_frame_buffer.buffer_size = new_frame_buffer.format.buffer_size;
   new_frame_buffer.data = malloc( new_frame_buffer.format.buffer_size );

   gettimeofday( &ltime, NULL );
   
   while( !vobj->quit_capture_thread )
   {
      struct timespec abs_timeout;
      struct timeval  ctime;
      GList *entry;
      ogg_page og;
      ogg_packet op;
      size_t bytes;

      int buffer_ready = 0;
      


      if( !eos && ( ogg_stream_packetout( &vobj->os, &op ) > 0 ) )
      {
	 yuv_buffer yuv;

	 theora_decode_packetin( &vobj->th, &op );
	 theora_decode_YUVout( &vobj->th, &yuv );
	 copy_yuv( new_frame_buffer.data, &yuv, &vobj->ti );

	 buffer_ready = 1;
      } 
      else if( !eos )
      {
	 bytes = buffer_data( vobj->f, &vobj->oy );      
	 if( !bytes )
	 {
	    TRACE( "End of stream\n" );
	    eos = 1;
	    
	 }
	 
	 while( ogg_sync_pageout( &vobj->oy, &og ) > 0 )
	 {
	    ogg_stream_pagein( &vobj->os, &og );
	 }
	 continue;
      }
      else
      {
	 buffer_ready = 1;
      }

      gettimeofday( &ctime, NULL );
      abs_timeout.tv_sec = ctime.tv_sec + 1;
      abs_timeout.tv_nsec = ctime.tv_usec * 1000;      
      if( sem_timedwait( &vobj->sema, &abs_timeout ) )
      {
	 TRACE( "SEM_WAIT FAILED\n" );
	 continue;
      }

      if( buffer_ready && vobj->event_callback )
      {
	 vobj->event_callback( vobj->event_unicap_handle, UNICAP_EVENT_NEW_FRAME, &new_frame_buffer );
	 TRACE( "New frame\n" );
      }
      
      unicap_data_buffer_t *data_buffer = g_queue_pop_head( vobj->in_queue );
      if( data_buffer )
      {
	 unicap_copy_format( &data_buffer->format, &vobj->format );
	 memcpy( data_buffer->data, new_frame_buffer.data, vobj->format.buffer_size );
	 
	 g_queue_push_tail( vobj->out_queue, data_buffer );
      }

      sem_post( &vobj->sema );
      
      if( buffer_ready )
      {
	 gettimeofday( &ctime, NULL );
	 if( ctime.tv_usec < ltime.tv_usec )
	 {
	    ctime.tv_usec += 1000000;
	    ctime.tv_sec -= 1;
	 }
	 
	 ctime.tv_usec -= ltime.tv_usec;
	 ctime.tv_sec -= ltime.tv_sec;
	 
	 if( ( ctime.tv_sec == 0 ) &&
	     ( ctime.tv_usec < vobj->frame_intervall ) )
	 {
	    usleep( vobj->frame_intervall - ctime.tv_usec );
	 }
      
	 gettimeofday( &ltime, NULL );
      }
   }

   free( new_frame_buffer.data );
   return NULL;
}


static unicap_status_t theoracpi_reenumerate_formats( ucil_theora_input_file_object_t vobj, int *count )
{
   *count = 1;

   return STATUS_SUCCESS;
}

static unicap_status_t theoracpi_enumerate_formats( ucil_theora_input_file_object_t *vobj, unicap_format_t *format, int index )
{
   unicap_status_t status = STATUS_NO_MATCH;
   if( index == 0 )
   {
      unicap_copy_format( format, &vobj->format );
      status = STATUS_SUCCESS;
   }
   
   return status;
}

static unicap_status_t theoracpi_set_format( ucil_theora_input_file_object_t *vobj, unicap_format_t *format )
{
   unicap_status_t status = STATUS_SUCCESS;
   if( ( format->size.width != vobj->format.size.width ) || 
       ( format->size.height != vobj->format.size.height ) ||
       ( format->bpp != vobj->format.bpp ) )
   {
      char buffer[1024];
      size_t size = 1024;

      unicap_describe_format( format, buffer, &size );
      TRACE( "Could not set format: %s\n", buffer );
      size = 1024;
      unicap_describe_format( &vobj->format, buffer, &size );
      TRACE( "Stored: %s\n" );
      status = STATUS_FAILURE;
   }   
   
   return status;
}

static unicap_status_t theoracpi_get_format( ucil_theora_input_file_object_t *vobj, unicap_format_t *format )
{
   unicap_copy_format( format, &vobj->format );
   return STATUS_SUCCESS;
}

static unicap_status_t theoracpi_reenumerate_properties( ucil_theora_input_file_object_t *vobj, int *count )
{
   *count = 0;
   return STATUS_SUCCESS;
}

static unicap_status_t theoracpi_enumerate_properties( ucil_theora_input_file_object_t *vobj, unicap_property_t *property, int index )
{
   return STATUS_NO_MATCH;
}

static unicap_status_t theoracpi_set_property( ucil_theora_input_file_object_t *vobj, unicap_property_t *property )
{
   return STATUS_FAILURE;
}

static unicap_status_t theoracpi_get_property( ucil_theora_input_file_object_t *vobj, unicap_property_t *property )
{
   return STATUS_FAILURE;
}

static unicap_status_t theoracpi_capture_start( ucil_theora_input_file_object_t *vobj )
{
   unicap_status_t status = STATUS_SUCCESS;
   
   if( pthread_create( &vobj->worker_thread, NULL, (void*(*)(void*))ucil_theora_worker_thread, vobj ) )
   {
      TRACE( "Failed to create worker thread!\n" );
      return STATUS_FAILURE;
   }

   return status;
}

static unicap_status_t theoracpi_capture_stop( ucil_theora_input_file_object_t *vobj )
{
   int res;

   vobj->quit_capture_thread = 1;
   res = pthread_join( vobj->worker_thread, NULL );

   return ( res == 0 ) ? STATUS_SUCCESS : STATUS_FAILURE;
}

static unicap_status_t theoracpi_queue_buffer( ucil_theora_input_file_object_t *vobj, unicap_data_buffer_t *buffer )
{
   unicap_status_t status = STATUS_SUCCESS;
   
   g_queue_push_tail( vobj->in_queue, buffer );

   return status;
}

static unicap_status_t theoracpi_dequeue_buffer( ucil_theora_input_file_object_t *vobj, unicap_data_buffer_t **buffer )
{
   unicap_status_t status = STATUS_SUCCESS;
   return status;
}

static unicap_status_t theoracpi_wait_buffer( ucil_theora_input_file_object_t *vobj, unicap_data_buffer_t **buffer )
{
   unicap_status_t status = STATUS_SUCCESS;
   return status;
}

static unicap_status_t theoracpi_poll_buffer( ucil_theora_input_file_object_t *vobj, int *count )
{
   *count = 1;
   return STATUS_SUCCESS;
}

static unicap_status_t theoracpi_set_event_notify( ucil_theora_input_file_object_t *vobj, unicap_event_callback_t func, unicap_handle_t handle )
{
   vobj->event_callback = func;
   vobj->event_unicap_handle = handle;

   return STATUS_SUCCESS;
}

#endif

static void encode_parse_parameters( ucil_theora_video_file_object_t *vobj, guint n_parameters, GParameter *parameters )
{
   int i;
   
   for( i = 0; i < n_parameters; i++ )
   {
      if( !strcmp( parameters[i].name, "fill_frames" ) )
      {
	 vobj->fill_frames = g_value_get_int( &parameters[i].value );
	 TRACE( "fill_frames: %d\n", vobj->fill_frames );
      }
      else if( !strcmp( parameters[i].name, "fps" ) )
      {
	 unsigned int num;
	 double fps;
	 
	 fps = g_value_get_double( &parameters[i].value );
	 num = fps * 1000000;
	             
	 vobj->ti.fps_numerator = num;
	 vobj->ti.fps_denominator = 1000000;

	 // The current frame interval in us / frame is stored in the frame_interval variable 
	 vobj->frame_interval = 1000000 / fps;
      }
      else if( !strcmp( parameters[i].name, "quality" ) )
      {
	 vobj->ti.quality = g_value_get_int( &parameters[i].value );
      }
      else if( !strcmp( parameters[i].name, "bitrate" ) )
      {
	 int bitrate;
	 
	 bitrate = g_value_get_int( &parameters[i].value );
	 
	 vobj->ti.target_bitrate = bitrate;
	 vobj->ti.keyframe_data_target_bitrate = bitrate * 2;
      }
      else if( !strcmp( parameters[i].name, "encode_frame_cb" ) )
      {
	 vobj->encode_frame_cb = (unicap_new_frame_callback_t)g_value_get_pointer( &parameters[i].value );
      }
      else if( !strcmp( parameters[i].name, "encode_frame_cb_data" ) )
      {
	 vobj->encode_frame_cb_data = (void*)g_value_get_pointer( &parameters[i].value );
      }
      else if( !strcmp( parameters[i].name, "audio" ) )
      {
	 vobj->audio = g_value_get_int( &parameters[i].value );
	 if( vobj->audio )
	 {
	    vobj->fill_frames = 1;
	 }
      }
      else if( !strcmp( parameters[i].name, "audio_card" ) )
      {
	 strncpy( vobj->audio_card, g_value_get_string( &parameters[i].value ), sizeof( vobj->audio_card ) );
      }
      else if( !strcmp( parameters[i].name, "audio_rate" ) )
      {
	 vobj->audio_rate = g_value_get_int( &parameters[i].value );
      }
      else if( !strcmp( parameters[i].name, "block" ) )
      {
	 vobj->blocking_mode = g_value_get_int( &parameters[i].value );
      }
      else if( !strcmp( parameters[i].name, "nocopy" ) )
      {
	 vobj->nocopy = g_value_get_int( &parameters[i].value );
      }
      else if( !strcmp( parameters[i].name, "async_audio_encoding" ) )
      {
	 vobj->async_audio_encoding = g_value_get_int( &parameters[i].value );
      }
      else if( !strcmp( parameters[i].name, "downsize" ) )
      {
	 int ds = g_value_get_int( &parameters[i].value );
	 if( ds > 1 )
	 {
	    vobj->downsize = ds;
	 }
      }
      else    
      {
	 TRACE( "Unknown arg: %s\n", parameters[i].name );
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
      
      if( !strcmp( arg, "fill_frames" ) ||
	  !strcmp( arg, "quality" ) ||
	  !strcmp( arg, "bitrate" ) ||
	  !strcmp( arg, "audio" ) ||
	  !strcmp( arg, "audio_rate" ) ||
	  !strcmp( arg, "block" ) ||
	  !strcmp( arg, "nocopy" ) ||
	  !strcmp( arg, "async_audio_encoding" ) ||
	  !strcmp( arg, "downsize" ) )
      {
	 g_value_init( &tmp[n].value, G_TYPE_INT );
	 g_value_set_int( &tmp[n].value, va_arg( ap, int ) );
      }
      else if( !strcmp( arg, "fps" ) )
      {
	 g_value_init( &tmp[n].value, G_TYPE_DOUBLE );
	 g_value_set_double( &tmp[n].value, va_arg( ap, double ) );
      }
      else if( !strcmp( arg, "encode_frame_cb" ) )
      {
	 g_value_init( &tmp[n].value, G_TYPE_POINTER );
	 g_value_set_pointer( &tmp[n].value, va_arg( ap, gpointer ) );
	 n++;
	 tmp[n].name = g_strdup( "encode_frame_cb_data" );
	 g_value_init( &tmp[n].value, G_TYPE_POINTER );
	 g_value_set_pointer( &tmp[n].value, va_arg( ap, gpointer ) );
      }
      else if( !strcmp( arg, "audio_card" ) )
      {
	 g_value_init( &tmp[n].value, G_TYPE_STRING );
	 g_value_set_string( &tmp[n].value, va_arg( ap, char *) );
      }
      else    
      {
	 TRACE( "Unknown arg: %s\n", arg );
	 break;
      }
      
      n++;
   }

   *parameters = g_new( GParameter, n );
   *n_parameters = n;
   memcpy( *parameters, tmp, sizeof( GParameter ) * n );
   g_free( tmp );
}


ucil_theora_video_file_object_t *ucil_theora_create_video_file( const char *path, unicap_format_t *format, const char *codec, va_list ap)
{
   GParameter *parameters;
   guint n_parameters;
   ucil_theora_video_file_object_t *vobj;
   
   encode_va_list_to_parameters( ap, &n_parameters, &parameters );
   vobj = ucil_theora_create_video_filev( path, format, codec, n_parameters, parameters );
   g_free( parameters );
   
   return vobj;
}

ucil_theora_video_file_object_t *ucil_theora_create_video_filev( const char *path, unicap_format_t *format, const char *codec, guint n_parameters, GParameter *parameters)
{
   ucil_theora_video_file_object_t *vobj;   
   ogg_packet op;
   ogg_page og;
   theora_comment tc;

#if HAVE_ALSA
   vorbis_comment vc;
#endif
   int i;
   
   vobj = malloc( sizeof( ucil_theora_video_file_object_t ) );
   memset( vobj, 0x0, sizeof( ucil_theora_video_file_object_t ) );


   if( ogg_stream_init( &vobj->os, 1 ) != 0 )
   {
      TRACE( "ogg_stream_init failed!\n" );
      free( vobj );
      return NULL;
   }

/*    ucil_alsa_list_cards(); */
   

   theora_info_init( &vobj->ti );

   vobj->ti.width = format->size.width;
   vobj->ti.height = format->size.height;
   vobj->ti.frame_width = format->size.width;
   vobj->ti.frame_height = format->size.height;
   vobj->ti.offset_x = 0;
   vobj->ti.offset_y = 0;
   vobj->ti.fps_numerator = 30000000;
   vobj->ti.fps_denominator = 1000000;
   vobj->ti.aspect_numerator = 1;//format->size.width;
   vobj->ti.aspect_denominator = 1;//format->size.height;
   vobj->ti.colorspace = OC_CS_UNSPECIFIED;
   vobj->ti.pixelformat = OC_PF_420;
   vobj->ti.target_bitrate = 800000;
   vobj->ti.quality = 16;
   vobj->ti.quick_p = 1;
   vobj->ti.dropframes_p = 0;
   vobj->ti.keyframe_auto_p = 1;
   vobj->ti.keyframe_frequency=64;
   vobj->ti.keyframe_frequency_force=64;
   vobj->ti.keyframe_data_target_bitrate=800000*2;
   vobj->ti.keyframe_auto_threshold=80;
   vobj->ti.keyframe_mindistance=8;
   vobj->ti.noise_sensitivity=1;

   vobj->audio = 0;
   vobj->audio_rate = 44100;
   vobj->vorbis_bitrate = 128000;

   vobj->downsize = 1;

   vobj->frame_interval = 33333;

   strcpy( vobj->audio_card, "hw: 0,0" );

   encode_parse_parameters( vobj, n_parameters, parameters );

   if( vobj->downsize != 1 )
   {
      vobj->ti.width /= vobj->downsize;
      vobj->ti.height /= vobj->downsize;
      vobj->ti.frame_width /= vobj->downsize;
      vobj->ti.frame_height /= vobj->downsize;
   }
   
   // The width and height parameter for the theora-encoder 
   // need to be multiple of 16, ensure this by croping the 
   // image!
   if ( vobj->ti.frame_width & 0xF || vobj->ti.frame_height & 0xF )
   {
      vobj->ti.frame_width = vobj->ti.frame_width & ~0xF;
      vobj->ti.frame_height = vobj->ti.frame_height & ~0xF;
      vobj->ti.width = vobj->ti.width & ~0xF;
      vobj->ti.height = vobj->ti.height & ~0xF;
      vobj->requires_resizing_frames = 1;
   }
 

   if( !vobj->audio )
   {
      vobj->async_audio_encoding = 0;
   }

   if( !vobj->async_audio_encoding )
   {
      vobj->f = fopen( path, "w" );
      if( !vobj->f )
      {
	 free( vobj );
	 return NULL;
      }
   }
   else
   {
      gchar *vpath;
      gchar *apath;
      
      vpath = g_strconcat( path, ".vtmp", NULL );
      apath = g_strconcat( path, ".atmp", NULL );
      vobj->f = fopen( vpath, "w" );
      if( !vobj->f )
      {
	 free( vobj );
	 return NULL;
      }
      vobj->audiof = fopen( apath, "w" );
      if( !vobj->audiof )
      {
	 fclose( vobj->f );
	 free( vobj );
	 return NULL;
      }
      
      g_free( vpath );
      g_free( apath );
   }

#if HAVE_ALSA
   if( vobj->audio && !vobj->async_audio_encoding )
   {
      if( ogg_stream_init( &vobj->vo, 2 ) != 0 )
      {
	 TRACE( "ogg_stream_init for vorbis stream failed!\n" );
	 free( vobj );
	 return NULL;
      }
      vobj->audio_data = ucil_alsa_init( vobj->audio_card, vobj->audio_rate );
      if( !vobj->audio_data )
      {
	 /* the error message has been already emitted */
	 free( vobj );
	 return NULL;
      }
      
      vorbis_info_init( &vobj->vi );
      if( vorbis_encode_init( &vobj->vi, 2, vobj->audio_rate, -1, vobj->vorbis_bitrate, -1 ) )
      {
	 TRACE( "Failed to init vorbis encoder\n" );
	 free( vobj );
	 return NULL;
      }
      
      vorbis_comment_init( &vc );
      vorbis_analysis_init( &vobj->vd, &vobj->vi );
      vorbis_block_init( &vobj->vd, &vobj->vb );
   }
   else if( vobj->audio )
   {
      vobj->audio_data = ucil_alsa_init( vobj->audio_card, vobj->audio_rate );
      if( !vobj->audio_data )
      {
	 /* the error message has been already emitted */
	 free( vobj );
	 return NULL;
      }
   }
   
#endif

   theora_encode_init( &vobj->th, &vobj->ti );   

   vobj->full_queue = g_queue_new();
   vobj->empty_queue = g_queue_new();

   unicap_copy_format( &vobj->format, format );
   if( !vobj->nocopy )
   {
      for( i = 0; i < ENCODEBUFFER_SIZE; i++ )
      {
	 unicap_data_buffer_t *data_buffer;
	 
	 data_buffer = malloc( sizeof( unicap_data_buffer_t ) );
	 unicap_copy_format( &data_buffer->format, format );
	 data_buffer->format.fourcc = UCIL_FOURCC( 'I', '4', '2', '0' );
	 data_buffer->format.bpp = 16;
	 data_buffer->buffer_size = format->size.width * format->size.height * 2;
	 
	 data_buffer->data = malloc( format->size.width * format->size.height * 1.5 );
	 data_buffer->buffer_size = format->size.width * format->size.height * 1.5;
	 
	 g_queue_push_head( vobj->empty_queue, data_buffer );
      }
   }
   
   theora_encode_header( &vobj->th, &op );
   ogg_stream_packetin( &vobj->os, &op );
   /* first packet will get its own page automatically */
   if( ogg_stream_pageout( &vobj->os, &og ) != 1 )
   {
      TRACE( "Internal Ogg error\n" );
      return NULL;
   }
   fwrite( og.header, 1, og.header_len, vobj->f );
   fwrite( og.body, 1, og.body_len, vobj->f );
   theora_comment_init(&tc);
   theora_encode_comment(&tc,&op);
   ogg_stream_packetin(&vobj->os,&op);
   theora_encode_tables(&vobj->th,&op);
   ogg_stream_packetin(&vobj->os,&op);


#if HAVE_ALSA
   if( vobj->audio && !vobj->async_audio_encoding )
   {
      ogg_packet header;
      ogg_packet header_comm;
      ogg_packet header_code;
      
      vorbis_analysis_headerout( &vobj->vd, &vc, &header, &header_comm, &header_code );
      ogg_stream_packetin( &vobj->vo, &header ); /* automatically placed in its own
                                         page */
      if( ogg_stream_pageout( &vobj->vo, &og ) != 1 )
      {
	 TRACE( "Internal Ogg library error.\n" );
      }
      fwrite( og.header, 1, og.header_len, vobj->f );
      fwrite( og.body, 1, og.body_len, vobj->f );
      
      /* remaining vorbis header packets */
      ogg_stream_packetin( &vobj->vo, &header_comm );
      ogg_stream_packetin( &vobj->vo, &header_code );
   }
#endif
      
   while( ogg_stream_flush( &vobj->os, &og ) > 0 )
   {
      fwrite( og.header, 1, og.header_len, vobj->f );
      fwrite( og.body, 1, og.body_len, vobj->f );
   }   

#if HAVE_ALSA
   if( vobj->audio && !vobj->async_audio_encoding )
   {
      while( ogg_stream_flush( &vobj->vo, &og ) > 0 )
      {
	 fwrite( og.header, 1, og.header_len, vobj->f );
	 fwrite( og.body, 1, og.body_len, vobj->f );
      }   
   }
#endif

   if( sem_init( &vobj->sema, 0, 1 ) )
   {
      TRACE( "sem_init failed!\n" );
   }

   if( sem_init( &vobj->lock, 0, 1 ) ){
      TRACE( "sem_init failed!\n" );
   }
      

   vobj->quit_thread = 0;
   if( pthread_create( &vobj->encoder_thread, NULL, (void*(*)(void*))ucil_theora_encode_thread, vobj ) )
   {
      TRACE( "Failed to create encoder thread!\n " );
   }
   
   return vobj;
}

unicap_status_t ucil_theora_encode_frame( ucil_theora_video_file_object_t *vobj, unicap_data_buffer_t *buffer )
{
   unicap_status_t status = STATUS_SUCCESS;
   
   buffer->flags |= UNICAP_FLAGS_BUFFER_LOCKED;
   
   if( vobj->nocopy ){
      sem_wait( &vobj->lock );
      g_queue_push_tail( vobj->full_queue, buffer );
      sem_post( &vobj->lock );
   }
   else
   {
      unicap_data_buffer_t *dst_buffer;
      sem_wait( &vobj->lock );
      dst_buffer = g_queue_pop_head( vobj->empty_queue );
      sem_post( &vobj->lock );
      if( !dst_buffer && vobj->blocking_mode )
      {
	 while( !(dst_buffer = g_queue_pop_head( vobj->empty_queue )) )
	 {
	    usleep( 1000 );
	 }
      }
      
      if( !dst_buffer )
      {
	 status = STATUS_FAILURE;
      }
      else
      {
	 ucil_convert_buffer( dst_buffer, buffer );
	 memcpy( &dst_buffer->fill_time, &buffer->fill_time, sizeof( struct timeval ) );
	 sem_wait( &vobj->lock );
	 g_queue_push_tail( vobj->full_queue, dst_buffer );
	 sem_post( &vobj->lock );
	 buffer->flags |= ~UNICAP_FLAGS_BUFFER_LOCKED;
      }
   }
   
   return status;
}

unicap_status_t ucil_theora_close_video_file( ucil_theora_video_file_object_t *vobj )
{
   if( !vobj->nocopy )
   {
      while( g_queue_get_length( vobj->full_queue ) )
      {
	 usleep( 1000 );
      }
   }

   vobj->quit_thread = 1;
   pthread_join( vobj->encoder_thread, NULL );

   if( !vobj->nocopy )
   {
      g_queue_free( vobj->full_queue );
      g_queue_free( vobj->empty_queue );
   }
   else
   {
      unicap_data_buffer_t *data_buffer;
      for( data_buffer = g_queue_pop_head( vobj->full_queue ); data_buffer; data_buffer = g_queue_pop_head( vobj->full_queue ) )
      {
	 data_buffer->flags &= ~UNICAP_FLAGS_BUFFER_LOCKED;
      }
      for( data_buffer = g_queue_pop_head( vobj->empty_queue ); data_buffer; data_buffer = g_queue_pop_head( vobj->empty_queue ) )
      {
	 data_buffer->flags &= ~UNICAP_FLAGS_BUFFER_LOCKED;
      }
      g_queue_free( vobj->full_queue );
      g_queue_free( vobj->empty_queue );
   }

   fclose( vobj->f );

   theora_info_clear( &vobj->ti );
#if HAVE_ALSA
   if( vobj->audio )
   {
      ucil_alsa_close( vobj->audio_data );
   }
#endif

   free( vobj );
   
   return STATUS_SUCCESS;
}

#if 0
unicap_status_t ucil_theora_open_video_file( unicap_handle_t *unicap_handle, char *filename )
{
   struct _unicap_handle *handle;
   ogg_page og;
   ogg_packet op;
   theora_comment tc;
   int pkt = 0;
   int i;
   ucil_theora_input_file_object_t *vobj;
   
   
   handle = malloc( sizeof( struct _unicap_handle ) );
   if( !handle )
   {
      return STATUS_FAILURE;
   }
   // initialize data structure
   memset( handle, 0, sizeof( struct _unicap_handle ) );
   handle->cb_info = malloc( sizeof( struct _unicap_callback_info ) * UNICAP_EVENT_LAST );
   for( i = 0; i < UNICAP_EVENT_LAST; i++ )
   {
      handle->cb_info[ i ].func = NULL;
      handle->cb_info[ i ].user_ptr = NULL;
   }

   handle->sem_key = -1;
   handle->sem_id = -1;
   
   handle->ref_count = malloc( sizeof( int ) );
   *(handle->ref_count) = 1;

   vobj = malloc( sizeof( ucil_theora_input_file_object_t ) );
   if( !vobj )
   {
      free( handle );
      return STATUS_FAILURE;
   }   

   memset( vobj, 0x0, sizeof( ucil_theora_input_file_object_t ) );


   vobj->f = fopen( filename, "r" );
   if( !vobj->f )
   {
      free( vobj );
      free( handle );
      return STATUS_FAILURE;
   }

   ogg_sync_init( &vobj->oy );
   theora_comment_init( &tc );
   theora_info_init( &vobj->ti );

   
   for( pkt = 0; pkt < 3;  )
   {   
      
      if( ogg_sync_pageout( &vobj->oy, &og ) > 0 )
      {
	 if( pkt == 0 )
	 {
	    if( !ogg_page_bos( &og ) )
	    {
	       TRACE( "Not a valid ogg stream\n" );
	       // TODO: fix page leak
	       return STATUS_FAILURE;
	    }
	    ogg_stream_init( &vobj->os, ogg_page_serialno( &og ) );
	 }
	 if( ogg_stream_pagein( &vobj->os, &og ) < 0 )
	 {
	    TRACE( "Failed to decode first page\n" );
	    fclose( vobj->f );
	    return STATUS_FAILURE;
	 }

	 while( ( pkt < 3 ) && ogg_stream_packetout( &vobj->os, &op ) > 0 )
	 {
	    if( theora_decode_header( &vobj->ti, &tc, &op ) != 0 )
	    {
	       TRACE( "Failed to decode header packet\n" );
	       fclose( vobj->f );
	       return STATUS_FAILURE;
	    }
	    pkt ++;
	 }
      }
      else
      {
	 int bytes;
	 bytes = buffer_data( vobj->f, &vobj->oy );
	 
	 if( bytes == 0 )
	 {
	    TRACE( "End of file while parsing headers\n" );
	    fclose( vobj->f );
	    return STATUS_FAILURE;
	 }  
      }
   }
   
   theora_comment_clear( &tc );

   unicap_void_format( &vobj->format );   
   strcpy( vobj->format.identifier, "UCIL default format" );
   vobj->format.size.width = vobj->ti.frame_width;
   vobj->format.size.height = vobj->ti.frame_height;
   switch( vobj->ti.pixelformat )
   {
      case OC_PF_420: 
	 vobj->format.fourcc = UCIL_FOURCC( 'I', '4', '2', '0' );
	 vobj->format.bpp = 12;
	 break;
	 
      case OC_PF_422:
	 vobj->format.fourcc = UCIL_FOURCC( 'U', 'Y', 'V', 'Y' );
	 vobj->format.bpp = 16;
	 break;
	 
      case OC_PF_444:
	 vobj->format.fourcc = UCIL_FOURCC( 'I', '4', '4', '4' );
	 vobj->format.bpp = 24;
	 break;

      default:
	 break;
   }

   vobj->format.buffer_size = vobj->format.size.width * vobj->format.size.height * vobj->format.bpp / 8;
   vobj->format.buffer_type = UNICAP_BUFFER_TYPE_SYSTEM;	    

   theora_decode_init( &vobj->th, &vobj->ti );

   handle->cpi.cpi_register = NULL;
   handle->cpi.cpi_version = 1<<16;
   handle->cpi.cpi_capabilities = 0x3ffff;
   handle->cpi.cpi_enumerate_devices = NULL;
   handle->cpi.cpi_open = NULL;
   handle->cpi.cpi_close = NULL;
   handle->cpi.cpi_reenumerate_formats = (cpi_reenumerate_formats_t ) theoracpi_reenumerate_formats;
   handle->cpi.cpi_enumerate_formats = ( cpi_enumerate_formats_t ) theoracpi_enumerate_formats;
   handle->cpi.cpi_set_format = ( cpi_set_format_t ) theoracpi_set_format;
   handle->cpi.cpi_get_format = ( cpi_get_format_t ) theoracpi_get_format;
   handle->cpi.cpi_reenumerate_properties = ( cpi_reenumerate_properties_t ) theoracpi_reenumerate_properties;
   handle->cpi.cpi_enumerate_properties = ( cpi_enumerate_properties_t ) theoracpi_enumerate_properties;
   handle->cpi.cpi_get_property = ( cpi_get_property_t ) theoracpi_get_property;
   handle->cpi.cpi_set_property = ( cpi_set_property_t ) theoracpi_set_property;
   handle->cpi.cpi_capture_start = ( cpi_capture_start_t ) theoracpi_capture_start;
   handle->cpi.cpi_capture_stop = ( cpi_capture_stop_t ) theoracpi_capture_stop;
   handle->cpi.cpi_queue_buffer = ( cpi_queue_buffer_t ) theoracpi_queue_buffer;
   handle->cpi.cpi_dequeue_buffer = ( cpi_dequeue_buffer_t ) theoracpi_dequeue_buffer;
   handle->cpi.cpi_wait_buffer = ( cpi_wait_buffer_t ) theoracpi_wait_buffer;
   handle->cpi.cpi_poll_buffer = ( cpi_poll_buffer_t ) theoracpi_poll_buffer;
   handle->cpi.cpi_set_event_notify = ( cpi_set_event_notify_t ) theoracpi_set_event_notify;
   handle->cpi_data = vobj;

   unicap_cpi_register_event_notification( theoracpi_set_event_notify, vobj, handle );   

   vobj->frame_intervall = 1000000 / ( vobj->ti.fps_numerator / vobj->ti.fps_denominator );

   sem_init( &vobj->sema, 0, 1 );

   *unicap_handle = handle;

   vobj->in_queue = g_queue_new();
   vobj->out_queue = g_queue_new();

   return STATUS_SUCCESS;
}
#endif
