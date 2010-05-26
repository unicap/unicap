#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/video/video.h>
#include <glib.h>
#include <unicap.h>
#include <ucil.h>
#include <ucil_gstreamer.h>
#include <string.h>

typedef void (*parse_parameters_func_t) (GstElement *encoder, ucil_gstreamer_video_file_object_t *vobj, gint n_parametes, GParameter *parameters );

struct _element_map
{
   gchar *idstring;
   gchar **name;
   parse_parameters_func_t parameters_func;
};

static gchar *mpeg2encoders[] = { 
#if ALLOW_GPL
   "ffenc_mpeg2video", 
#endif
   "mpeg2enc", NULL };
static gchar *x264encoders[] = { 
#if ALLOW_GPL
   "x264enc",
#endif
   NULL };
static gchar *rawencoders[] = { "identity", NULL };
static gchar *theoraencoders[] = { "theoraenc", NULL };
static gchar *vp8encoders[] = { "vp8enc", NULL };
   

static gboolean is_initialized = FALSE;


struct _element_map videnc_map[] = {
   {
      "mpeg2", 
      mpeg2encoders,
   },
   {
      "x264", 
      x264encoders,
   },
   {
      "raw", 
      rawencoders,
   },
   {
      "theora",
      theoraencoders,
   },
   {
      "vp8",
      vp8encoders,
   },
   {
      NULL, 
      NULL
   }   
};

static char *avimuxers[] = { "avimux", "ffmux_avi", NULL };
static char *oggmuxers[] = { "oggmux", NULL  };
static char *mkvmuxers[] = { "matroskamux", "ffmux_matroska", NULL  };
struct _element_map muxer_map[] = {
   {
      "avi", 
      avimuxers,
   },
   {
      "ogg",
      oggmuxers,
   },
   {
      "mkv",
      mkvmuxers,
   },
   {
      NULL,
      NULL
   }
};    

static char* mp3encoders[] = { "lame", NULL };
static char *vorbisencoders[] = { "vorbisenc", NULL };
   

struct _element_map audioenc_map[] = {
   {
      "mp3", 
      mp3encoders,
   },
   {
      "vorbis", 
      vorbisencoders, 
   },
   {
      NULL,
      NULL
   }      
};


static GstElement *create_audio_encoder( gchar *codec_name )
{
   int i;
   GstElement *encoder = NULL;
   
   for( i = 0; (encoder == NULL) && ( audioenc_map[i].idstring != NULL ); i++ ){
      if( !strcmp( audioenc_map[i].idstring, codec_name ) ){
	 int j;
	 for( j = 0; ( encoder == NULL ) && ( audioenc_map[i].name[j] != NULL ); j++ ){
	    encoder = gst_element_factory_make( audioenc_map[i].name[j], "audioencoder" );
	 }
      }
   }
   
   return encoder;
};

static gboolean create_encoder_and_muxer( gchar *codec_name, GstElement **_encoder, GstElement **_muxer )
{
   gchar **spl = g_strsplit( codec_name, "/", 2 );
   
   if( g_strv_length( spl ) != 2 ){
      g_strfreev( spl );
      return FALSE;
   }
   
   int i;
   GstElement *encoder = NULL;
   
   // find a working encoder by iterating through the element names in the videnc_map
   for (i = 0; (encoder == NULL ) && ( videnc_map[i].idstring != NULL ); i++ ){
      if( !strcmp( videnc_map[i].idstring, spl[1] ) ){
	 int j;
	 for( j = 0; ( encoder == NULL ) && ( videnc_map[i].name[j] != NULL ); j++ ){
	    encoder = gst_element_factory_make( videnc_map[i].name[j], "videoencoder" );
	 }
      }
   }
   
   if( encoder == NULL ){
      g_strfreev( spl );
      return FALSE;
   }

   GstElement *muxer = NULL;
   
   // now the same for a muxer
   for( i = 0; ( muxer == NULL ) && ( muxer_map[i].idstring != NULL ); i++ ){
      if( !strcmp( muxer_map[i].idstring, spl[0] ) ){
	 int j;
	 for( j = 0; ( muxer == NULL ) && ( muxer_map[i].name[j] != NULL ); j++ ){
	    muxer = gst_element_factory_make( muxer_map[i].name[j], "muxer" );
	 }
      }
   }
   
   g_strfreev( spl );

   if( muxer == NULL ){
      gst_object_unref( encoder );
      return FALSE;
   }
   
   *_encoder = encoder;
   *_muxer = muxer;
   
   return TRUE;
}

static gboolean create_pipeline( ucil_gstreamer_video_file_object_t *vobj )
{
   GstElement *pipeline = NULL;
   GstElement *source = NULL;
   GstElement *videorate = NULL;
   GstElement *encoder = NULL;
   GstElement *vidqueue = NULL;
   GstElement *muxer = NULL;
   GstElement *colorspace = NULL;
   GstElement *outqueue = NULL;
   GstElement *sink = NULL;
   GstElement *audiosource = NULL;
   GstElement *audioconvert = NULL;
   GstElement *audioresample = NULL;
   GstElement *audioqueue = NULL;
   GstElement *audioencode = NULL;

   GstCaps *caps = NULL;

   GstVideoFormat fmt = gst_video_format_from_fourcc( vobj->format.fourcc );
   if (fmt == GST_VIDEO_FORMAT_UNKNOWN ){
      return FALSE;
   }

   if( !create_encoder_and_muxer( vobj->codec_name, &encoder, &muxer ) ){
      return FALSE;
   }

   pipeline = gst_pipeline_new( NULL );

   source = gst_element_factory_make( "appsrc", "source" );
   colorspace = gst_element_factory_make( "ffmpegcolorspace", "colorspace" );
   if( vobj->fill_frames ){
      videorate = gst_element_factory_make( "videorate", "videorate" );
      g_object_set( videorate, "skip-to-first", TRUE, NULL );
   } else {
      videorate = gst_element_factory_make( "identity", "videorate" );
   }
   vidqueue = gst_element_factory_make( "queue", "vidqueue" );
   g_object_set( vidqueue, "max-size-time", 10 * GST_SECOND, "max-size-bytes", 100000000, "max-size-buffers", 1000, NULL );
   outqueue = gst_element_factory_make( "queue", "outqueue" );
   sink = gst_element_factory_make( "filesink", "sink" );

   g_object_set( sink, "location", vobj->filename, NULL );
   g_object_set( source, "block", FALSE, NULL );

   gst_bin_add_many( GST_BIN( pipeline ), source, colorspace, videorate, encoder, vidqueue, muxer, outqueue, sink, NULL );

   if( vobj->record_audio ){
      audiosource = gst_element_factory_make( "gconfaudiosrc", "audiosource" );
      if( !audiosource )
	 audiosource = gst_element_factory_make( "autoaudiosrc", "audiosource" );

      if( audiosource ){
	 audioconvert = gst_element_factory_make( "audioconvert", "audioconvert" );
	 audioresample = gst_element_factory_make( "audioresample", "audioresample" );
	 audioqueue = gst_element_factory_make( "queue", "audioqueue" );
	 audioencode = create_audio_encoder( vobj->audio_codec );
	 
	 gst_bin_add_many( GST_BIN( pipeline ), audiosource, audioconvert, audioresample, audioencode, audioqueue, NULL );
      }
   }
   
   caps = gst_caps_new_simple( gst_video_format_is_yuv(fmt) ? "video/x-raw-yuv" : "video/x-raw-rgb", 
			       "format", GST_TYPE_FOURCC, vobj->format.fourcc,
			       "framerate", GST_TYPE_FRACTION, vobj->fps_numerator, vobj->fps_denominator,
			       "width", G_TYPE_INT, vobj->format.size.width,
			       "height", G_TYPE_INT, vobj->format.size.height,
			       NULL );
      
      
   gst_app_src_set_caps( GST_APP_SRC( source ), caps );
   gst_caps_unref( caps );

   gst_element_link_many( source, colorspace, videorate, encoder, vidqueue, muxer, NULL );
   if( audiosource )
      gst_element_link_many( audiosource, audioconvert, audioresample, audioencode, muxer, NULL );
   gst_element_link_many( muxer, outqueue, sink, NULL );

   vobj->pipeline = pipeline;
   vobj->appsrc = source;
   
   return TRUE;
}


static void destroy_vobj( ucil_gstreamer_video_file_object_t *vobj )
{
   gst_element_set_state( vobj->pipeline, GST_STATE_NULL );
   gst_element_get_state( vobj->pipeline, NULL, NULL, GST_CLOCK_TIME_NONE );
   gst_object_unref( vobj->pipeline );
   g_free( vobj->codec_name );
   g_free( vobj->filename );
   g_free( vobj->audio_codec );
   g_free( vobj );
}

static void parse_ogg_theora_parameters( GstElement *encoder, ucil_gstreamer_video_file_object_t *vobj, guint n_parameters, GParameter *parameters )
{
   int i;
   for( i = 0; i < n_parameters; i++ ){
      if( !strcmp( parameters[i].name, "quality" ) ){
	 g_object_set( encoder, "quality", g_value_get_int( &parameters[i].value ), NULL );
      } else if ( !strcmp( parameters[i].name, "bitrate" ) ){
	 g_object_set( encoder, "bitrate", g_value_get_int( &parameters[i].value ), NULL );
      }
   }
}

static void parse_parameters( ucil_gstreamer_video_file_object_t *vobj, guint n_parameters, GParameter *parameters )
{
   int i;
   
   for( i = 0; i < n_parameters; i++ ){
      if( !strcmp( parameters[i].name, "fill_frames" ) ){
	 vobj->fill_frames = g_value_get_boolean( &parameters[i].value );
	 /* TRACE( "fill_frames: %d\n", vobj->fill_frames ); */
      }else if( !strcmp( parameters[i].name, "fps" ) ){
	 unsigned int num;
	 double fps;
	 
	 fps = g_value_get_double( &parameters[i].value );
	 num = fps * 1000000;
	             
	 vobj->fps_numerator = num;
	 vobj->fps_denominator = 1000000;

	 // The current frame interval in us / frame is stored in the frame_interval variable 
	 vobj->frame_interval = 1000000 / fps;

      }else if( !strcmp( parameters[i].name, "quality" ) ){
	 vobj->quality = g_value_get_int( &parameters[i].value );
      }else if( !strcmp( parameters[i].name, "bitrate" ) ){
	 vobj->bitrate = g_value_get_int( &parameters[i].value );
      }else if( !strcmp( parameters[i].name, "encode_frame_cb" ) ){
      	 vobj->encode_frame_cb = (unicap_new_frame_callback_t)g_value_get_pointer( &parameters[i].value );
      }else if( !strcmp( parameters[i].name, "encode_frame_cb_data" ) ){
      	 vobj->encode_frame_cb_data = (void*)g_value_get_pointer( &parameters[i].value );
      }else if( !strcmp( parameters[i].name, "audio" ) ){
      	 vobj->record_audio = g_value_get_int( &parameters[i].value ) ? TRUE : FALSE;
      	 if( vobj->record_audio )
      	    vobj->fill_frames = 1;
      }else if( !strcmp( parameters[i].name, "audio_codec" ) ){
	 g_free( vobj->audio_codec );
	 vobj->audio_codec = g_strdup( g_value_get_string( &parameters[i].value ) );
      }
      /* else if( !strcmp( parameters[i].name, "audio_card" ) ) */
      /* { */
      /* 	 strncpy( vobj->audio_card, g_value_get_string( &parameters[i].value ), sizeof( vobj->audio_card ) ); */
      /* } */
      /* else if( !strcmp( parameters[i].name, "audio_rate" ) ) */
      /* { */
      /* 	 vobj->audio_rate = g_value_get_int( &parameters[i].value ); */
      /* } */
      /* else if( !strcmp( parameters[i].name, "block" ) ) */
      /* { */
      /* 	 vobj->blocking_mode = g_value_get_int( &parameters[i].value ); */
      /* } */
      /* else if( !strcmp( parameters[i].name, "nocopy" ) ) */
      /* { */
      /* 	 vobj->nocopy = g_value_get_int( &parameters[i].value ); */
      /* } */
      /* else if( !strcmp( parameters[i].name, "async_audio_encoding" ) ) */
      /* { */
      /* 	 vobj->async_audio_encoding = g_value_get_int( &parameters[i].value ); */
      /* } */
      /* else if( !strcmp( parameters[i].name, "downsize" ) ) */
      /* { */
      /* 	 int ds = g_value_get_int( &parameters[i].value ); */
      /* 	 if( ds > 1 ) */
      /* 	 { */
      /* 	    vobj->downsize = ds; */
      /* 	 } */
      /* } */
      else    
      {
	 /* TRACE( "Unknown arg: %s\n", parameters[i].name ); */
      }      
   }
   
}


ucil_gstreamer_video_file_object_t *ucil_gstreamer_create_video_file( const char *path, unicap_format_t *format, const char *codec, va_list ap)
{
   ucil_gstreamer_video_file_object_t *vobj;
   vobj = ucil_gstreamer_create_video_filev( path, format, codec, 0, NULL );
   return vobj;
}

ucil_gstreamer_video_file_object_t *ucil_gstreamer_create_video_filev( const char *path, unicap_format_t *format, const char *codec, guint n_parameters, GParameter *parameters)
{

   ucil_gstreamer_video_file_object_t *vobj = g_new0( ucil_gstreamer_video_file_object_t, 1 );

   if( !is_initialized ){
      if( !gst_init_check( NULL, NULL, NULL ) )
	 return NULL;
      /* gst_debug_set_active( TRUE ); */
      /* gst_debug_set_default_threshold( GST_LEVEL_DEBUG ); */
      is_initialized = TRUE;
   }
   
   unicap_copy_format( &vobj->format, format );   
   switch( vobj->format.fourcc ){
   case UCIL_FOURCC( 'Y', 'U', 'Y', 'V' ):
      vobj->format.fourcc = UCIL_FOURCC( 'Y', 'U', 'Y', '2' );
      break;
   default:
      break;
   }
   
      
   vobj->codec_name = g_strdup( codec );
   vobj->filename = g_strdup( path );
   vobj->audio_codec = g_strdup( "mp3" );
   
   vobj->fps_numerator = 30;
   vobj->fps_denominator = 1;
   vobj->frame_interval = 1000000 / 30;

   parse_parameters( vobj, n_parameters, parameters );


   if( !create_pipeline( vobj ) ){
      g_free( vobj );
      return NULL;
   }
   
   GstStateChangeReturn ret = gst_element_set_state( vobj->pipeline, GST_STATE_PLAYING );
   if( ret == GST_STATE_CHANGE_FAILURE ){
      destroy_vobj( vobj );
      return NULL;
   }

   /* gst_element_get_state( vobj->pipeline, NULL, NULL, GST_CLOCK_TIME_NONE ); */

   return vobj;
}

unicap_status_t ucil_gstreamer_encode_frame( ucil_gstreamer_video_file_object_t *vobj, unicap_data_buffer_t *buffer )
{
   GstBuffer *buf;

   if( !vobj->base_time_set ){
      gst_element_set_base_time( vobj->pipeline, GST_TIMEVAL_TO_TIME( buffer->fill_time ) );
      vobj->base_time_set = TRUE;
   }

   if( vobj->encode_frame_cb ){
      vobj->encode_frame_cb( UNICAP_EVENT_NEW_FRAME, NULL, buffer, vobj->encode_frame_cb_data );
   }

   buf = gst_buffer_new_and_alloc( buffer->buffer_size );
   memcpy( GST_BUFFER_DATA( buf ), buffer->data, buffer->buffer_size );
   GST_BUFFER_TIMESTAMP(buf) = GST_TIMEVAL_TO_TIME( buffer->fill_time );
   GST_BUFFER_DURATION(buf) = vobj->frame_interval * 1000;
   if( gst_app_src_push_buffer( GST_APP_SRC( vobj->appsrc ), buf ) != GST_FLOW_OK ){
      return STATUS_FAILURE;
   }
   
   
   return STATUS_SUCCESS;
}

unicap_status_t ucil_gstreamer_close_video_file( ucil_gstreamer_video_file_object_t *vobj )
{
   destroy_vobj( vobj );
   
   return STATUS_SUCCESS;
}
