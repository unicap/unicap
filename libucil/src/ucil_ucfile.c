unicap_status_t ucil_read_ucfile( unicap_data_buffer_t **dest, gchar *filename )
{
   FILE *f;
   
   f = fopen( filename, "r" );
   if( !f ){
      return FALSE;
   }
   
   unicap_data_buffer_t *buffer = malloc( sizeof( unicap_data_buffer_t ) );
   gchar str[128];

   fgets( str, sizeof( str ), f );
   buffer->format.fourcc = str[0] | ( str[1] << 8 ) | ( str[2] << 16 ) | ( str[3] << 24 );
   fgets( str, sizeof( str ), f );
   buffer->format.bpp = atoi( str );
   fgets( str, sizeof( str ), f );
   buffer->format.size.width = atoi( str );
   fgets( str, sizeof( str ), f );
   buffer->format.size.height = atoi( str );
   
   g_printf( "File Info: %c%c%c%c %d bpp %dx%d\n", 
	     buffer->format.fourcc & 0xff, buffer->format.fourcc >> 8 & 0xff, buffer->format.fourcc >> 16 & 0xff, buffer->format.fourcc >> 24 & 0xff, 
	     buffer->format.bpp, buffer->format.size.width, buffer->format.size.height );
   buffer->buffer_size = buffer->format.size.width * buffer->format.size.height * buffer->format.bpp / 8;
   
}

gboolean write_frame( unicap_data_buffer_t *buffer, gchar *cam_name, gboolean y800_mode )
{
   gchar filename[128];
   gchar str[128];
   gchar timestr[128];
   gchar *path;
   gchar foldernum[128];
   struct tm *tm;
   int fd;

   gboolean ret = TRUE;
   gchar *tndata;
   GdkPixbuf *tn;

   int i;


   if( ( current_image_count % options.num_images_per_folder ) == 0 )      {
	 char tmp[128];
	 current_folder_nr++;
	 sprintf( tmp, "%d", current_folder_nr );
	 path = g_build_path( G_DIR_SEPARATOR_S, options.imagepath, tmp, NULL );
	 mkdir( path, 0777 );
	 g_free( path );
	 path = g_build_path( G_DIR_SEPARATOR_S, options.imagepath, tmp, TN_DIRNAME, NULL );
	 mkdir( path, 0777 );
	 g_free( path );
      }
      
   current_image_count++;
   sprintf( foldernum, "%d", current_folder_nr );

   buffer->fill_time.tv_usec += options.time_correction * 1000;
   if( buffer->fill_time.tv_usec < 0 ){
      buffer->fill_time.tv_usec += 1000000;
      buffer->fill_time.tv_sec -= 1;
   }

   tm = localtime( (time_t *)&buffer->fill_time.tv_sec );
/*    g_assert( tm ); */
   strftime( timestr, sizeof(timestr), "%Y_%m_%d_%H_%M_%S", tm );

   g_sprintf( filename, "%s_%s_%03d.uc", cam_name, timestr, buffer->fill_time.tv_usec/1000 );
   
   path = g_build_filename( options.imagepath, foldernum, filename, NULL );
   
   fd = open( path, O_CREAT | O_APPEND | O_RDWR, S_IRUSR | S_IWUSR );
   g_free( path );

   if( !fd ) {
      g_log( G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "'open' fehlgeschlagen : %s", path ); 
      perror( "open" );
      return FALSE;
   }
   

   g_sprintf( str, "%c%c%c%c\n", 
	      buffer->format.fourcc & 0xff, 
	      ( buffer->format.fourcc >> 8 ) & 0xff, 
	      ( buffer->format.fourcc >> 16 ) & 0xff, 
	      buffer->format.fourcc >>24 );
   g_return_if_fail( write_str( fd, str ) );
   
   g_sprintf( str, "%d\n", buffer->format.bpp );
   g_return_if_fail( write_str( fd, str ) );
   
   g_sprintf( str, "%d\n", buffer->format.size.width );
   g_return_if_fail( write_str( fd, str ) );
   
   g_sprintf( str, "%d\n", buffer->format.size.height );
   g_return_if_fail( write_str( fd, str ) );

   if( write( fd, buffer->data, buffer->buffer_size ) < buffer->buffer_size ){
      g_log( G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "'write' fehlgeschlagen : %s", path ); 
      perror( "write" );
      ret = FALSE;
   }


   for( i = 0; i < 8; i++ ){
      write( fd, &buffer->reserved[i], sizeof( unsigned int ) );
   }
   

   close( fd );

   if( !options.disable_thumbnails ) {
      gchar *params_path = NULL;
      gchar str[256];
      union
      {
	    double as_double;
	    struct
	    {
		  unsigned int hi;
		  unsigned int lo;
	    };
      }dval;
      double prop_shutter;
      double prop_gain;
      FILE *f;
      
      g_sprintf( filename, "%s_%s_%03d." TN_FORMAT, cam_name, timestr, buffer->fill_time.tv_usec/1000 );
      path = g_build_path( G_DIR_SEPARATOR_S, options.imagepath, foldernum, TN_DIRNAME, filename, NULL );
      tndata = malloc( (( buffer->format.size.width / 10 )+1) * (( buffer->format.size.height / 10 )+1) * 4 );
      if( !y800_mode ){
	 by8tn_swp( tndata, buffer->data, buffer->format.size.width, buffer->format.size.height, 10, 10 );
      } else {
	 y800tn( tndata, buffer->data, buffer->format.size.width, buffer->format.size.height, 10, 10 );
      }
      tn = gdk_pixbuf_new_from_data( tndata, GDK_COLORSPACE_RGB, TRUE, 
				     8, buffer->format.size.width / 10, buffer->format.size.height / 10, ( buffer->format.size.width / 10 ) * 4, 
				     (GdkPixbufDestroyNotify)free, NULL );

      if( tn )	{
	 gdk_pixbuf_save( tn, path, TN_FORMAT, NULL, NULL );
	 g_object_unref( tn );
      }
      
      params_path = g_strconcat( path, ".params", NULL );
      dval.hi = buffer->reserved[1];
      dval.lo = buffer->reserved[2];
      prop_shutter = dval.as_double;
      dval.hi = buffer->reserved[3];
      dval.lo = buffer->reserved[4];
      prop_gain = dval.as_double;

      sprintf( str, "%f\n%f\n", prop_shutter, prop_gain );
      
      f = fopen( params_path, "w" );
      fwrite( str, 1, strlen( str ) + 1, f );
      fclose( f );
      
      g_free( path );
      g_free( params_path );
      
   }
   
   return ret;
}
