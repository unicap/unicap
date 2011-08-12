/*
  unicap euvccam plugin

  Copyright (C) 2009  Arne Caspari

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
