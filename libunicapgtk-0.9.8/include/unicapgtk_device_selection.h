/* 
 *
 * Copyright (C) 2006 JPK Instruments AG; used with permission
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __UNICAPGTK_DEVICE_SELECTION_H__
#define __UNICAPGTK_DEVICE_SELECTION_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

enum
{
  UNICAPGTK_DEVICE_ID,
  UNICAPGTK_DEVICE_MODEL,
  UNICAPGTK_DEVICE_VENDOR,
  UNICAPGTK_DEVICE_LAST_PUBLIC_COLUMN = UNICAPGTK_DEVICE_VENDOR
};

#define UNICAPGTK_TYPE_DEVICE_SELECTION           (unicapgtk_device_selection_get_type())
#define UNICAPGTK_DEVICE_SELECTION(obj)           (G_TYPE_CHECK_INSTANCE_CAST( (obj), \
									       UNICAPGTK_TYPE_DEVICE_SELECTION, \
									       UnicapgtkDeviceSelection ) )
#define UNICAPGTK_DEVICE_SELECTION_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST( (klass), \
									    UNICAPGTK_TYPE_DEVICE_SELECTION, \
									    UnicapgtkDeviceSelectionClass ) )
#define UNICAPGTK_IS_DEVICE_SELECTION(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
									       UNICAPGTK_TYPE_DEVICE_SELECTION))
#define UNICAPGTK_IS_DEVICE_SELECTION_CLASS(klass)(G_TYPE_CHECK_CLASS_TYPE ((klass), \
									    UNICAPGTK_TYPE_DEVICE_SELECTION))
#define UNICAPGTK_DEVICE_SELECTION_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), \
									      UNICAPGTK_TYPE_DEVICE_SELECTION, \
									      UnicapgtkDeviceSelectionClass))


typedef struct _UnicapgtkDeviceSelection       UnicapgtkDeviceSelection;
typedef struct _UnicapgtkDeviceSelectionClass  UnicapgtkDeviceSelectionClass;

struct _UnicapgtkDeviceSelectionClass
{
  GtkComboBoxClass  parent_instance;
};

struct _UnicapgtkDeviceSelection
{
      GtkComboBox       parent_instance;
      
      /*< private >*/
      gboolean          include_rescan_entry;
      gchar            *active_device;
      GdkCursor        *busy_cursor;
      gchar            *label_fmt_string;
};


GType       unicapgtk_device_selection_get_type   (void) G_GNUC_CONST;

GtkWidget * unicapgtk_device_selection_new        (gboolean include_rescan_entry);
gint        unicapgtk_device_selection_rescan     (UnicapgtkDeviceSelection *combo);
gboolean    unicapgtk_device_selection_set_device (UnicapgtkDeviceSelection *combo,
                                                   unicap_device_t      *device);
void        unicapgtk_device_selection_set_label_fmt( UnicapgtkDeviceSelection *combo,
						      const gchar *fmt );

G_END_DECLS						      

#endif /* __DEVICE_COMBO_BOX_H__ */
