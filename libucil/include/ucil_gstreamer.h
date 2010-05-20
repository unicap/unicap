#ifndef __UCIL_GSTREAMER_H__
#define __UCIL_GSTREAMER_H__

#include <unicap.h>
#include <gst/gst.h>

struct _ucil_gstreamer_video_file_object
{
   GstElement *pipeline;
   GstElement *appsrc;
 
   gchar *filename;

   unicap_format_t format;
   gchar *codec_name;

   gchar *audio_codec;

   gboolean record_audio;
   gboolean fill_frames;

   gboolean base_time_set;

   unicap_new_frame_callback_t encode_frame_cb;
   void *encode_frame_cb_data;

   // Parameters
   gint fps_numerator;
   gint fps_denominator;
   gint frame_interval;
   gint quality;
   gint bitrate;
   
};

typedef struct _ucil_gstreamer_video_file_object ucil_gstreamer_video_file_object_t;

ucil_gstreamer_video_file_object_t *ucil_gstreamer_create_video_file( const char *path, unicap_format_t *format, const char *codec, va_list ap);
ucil_gstreamer_video_file_object_t *ucil_gstreamer_create_video_filev( const char *path, unicap_format_t *format, const char *codec, guint n_parameters, GParameter *parameters);
unicap_status_t ucil_gstreamer_encode_frame( ucil_gstreamer_video_file_object_t *vobj, unicap_data_buffer_t *buffer );
unicap_status_t ucil_gstreamer_close_video_file( ucil_gstreamer_video_file_object_t *vobj );





#endif//__UCIL_GSTREAMER_H__
