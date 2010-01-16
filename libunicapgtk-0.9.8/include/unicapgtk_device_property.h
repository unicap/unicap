/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
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
#ifndef __UNICAPGTK_DEVICE_PROPERTY_H__
#define __UNICAPGTK_DEVICE_PROPERTY_H__


#include <glib.h>
#include <glib-object.h>
#include <gtk/gtkhbox.h>

#include <unicap.h>
#include <unicap_status.h>

G_BEGIN_DECLS

#define UNICAPGTK_TYPE_DEVICE_PROPERTY            (unicapgtk_device_property_get_type ())
#define UNICAPGTK_DEVICE_PROPERTY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), UNICAPGTK_DEVICE_PROPERTY_TYPE, UnicapgtkDeviceProperty))
#define UNICAPGTK_DEVICE_PROPERTY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), UNICAPGTK_DEVICE_PROPERTY_TYPE, UnicapgtkDevicePropertyClass))
#define UNICAPGTK_IS_DEVICE_PROPERTY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UNICAPGTK_DEVICE_PROPERTY_TYPE))
#define UNICAPGTK_IS_DEVICE_PROPERTY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), UNICAPGTK_DEVICE_PROPERTY_TYPE))


// Deprecated
#define IS_UNICAPGTK_DEVICE_PROPERTY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UNICAPGTK_DEVICE_PROPERTY_TYPE))
#define IS_UNICAPGTK_DEVICE_PROPERTY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), UNICAPGTK_DEVICE_PROPERTY_TYPE))

// !! Differs from Gtk convention !!
#define UNICAPGTK_DEVICE_PROPERTY_TYPE            (unicapgtk_device_property_get_type ())



typedef struct _UnicapgtkDeviceProperty       UnicapgtkDeviceProperty;
typedef struct _UnicapgtkDevicePropertyClass  UnicapgtkDevicePropertyClass;

struct mapping
{
      double val;
      char *repr;
};

struct _UnicapgtkDeviceProperty
{
      GtkHBox parent_hbox;
      GtkWidget *hbox;
      GtkWidget *label;
      GtkWidget *label_box;

      GtkWidget *scale;
      GtkWidget *scale_value_label;
      GtkWidget *one_push_button;
      GtkWidget *auto_check_box;
      GtkWidget *value_combo_box;
      GtkWidget *menu_combo_box;
      GtkWidget *flags_enable_check_box;

      gulong scale_changed_hid;
      gint ignore_scale_changed;
      gulong one_push_clicked_hid;
      gint ignore_one_push_clicked;
      gulong auto_check_box_toggled_hid;
      gint ignore_auto_check_box_toggled;
      gulong value_combo_box_changed_hid;
      gint ignore_value_combo_box_changed;
      gulong menu_combo_box_changed_hid;
      gint ignore_menu_combo_box_changed;
      gulong flags_enable_changed_hid;
      gint ignore_flags_enable_changed;

      unicap_handle_t unicap_handle;
      unicap_device_t device;
      unicap_property_t property_default;
      unicap_property_t property;
		
      gint label_width;
      gint label_height;

      struct mapping *mapping;
      int nmappings;
      int type;
};

struct _UnicapgtkDevicePropertyClass
{
      GtkHBoxClass parent_class;

      void (* unicapgtk_device_property) (UnicapgtkDeviceProperty *ugtk);
};

GType      unicapgtk_device_property_get_type          ( void );
GtkWidget* unicapgtk_device_property_new               ( unicap_property_t *property_spec );
GtkWidget* unicapgtk_device_property_new_by_handle     ( unicap_handle_t handle, 
							 unicap_property_t *property_spec );
gboolean   unicapgtk_device_property_set               ( UnicapgtkDeviceProperty *ugtk, 
							 unicap_property_t *property );
GtkWidget *unicapgtk_device_property_get_label         ( UnicapgtkDeviceProperty *ugtk );
void       unicapgtk_device_property_set_label         ( UnicapgtkDeviceProperty *ugtk, 
							 GtkWidget *label );
void       unicapgtk_device_property_redraw            ( UnicapgtkDeviceProperty *ugtk );
void       unicapgtk_device_property_update            ( UnicapgtkDeviceProperty *ugtk );

void       unicapgtk_device_property_redraw_sensitivity( UnicapgtkDeviceProperty *ugtk );
void       unicapgtk_device_property_update_sensitivity( UnicapgtkDeviceProperty *ugtk );


G_END_DECLS

#endif /* __UNICAPGTK_DEVICE_PROPERTY_H__ */
