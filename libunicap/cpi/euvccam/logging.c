#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

FILE *g_logfp = NULL;
int g_log_modules_mask = 0xffff;
int g_log_level = 0;


void log_init( void )
{
   char *tmp = getenv( "UNICAP_EUVCCAM_LOG_PATH" );
   
   if( tmp )
      g_logfp = fopen( tmp, "w" );
   
   tmp = getenv( "UNICAP_EUVCCAM_LOG_LEVEL" );
   if( tmp )
      g_log_level = atoi( tmp );
   
   tmp = getenv( "UNICAP_EUVCCAM_LOG_MODULES_MASK" );
   if( tmp )
      g_log_modules_mask = atoi( tmp );
}

void log_close( void )
{
   if( g_logfp )
      fclose( g_logfp );
   g_logfp = NULL;
}

void log_message( int module, int log_level, const char *format, ... )
{
   if( ( module & g_log_modules_mask ) && ( log_level > g_log_level ) ){
      char message[ 128 ];
      va_list ap;
      va_start( ap, format );
      vsnprintf( message, sizeof( message ), format, ap );
      va_end( ap );
      if( g_logfp ){
	 fwrite( message, strlen( message ), 1, g_logfp );
	 fflush( g_logfp );
      }else{
	 printf( "%s", message );
      }
   }   
}
