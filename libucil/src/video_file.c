/*
** video_file.c
** 
** Made by (Arne Caspari)
** Login   <arne@arne-laptop>
** 
** Started on  Tue Mar 13 06:49:01 2007 Arne Caspari
** Last update Mon Mar 19 18:06:16 2007 Arne Caspari
*/


#include "config.h"
#include "video_file.h"

#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <glib-object.h>

/* #if HAVE_AVCODEC */
/* #include "mpeg.h" */
/* #endif */

#if HAVE_THEORA
#include "ucil_theora.h"
#endif

#if HAVE_GSTREAMER
#include "ucil_gstreamer.h"
#endif

#include "ucil.h"

#include "ucil_rawavi.h"

#define DEBUG
#include "debug.h"


#define MAX_CODECS 8
static gboolean ucil_video_is_initialized = FALSE;

struct video_codec_cpi {
   const gchar **                 codec_names;
   const gchar *                  file_extension;

   ucil_cpi_create_video_filev_t create_filev;
   ucil_cpi_create_video_file_t  create_file;
   ucil_cpi_encode_frame_t       encode_frame;
   ucil_cpi_close_file_t         close_file;
   ucil_cpi_open_video_file_t    open_file;
   ucil_cpi_combine_av_file_t    combine_file;
};

typedef struct video_codec_cpi video_codec_cpi;
typedef gboolean (*ucil_cpi_register_module_t)          ( video_codec_cpi *vcp );

const gchar *gstreamer_avi_codecs[] = { "avi/raw", "avi/mpeg2", "avi/vp8", NULL };
const gchar *gstreamer_ogg_codecs[] = { "ogg/theora", "ogg/vp8", NULL };
const gchar *gstreamer_mkv_codecs[] = { "mkv/vp8", NULL };
const gchar *theora_codecs[] = { "ogg/theora", NULL };
const gchar *rawavi_codecs[] = { "avi/raw", NULL };

static video_codec_cpi codecs[ ] = {
#if HAVE_GSTREAMER
   {
      codec_names: 	gstreamer_avi_codecs,
      file_extension: 	"avi",

      create_filev: 	(ucil_cpi_create_video_filev_t) ucil_gstreamer_create_video_filev,
      create_file: 	(ucil_cpi_create_video_file_t) ucil_gstreamer_create_video_file,

      encode_frame: 	(ucil_cpi_encode_frame_t) ucil_gstreamer_encode_frame,

      close_file: 	(ucil_cpi_close_file_t) ucil_gstreamer_close_video_file,

      open_file:	NULL, 

      combine_file:	NULL,
   },
   {
      codec_names: 	gstreamer_ogg_codecs,
      file_extension: 	"ogg",

      create_filev: 	(ucil_cpi_create_video_filev_t) ucil_gstreamer_create_video_filev,
      create_file: 	(ucil_cpi_create_video_file_t) ucil_gstreamer_create_video_file,

      encode_frame: 	(ucil_cpi_encode_frame_t) ucil_gstreamer_encode_frame,

      close_file: 	(ucil_cpi_close_file_t) ucil_gstreamer_close_video_file,

      open_file:	NULL, 

      combine_file:	NULL,
   },
   {
      codec_names: 	gstreamer_mkv_codecs,
      file_extension: 	"mkv",

      create_filev: 	(ucil_cpi_create_video_filev_t) ucil_gstreamer_create_video_filev,
      create_file: 	(ucil_cpi_create_video_file_t) ucil_gstreamer_create_video_file,

      encode_frame: 	(ucil_cpi_encode_frame_t) ucil_gstreamer_encode_frame,

      close_file: 	(ucil_cpi_close_file_t) ucil_gstreamer_close_video_file,

      open_file:	NULL, 

      combine_file:	NULL,
   },
#endif
#if HAVE_THEORA
   {
      codec_names: 	theora_codecs,
      file_extension: 	"ogg",

      create_filev: 	(ucil_cpi_create_video_filev_t) ucil_theora_create_video_filev,
      create_file: 	(ucil_cpi_create_video_file_t) ucil_theora_create_video_file,

      encode_frame: 	(ucil_cpi_encode_frame_t) ucil_theora_encode_frame,

      close_file: 	(ucil_cpi_close_file_t) ucil_theora_close_video_file,

      open_file:	NULL, 

      combine_file:	ucil_theora_combine_av_file,
   },
#endif
   {
      codec_names: 	rawavi_codecs,
      file_extension:	"avi",

      create_filev: 	(ucil_cpi_create_video_filev_t) ucil_rawavi_create_video_filev,
      create_file: 	(ucil_cpi_create_video_file_t) ucil_rawavi_create_video_file,

      encode_frame: 	(ucil_cpi_encode_frame_t) ucil_rawavi_encode_frame,

      close_file: 	(ucil_cpi_close_file_t) ucil_rawavi_close_video_file,

      open_file: 	NULL,

      combine_file:	NULL,
   },
   

};


static enum ucil_codec_id get_codec_id( const char *codec )
{
   int id;
   
   // default to first codec in the list
   if( codec == NULL )
   {
      return UCIL_CODEC_ID_FIRST_CODEC;
   }

   for (id = 0; id < sizeof(codecs) / sizeof(video_codec_cpi); id ++)
   {
      gboolean found = FALSE;
      int i;
      
      for( i = 0; codecs[id].codec_names[i] != NULL; i++ ){
	 if( !strncmp( codec, codecs[id].codec_names[i], strlen( codecs[id].codec_names[i] ) ) )
	    return id;
      }      
   }
   
   return UCIL_CODEC_ID_INVALID;
}


static void ucil_video_initialize( void )
{
/*    load_vcp_modules(); */
}


ucil_video_file_object_t *ucil_create_video_filev( const char *path, unicap_format_t *format, const char *codec, 
						   guint n_parameters, GParameter *parameters )
{
   ucil_video_file_object_t *vobj = NULL;
   const video_codec_cpi *cpi = NULL;
   int codec_id = UCIL_CODEC_ID_INVALID;
   
   codec_id = get_codec_id( codec );
   
   if( codec_id == UCIL_CODEC_ID_INVALID )
   {
      TRACE( "invalid codec id!\n" );
      return NULL;
   }

   vobj = malloc( sizeof( ucil_video_file_object_t ) );
   vobj->ucil_codec_id = codec_id;

   cpi = &codecs[ codec_id ];
   if ( !cpi->create_filev )
      {
	 TRACE( "Unsupported codec ID: %s\n", codec );
	 vobj->codec_data = NULL;
   }
   else
   {
      vobj->codec_data = cpi->create_filev( path, format, codec, n_parameters, parameters );
   }

   if( !vobj->codec_data )
   {
      TRACE( "Failed to initialize codec %d\n", vobj->ucil_codec_id );
      free( vobj );
      return NULL;
   }
   
   return vobj;   
}


ucil_video_file_object_t *ucil_create_video_file( const char *path, unicap_format_t *format, const char *codec, ...)
{
   ucil_video_file_object_t *vobj = NULL;
   const video_codec_cpi *cpi = NULL;
   int codec_id = UCIL_CODEC_ID_INVALID;
   
   codec_id = get_codec_id( codec );
   
   if( codec_id == UCIL_CODEC_ID_INVALID )
   {
      TRACE( "invalid codec id!\n" );
      return NULL;
   }

   vobj = malloc( sizeof( ucil_video_file_object_t ) );
   vobj->ucil_codec_id = codec_id;

   cpi = &codecs[ codec_id ];
   if ( !cpi->create_file )      {
      TRACE( "Unsupported codec ID: %s\n", codec );
      vobj->codec_data = NULL;
   }else{
      va_list ap;
      
      va_start( ap, codec );
      vobj->codec_data = cpi->create_file( path, format, codec, ap );
      va_end( ap );
   }

   if( !vobj->codec_data )
   {
      TRACE( "Failed to initialize codec %d\n", vobj->ucil_codec_id );
      free( vobj );
      return NULL;
   }
   
   return vobj;
}

unicap_status_t ucil_encode_frame( ucil_video_file_object_t *vobj, unicap_data_buffer_t *buffer )
{
   unicap_status_t status = STATUS_FAILURE;

	 
   if ( ( vobj->ucil_codec_id < 0)
		   || ( vobj->ucil_codec_id >= sizeof( codecs ) / sizeof( video_codec_cpi ) )
		   || !codecs[ vobj->ucil_codec_id ].encode_frame )
   {
	 TRACE( "Unsupported codec ID\n" );
   }
   else
   {
      status = codecs[ vobj->ucil_codec_id ].encode_frame( vobj->codec_data, buffer );
   }
   
   return status;
}

unicap_status_t ucil_close_video_file( ucil_video_file_object_t *vobj )
{
   unicap_status_t status = STATUS_FAILURE;
   
   if ( ( vobj->ucil_codec_id < 0)
		   || ( vobj->ucil_codec_id >= sizeof( codecs ) / sizeof( video_codec_cpi ) )
		   || !codecs[ vobj->ucil_codec_id ].close_file )
   {
      TRACE( "Unsupported codec ID\n" );
   }
   else
   {
      status = codecs[ vobj->ucil_codec_id ].close_file( vobj->codec_data );
   }

   return status;
}

unicap_status_t ucil_open_video_file( unicap_handle_t *unicap_handle, char *filename )
{
   unicap_status_t status = STATUS_FAILURE;
   int id;

   for (id = 0; id < sizeof(codecs) / sizeof(video_codec_cpi); id ++)
   {
      if ( codecs[ id ].open_file )
      {
	 status = codecs[ id ].open_file( unicap_handle, filename );
	 if( SUCCESS ( status ) ) 
	 {
	    return status;
	 }
      }
   }

   return status;
}

const char * ucil_get_video_file_extension( const char *codec )
{
   enum ucil_codec_id codec_id;
   
   codec_id = get_codec_id( codec );
   
   if ( codec_id == UCIL_CODEC_ID_INVALID )
   {
      return NULL;
   }
   else
   {
      return codecs[ codec_id ].file_extension;
   }
}

unicap_status_t ucil_combine_av_file( const char *path, const char *codec, 
				      gboolean remove, ucil_processing_info_func_t procfunc, void *func_data, GError **error )
{
   enum ucil_codec_id codec_id;
   unicap_status_t status = STATUS_NOT_IMPLEMENTED;
   
   codec_id = get_codec_id( codec );
   if ( ( codec_id > 0) && codecs[ codec_id ].combine_file )
   {
      status = codecs[ codec_id ].combine_file( path, codec, remove, procfunc, func_data, error )
		      ? STATUS_SUCCESS : STATUS_FAILURE;
   }
   return status;
}

