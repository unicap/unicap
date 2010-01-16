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
#ifndef __UNICAPGTK_PROPERTY_DIALOG_H__
#define __UNICAPGTK_PROPERTY_DIALOG_H__


#include <glib.h>
#include <glib-object.h>
#include <gtk/gtkwindow.h>

#include <unicap.h>
#include <unicap_status.h>


G_BEGIN_DECLS

#define UNICAPGTK_TYPE_PROPERTY_DIALOG            (unicapgtk_property_dialog_get_type())
#define UNICAPGTK_PROPERTY_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), UNICAPGTK_PROPERTY_DIALOG_TYPE, UnicapgtkPropertyDialog))
#define UNICAPGTK_PROPERTY_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), UNICAPGTK_PROPERTY_DIALOG_TYPE, UnicapgtkPropertyDialogClass))
#define UNICAPGTK_IS_PROPERTY_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UNICAPGTK_PROPERTY_DIALOG_TYPE))
#define UNICAPGTK_IS_PROPERTY_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), UNICAPGTK_PROPERTY_DIALOG_TYPE))


// Deprecated
#define IS_UNICAPGTK_PROPERTY_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UNICAPGTK_PROPERTY_DIALOG_TYPE))
#define IS_UNICAPGTK_PROPERTY_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), UNICAPGTK_PROPERTY_DIALOG_TYPE))

// !! Differs from Gtk convention !!
#define UNICAPGTK_PROPERTY_DIALOG_TYPE            (unicapgtk_property_dialog_get_type())



typedef struct _UnicapgtkPropertyDialog       UnicapgtkPropertyDialog;
typedef struct _UnicapgtkPropertyDialogClass  UnicapgtkPropertyDialogClass;

typedef gboolean (*unicap_property_filter_func_t)(unicap_property_t *property, gpointer user_data );

struct _UnicapgtkPropertyDialog
{
      GtkDialog window;

      /* private */
      GtkWidget *tree_view;
      GtkWidget *notebook;

      unicap_handle_t unicap_handle;
      
      GList *property_list;
      GList *update_entry;
      GList *size_groups;
      guint  update_timeout_id;
      gint   update_interval;
      guint  update_lock_id;

      unicap_property_filter_func_t property_filter_func;
      gpointer                     property_filter_user_data;
};

struct _UnicapgtkPropertyDialogClass
{
		GtkDialogClass parent_class;

		void (* unicapgtk_property_dialog) (UnicapgtkPropertyDialog *ugtk);
};



GType              unicapgtk_property_dialog_get_type        ( void );
GtkWidget*         unicapgtk_property_dialog_new             ( void );
GtkWidget*         unicapgtk_property_dialog_new_by_handle   ( unicap_handle_t handle );
void               unicapgtk_property_dialog_reset           ( UnicapgtkPropertyDialog *ugtk );
void               unicapgtk_property_dialog_set_device      ( UnicapgtkPropertyDialog *ugtk, 
							       unicap_device_t *device_spec );
void               unicapgtk_property_dialog_set_handle      ( UnicapgtkPropertyDialog *ugtk, 
							       unicap_handle_t handle );
void               unicapgtk_property_dialog_set_filter      ( UnicapgtkPropertyDialog *ugtk, 
							       unicap_property_filter_func_t func, 
							       gpointer user_data );


G_END_DECLS

#endif /* __UNICAPGTK_PROPERTY_DIALOG_H__ */
