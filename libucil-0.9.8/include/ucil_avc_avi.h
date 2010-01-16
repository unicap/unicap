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

#ifndef   	__UCIL_AVC_AVI_H__
# define   	__UCIL_AVC_AVI_H__

#include <unicap.h>

typedef struct _ucil_avcavi_video_file_object ucil_avcavi_video_file_object_t;

ucil_avcavi_video_file_object_t *ucil_avcavi_create_video_file( const char *path, unicap_format_t *format, const char *codec, va_list ap );
ucil_avcavi_video_file_object_t *ucil_avcavi_create_video_filev( const char *path, unicap_format_t *format, const char *codec, guint n_parameters, GParameter *parameters );
unicap_status_t ucil_avcavi_close_video_file( ucil_avcavi_video_file_object_t *vobj );
unicap_status_t ucil_avcavi_encode_frame( ucil_avcavi_video_file_object_t *vobj, unicap_data_buffer_t *buffer );

#endif 	    /* !__UCIL_AVCAVI_H__ */
