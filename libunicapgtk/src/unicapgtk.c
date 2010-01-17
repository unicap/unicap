/* unicap
 *
 * Copyright (C) 2006 Arne Caspari ( arne_caspari@users.sourceforge.net )
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

#include "config.h"

#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <unicap.h>
#include <string.h>
#include <stdlib.h>

#if UNICAPGTK_DEBUG
#define DEBUG
#endif
#include <debug.h>

#include "unicapgtk_version.h"
#include "unicapgtk.h"

const unsigned int unicapgtk_major_version=UNICAPGTK_MAJOR_VERSION;
const unsigned int unicapgtk_minor_version=UNICAPGTK_MINOR_VERSION;
const unsigned int unicapgtk_micro_version=UNICAPGTK_MICRO_VERSION;
static gboolean unicapgtk_is_initialized = FALSE;

#if GLIB_CHECK_VERSION(2,12,0)

#define KEY_FILE_GET_DOUBLE g_key_file_get_double
#define KEY_FILE_SET_DOUBLE g_key_file_set_double

#else

#define KEY_FILE_GET_DOUBLE unicapgtk_key_file_get_double
#define KEY_FILE_SET_DOUBLE unicapgtk_key_file_set_double


static gdouble unicapgtk_key_file_get_double( GKeyFile *key_file, 
					      const gchar *group_name, 
					      const gchar *key, 
					      GError **error )
{
   gchar *str;
   gdouble res = 0;
   
   str = g_key_file_get_string( key_file, group_name, key, error );
   if( str )
   {
      res = g_ascii_strtod( str, NULL );
      g_free( str );
   }

   return res;
}

static void unicapgtk_key_file_set_double( GKeyFile *key_file, 
					   const gchar *group_name, 
					   const gchar *key, 
					   gdouble value )
{
   gchar str[64];
   
   g_sprintf( str, "%f", value );
   g_key_file_set_string( key_file, group_name, key, str );
}

#endif

/**
 * unicapgtk_get_user_config_path: 
 * 
 * Get path to the directory containing the configuration files
 *
 * Returns: Path to configuration directory
 *
 */
gchar *unicapgtk_get_user_config_path( void )
{
   char *homepath;
   gchar *path;
   
   homepath = getenv( "HOME" );
   if( !homepath )
   {
      return NULL;
   }
   
   path = g_build_path( G_DIR_SEPARATOR_S, homepath, ".unicap", NULL );
   if( g_file_test( path, G_FILE_TEST_EXISTS ) )
   {
      if( !g_file_test( path, G_FILE_TEST_IS_DIR ) )
      {
	 g_warning( "%s is not a directory\n", path );
	 g_free( path );
	 return NULL;
      }
   }
   else
   {
      if( g_mkdir( path, 0755 ) )
      {
	 g_warning( "Failed to create '%s'\n", path );
	 g_free( path );
	 return NULL;
      }
   }

   return path;
}

/**
 * unicapgtk_check_version:
 * @major: Major version to check
 * @minor: Minor version to check
 * @micro: Micro version to check
 *
 * Checks whether the installed library version is newer or equal to
 * the given version
 *
 * Returns: 
 * STATUS_INCOMPATIBLE_MAJOR_VERSION if the given major version is too old
 * STATUS_INCOMPATIBLE_MINOR_VERSION if the given minor version is too old
 * STATUS_INCOMPATIBLE_MACRO_VERSION if the given macro version is too old
 * STATUS_SUCCESS if the version is compatible
 */
unicap_status_t unicapgtk_check_version( unsigned int major, unsigned int minor, unsigned int micro )
{
   unicap_status_t status = STATUS_SUCCESS;
   
   if( UNICAPGTK_MAJOR_VERSION < major )
   {
      status = STATUS_INCOMPATIBLE_MAJOR_VERSION;
   }
   else if( major == UNICAPGTK_MAJOR_VERSION )
   {
      if( UNICAPGTK_MINOR_VERSION < minor )
      {
	 status = STATUS_INCOMPATIBLE_MINOR_VERSION;
      }
      else if( minor == UNICAPGTK_MINOR_VERSION )
      {
	 if( UNICAPGTK_MICRO_VERSION < micro )
	 {
	    status = STATUS_INCOMPATIBLE_MICRO_VERSION;
	 }
      }
   }
   
   return status;
}

/**
 * unicapgtk_save_device_state: 
 * @handle: unicap handle
 * @flags: flags
 *
 * Saves the current device state. If UNICAPGTK_DEVICE_STATE_VIDEO_FORMAT is set in the @flags field, a section containing information about
 * the current video format is added to the configuration file. If UNICAPGTK_DEVICE_STATE_PROPERTIES is set, a section containing the current 
 * state of the device properties is added to the configuration file. 
 *
 * Returns: A newly allocated GKeyfile. Must be freed with g_key_file_free() by the application.
 */
GKeyFile *unicapgtk_save_device_state( unicap_handle_t handle, UnicapgtkDeviceStateFlags flags )
{
   GKeyFile *keyfile;
   unicap_device_t device;
   

   if( !SUCCESS( unicap_get_device( handle, &device ) ) )
   {
      return NULL;
   }

   keyfile = g_key_file_new();
   g_key_file_set_string( keyfile, "Device", "Identifier", device.identifier );
   
   if( flags & UNICAPGTK_DEVICE_STATE_VIDEO_FORMAT )
   {
      unicap_format_t format;
      unicap_get_format( handle, &format );
      g_key_file_set_string( keyfile, "VideoFormat", "Identifier", format.identifier );
      g_key_file_set_integer( keyfile, "VideoFormat", "FourCC", format.fourcc );
      g_key_file_set_integer( keyfile, "VideoFormat", "Width", format.size.width );
      g_key_file_set_integer( keyfile, "VideoFormat", "Height", format.size.height );
   }

   
   if( flags & UNICAPGTK_DEVICE_STATE_PROPERTIES )
   {
      unicap_property_t property;
      int i;

      for( i = 0; SUCCESS( unicap_enumerate_properties( handle, NULL, &property, i ) ); i++ )
      {
	 gchar *group;
	 unicap_status_t status;
      
	 group = g_strconcat( "Prop-", property.identifier, NULL );
	 status = unicap_get_property( handle, &property );
	 if( !SUCCESS( status ) )
	 {
	    // TODO: 
	    // Check if device is still present
	    break;
	 }

	 switch( property.type )
	 {
	    case UNICAP_PROPERTY_TYPE_RANGE:
	    case UNICAP_PROPERTY_TYPE_VALUE_LIST:
	    {
	       KEY_FILE_SET_DOUBLE( keyfile, group, "Value", property.value );
	    }
	    break;
	 
	    case UNICAP_PROPERTY_TYPE_MENU:
	    {
	       g_key_file_set_string( keyfile, group, "Menu", property.menu_item );
	    }
	    break;
	 
	    default:
	       break;
	 }

	 g_key_file_set_integer( keyfile, group, "FlagsLo", (int)(property.flags & 0xffffffff ) );
	 g_key_file_set_integer( keyfile, group, "FlagsHi", (int)(property.flags >> 31 ) );
      
	 g_free( group );
      }   
   }

   return keyfile;
}

/**
   unicapgtk_load_device_state:
   @handle: handle of the device
   @keyfile: GKeyFile to load the device state information from
   @flags: #UnicapgtkDeviceStateFlags specifying what parts to
   configure
   Reads device state information from a key-file and sets the
   parameters of a device accordingly. 
 */
gboolean unicapgtk_load_device_state( unicap_handle_t handle, GKeyFile *keyfile, UnicapgtkDeviceStateFlags flags )
{
   unicap_device_t device;
   gchar *str;
   GError *err = NULL;
   
   if( !SUCCESS( unicap_get_device( handle, &device ) ) )
   {
      return FALSE;
   }

   str = g_key_file_get_string( keyfile, "Device", "Identifier", NULL );
   if( !str || strcmp( device.identifier, str ) )
   {
      str = g_key_file_get_string( keyfile, "Device", "Vendor", NULL );
      if( !str || strcmp( device.vendor_name, str ) )
      {
	 g_warning( "KeyFile does not match handle\n" );
	 return FALSE;
      }
      
      str = g_key_file_get_string( keyfile, "Device", "Model", NULL );
      if( !str || strcmp( device.model_name, str ) )
      {
	 g_warning( "KeyFile does not match handle\n" );
	 return FALSE;
      }
   }
   
   if( flags & UNICAPGTK_DEVICE_STATE_VIDEO_FORMAT )
   {
      unicap_format_t format_spec, format;

      unicap_void_format( &format_spec );
      str = g_key_file_get_string( keyfile, "VideoFormat", "Identifier", &err );
      if( !str )
      {
	 g_warning( "Format identifier missing\n" );
	 return FALSE;
      }
      strcpy( format_spec.identifier, str );
   
      format_spec.fourcc = g_key_file_get_integer( keyfile, "VideoFormat", "FourCC", &err );
      if( err && ( err->code == G_KEY_FILE_ERROR_KEY_NOT_FOUND ) )
      {
	 g_warning( "FourCC missing\n" );
	 g_error_free( err );
	 return FALSE;
      }
   
      if( !SUCCESS( unicap_enumerate_formats( handle, &format_spec, &format, 0 ) ) )
      {
	 g_warning( "Failed to enumerate format\n" );
	 return FALSE;
      }

      format.size.width = g_key_file_get_integer( keyfile, "VideoFormat", "Width", &err );
      if( err && ( err->code == G_KEY_FILE_ERROR_KEY_NOT_FOUND ) )
      {
	 g_warning( "Width missing\n" );
	 g_error_free( err );
	 return FALSE;
      }
   
      format.size.height = g_key_file_get_integer( keyfile, "VideoFormat", "Height", &err );
      if( err && ( err->code == G_KEY_FILE_ERROR_KEY_NOT_FOUND ) )
      {
	 g_warning( "Height missing\n" );
	 g_error_free( err );
	 return FALSE;
      }

   
      if( !SUCCESS( unicap_set_format( handle, &format ) ) )
      {
	 g_warning( "Failed to set video format\n" );
	 return FALSE;
      }
   }
   
   if( flags & UNICAPGTK_DEVICE_STATE_PROPERTIES )
   {
      unicap_property_t property;
      int i;
      
      for( i = 0; SUCCESS( unicap_enumerate_properties( handle, NULL, &property, i ) ); i++ )
      {
	 gchar *group;
	 gint tmp;
      
	 group = g_strconcat( "Prop-", property.identifier, NULL );
      
	 switch( property.type )
	 {
	    case UNICAP_PROPERTY_TYPE_RANGE:
	    case UNICAP_PROPERTY_TYPE_VALUE_LIST:
	    {
	       property.value = KEY_FILE_GET_DOUBLE( keyfile, group, "Value", &err );
	       if( err )
	       {
		  TRACE( "load device state: Value missing for %s\n", group );
		  return FALSE;
	       }
	    }
	    break;
	 
	    case UNICAP_PROPERTY_TYPE_MENU:
	    {
	       str = g_key_file_get_string( keyfile, group, "Menu", &err );
	       if( err )
	       {
		  TRACE( "Menu item missing\n" );
		  return FALSE;
	       }
	       strcpy( property.menu_item, str );
	    }
	    break;

	    default:
	       break;
	 } 
      
	 tmp = g_key_file_get_integer( keyfile, group, "FlagsLo", &err );
	 if( err )
	 {
	    TRACE( "Failed to read FlagsLo\n" );
	    return FALSE;
	 }
      
	 property.flags = tmp;
      
	 tmp = g_key_file_get_integer( keyfile, group, "FlagsHi", &err );
	 if( err )
	 {
	    TRACE( "Failed to read FlagsHi\n" );
	    return FALSE;
	 }
      
	 property.flags |= ((unsigned long long)tmp)<<31;

	 if( !SUCCESS( unicap_set_property( handle, &property ) ) )
	 {
	    g_warning( "Failed to set property: %s\n", property.identifier );
	 }

	 g_free( group );
      }
   }

   return TRUE;
}

void unicapgtk_initialize( void )
{
   if( !unicapgtk_is_initialized )
   {
      bindtextdomain( GETTEXT_PACKAGE, UNICAP_LOCALEDIR );
      
      unicapgtk_is_initialized = TRUE;
   }
}
