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

#ifndef __UNICAPGTK_VIDEO_FORMAT_SELECTION_H__
#define __UNICAPGTK_VIDEO_FORMAT_SELECTION_H__


#include <glib.h>
#include <glib-object.h>
#include <gtk/gtkhbox.h>

#include <unicap.h>
#include <unicap_status.h>

G_BEGIN_DECLS

#define UNICAPGTK_TYPE_VIDEO_FORMAT_SELECTION            (unicapgtk_video_format_selection_get_type ())
#define UNICAPGTK_VIDEO_FORMAT_SELECTION(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                                          UNICAPGTK_VIDEO_FORMAT_SELECTION_TYPE, UnicapgtkVideoFormatSelection))
#define UNICAPGTK_VIDEO_FORMAT_SELECTION_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                                          UNICAPGTK_VIDEO_FORMAT_SELECTION_TYPE, UnicapgtkVideoFormatSelectionClass))
#define UNICAPGTK_IS_VIDEO_FORMAT_SELECTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UNICAPGTK_VIDEO_FORMAT_SELECTION_TYPE))
#define UNICAPGTK_IS_VIDEO_FORMAT_SELECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), UNICAPGTK_VIDEO_FORMAT_SELECTION_TYPE))


//Deprecated
#define IS_UNICAPGTK_VIDEO_FORMAT_SELECTION(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UNICAPGTK_VIDEO_FORMAT_SELECTION_TYPE))
#define IS_UNICAPGTK_VIDEO_FORMAT_SELECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), UNICAPGTK_VIDEO_FORMAT_SELECTION_TYPE))

// !! Differs from Gtk convention !!
#define UNICAPGTK_VIDEO_FORMAT_SELECTION_TYPE            (unicapgtk_video_format_selection_get_type ())


typedef struct _UnicapgtkVideoFormatSelection       UnicapgtkVideoFormatSelection;
typedef struct _UnicapgtkVideoFormatSelectionClass  UnicapgtkVideoFormatSelectionClass;

struct _UnicapgtkVideoFormatSelection
{
		GtkHBox hbox;
		GtkWidget *fourcc_menu;
		GtkWidget *size_menu;

		unicap_handle_t unicap_handle;
		unicap_device_t device;
		unicap_format_t format;
};

struct _UnicapgtkVideoFormatSelectionClass
{
  GtkHBoxClass parent_class;

  void (* unicapgtk_video_format_selection) (UnicapgtkVideoFormatSelection *ugtk);
};

GType            unicapgtk_video_format_selection_get_type      ( void );
GtkWidget*       unicapgtk_video_format_selection_new           ( void );
GtkWidget*       unicapgtk_video_format_selection_new_by_handle ( unicap_handle_t  handle );
GtkWidget*       unicapgtk_video_format_selection_new_by_device ( unicap_device_t *device_spec );

unicap_format_t* unicapgtk_video_format_selection_get_format( UnicapgtkVideoFormatSelection *ugtk );
gboolean         unicapgtk_video_format_selection_set_device( UnicapgtkVideoFormatSelection *ugtk, 
							      unicap_device_t               *device_spec );
void             unicapgtk_video_format_selection_set_handle( UnicapgtkVideoFormatSelection *ugtk, 
							      unicap_handle_t handle );
unicap_handle_t unicapgtk_video_format_selection_get_handle( UnicapgtkVideoFormatSelection *ugtk );

G_END_DECLS

#endif /* __UNICAPGTK_VIDEO_FORMAT_SELECTION_H__ */
