/*
** ucil_theora.h
** 
** Made by Arne Caspari
** Login   <arne@arne-laptop>
** 
** Started on  Mon Mar 12 07:27:55 2007 Arne Caspari
** Last update Wed Mar 21 18:37:05 2007 Arne Caspari
*/

#ifndef   	UCIL_THEORA_H_
# define   	UCIL_THEORA_H_

#include <ogg/ogg.h>
#if HAVE_THEORA
#include <vorbis/vorbisenc.h>
#endif
#include <theora/theora.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>
#include <glib.h>
#include "ucil.h"
#include "unicap.h"

struct _ucil_theora_video_file_object
{
   ogg_stream_state os;
   theora_info ti;
   theora_state th;
   FILE *f;

   int blocking_mode;

   int downsize;

   unicap_format_t format;
      
   int fill_frames;
   unsigned long long frame_interval;
   struct timeval last_frame_time;
   struct timeval recording_start_time;
   int frame_count;
   ogg_packet op;

   pthread_t encoder_thread;
   int quit_thread;
   GQueue *full_queue;
   GQueue *empty_queue;
   unicap_data_buffer_t *last_frame;

   unicap_new_frame_callback_t encode_frame_cb;
   void *encode_frame_cb_data;

   vorbis_info      vi;
   vorbis_dsp_state vd;
   ogg_stream_state vo;
   vorbis_block     vb;
      
      
   int          audio;
   void        *audio_data;
   char         audio_card[64];
   unsigned int audio_rate;
   int          vorbis_bitrate;
   unsigned int total_samples;
   int          async_audio_encoding;
   FILE        *audiof;

   int nocopy;

   sem_t sema;      
   sem_t lock;
   
   int requires_resizing_frames;
};

typedef struct _ucil_theora_video_file_object ucil_theora_video_file_object_t;

ucil_theora_video_file_object_t *ucil_theora_create_video_file( const char *path, unicap_format_t *format, const char *codec, va_list ap);
ucil_theora_video_file_object_t *ucil_theora_create_video_filev( const char *path, unicap_format_t *format, const char *codec, guint n_parameters, GParameter *parameters);
unicap_status_t ucil_theora_encode_frame( ucil_theora_video_file_object_t *vobj, unicap_data_buffer_t *buffer );
unicap_status_t ucil_theora_close_video_file( ucil_theora_video_file_object_t *vobj );
gboolean ucil_theora_combine_av_file( const char *path, const char *codec, gboolean remove, ucil_processing_info_func_t procfunc, void *func_data, GError **error );


#endif 	    /* !UCIL_THEORA_H_ */
