/*
** count_devices.c
** 
** Made by (Arne Caspari)
** Login   <arne@arne-laptop>
** 
** Started on  Fri Jul 21 11:31:43 2006 Arne Caspari
** Last update Fri Jul 21 11:33:08 2006 Arne Caspari
*/


#include <stdlib.h>
#include <stdio.h>
#include <unicap.h>

int main( int argc, char **argv )
{
   unicap_handle_t handle;
   unicap_device_t device;
   
   int i = 0;
   
   printf( "\n\nUnicap device list:\n\n" );
   
   while( SUCCESS( unicap_enumerate_devices( NULL, &device, i++ ) ) )
   {
      
      if( !SUCCESS( unicap_open( &handle, &device ) ) )
      {
	 fprintf( stderr, "Failed to open: %s\n", device.identifier );
	 continue;
      }

      printf( "Device: %s\n", device.identifier );
      printf( "\tInfo:\n" );
      printf( "\t\tModel: %s\n", device.model_name );
      printf( "\t\tVendor: %s\n", device.vendor_name );
      printf( "\t\tcpi: %s\n", device.cpi_layer );
      printf( "\t\tdevice: %s\n", device.device );
      
      unicap_close( handle );
   }

   return 0;
}


