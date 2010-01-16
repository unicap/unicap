/*
** font.c
** 
** Made by (Arne Caspari)
** Login   <arne@arne-laptop>
** 
** Started on  Tue Dec 19 18:30:39 2006 Arne Caspari
** Last update Tue Jan  9 11:38:49 2007 Arne Caspari
*/

//#include "yuvfont.h"
#include "unicap.h"
#include "ucil.h"

#include <pango/pangoft2.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#define DEFAULT_FONT "Sans"

static void draw_bitmap( unicap_data_buffer_t *dest,
			 ucil_color_t *color,
			 int xpos, int ypos,
			 FT_Bitmap*  bitmap,
			 FT_Int      x,
			 FT_Int      y)
{
   FT_Int  i, j, p, q;
   FT_Int  x_max = x + bitmap->width;
   FT_Int  y_max = y + bitmap->rows;
   
   
   for ( i = x, p = 0; i < x_max; i++, p++ )
   {
      for ( j = y, q = 0; j < y_max; j++, q++ )
      {
	 if ( ( i < 0 ) || 
	      ( j < 0 ) ||
	      ( ( i + xpos ) >= dest->format.size.width ) || 
	      ( ( j + ypos ) >= dest->format.size.height ) )
	 {
	    continue;
	 }

	 if( bitmap->buffer[q * bitmap->width + p] )
	 {
	    ucil_set_pixel_alpha( dest, color, bitmap->buffer[q * bitmap->width + p], i + xpos, j + ypos );
	 }
      }
   }
}

ucil_font_object_t *ucil_create_font_object( int size, const char *font )
{
   ucil_font_object_t *fobj;
   PangoFontDescription *desc;
   PangoFT2FontMap      *fontmap;
   
   fobj = g_malloc( sizeof( ucil_font_object_t ) );

   if( font == NULL )
   {
      font = DEFAULT_FONT;
   }

   fontmap = PANGO_FT2_FONT_MAP (pango_ft2_font_map_new ());
   fobj->context = pango_ft2_font_map_create_context (fontmap);
   g_object_weak_ref( G_OBJECT( (PangoContext*)fobj->context ),
		      (GWeakNotify) pango_ft2_font_map_substitute_changed,
		      fontmap );
   g_object_unref( fontmap );

   desc = pango_font_description_from_string( font );

   if (size > 0)
   {
      pango_font_description_set_size (desc, size * PANGO_SCALE);
   }

   fobj->layout = pango_layout_new( (PangoContext*)fobj->context );
   pango_layout_set_font_description( (PangoLayout*)fobj->layout, desc );
   pango_font_description_free( desc );
   
   return fobj;
}

void ucil_destroy_font_object( ucil_font_object_t *fobj )
{
   g_object_unref( (PangoLayout*)fobj->layout );
   g_object_unref( (PangoContext*)fobj->context );

   g_free( fobj );
}

void ucil_draw_text( unicap_data_buffer_t *dest, ucil_color_t *color, ucil_font_object_t *fobj, const char *text, int x, int y )
{
   FT_Bitmap             bitmap;
   int                   width;
   int                   height;


   pango_layout_set_text( (PangoLayout*)fobj->layout, text, -1 );

   pango_layout_get_size( (PangoLayout*)fobj->layout, &width, &height );

   width /= PANGO_SCALE;
   height /= PANGO_SCALE;

   bitmap.width = width;
   bitmap.pitch = width;//(bitmap.width + 3) & ~3;
   bitmap.rows = height;
   bitmap.buffer = malloc( bitmap.pitch * bitmap.rows );
   bitmap.num_grays = 256;
   bitmap.pixel_mode = ft_pixel_mode_grays;
   memset( bitmap.buffer, 0x00, bitmap.pitch * bitmap.rows );

   pango_ft2_render_layout( &bitmap, (PangoLayout*)fobj->layout, 0, 0 );

   draw_bitmap (dest, color, x, y, &bitmap, 0, 0);

   free (bitmap.buffer);
}

void ucil_text_get_size( ucil_font_object_t *fobj, const char *text, int *width, int *height )
{
   pango_layout_set_text( (PangoLayout*)fobj->layout, text, -1 );
   pango_layout_get_pixel_size( (PangoLayout*)fobj->layout, width, height );
}
