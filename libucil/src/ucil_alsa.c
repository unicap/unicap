/*
** ucil_alsa.c
** 
** Made by (Arne Caspari)
** Login   <arne@localhost>
** 
** Started on  Wed Aug 22 18:13:16 2007 Arne Caspari
*/

/*
    unicap
    Copyright (C) 2004-2007  Arne Caspari

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

#include "ucil_alsa.h"

#include <stdlib.h>
#include <asoundlib.h>
#include <glib-2.0/glib.h>
/* #if UCIL_DEBUG */
#define DEBUG
/* #endif */
#include "debug.h"

typedef struct alsa_data
{
      snd_pcm_t *capture_handle;

      short audio_buffer[ 44100 * 2 ]; // buffer up to 1 second of
      // data
      int buff_fill;
} alsa_data_t;

GList *ucil_alsa_list_cards()
{
   snd_ctl_card_info_t *info;
   snd_pcm_info_t *pcminfo;
   snd_ctl_t *handle;
   int card, err, dev;
   GList *list = NULL;
   
   snd_ctl_card_info_alloca (&info);
   snd_pcm_info_alloca (&pcminfo);

   snd_ctl_card_info_alloca (&info);
   snd_pcm_info_alloca (&pcminfo);
   card = -1;
   
   if (snd_card_next (&card) < 0 || card < 0) {
      TRACE( "no soundcard found\n" );
      /* no soundcard found */
      return NULL;
   }

   while (card >= 0) {
      char name[32];
      
      g_snprintf (name, sizeof (name), "default");
      if ((err = snd_ctl_open (&handle, name, 0)) < 0) {
	 goto next_card;
      }
      if ((err = snd_ctl_card_info (handle, info)) < 0) {
	 snd_ctl_close (handle);
	 goto next_card;
      }
      
      dev = -1;
      while (1) {
	 gchar *gst_device;
	 
	 snd_ctl_pcm_next_device (handle, &dev);
	 
	 if (dev < 0)
	    break;
	 snd_pcm_info_set_device (pcminfo, dev);
	 snd_pcm_info_set_subdevice (pcminfo, 0);
	 if ((err = snd_ctl_pcm_info (handle, pcminfo)) < 0) {
	    continue;
	 }
	 
	 gst_device = g_strdup_printf ("hw:%d,%d", card, dev);
	 list = g_list_append( list, gst_device );
      }
      snd_ctl_close (handle);
     next_card:
      if (snd_card_next (&card) < 0) {
	 break;
      }
   }

   return list;
}

void *ucil_alsa_init( char *dev, unsigned int rate )
{
   alsa_data_t *data = malloc( sizeof( alsa_data_t ) );
   snd_pcm_hw_params_t *hw_params;
   int err;
   unsigned int buffer_time = 1000000;
   int dir;

   data->buff_fill = 0;

   if( ( err = snd_pcm_open( &data->capture_handle, dev, SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK) ) < 0 ) 
   {
      TRACE( "cannot open audio device %s (%s)\n", dev, snd_strerror (err));
      goto err;
   } 

   if( ( err = snd_pcm_hw_params_malloc( &hw_params ) ) < 0 ) 
   {
      TRACE( "cannot allocate hardware parameter structure (%s)\n", snd_strerror( err ) );
      goto err;
   }
				 
   if( ( err = snd_pcm_hw_params_any( data->capture_handle, hw_params ) ) < 0 ) 
   {
      TRACE( "cannot initialize hardware parameter structure (%s)\n", snd_strerror( err ) );
      goto err;
   }
   
   if( ( err = snd_pcm_hw_params_set_access( data->capture_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED ) ) < 0 ) 
   {
      TRACE( "cannot set access type (%s)\n", snd_strerror( err ) );
      goto err;
   }
	
   if( ( err = snd_pcm_hw_params_set_format( data->capture_handle, hw_params, SND_PCM_FORMAT_S16_LE ) ) < 0 ) 
   {
      TRACE( "cannot set sample format (%s)\n", snd_strerror( err));
      goto err;
   }
	
   if( ( err = snd_pcm_hw_params_set_rate_near( data->capture_handle, hw_params, &rate, 0 ) ) < 0 ) 
   {
      TRACE( "cannot set sample rate (%s)\n", snd_strerror( err ) );
      goto err;
   }

   if( ( err = snd_pcm_hw_params_set_channels( data->capture_handle, hw_params, 2 ) ) < 0 ) 
   {
      TRACE( "cannot set channel count (%s)\n", snd_strerror( err ) );
      goto err;
   }

   if( ( err = snd_pcm_hw_params_set_buffer_time_near( data->capture_handle, hw_params, &buffer_time, &dir ) ) )
   {
      TRACE( "cannot set buffer time (%s)\n", snd_strerror( err ) );
      goto err;
   }

   if( ( err = snd_pcm_hw_params( data->capture_handle, hw_params ) ) < 0 ) 
   {
      TRACE( "cannot set parameters (%s)\n", snd_strerror( err ) );
      goto err;
   }
	
   snd_pcm_hw_params_free( hw_params );
	
   if( ( err = snd_pcm_prepare( data->capture_handle ) ) < 0 ) 
   {
      TRACE( "cannot prepare audio interface for use (%s)\n", snd_strerror (err) );
      goto err;
   }

   return data;
   
  err:
   free( data );
   return NULL;
	
}


long ucil_alsa_fill_audio_buffer( void *_data )
{
   alsa_data_t *data = _data;
   long nread;

   if( data->buff_fill >= ( sizeof( data->audio_buffer ) / sizeof( short ) - 1 ) )
   {
      return 0;
   }
   
   nread = snd_pcm_readi( data->capture_handle, data->audio_buffer + data->buff_fill, 
			  ( 44100 - 1 ) - data->buff_fill );
   if( nread > 0 )
   {
      data->buff_fill += nread;
/*       printf( "read: %d\n", nread ); */
   }
   else if( nread < 0 )
   {
      int err;
      if( nread == -EPIPE )
      {
	 if( ( err = snd_pcm_prepare( data->capture_handle ) ) < 0 ) 
	 {
	    TRACE( "cannot prepare audio interface for use (%s)\n", snd_strerror (err) );
	 }
      }
      else
      {
/* 	 perror( "read" ); */
      }
      nread = 0;
   }
   else
   {
      nread = 0;
   }
   
   return nread;
}

int ucil_alsa_get_audio_buffer( void *_data, short **buffer )
{
   int ret;
   alsa_data_t *data = _data;

   *buffer = data->audio_buffer;
   ret = data->buff_fill;
   data->buff_fill = 0;

   return ret;
}


void ucil_alsa_close( void *_data )
{
   alsa_data_t *data = _data;
   snd_pcm_close( data->capture_handle );
   free( data );
}

void ucil_alsa_wait( void *_data, int usec )
{
   alsa_data_t *data = _data;
   snd_pcm_wait( data->capture_handle, usec );
}
