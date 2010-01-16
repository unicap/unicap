/*
** backend_gtk.h
** 
** Made by Arne Caspari
** Login   <arne@localhost>
** 
** Started on  Tue Aug 21 18:20:46 2007 Arne Caspari
** Last update Tue Aug 21 18:20:46 2007 Arne Caspari
*/

#ifndef   	BACKEND_GTK_H_
# define   	BACKEND_GTK_H_

#include "unicapgtk_priv.h"

__HIDDEN__ gint backend_gtk_get_merit();
__HIDDEN__ gboolean backend_gtk_init( GtkWidget *widget, unicap_format_t *format, gpointer *_data, GError **err );
__HIDDEN__ void backend_gtk_destroy( gpointer _data );
__HIDDEN__ void backend_gtk_update_image( gpointer _data, unicap_data_buffer_t *data_buffer, GError **err );
__HIDDEN__ void backend_gtk_display_image( gpointer _data );
__HIDDEN__ void backend_gtk_size_allocate( gpointer _data, GtkWidget *widget, GtkAllocation *allocation );
__HIDDEN__ void backend_gtk_set_crop( gpointer _data, int crop_x, int crop_y, int crop_w, int crop_h );
__HIDDEN__ void backend_gtk_redraw( gpointer _data );
__HIDDEN__ void backend_gtk_expose_event( gpointer _data, GtkWidget *da, GdkEventExpose *event );
__HIDDEN__ void backend_gtk_get_image_data( gpointer _data, unicap_data_buffer_t *data_buffer, int b );
__HIDDEN__ void backend_gtk_lock( gpointer _data );
__HIDDEN__ void backend_gtk_unlock( gpointer _data );
__HIDDEN__ void backend_gtk_set_scale_to_fit( gpointer _data, gboolean scale_to_fit );
__HIDDEN__ guint backend_gtk_get_flags( gpointer _data );
__HIDDEN__ void backend_gtk_set_pause_state( gpointer _data, gboolean state );
__HIDDEN__ void backend_gtk_set_color_conversion_callback( void *_backend_data, 
							   unicapgtk_color_conversion_callback_t cb,
							   void *data );



#endif 	    /* !BACKEND_GTK_H_ */
