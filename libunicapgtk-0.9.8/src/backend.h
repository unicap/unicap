/* unicap
 *
 * Copyright (C) 2004 Arne Caspari ( arne_caspari@users.sourceforge.net )
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

#ifndef __BACKEND_H__
#define __BACKEND_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <unicap.h>
#include "unicapgtk_video_display.h"

enum BackendErrorEnum
{
   BACKEND_ERROR_FORMAT =0,

};

#define BACKEND_MERIT_XV  110
#define BACKEND_MERIT_GTK 100

#define BE_INVALID_FORMAT "Data buffer format does not match initialized format"


#define BACKEND_FLAGS_SCALING_SUPPORTED ( 1 << 0 )



gboolean backend_init( GtkWidget *widget, unicap_format_t *format, gpointer *_data, GError **err );
void backend_update_image( gpointer _data, unicap_data_buffer_t *data_buffer, GError **err );
void backend_display_image( gpointer _data );
void backend_redraw( gpointer _data );
void backend_destroy( gpointer _data );
gint backend_get_merit();
void backend_expose_event( gpointer _data, GtkWidget *da, GdkEventExpose *event );
void backend_get_image_data( gpointer _data, unicap_data_buffer_t *data_buffer, int b );
void backend_lock( gpointer _data );
void backend_unlock( gpointer _data );

void backend_size_allocate( gpointer _data, GtkWidget *widget, GtkAllocation *allocation );
void backend_set_scale_to_fit( gpointer _data, gboolean scale );
void backend_set_pause_state( gpointer _data, gboolean state );
guint backend_get_flags( gpointer _data );

typedef gboolean (*backend_init_func_t) ( GtkWidget *widget, unicap_format_t *format, gpointer *_data, GError **err );
typedef void (*backend_destroy_func_t) ( gpointer _data );
typedef void (*backend_update_image_func_t ) ( gpointer _data, unicap_data_buffer_t *data_buffer, GError **err );
typedef void (*backend_display_image_t ) ( gpointer _data );
typedef void (*backend_redraw_func_t ) ( gpointer _data );
typedef gint (*backend_get_merit_func_t)();
typedef void (*backend_set_output_size_func_t)(gpointer _data, gint width, gint height );
typedef void (*backend_expose_event_t)( gpointer _data, GtkWidget *da, GdkEventExpose *event );
typedef void (*backend_get_image_data_t)( gpointer _data, unicap_data_buffer_t *data_buffer, int b );
typedef void (*backend_lock_t)( gpointer _data );
typedef void (*backend_unlock_t) ( gpointer _data );

typedef void (*backend_size_allocate_t) ( gpointer _data, GtkWidget *widget, GtkAllocation *allocation );
typedef void (*backend_set_crop_t) ( gpointer _data, int x, int y, int w, int h );
typedef void (*backend_set_scale_to_fit_t) ( gpointer _data, gboolean scale );

typedef void (*backend_set_pause_state_t) ( gpointer _data, gboolean state );

typedef guint (*backend_get_flags_t) ( gpointer _data );

typedef void (*backend_set_color_conversion_callback_t)( void *_backend_data, 
							 unicapgtk_color_conversion_callback_t cb,
							 void *data );

#endif//__BACKEND_H__
