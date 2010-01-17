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
#ifndef __UNICAPGTK_VIDEO_DISPLAY_H__
#define __UNICAPGTK_VIDEO_DISPLAY_H__

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtkaspectframe.h>
#include <gmodule.h>

#include <unicap.h>
#include <unicap_status.h>

G_BEGIN_DECLS

#define UNICAPGTK_TYPE_VIDEO_DISPLAY            (unicapgtk_video_display_get_type ())
#define UNICAPGTK_VIDEO_DISPLAY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), UNICAPGTK_VIDEO_DISPLAY_TYPE, UnicapgtkVideoDisplay))
#define UNICAPGTK_VIDEO_DISPLAY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), UNICAPGTK_VIDEO_DISPLAY_TYPE, UnicapgtkVideoDisplayClass))
#define UNICAPGTK_VIDEO_DISPLAY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), 

#define UNICAPGTK_IS_VIDEO_DISPLAY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UNICAPGTK_VIDEO_DISPLAY_TYPE))
#define UNICAPGTK_IS_VIDEO_DISPLAY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), UNICAPGTK_VIDEO_DISPLAY_TYPE))



// Deprecated 
#define IS_UNICAPGTK_VIDEO_DISPLAY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UNICAPGTK_VIDEO_DISPLAY_TYPE))
#define IS_UNICAPGTK_VIDEO_DISPLAY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), UNICAPGTK_VIDEO_DISPLAY_TYPE))

// !! Differs from Gtk convention !!
#define UNICAPGTK_VIDEO_DISPLAY_TYPE            (unicapgtk_video_display_get_type ())


typedef struct _UnicapgtkVideoDisplay       UnicapgtkVideoDisplay;
typedef struct _UnicapgtkVideoDisplayClass  UnicapgtkVideoDisplayClass;


typedef gboolean (*unicapgtk_color_conversion_callback_t)(unicap_data_buffer_t *dest, unicap_data_buffer_t *src, void *user_data);


struct _UnicapgtkVideoDisplayCropping
{
  int crop_x;
  int crop_y;
  int crop_width;
  int crop_height;
};

typedef struct _UnicapgtkVideoDisplayCropping UnicapgtkVideoDisplayCropping;


struct _UnicapgtkVideoDisplayClass
{
      GtkAspectFrameClass parent_class;

      void (* unicapgtk_video_display) (UnicapgtkVideoDisplay *ugtk);
};

typedef enum
{
   UNICAPGTK_CALLBACK_FLAGS_BEFORE = 1 << 0,
   UNICAPGTK_CALLBACK_FLAGS_AFTER  = 1 << 1,


   UNICAPGTK_CALLBACK_FLAGS_XV_CB  = 1 << 16,
} UnicapgtkCallbackFlags;


GType            unicapgtk_video_display_get_type        ( void );
GtkWidget*       unicapgtk_video_display_new             ( void );
GtkWidget*       unicapgtk_video_display_new_by_device   ( unicap_device_t *device_spec );
GtkWidget*       unicapgtk_video_display_new_by_handle   ( unicap_handle_t handle );
gboolean         unicapgtk_video_display_start           ( UnicapgtkVideoDisplay *ugtk );
void             unicapgtk_video_display_stop            ( UnicapgtkVideoDisplay *ugtk );
unicap_device_t* unicapgtk_video_display_get_device      ( UnicapgtkVideoDisplay *ugtk );
unicap_handle_t  unicapgtk_video_display_get_handle      ( UnicapgtkVideoDisplay *ugkt );
gboolean         unicapgtk_video_display_set_format      ( UnicapgtkVideoDisplay *ugtk, 
							   unicap_format_t *format_spec );
void             unicapgtk_video_display_get_format      ( UnicapgtkVideoDisplay *ugtk, unicap_format_t *format );
void             unicapgtk_video_display_set_pause       ( UnicapgtkVideoDisplay *ugtk, gboolean state );
gboolean         unicapgtk_video_display_get_pause       ( UnicapgtkVideoDisplay *ugtk );
gboolean         unicapgtk_video_display_set_device      ( UnicapgtkVideoDisplay *ugtk, unicap_device_t *device_spec );
gboolean         unicapgtk_video_display_set_handle      ( UnicapgtkVideoDisplay *ugtk, unicap_handle_t handle );
GdkPixbuf*       unicapgtk_video_display_get_still_image ( UnicapgtkVideoDisplay *ugtk );
void             unicapgtk_video_display_set_still_image ( UnicapgtkVideoDisplay *ugtk, unicap_data_buffer_t *buffer );
void             unicapgtk_video_display_get_data_buffer ( UnicapgtkVideoDisplay *ugtk, unicap_data_buffer_t **dest_buffer );
void             unicapgtk_video_display_set_data_buffer ( UnicapgtkVideoDisplay *ugtk, unicap_data_buffer_t *buffer );

void             unicapgtk_video_display_set_crop        ( UnicapgtkVideoDisplay *ugtk, 
							   UnicapgtkVideoDisplayCropping *crop );
void             unicapgtk_video_display_get_crop        ( UnicapgtkVideoDisplay *ugtk, 
							   UnicapgtkVideoDisplayCropping *crop );

void             unicapgtk_video_display_set_size        ( UnicapgtkVideoDisplay *ugtk, 
							   gint width, gint height );
void             unicapgtk_video_display_set_scale_to_fit( UnicapgtkVideoDisplay *ugtk,
                                                           gboolean scale );

unicap_new_frame_callback_t unicapgtk_video_display_set_new_frame_callback( UnicapgtkVideoDisplay *ugtk, 
									    UnicapgtkCallbackFlags flags,  
									    unicap_new_frame_callback_t cb, void *data );


unicapgtk_color_conversion_callback_t unicapgtk_video_display_set_color_conversion_callback( UnicapgtkVideoDisplay *ugtk, 
											     unicapgtk_color_conversion_callback_t cb, 
											     void *data );

G_END_DECLS

#endif /* __UNICAPGTK_VIDEO_DISPLAY_H__ */

