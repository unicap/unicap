/*
** backend_xv.h
** 
** Made by Arne Caspari
** Login   <arne@localhost>
** 
** Started on  Tue Aug 21 18:18:17 2007 Arne Caspari
*/

#ifndef   	BACKEND_XV_H_
# define   	BACKEND_XV_H_

#include "unicapgtk_priv.h"

__HIDDEN__ void backend_xv_size_allocate( gpointer _data, GtkWidget *widget, GtkAllocation *allocation );
__HIDDEN__ gboolean backend_xv_init( GtkWidget *widget, unicap_format_t *format, guint32 req_fourcc, gpointer *_data, GError **err );
__HIDDEN__ void backend_xv_destroy( gpointer _data );
__HIDDEN__ void backend_xv_set_crop( gpointer _data, int crop_x, int crop_y, int crop_w, int crop_h );
__HIDDEN__ void backend_xv_redraw( gpointer _data );
__HIDDEN__ void backend_xv_update_image( gpointer _data, unicap_data_buffer_t *data_buffer, GError **err );
__HIDDEN__ void backend_xv_display_image( gpointer _data );
__HIDDEN__ void backend_xv_expose_event( gpointer _data, GtkWidget *da, GdkEventExpose *event );
__HIDDEN__ void backend_xv_get_image_data( gpointer _data, unicap_data_buffer_t *data_buffer, int b );
__HIDDEN__ void backend_xv_lock( gpointer _data );
__HIDDEN__ void backend_xv_unlock( gpointer _data );
__HIDDEN__ void backend_xv_set_scale_to_fit( gpointer _data, gboolean scale_to_fit );
__HIDDEN__ void backend_xv_set_pause_state( gpointer _data, gboolean state );
__HIDDEN__ guint backend_xv_get_flags( gpointer _data );
__HIDDEN__ unicap_new_frame_callback_t backend_xv_set_new_frame_callback( void *backend_data, 
									  unicap_new_frame_callback_t cb, 
									  unicap_handle_t handle, 
									  void *data );
__HIDDEN__ void backend_xv_set_color_conversion_callback( void *_backend_data, 
							  unicapgtk_color_conversion_callback_t cb,
							  void *data );
__HIDDEN__ gint backend_xv_get_merit();

#endif 	    /* !BACKEND_XV_H_ */
