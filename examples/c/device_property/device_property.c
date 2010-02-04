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
   device_property
   
   Print a list of supported properties and let the user edit the value of a specific property

**/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <unicap.h>
#include <unicap_status.h>

struct flag_string
{
      char *name;
      unsigned long long flag;
};

static struct flag_string property_flag_strings[] =
{
   { "enable", UNICAP_FLAGS_ON_OFF },
   { "manual", UNICAP_FLAGS_MANUAL },
   { "auto", UNICAP_FLAGS_AUTO }, 
   { "onepush", UNICAP_FLAGS_ONE_PUSH },
};
   

static char *property_type_strings[] =
{
   "RANGE", 
   "VALUE_LIST", 
   "MENU", 
   "DATA", 
   "FLAGS"
};



void print_usage( char *progname )
{
   printf( "Usage: \n" );
   printf( "%s PropertyId [-M Model] [-v Value] [-m MenuItem] [-l]\n", progname );
   printf( "\n" );
}


int main( int argc, char **argv )
{
   unicap_handle_t handle;
   unicap_device_t device;
   unicap_device_t device_spec;
   unicap_property_t property;
   unicap_property_t property_spec;

   int list_only = 0;
   int get_property = 0;
   int value_set = 0;
	
   int i;
   int c = 0;

   unicap_void_device( &device_spec );
   unicap_void_property( &property );
	
   while( c != -1 )
   {
      c = getopt( argc, argv, "gp:M:v:m:l" );
      switch( c )
      {
	 case 'p':
	    if( optarg )
	    {
	       strcpy( property.identifier, optarg );
	    }
	    break;
		      
	 case 'M':
	    if( optarg )
	    {
	       printf( "Model: %s\n", optarg );
	       strcpy( device_spec.model_name, optarg );
	    }
	    else
	    {
	       print_usage( argv[0] );
	       exit( 1 );
	    }
	    break;
				
	 case 'v':
	    if( optarg )
	    {
	       sscanf( optarg, "%f", &property_spec.value );
	       value_set = 1;
	    }
	    else
	    {
	       print_usage( argv[0] );
	       exit( 1 );
	    }
	    break;
				
	 case 'm':
	    if( optarg )
	    {
	       strcpy( property.menu_item, optarg );
	    }
	    else
	    {
	       print_usage( argv[0] );
	       exit( 1 );
	    }
	    break;
				
	 case 'l':
	    list_only = 1;
	    break;
	 case 'g':
	    get_property = 1;
	    break;
				
	 case -1:
	    printf( "optind: %d\n", optind );
	    break;
				
	 default:
	    print_usage( argv[0] );
	    exit( 1 );
	    break;
      }
   }
	
	

   //
   // Enumerate all video capture devices; if more than 1 device present, ask for which device to open
   //
   for( i = 0; SUCCESS( unicap_enumerate_devices( &device_spec, &device, i ) ); i++ )
   {
      printf( "%i: %s\n", i, device.identifier );
   }
   if( --i > 0 )
   {
      printf( "Select video capture device: " );
      scanf( "%d", &i );
   }

   if( !SUCCESS( unicap_enumerate_devices( NULL, &device, i ) ) )
   {
      fprintf( stderr, "Failed to get info for device '%s'\n", device.identifier );
      exit( 1 );
   }

   /*
     Acquire a handle to this device
   */
   if( !SUCCESS( unicap_open( &handle, &device ) ) )
   {
      fprintf( stderr, "Failed to open device: %s\n", device.identifier );
      exit( 1 );
   }

   printf( "Opened video capture device: %s\n", device.identifier );

	
   // Initialize a property specifier structure
   unicap_void_property( &property_spec );
   strcpy( property_spec.identifier, property.identifier );

   printf( "Supported properties: \n" );
   printf( "\tName\t\tValue\n" );

   //
   // Print a list of all properties matching the specifier. For a "void" specifier, this matches all 
   //   properties supported by the device
   // If more than 1 property is supported, ask for which property to modify
   for( i = 0; SUCCESS( unicap_enumerate_properties( handle, &property_spec, &property, i ) ); i++ )
   {
      unicap_get_property( handle, &property );
      printf( "%i: \t%s\t\t", i, property.identifier );
      switch( property.type )
      {
	 case UNICAP_PROPERTY_TYPE_RANGE:
	 case UNICAP_PROPERTY_TYPE_VALUE_LIST:
	    printf( "%f", property.value );
	    break;
		      
	 case UNICAP_PROPERTY_TYPE_MENU:
	    printf( "%s", property.menu_item );
	    break;
		      
	 case UNICAP_PROPERTY_TYPE_FLAGS:
	 {
	    int j;
	    const char *flags[] =
	       { "MANUAL", 
		 "AUTO", 
		 "ONE_PUSH", 
		 "READ_OUT", 
		 "ON_OFF", 
		 "READ_ONLY", 
		 "FORMAT_CHANGE" 
	       };
		      
		      
	    for( j = 0; j < ( sizeof( flags ) / sizeof( char* ) ); j++ )
	    {
	       if( ( property.flags & ( 1<<j ) ) == ( 1<<j ) )
	       {
		  printf( "%s ",flags[j] );
	       }
	    }
	 }
	 break;
		   
	 default:
	    break;
      }
		
      printf( "\n" );
   }
   if( --i > 1 )
   {
      printf( "Select a property to edit: " );
      scanf( "%d", &i );
   }

   // Now get the description of the requested property. unicap_enumerate_property gives the 
   // property structure initialized to the default values. So just change the value of the 
   // returned property to the new one and set it using 'unicap_set_property'
   if( !SUCCESS( unicap_enumerate_properties( handle, &property_spec, &property, i ) ) )
   {
      printf( "Failed to enumerate property\n" );
      exit( -1 );
   }

   
   switch( property.type )
   {
      case UNICAP_PROPERTY_TYPE_RANGE:
      case UNICAP_PROPERTY_TYPE_VALUE_LIST:
      {
	 if( !value_set )
	 {
	    double new_value;
	    printf( "Enter new value for property '%s' ( floating point value ): ", property.identifier );
	    scanf( "%lf", &new_value );
	    property.value = new_value;
	    property.flags = UNICAP_FLAGS_MANUAL;
	 }
	 else
	 {
	    property.value = property_spec.value;
	 }
      }
      break;
	   
      case UNICAP_PROPERTY_TYPE_MENU:
      {
	 printf( "Enter new value for property '%s' ( string ): ", property.identifier );
	 scanf( "%128s", property.menu_item );
      }
      break;

      case UNICAP_PROPERTY_TYPE_FLAGS:
      {
	 int i;
	 char buf[128];
	 char *str;
	 printf( "Enter new flags for property '%s' ( out of ", property.identifier );

	 for( i = 0; i < ( sizeof( property_flag_strings ) / sizeof( struct flag_string ) ); i++ )
	 {
	    if( property.flags_mask & property_flag_strings[i].flag )
	    {
	       printf( "%s ", property_flag_strings[i].name );
	    }
	 }
	 printf( "): " );
	 scanf( "%128s", buf );
	 str = strtok( buf, " " );
	 property.flags = 0;
	 while( str )
	 {
	    for( i = 0; i < ( sizeof( property_flag_strings ) / sizeof( struct flag_string ) ); i++ )
	    {
	       if( !strcmp( str, property_flag_strings[i].name ) )
	       {
		  property.flags |= property_flag_strings[i].flag;
		  break;
	       }
	    }
		 
	    if( i == ( sizeof( property_flag_strings ) / sizeof( struct flag_string ) ) )
	    {
	       printf( "Unknown flag: %s\n", str );
	       break;
	    }
	    str = strtok( NULL, " " );
	 }
      }   
   }
	

   if( !get_property )
   {
      if( !SUCCESS( unicap_set_property( handle, &property ) ) )
      {
	 printf( "Failed to set property!\n" );
      }
   }
   else
   {
      char buffer[512];
      int s = sizeof( buffer );
      if( !SUCCESS( unicap_get_property( handle, &property ) ) )
      {
	 printf( "Failed to set property!\n" );
      }

      unicap_describe_property( &property, buffer, &s );
	   
      printf( "%s\n", buffer );
   }
	

   unicap_close( handle );
	
	
   return 0;
}
