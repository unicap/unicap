/*
** ucil_png.c
** 
** Made by (Arne Caspari)
** Login   <arne@localhost>
** 
** Started on  Mon Oct 15 21:54:19 2007 Arne Caspari
*/

#include "config.h"

#include <stdlib.h>
#include <png.h>
#include <unicap.h>
#include <glib.h>
#include "ucil.h"

#if UCIL_DEBUG
#define DEBUG
#endif
#include "debug.h"


//#include "ucil_png.h"

#ifndef png_jmpbuf
#  define png_jmpbuf(png_ptr) ((png_ptr)->jmpbuf)
#endif


#define PNG_BYTES_TO_CHECK 4
static int check_if_png(char *file_name, FILE **fp)
{
   unsigned char buf[PNG_BYTES_TO_CHECK];

   /* Open the prospective PNG file. */
   if ((*fp = fopen(file_name, "rb")) == NULL)
      return 0;

   /* Read in some of the signature bytes */
   if (fread(buf, 1, PNG_BYTES_TO_CHECK, *fp) != PNG_BYTES_TO_CHECK)
      return 0;

   /* Compare the first PNG_BYTES_TO_CHECK bytes of the signature.
      Return nonzero (true) if they match */

   return(!png_sig_cmp(buf, (png_size_t)0, PNG_BYTES_TO_CHECK));
}


static int read_png(FILE *fp, unsigned int sig_read, unicap_data_buffer_t *buffer )  /* file is already open */
{
   png_structp png_ptr;
   png_infop info_ptr;
   png_uint_32 width, height;
/*    int bit_depth, color_type, interlace_type; */
   int i;
   png_bytepp row_pointers;

   /* Create and initialize the png_struct with the desired error handler
    * functions.  If you want to use the default stderr and longjump method,
    * you can supply NULL for the last three parameters.  We also supply the
    * the compiler header file version, so that we know if the application
    * was compiled with a compatible version of the library.  REQUIRED
    */
   png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING,
				     NULL, NULL, NULL );

   if (png_ptr == NULL)
   {
      fclose(fp);
      return( -1 );
   }

   /* Allocate/initialize the memory for image information.  REQUIRED. */
   info_ptr = png_create_info_struct(png_ptr);
   if (info_ptr == NULL)
   {
      fclose(fp);
      png_destroy_read_struct(&png_ptr, NULL, NULL);
      return( -1 );
   }

   /* Set error handling if you are using the setjmp/longjmp method (this is
    * the normal method of doing things with libpng).  REQUIRED unless you
    * set up your own error handlers in the png_create_read_struct() earlier.
    */

   if (setjmp(png_jmpbuf(png_ptr)))
   {
      /* Free all of the memory associated with the png_ptr and info_ptr */
      png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
      fclose(fp);
      /* If we get here, we had a problem reading the file */
      return( -1 );
   }

   /* Set up the input control if you are using standard C streams */
   png_init_io(png_ptr, fp);

   /* If we have already read some of the signature */
   png_set_sig_bytes(png_ptr, sig_read);

   /*
    * If you have enough memory to read in the entire image at once,
    * and you need to specify only transforms that can be controlled
    * with one of the PNG_TRANSFORM_* bits (this presently excludes
    * dithering, filling, setting background, and doing gamma
    * adjustment), then you can read the entire image (including
    * pixels) into the info structure with this call:
    */
   png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

   /* At this point you have read the entire image */
   
   row_pointers = png_get_rows( png_ptr, info_ptr );
   width = png_get_image_width( png_ptr, info_ptr );
   height = png_get_image_height( png_ptr, info_ptr );

   buffer->buffer_size = buffer->format.buffer_size = width * height * buffer->format.bpp / 8;
   buffer->format.size.width = width;
   buffer->format.size.height = height;
   buffer->data = malloc( buffer->buffer_size );
   

   for( i = 0; i < height; i++ )
   {
      int j;
      unsigned char *dest;
      int channels = png_get_channels( png_ptr, info_ptr );

      dest = buffer->data + ( i * ( buffer->format.size.width * buffer->format.bpp / 8 ) );

	 
      switch( buffer->format.fourcc )
      {
	 case( UCIL_FOURCC( 'Y', 'U', 'V', 'A' ) ):
	 {
	    for( j = 0; j < width; j++ )
	    {
	       int xoff = j * channels;
	       unsigned char r,g,b,a;
	       int y, u, v;
	       
	       r = row_pointers[i][xoff];
	       g = row_pointers[i][xoff+1];
	       b = row_pointers[i][xoff+2];
	       a = channels == 4 ? row_pointers[i][xoff+3] : 255;
	       
	       y = ( 0.2990 * (float)r + 0.5870 * (float)g + 0.1140 * (float)b );
	       u = (-0.1687 * (float)r - 0.3313 * (float)g + 0.5000 * (float)b + 128.0 );
	       v = ( 0.5000 * (float)r - 0.4187 * (float)g - 0.0813 * (float)b + 128.0);
	       
	       *dest++ = y;
	       *dest++ = u;
	       *dest++ = v;
	       *dest++ = a;
	    }
	 }
	 break;

	 case( UCIL_FOURCC( 'R', 'G', 'B', 'A' ) ):
	 {
	    memcpy( dest, row_pointers[i], width * 4 );
	    dest += width * 4;
	 }
	 break;

	 default:
	 {
	    TRACE( "Operation not defined for color format!\n" );
	 }
      }
   }

   /* clean up after the read, and free any memory allocated - REQUIRED */
   png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

   /* close the file */
   fclose(fp);

   /* that's it */
   return( 0 );
}


//write a png file
/* int write_png(char *file_name, unicap_data_buffer_t *buffer) */
/* { */
/*    FILE *fp; */
/*    png_structp png_ptr; */
/*    png_infop info_ptr; */
/*    png_colorp palette; */

/*    if( ( buffer->format.fourcc != UCIL_FOURCC( 'R', 'G', 'B', 0 ) ) && */
/*        ( buffer->format.fourcc != UCIL_FOURCC( 'R', 'G', 'B', '3' ) ) && */
/*        ( buffer->format.fourcc != UCIL_FOURCC( 'R', 'G', 'B', '4' ) ) && */
/*        ( buffer->format.fourcc != UCIL_FOURCC( 'R', 'G', 'B', 'A' ) ) ) */
/*    { */
/*       g_warning( "write_png: unsupported color format!" ); */
/*       return -1; */
/*    } */
   

/*    /\* open the file *\/ */
/*    fp = fopen(file_name, "wb"); */
/*    if (fp == NULL) */
/*    { */
/*       return( -1 ); */
/*    } */
   

/*    /\* Create and initialize the png_struct with the desired error handler */
/*     * functions.  If you want to use the default stderr and longjump method, */
/*     * you can supply NULL for the last three parameters.  We also check that */
/*     * the library version is compatible with the one used at compile time, */
/*     * in case we are using dynamically linked libraries.  REQUIRED. */
/*     *\/ */
/*    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, */
/* 				     NULL, NULL, NULL); */

/*    if (png_ptr == NULL) */
/*    { */
/*       fclose(fp); */
/*       return( -1 ); */
/*    } */

/*    /\* Allocate/initialize the image information data.  REQUIRED *\/ */
/*    info_ptr = png_create_info_struct(png_ptr); */
/*    if (info_ptr == NULL) */
/*    { */
/*       fclose(fp); */
/*       png_destroy_write_struct(&png_ptr,  png_infopp_NULL); */
/*       return( -1 ); */
/*    } */

/*    /\* Set error handling.  REQUIRED if you aren't supplying your own */
/*     * error handling functions in the png_create_write_struct() call. */
/*     *\/ */
/*    if (setjmp(png_jmpbuf(png_ptr))) */
/*    { */
/*       /\* If we get here, we had a problem reading the file *\/ */
/*       fclose(fp); */
/*       png_destroy_write_struct(&png_ptr, &info_ptr); */
/*       return( -1 ); */
/*    } */

/*    /\* set up the output control if you are using standard C streams *\/ */
/*    png_init_io(png_ptr, fp); */

/*    /\* This is the easy way.  Use it if you already have all the */
/*     * image info living info in the structure.  You could "|" many */
/*     * PNG_TRANSFORM flags into the png_transforms integer here. */
/*     *\/ */
/*    png_write_png( png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, png_voidp_NULL ); */

/*    /\* If you png_malloced a palette, free it here (don't free info_ptr->palette, */
/*       as recommended in versions 1.0.5m and earlier of this example; if */
/*       libpng mallocs info_ptr->palette, libpng will free it).  If you */
/*       allocated it with malloc() instead of png_malloc(), use free() instead */
/*       of png_free(). *\/ */
/* /\*    png_free(png_ptr, palette); *\/ */
/* /\*    palette=NULL; *\/ */

/*    /\* Similarly, if you png_malloced any data that you passed in with */
/*       png_set_something(), such as a hist or trans array, free it here, */
/*       when you can be sure that libpng is through with it. *\/ */
/* /\*    png_free(png_ptr, trans); *\/ */
/* /\*    trans=NULL; *\/ */

/*    /\* clean up after the write, and free any memory allocated *\/ */
/*    png_destroy_write_struct( &png_ptr, &info_ptr ); */

/*    /\* close the file *\/ */
/*    fclose( fp ); */

/*    /\* that's it *\/ */
/*    return( 0 ); */
/* } */


unicap_status_t ucil_load_png( char *filename, unicap_data_buffer_t *buffer )
{
   FILE *f;
   
   if( !check_if_png( filename, &f ) )
   {
      TRACE( "File '%s' is not a valid PNG image\n", filename );
      return STATUS_FAILURE;
   }
   
   if( read_png( f, PNG_BYTES_TO_CHECK, buffer ) < 0 )
   {
      TRACE( "File '%s' could not be loaded\n", filename );
      return STATUS_FAILURE;
   }
   
   return STATUS_SUCCESS;
}

