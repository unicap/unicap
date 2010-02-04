/*
  unicap
  Copyright (C) 2004  Arne Caspari

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

/**
   device_list.c

   Sample program demonstrating the basic usage of the unicap library

   
   This example opens all devices found by the unicap library and outputs their name, 
   supported properties and supported formats to stdout. 

**/


#include <stdlib.h>
#include <stdio.h>
#include <unicap.h>
#include <unicap_status.h>

void print_property( unicap_property_t *property );



int main( int argc, char **argv )
{
   unicap_handle_t handle;
   unicap_device_t device;

   int i = 0;

   printf( "\n\nUnicap device list:\n\n" );
	
   while( SUCCESS( unicap_enumerate_devices( NULL, &device, i++ ) ) )
      {
	 unicap_property_t property;
	 unicap_format_t format;
	 int property_count;
	 int format_count;
	 int j;

	 if( !SUCCESS( unicap_open( &handle, &device ) ) )
	    {
	       fprintf( stderr, "Failed to open: %s\n", device.identifier );
	       continue;
	    }
		
	 unicap_reenumerate_properties( handle, &property_count );
	 unicap_reenumerate_formats( handle, &format_count );
		
	 printf( "Device: %s\n", device.identifier );
	 printf( "\tInfo:\n" );
	 printf( "\t\tModel: %s\n", device.model_name );
	 printf( "\t\tVendor: %s\n", device.vendor_name );
	 printf( "\t\tcpi: %s\n", device.cpi_layer );
	 printf( "\t\tdevice: %s\n", device.device );

	 printf( "\tProperties[%d]:\n", property_count );
		
	 for( j = 0; SUCCESS( unicap_enumerate_properties( handle, NULL, &property, j ) ); j++ )
	    {
	       unicap_get_property( handle, &property );
	       print_property( &property );
	    }
		
	 printf( "\tFormats[%d]:\n", format_count );

	 for( j = 0; SUCCESS( unicap_enumerate_formats( handle, NULL, &format, j ) ); j++ )
	    {
	       int k;

	       printf( "\t\t%s\n", format.identifier );
	       printf( "\t\t\tFOURCC: %c%c%c%c\n", format.fourcc, format.fourcc>>8, 
		       format.fourcc>>16, format.fourcc>>24 );
	       printf( "\t\t\tsize: %dx%dx%d\n", format.size.width, format.size.height, format.bpp );

	       printf( "\t\t\tsize_count: %d\n", format.size_count );
	       for( k = 0; k < format.size_count; k++ )
		  {
		     printf( "\t\t\t\t%dx%d\n", format.sizes[k].width, format.sizes[k].height );
		  }

	    }
		
	 unicap_close( handle );
      }
	
   return 0;
}

void print_property( unicap_property_t *property )
{
   printf( "\t\t%s\n", property->identifier );

   switch( property->type ){
   case UNICAP_PROPERTY_TYPE_RANGE:
      {
	 printf( "\t\t\tmin_value: %f\n", property->range.min );
	 printf( "\t\t\tmax_value: %f\n", property->range.max );
	 printf( "\t\t\tstepping : %f\n", property->stepping );
	 printf( "\t\t\tvalue: %f\n", property->value );
      }
      break;

      case UNICAP_PROPERTY_TYPE_MENU:
      {
	 int i;
			
	 printf( "\t\t\tmenu items: %d\n", property->menu.menu_item_count );
			
	 for( i = 0; i < property->menu.menu_item_count; i++ )
	    {
	       printf( "\t\t\t\t[%d]: %s", i, property->menu.menu_items[i] );
	       if( !strcmp( property->menu.menu_items[i], property->menu_item ) )
		  {
		     printf( " (*)\n" );
		  }
	       else
		  {
		     printf( "\n" );
		  }
	    }
      }
      break;

      case UNICAP_PROPERTY_TYPE_VALUE_LIST:
      {
	 int i;
	 printf( "\t\t\tvalid values: %d\n", property->value_list.value_count );
			
	 for( i = 0; i < property->value_list.value_count; i++ )
	    {
	       printf( "\t\t\t\t[%d]: %f", i, property->value_list.values[i] );
	       if( property->value_list.values[i] == property->value )
		  {
		     printf( " (*)\n" );
		  }
	       else
		  {
		     printf( "\n" );
		  }
	    }
      }
      break;
   }
   
   if( property->relations_count ){
      int i;
      printf( "\t\t\tRelations[%d]:\n", property->relations_count );
      for( i = 0; i < property->relations_count; i++ ){
	 printf( "\t\t\t\t%s\n", property->relations[i] );
      }
   }

}
