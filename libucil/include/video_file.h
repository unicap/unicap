/*
** video_file.h
** 
** Made by Arne Caspari
** Login   <arne@arne-laptop>
** 
** Started on  Fri Mar 16 07:35:06 2007 Arne Caspari
** Last update Fri Mar 16 18:39:14 2007 Arne Caspari
*/

#ifndef   	VIDEO_FILE_H_
# define   	VIDEO_FILE_H_

#include "ucil.h"

enum ucil_codec_id
{
   UCIL_CODEC_ID_INVALID = -1,
   UCIL_CODEC_ID_FIRST_CODEC = 0
};



typedef void * (*ucil_cpi_create_video_filev_t)       ( const char *path, unicap_format_t *format,
							const char *codec,
							guint n_parameters, GParameter *parameters );

typedef void * (*ucil_cpi_create_video_file_t)        ( const char *path, unicap_format_t *format,
							const char *codec, va_list ap );

typedef unicap_status_t (*ucil_cpi_encode_frame_t)    ( void *vobj, unicap_data_buffer_t *buffer );

typedef unicap_status_t (*ucil_cpi_close_file_t)      ( void *vobj );

typedef unicap_status_t (*ucil_cpi_video_file_t)      ( unicap_handle_t *unicap_handle, char *filename );

typedef gboolean (*ucil_cpi_combine_av_file_t)        ( const char *path, const char *codec,
							gboolean remove, ucil_processing_info_func_t procfunc,
							void *func_data, GError **error );

typedef unicap_status_t (*ucil_cpi_open_video_file_t) ( unicap_handle_t *unicap_handle, char *filename );




#endif 	    /* !VIDEO_FILE_H_ */
