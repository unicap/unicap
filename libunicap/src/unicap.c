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

#include "config.h"


#include <errno.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/file.h>

#include <fcntl.h>
#include <dirent.h>

#include <math.h>

#if !ENABLE_STATIC_CPI
#include <dlfcn.h> // dynamic loader
#else
#include "static_cpi.h"
#endif

#include "unicap.h"
#include "unicap_version.h"
#include "unicap_private.h"
#include "unicap_cpi.h"
#include "unicap_helpers.h"
#include "check_match.h"
#include "unicap_status.h"

#if UNICAP_DEBUG
#define DEBUG
#endif
#include "debug.h"

union semun_linux {
      int              val;    /* Value for SETVAL */
      struct semid_ds *buf;    /* Buffer for IPC_STAT, IPC_SET */
      unsigned short  *array;  /* Array for GETALL, SETALL */
      struct seminfo  *__buf;  /* Buffer for IPC_INFO
				  (Linux specific) */
};

struct unicap_device_cache{
   unicap_device_t device;
   unicap_handle_t handle;
};





static unicap_handle_t unicap_copy_handle( unicap_handle_t handle );
static struct unicap_device_cache *lookup_device_cache( unicap_device_t *device );


const unsigned int unicap_major_version=UNICAP_MAJOR_VERSION;
const unsigned int unicap_minor_version=UNICAP_MINOR_VERSION;
const unsigned int unicap_micro_version=UNICAP_MICRO_VERSION;

static int g_device_count = 0;
static unicap_device_t g_devices[UNICAP_MAX_DEVICES];
static int g_filter_remote = 0;

static struct unicap_device_cache g_device_cache[UNICAP_MAX_DEVICES];
static volatile int g_initialized = 0;


static void unicap_initialize( void )
{
   int i;
   if( !g_initialized ){
      for( i = 0; i < UNICAP_MAX_DEVICES; i++ ){
	 unicap_void_device( &g_device_cache[i].device );
	 g_device_cache[i].handle = NULL;
      }
      
      g_initialized = 1;
   }
}

#if ENABLE_STATIC_CPI
static int g_static_cpi_initialized = 0;

static void initialize_static_cpis()
{
   if( !g_static_cpi_initialized ){
      int i;

#if BUILD_DCAM
      unicap_dcam_register_static_cpi( &static_cpis_s[UNICAP_STATIC_CPI_DCAM] );
#endif
#if BUILD_VID21394
      unicap_vid21394_register_static_cpi( &static_cpis_s[UNICAP_STATIC_CPI_VID21394] );
#endif
#if BUILD_V4L
      unicap_v4l_register_static_cpi( &static_cpis_s[UNICAP_STATIC_CPI_V4L] );
#endif
#if BUILD_V4L2
      unicap_v4l2_register_static_cpi( &static_cpis_s[UNICAP_STATIC_CPI_V4L2] );
#endif
      static_cpis_s[UNICAP_STATIC_CPI_COUNT] = 0;
   }
   g_static_cpi_initialized = 1;
}
#endif

static struct unicap_device_cache *lookup_device_cache( unicap_device_t *device )
{
   int i;
   for( i = 0; i < UNICAP_MAX_DEVICES; i++ ){
      if( _check_device_match( device, &g_device_cache[i].device ) ){
	 return &g_device_cache[i];
      }
   }
   
   return NULL;
}

static void store_device_cache( unicap_device_t *device, unicap_handle_t handle )
{
   int i;
   
   for( i = 0; i < UNICAP_MAX_DEVICES; i++ ){
      if( g_device_cache[i].handle == NULL ){
	 TRACE ("Storing in cache: %s\n", device->identifier);
	 memcpy( &g_device_cache[i].device, device, sizeof( unicap_device_t ) );
	 g_device_cache[i].handle = unicap_clone_handle( handle );
	 break;
      }
   }
}

static void free_device_cache_entries( void )
{
   int i;

   for( i = 0; i < UNICAP_MAX_DEVICES; i++ ){
      if( g_device_cache[i].handle && ( *g_device_cache[i].handle->ref_count == 1 ) ){
	 unicap_close( g_device_cache[i].handle );
	 unicap_void_device( &g_device_cache[i].device );
	 g_device_cache[i].handle = NULL;
      }
   }   
}

unicap_status_t unicap_set_filter_remote( int do_filter )
{
   if( do_filter ){
      g_filter_remote = 1;
   }else{
      g_filter_remote = 0;
   }
	
   return STATUS_SUCCESS;
}

static unicap_status_t _unicap_acquire_cpi_lock( int sem_id )
{
#ifdef UNICAP_THREAD_LOCKING
   if( sem_id != -1 ){
      struct sembuf sops;
      sops.sem_num = 0;
      sops.sem_op = -1;
      sops.sem_flg = SEM_UNDO;
		
		
      if( semop( sem_id, &sops, 1 ) < 0 ){
	 return STATUS_FAILURE;
      }
		
   }
#endif
   return STATUS_SUCCESS;
}

static unicap_status_t _unicap_release_cpi_lock( int sem_id )
{
#ifdef UNICAP_THREAD_LOCKING
   if( sem_id != -1 )
   {
      struct sembuf sops;
      sops.sem_num = 0;
      sops.sem_op = 1;
      sops.sem_flg = SEM_UNDO;
		
      if( semop( sem_id, &sops, 1 ) < 0 ){
	 return STATUS_FAILURE;
      }
		
   }
#endif	
   return STATUS_SUCCESS;
}

static void unicap_event_callback( unicap_handle_t handle, unicap_event_t event, ... )
{

   if( !handle->cb_info || !handle->cb_info[event].func ){
      return;
   }
   
   switch( event ){
      case UNICAP_EVENT_DEVICE_REMOVED:
      {
	 handle->cb_info[event].func( event, handle, handle->cb_info[event].user_ptr );
      }
      break;
      
      case UNICAP_EVENT_NEW_FRAME:
      {
	 va_list ap;
	 unicap_data_buffer_t *buffer;
	 
	 va_start( ap, event );
	 buffer = va_arg(ap, unicap_data_buffer_t *);
	 va_end( ap );
	 
	 handle->cb_info[event].func( event, handle, buffer, handle->cb_info[event].user_ptr );
      }
      break;
      
      default:
      {
	 handle->cb_info[event].func( event, handle, handle->cb_info[event].user_ptr );
      }
      break;
   }
}

unicap_status_t unicap_check_version( unsigned int major, unsigned int minor, unsigned int micro )
{
   unicap_status_t status = STATUS_SUCCESS;
   
   if( UNICAP_MAJOR_VERSION < major ){
      status = STATUS_INCOMPATIBLE_MAJOR_VERSION;
   }else if( major == UNICAP_MAJOR_VERSION ){
      if( UNICAP_MINOR_VERSION < minor ){
	 status = STATUS_INCOMPATIBLE_MINOR_VERSION;
      }
      else if( minor == UNICAP_MINOR_VERSION ){
	 if( UNICAP_MICRO_VERSION < micro ){
	    status = STATUS_INCOMPATIBLE_MICRO_VERSION;
	 }
      }
   }
   
   return status;
}

unicap_status_t unicap_reenumerate_devices( int *_pcount )
{
   int count = 128;

   unicap_initialize();

   unicap_real_enumerate_devices( &count );
	
   g_device_count = count;
	
   if( _pcount ){
      *_pcount = count;
   }
	
   return g_device_count ? STATUS_SUCCESS : STATUS_NO_DEVICE;
}

unicap_status_t unicap_enumerate_devices( unicap_device_t *specifier, unicap_device_t *device, int index )
{
   int current_index = -1;
	
   unicap_initialize();

   if( g_device_count != 0){
      int i;
		
      if( index >= g_device_count ){
	 return STATUS_NO_MATCH;
      }
		
      for( i = 0; i < g_device_count; i++ ){
	 if( _check_device_match( specifier, &g_devices[i] ) ){
	    current_index++;
	    if( current_index == index ){
	       memcpy( device, &g_devices[i], sizeof( unicap_device_t ) );
	       return STATUS_SUCCESS;
	    }
	 }
      }
   } else {
      if( SUCCESS( unicap_reenumerate_devices( NULL ) ) ){
	 if( g_device_count ){
	    return unicap_enumerate_devices( specifier, device, index );
	 }
      }
   }

   return STATUS_NO_MATCH;
}

#if !ENABLE_STATIC_CPI
static unicap_status_t enumerate_devices( int *count )
{
   void *dlhandle;
   cpi_register_t cpi_register;
   unicap_status_t status = STATUS_SUCCESS;
   struct _unicap_cpi cpi;

   int current_index = 0;

   DIR *cpi_dir;

   struct dirent *cpi_dirent;
   char *dirname;

   char buffer[PATH_MAX];	
   const char suffix[] = ".so";

   strcpy( buffer, PKGLIBDIR "/cpi" );

   TRACE( "cpi search path: %s, suffix: %s\n", buffer, suffix ); 	
   cpi_dir = opendir( buffer );
   if( !cpi_dir ){
      return STATUS_FAILURE;
   }

   dirname = buffer;

   while( ( cpi_dirent = readdir( cpi_dir ) ) > 0 ){
      char filename[512];
      int tmp_index = 0;
      sprintf( filename, "%s/%s", dirname, cpi_dirent->d_name );
      if( ( !strstr( filename, suffix ) ) ||
	  ( strlen( strstr( filename, suffix ) ) != strlen( suffix ) ) ){
	 continue;
      }
		
      TRACE( "open cpi: %s\n", filename );
		
      // load the cpi as a .so
      dlhandle = dlopen( filename, RTLD_NOW );
      if( !dlhandle ){
/* 	 TRACE( "cpi load: %s\n", dlerror() ); */
	 fprintf( stderr, "cpi load '%s': %s\n", filename, dlerror() );
	 continue;
      }
		
      cpi_register = dlsym( dlhandle, "cpi_register" );
      if( !cpi_register ){
	 TRACE( "cpi load: %s\n", dlerror() );
	 dlclose( dlhandle );
	 continue;
      }

      memset( &cpi, 0x0, sizeof( struct _unicap_cpi ) );

      cpi_register( &cpi );

      if( g_filter_remote && ( cpi.cpi_flags & UNICAP_CPI_REMOTE ) ){
	 TRACE( "cpi load: remote cpi filtered out\n" );
	 dlclose( dlhandle );
	 continue;
      }
		
      if( CPI_VERSION_HI(cpi.cpi_version) > 2 ){
	 TRACE( "cpi load: unknown version\n" );
	 dlclose( dlhandle );
	 continue;
      }
		
      TRACE( "cpi.cpi_enumerate_devices: %p\n", cpi.cpi_enumerate_devices );

      // look at all devices supplied by the cpi
      while( ( current_index < *count ) && 
	     ( SUCCESS( status = cpi.cpi_enumerate_devices( &g_devices[current_index], tmp_index ) ) ) ){
	 TRACE( "current_index: %d\n", current_index );
	 strcpy( g_devices[current_index].cpi_layer, filename );
	 current_index++;
	 tmp_index++;
      }
		
      TRACE( "status: %08x\n", status );
		
      dlclose( dlhandle );
   }
   *count = current_index;
   closedir( cpi_dir );
	
   return status;
}
#else
static unicap_status_t enumerate_devices( int *count )
{
   int idx_cpi = 0;
   int idx_device = 0;
   unicap_status_t status = STATUS_SUCCESS;
   
   initialize_static_cpis();

   while( static_cpis_s[idx_cpi] != 0 ){
      struct _unicap_cpi *cpi = static_cpis_s[idx_cpi];
      int idx_tmp = 0;
      
      while( ( idx_device < *count ) &&
	     ( SUCCESS( status = cpi->cpi_enumerate_devices( &g_devices[idx_device], idx_tmp++ ) ) ) ){
	 TRACE( "current index: %d\n", idx_device );
	 sprintf( g_devices[idx_device].cpi_layer, "%d", idx_cpi );
	 idx_device++;
      }
      
      TRACE( "status: %08x\n", status );
      
      idx_cpi++;
   }
   *count = idx_device;
   
   return status;
}
#endif //ENABLE_STATIC_CPI

unicap_status_t unicap_real_enumerate_devices( int *count )
{
   return enumerate_devices( count );
}

#if !ENABLE_STATIC_CPI
static unicap_status_t open_cpi( unicap_handle_t *unicap_handle, unicap_device_t *device )
{
   void *dlhandle;
   unicap_handle_t handle;
   unicap_status_t status;
   int i;
	
   TRACE( "open----------%s\n", device->identifier );

   if( !device || !unicap_handle ){
      return STATUS_INVALID_PARAMETER;
   }

   *unicap_handle = NULL;	

   handle = malloc( sizeof( struct _unicap_handle ) );
   if( !handle ){
      return STATUS_NO_MEM;
   }


   // initialize data structure
   memset( handle, 0, sizeof( struct _unicap_handle ) );

   handle->cb_info = malloc( sizeof( struct _unicap_callback_info ) * UNICAP_EVENT_LAST );
   for( i = 0; i < UNICAP_EVENT_LAST; i++ ){
      handle->cb_info[ i ].func = NULL;
      handle->cb_info[ i ].user_ptr = NULL;
   }

   handle->lock = malloc( sizeof( struct unicap_device_lock ) );
   memset( handle->lock, 0x0, sizeof( struct unicap_device_lock ) );

   dlhandle = dlopen( device->cpi_layer, RTLD_LAZY );
   if( !dlhandle ){
      free( handle );
      return STATUS_CPI_OPEN_FAILED;
   }
	
   //
   // resolve cpi symbols
   //
   handle->cpi.cpi_register = dlsym( dlhandle, "cpi_register" );
   if( !handle->cpi.cpi_register ){
      TRACE( "unicap: cpi %s : INVALID_CPI\n", device->cpi_layer );
      free( handle );
      dlclose( dlhandle );
      return STATUS_INVALID_CPI;
   }
   handle->cpi.cpi_register( &handle->cpi );
	
   if( CPI_VERSION_HI(handle->cpi.cpi_version) > 2 ){
      TRACE( "cpi_version hi: %d\n", CPI_VERSION_HI(handle->cpi.cpi_version) );
		
      free( handle );
      dlclose( dlhandle );
      return STATUS_UNSUPPORTED_CPI_VERSION;
   }
   // TODO:: check capabilities	

   // if serialized operation is requested, use a semaphore to prevent that more than one thread 
   // could enter a cpi function at the time
   if( handle->cpi.cpi_flags & UNICAP_CPI_SERIALIZED ){
      struct semid_ds semid;
      union semun_linux semun;
      // serialize per cpi. TODO: serialize per device optionally
      handle->sem_key = ftok( device->cpi_layer, 'a' );
      if( handle->sem_key == -1 ){
	 free( handle );
	 dlclose( dlhandle );
	 return STATUS_FAILURE;
      }
		
      handle->sem_id = semget( handle->sem_key, 1, IPC_CREAT );
      if( handle->sem_id == -1 ){
	 free( handle );
	 dlclose( dlhandle );
	 return STATUS_FAILURE;
      }
		
      semun.buf = &semid;
      // check if the semaphore is used the first time -> initialize it
      semctl( handle->sem_id, 0, IPC_STAT, semun );
      if( semid.sem_otime == 0 ){
	 semun.val = 1;
	 semctl( handle->sem_id, 0, SETVAL, semun );
      }
   }else{
      handle->sem_key = -1;
      handle->sem_id = -1;
   }
	
   _unicap_acquire_cpi_lock( handle->sem_id );
	
   status = handle->cpi.cpi_open( &handle->cpi_data, device );
   if( !SUCCESS( status ) ){
      _unicap_release_cpi_lock( handle->sem_id );
      free( handle );
      dlclose( dlhandle );
      return status;
   }
	
   _unicap_release_cpi_lock( handle->sem_id );

   handle->dlhandle = dlhandle;
   memcpy( &handle->device, device, sizeof( unicap_device_t ) );

   handle->ref_count = malloc( sizeof( int ) );
   *(handle->ref_count) = 1;
	
   *unicap_handle = handle;

   if( handle->cpi.cpi_set_event_notify ){
      handle->cpi.cpi_set_event_notify( handle->cpi_data, unicap_event_callback, unicap_copy_handle( handle ) );
   }

   TRACE( "open handle = %p\n", handle );

   return STATUS_SUCCESS;
}
#else
static unicap_status_t open_cpi( unicap_handle_t *unicap_handle, unicap_device_t *device )
{
   unicap_handle_t handle;
   unicap_status_t status;
   int cpi_count;
   int idx_cpi;
   int i;

   TRACE( "open----------%s\n", device->identifier );

   initialize_static_cpis();

   if( !device || !unicap_handle ){
      return STATUS_INVALID_PARAMETER;
   }

   *unicap_handle = NULL;	

   handle = malloc( sizeof( struct _unicap_handle ) );
   if( !handle ){
      return STATUS_NO_MEM;
   }

   // initialize data structure
   memset( handle, 0, sizeof( struct _unicap_handle ) );

   handle->cb_info = malloc( sizeof( struct _unicap_callback_info ) * UNICAP_EVENT_LAST );
   for( i = 0; i < UNICAP_EVENT_LAST; i++ ){
      handle->cb_info[ i ].func = NULL;
      handle->cb_info[ i ].user_ptr = NULL;
   }

   handle->lock = malloc( sizeof( struct unicap_device_lock ) );
   memset( handle->lock, 0x0, sizeof( struct unicap_device_lock ) );

   sscanf( device->cpi_layer, "%d", &idx_cpi );
   if( ( idx_cpi < 0 ) || ( idx_cpi > UNICAP_STATIC_CPI_COUNT ) ){
      free( handle );
      return STATUS_INVALID_PARAMETER;
   }
   
   memcpy( &handle->cpi, static_cpis_s[idx_cpi], sizeof( struct _unicap_cpi ) );
   handle->sem_key = -1;
   handle->sem_id = -1;
   
   status = handle->cpi.cpi_open( &handle->cpi_data, device );
   if( !SUCCESS( status ) ){
      free( handle );
      return status;
   }
   
   handle->ref_count = malloc( sizeof( int ) );
   *(handle->ref_count) = 1;
   
   *unicap_handle = handle;
   
   if( handle->cpi.cpi_set_event_notify ){
      handle->cpi.cpi_set_event_notify( handle->cpi_data, unicap_event_callback, unicap_copy_handle( handle ) );
   }

   return STATUS_SUCCESS;
}
#endif //ENABLE_STATIC_CPI

unicap_status_t unicap_open( unicap_handle_t *unicap_handle, unicap_device_t *device )
{
   unicap_status_t status = STATUS_SUCCESS;
   struct unicap_device_cache *cache_entry;

   unicap_initialize();

   cache_entry = lookup_device_cache( device );
   if( !cache_entry || !cache_entry->handle ){
      status = open_cpi( unicap_handle, device );
      store_device_cache( device, *unicap_handle );
   } else {
      *unicap_handle = unicap_clone_handle( cache_entry->handle );
      TRACE( "device in cache refcount = %d\n", *cache_entry->handle->ref_count );
   }   

   return status;
}
	
void unicap_cpi_register_event_notification( void* _func,
					     void *data, unicap_handle_t handle )
{
   cpi_set_event_notify_t func = (cpi_set_event_notify_t)_func;

   func( data, unicap_event_callback, unicap_clone_handle( handle ) );
}


/*
  Create a new instance of an unicap handle and increase ref count. 

  Input: handle

  Output: A copy of handle; ref count increased
*/
unicap_handle_t unicap_clone_handle( unicap_handle_t handle )
{
   unicap_handle_t cloned_handle;


   if( !handle ){
      return NULL;
   }
	
   cloned_handle = malloc( sizeof( struct _unicap_handle ) );
   if( !cloned_handle ){
      return NULL;
   }
	
   _unicap_acquire_cpi_lock( handle->sem_id );
   memcpy( cloned_handle, handle, sizeof( struct _unicap_handle ) );
   *(handle->ref_count) = *(handle->ref_count) + 1;
   _unicap_release_cpi_lock( handle->sem_id );
   TRACE( "clone handle = %p: handle = %p refcount = %d\n", cloned_handle, handle, *(handle->ref_count) );

   return cloned_handle;
}

/**
 * Create a new unicap_handle instance WITHOUT incrementing refcount
 * @handle: handle to copy
 *
 * Returns: copy of handle
 */
static unicap_handle_t unicap_copy_handle( unicap_handle_t handle )
{
   unicap_handle_t cloned_handle;

   if( !handle ){
      return NULL;
   }
	
   cloned_handle = malloc( sizeof( struct _unicap_handle ) );
   if( !cloned_handle ){
      return NULL;
   }
	
   memcpy( cloned_handle, handle, sizeof( struct _unicap_handle ) );
   
   return cloned_handle;
}



unicap_status_t unicap_close( unicap_handle_t handle )
{

   if( *(handle->ref_count) == 0 ){
      TRACE( "Tried to close a handle with ref_count == 0!\n" );
      return STATUS_FAILURE;
   }
	
   *(handle->ref_count) = *(handle->ref_count) - 1;
   TRACE( "close: handle = %p new refcount = %d\n", handle, *(handle->ref_count ) );

   if( *(handle->ref_count) == 1 ){
      free_device_cache_entries();
      free( handle );
      return STATUS_SUCCESS;
   }   

   if( *(handle->ref_count) == 0 ){
      unicap_unlock_properties( handle );
      unicap_unlock_stream( handle );

      // free resources
      _unicap_acquire_cpi_lock( handle->sem_id );
      handle->cpi.cpi_capture_stop( handle->cpi_data );
      handle->cpi.cpi_close( handle->cpi_data );
      if( handle->dlhandle ){
	 dlclose( handle->dlhandle );
      }
      _unicap_release_cpi_lock( handle->sem_id );
      free( handle->ref_count );
      free( handle->cb_info );
      free( handle->lock );
   }
   free( handle );
   return STATUS_SUCCESS;
}

unicap_status_t unicap_get_device( unicap_handle_t handle, unicap_device_t *device )
{
   if( !handle || !device ){
      return STATUS_INVALID_PARAMETER;
   }
	
   memcpy( device, &handle->device, sizeof( unicap_device_t ) );
	
   return STATUS_SUCCESS;
}

unicap_status_t unicap_reenumerate_formats( unicap_handle_t handle, int *count )
{
   unicap_status_t status;
	
   status = handle->cpi.cpi_reenumerate_formats( handle->cpi_data, count );
	
   return status;
}

unicap_status_t unicap_enumerate_formats( unicap_handle_t handle, 
					  unicap_format_t *specifier, 
					  unicap_format_t *format, 
					  int index )
{
   int current_index = -1;
   int tmp_index = 0;
   unicap_status_t status = STATUS_NO_MATCH;

   if( !handle ){
      TRACE( "handle is NULL\n" );
      return STATUS_INVALID_PARAMETER;
   }

   TRACE( "unicap_enumerate_formats, index: %d\n", index );

   _unicap_acquire_cpi_lock( handle->sem_id );

   while( ( current_index != index ) && 
	  ( ( status = handle->cpi.cpi_enumerate_formats( handle->cpi_data, format, tmp_index ) ) 
	    == STATUS_SUCCESS ) ){
      if( _check_format_match( specifier, format ) ){
	 current_index++;
      }
		
      tmp_index++;
   }
   _unicap_release_cpi_lock( handle->sem_id );

   return status;
}

unicap_status_t unicap_set_format( unicap_handle_t handle, 
				   unicap_format_t *format )
{
   unicap_status_t status;

   if( !format ){
      return STATUS_INVALID_PARAMETER;
   }

   if( !strlen( format->identifier ) ){
      return STATUS_INVALID_PARAMETER;
   }

   _unicap_acquire_cpi_lock( handle->sem_id );
   status = handle->cpi.cpi_set_format( handle->cpi_data, format );
   _unicap_release_cpi_lock( handle->sem_id );
	
   return status;
}

unicap_status_t unicap_get_format( unicap_handle_t handle, 
				   unicap_format_t *format )
{
   unicap_status_t status;
	
   if( !format ){
      return STATUS_INVALID_PARAMETER;
   }
	
   _unicap_acquire_cpi_lock( handle->sem_id );
   status = handle->cpi.cpi_get_format( handle->cpi_data, format );
   _unicap_release_cpi_lock( handle->sem_id );

   return status;
}

unicap_status_t unicap_reenumerate_properties( unicap_handle_t handle, int *_pcount )
{
   unicap_status_t status;
   int count;

   status = handle->cpi.cpi_reenumerate_properties( handle->cpi_data, &count );

   if( _pcount ){
      *_pcount = count;
   }
	
   return status;
}

unicap_status_t unicap_enumerate_properties( unicap_handle_t handle, unicap_property_t *specifier, unicap_property_t *property, int index )
{
   int current_index = -1;
   int tmp_index = 0;
   unicap_status_t status;

   if( !property || !handle || ( index < 0 ) ){
      return STATUS_INVALID_PARAMETER;
   }

/*    TRACE( "enumerate properties: %d\n", index ); */

   _unicap_acquire_cpi_lock( handle->sem_id );
   while( ( current_index != index ) && 
	  ( ( status = handle->cpi.cpi_enumerate_properties( handle->cpi_data, property, tmp_index ) ) == STATUS_SUCCESS ) ){
/*       TRACE( "%s <> %s: %d\n", specifier->identifier, property->identifier, _check_property_match( specifier, property ) ); */
      if( _check_property_match( specifier, property ) ){
	 current_index++;
      }
		
      tmp_index++;
   }
   _unicap_release_cpi_lock( handle->sem_id );

   return status;	
}

unicap_status_t unicap_set_property( unicap_handle_t handle, unicap_property_t *property )
{
   unicap_status_t status;

   if( !handle || ! property ){
      TRACE( "invalid param\n" );
      return STATUS_INVALID_PARAMETER;
   }

	
   _unicap_acquire_cpi_lock( handle->sem_id );
   status = handle->cpi.cpi_set_property( handle->cpi_data, property );
   _unicap_release_cpi_lock( handle->sem_id );

   return status;
}

unicap_status_t unicap_set_property_value( unicap_handle_t handle, char *identifier, double value )
{
   unicap_status_t status;
   unicap_property_t spec, prop;
   

   unicap_void_property( &spec );
   unicap_void_property( &prop );
   strcpy( spec.identifier, identifier );
   status = unicap_enumerate_properties( handle, &spec, &prop, 0 );
   if( !SUCCESS( status ) ){
      return status;
   }
   
   prop.value = value;
   prop.flags = UNICAP_FLAGS_MANUAL;
   prop.flags_mask = UNICAP_FLAGS_MANUAL;
   
   return unicap_set_property( handle, &prop );
}

unicap_status_t unicap_set_property_manual( unicap_handle_t handle, char *identifier )
{
   unicap_status_t status;
   unicap_property_t spec, prop;
   

   unicap_void_property( &spec );
   unicap_void_property( &prop );
   strcpy( spec.identifier, identifier );
   status = unicap_enumerate_properties( handle, &spec, &prop, 0 );
   if( !SUCCESS( status ) ){
      return status;
   }

   status = unicap_get_property( handle, &prop );
   if( !SUCCESS( status ) ){
      return status;
   }
   
   prop.flags = UNICAP_FLAGS_MANUAL;
   prop.flags_mask = UNICAP_FLAGS_MANUAL;
   
   return unicap_set_property( handle, &prop );
}

unicap_status_t unicap_set_property_auto( unicap_handle_t handle, char *identifier )
{
   unicap_status_t status;
   unicap_property_t spec, prop;
   

   unicap_void_property( &spec );
   unicap_void_property( &prop );
   strcpy( spec.identifier, identifier );
   status = unicap_enumerate_properties( handle, &spec, &prop, 0 );
   if( !SUCCESS( status ) ){
      return status;
   }

   status = unicap_get_property( handle, &prop );
   if( !SUCCESS( status ) ){
      return status;
   }
   
   prop.flags = UNICAP_FLAGS_AUTO;
   prop.flags_mask = UNICAP_FLAGS_AUTO;
   
   return unicap_set_property( handle, &prop );
}

unicap_status_t unicap_set_property_one_push( unicap_handle_t handle, char *identifier )
{
   unicap_status_t status;
   unicap_property_t spec, prop;
   

   unicap_void_property( &spec );
   unicap_void_property( &prop );
   strcpy( spec.identifier, identifier );
   status = unicap_enumerate_properties( handle, &spec, &prop, 0 );
   if( !SUCCESS( status ) ){
      return status;
   }

   status = unicap_get_property( handle, &prop );
   if( !SUCCESS( status ) )
   {
      return status;
   }
   
   prop.flags = UNICAP_FLAGS_ONE_PUSH;
   prop.flags_mask = UNICAP_FLAGS_ONE_PUSH;
   
   return unicap_set_property( handle, &prop );
}

unicap_status_t unicap_get_property( unicap_handle_t handle, unicap_property_t *property )
{
   unicap_status_t status = STATUS_SUCCESS;
   if( !handle || !property ){
      return STATUS_INVALID_PARAMETER;
   }
	
   _unicap_acquire_cpi_lock( handle->sem_id );
   status = handle->cpi.cpi_get_property( handle->cpi_data, property );
   _unicap_release_cpi_lock( handle->sem_id );
	
   return status;
}

unicap_status_t unicap_get_property_value( unicap_handle_t handle, const char *identifier, double *value )
{
   if( !handle || !identifier )
   {
      return STATUS_INVALID_PARAMETER;
   }  

   unicap_status_t status = STATUS_SUCCESS;
   unicap_property_t spec, prop;   

   unicap_void_property( &spec );
   unicap_void_property( &prop );
   strcpy( spec.identifier, identifier );
   status = unicap_enumerate_properties( handle, &spec, &prop, 0 );
   if( !SUCCESS( status ) ){
      return status;
   }

   status = unicap_get_property( handle, &prop );
   if( !SUCCESS( status ) ){
      return STATUS_FAILURE;
   }

   if( ( prop.type != UNICAP_PROPERTY_TYPE_RANGE ) &&
       ( prop.type != UNICAP_PROPERTY_TYPE_VALUE_LIST ) ){
      return STATUS_INVALID_PARAMETER;
   }
   
   *value = prop.value;
   
   return status;
}

unicap_status_t unicap_get_property_menu( unicap_handle_t handle, const char *identifier, char **value )
{
   static char menu_item[128];

   if( !handle || !identifier ){
      return STATUS_INVALID_PARAMETER;
   }  

   unicap_status_t status = STATUS_SUCCESS;
   unicap_property_t spec, prop;   

   unicap_void_property( &spec );
   unicap_void_property( &prop );
   strcpy( spec.identifier, identifier );
   status = unicap_enumerate_properties( handle, &spec, &prop, 0 );
   if( !SUCCESS( status ) ){
      return status;
   }

   status = unicap_get_property( handle, &prop );
   if( !SUCCESS( status ) ){
      return STATUS_FAILURE;
   }

   if( prop.type != UNICAP_PROPERTY_TYPE_MENU ){
      return STATUS_INVALID_PARAMETER;
   }

   strcpy( menu_item, prop.menu_item );
   
   *value = menu_item;
   
   return status;
}

unicap_status_t unicap_get_property_auto( unicap_handle_t handle, const char *identifier, int *enabled )
{
   if( !handle || !identifier ){
      return STATUS_INVALID_PARAMETER;
   }  

   unicap_status_t status = STATUS_SUCCESS;
   unicap_property_t spec, prop;   

   unicap_void_property( &spec );
   unicap_void_property( &prop );
   strcpy( spec.identifier, identifier );
   status = unicap_enumerate_properties( handle, &spec, &prop, 0 );
   if( !SUCCESS( status ) ){
      return status;
   }

   status = unicap_get_property( handle, &prop );
   if( !SUCCESS( status ) ){
      return STATUS_FAILURE;
   }

   *enabled = ( prop.flags & UNICAP_FLAGS_AUTO ) ? 1 : 0;
   
   return status;
}

unicap_status_t unicap_start_capture( unicap_handle_t handle )
{
   unicap_status_t status = STATUS_SUCCESS;
	
   if( !handle ){
      TRACE( "Invalid parameter\n" );
      return STATUS_INVALID_PARAMETER;
   }

   if( !handle->lock->have_stream_lock ){
      status = unicap_lock_stream( handle );
      if( SUCCESS( status ) ){
	 handle->lock->temporary_stream_lock = 1;
      }
   }

   if( SUCCESS( status ) ){
      TRACE( "Capture start...\n" );
      _unicap_acquire_cpi_lock( handle->sem_id );
      status = handle->cpi.cpi_capture_start( handle->cpi_data );
      _unicap_release_cpi_lock( handle->sem_id );
   }else{
      TRACE( "FAILED TO GET STREAM LOCK\n\n\n\n\n\n" );
   }
	
   return status;
}

unicap_status_t unicap_stop_capture( unicap_handle_t handle )
{
   unicap_status_t status = STATUS_SUCCESS;
	
   if( !handle ){
      return STATUS_INVALID_PARAMETER;
   }

   if( !handle->lock->have_stream_lock ){
      status = STATUS_PERMISSION_DENIED;
   }else{
      if( handle->lock->temporary_stream_lock ){
	 handle->lock->temporary_stream_lock = 0;
	 unicap_unlock_stream( handle );
      }
		
      _unicap_acquire_cpi_lock( handle->sem_id );
      status = handle->cpi.cpi_capture_stop( handle->cpi_data );
      _unicap_release_cpi_lock( handle->sem_id );
   }

   return status;
}

unicap_status_t unicap_queue_buffer( unicap_handle_t handle, unicap_data_buffer_t *buffer )
{
   unicap_status_t status = STATUS_SUCCESS;
	
   if( !handle || !buffer ){
      TRACE( "Invalid parameter\n" );
      return STATUS_INVALID_PARAMETER;
   }
	

   if( !handle->lock->have_stream_lock ){
      status = STATUS_PERMISSION_DENIED;
   }else{
      _unicap_acquire_cpi_lock( handle->sem_id );
      status = handle->cpi.cpi_queue_buffer( handle->cpi_data, buffer );
      _unicap_release_cpi_lock( handle->sem_id );
   }
	
   return status;
}

unicap_status_t unicap_dequeue_buffer( unicap_handle_t handle, unicap_data_buffer_t **buffer )
{
   unicap_status_t status;
	
   if( !handle || !buffer ){
      return STATUS_INVALID_PARAMETER;
   }

   if( !handle->lock->have_stream_lock && unicap_is_stream_locked( &handle->device ) ){
      status = STATUS_PERMISSION_DENIED;
   }else{
      _unicap_acquire_cpi_lock( handle->sem_id );
      status = handle->cpi.cpi_dequeue_buffer( handle->cpi_data, buffer );
      _unicap_release_cpi_lock( handle->sem_id );
   }
	
   return status;
}

unicap_status_t unicap_wait_buffer( unicap_handle_t handle, unicap_data_buffer_t **buffer )
{
   unicap_status_t status = STATUS_SUCCESS;
	
   if( !handle || !buffer ){
      TRACE( "!handle || !buffer\n" );
      return STATUS_INVALID_PARAMETER;
   }

   if( !handle->lock->have_stream_lock ){
      status = STATUS_PERMISSION_DENIED;
   }else{
      _unicap_acquire_cpi_lock( handle->sem_id );
      status = handle->cpi.cpi_wait_buffer( handle->cpi_data, buffer );
      _unicap_release_cpi_lock( handle->sem_id );
   }

   return status;
}

unicap_status_t unicap_poll_buffer( unicap_handle_t handle, int *count )
{
   unicap_status_t status = STATUS_SUCCESS;
	
   if( !handle || !count ){
      return STATUS_INVALID_PARAMETER;
   }
	
   _unicap_acquire_cpi_lock( handle->sem_id );
   status = handle->cpi.cpi_poll_buffer( handle->cpi_data, count );
   _unicap_release_cpi_lock( handle->sem_id );
	
   return status;
}

static key_t uidtok( char *string )
{
   int i;
   key_t key = 0x00345678;
	
   for( i = 0; string[i] != 0; i++ ){
      key = key ^ ( string[i] << ( i%3 ) );
   }

   return key;
}

	
int unicap_is_stream_locked( unicap_device_t *device )
{
   key_t key;
   int sem_id;
   int val;
   char uid[2048];
	
   unicap_initialize();

#ifndef UNICAP_THREAD_LOCKING
   return 0;
#endif
   sprintf( uid, "%s", device->identifier );
   key = uidtok( uid );

/*    key = uidtok( device->identifier ); */

   sem_id = semget( key, 1, S_IRWXU | S_IRWXG | S_IRWXO );
   if( sem_id == -1 ){
      return 0;
   }
	
   val = semctl( sem_id, 0, GETVAL ) ? 0 : UNICAP_FLAGS_LOCKED;
   if( val == UNICAP_FLAGS_LOCKED ){
      struct unicap_device_cache *cache_entry;
      
      cache_entry = lookup_device_cache( device );
      if( cache_entry && cache_entry->handle ){
	 if( cache_entry->handle->lock->have_stream_lock ){
	    val |= UNICAP_FLAGS_LOCK_CURRENT_PROCESS;
	 }
      }
   } 

   TRACE( "STREAM IS LOCKED %s%s >>>> %d (%d) key + %d\n", device->identifier, device->cpi_layer, val == 0, val, key );

   return val;
}


unicap_status_t unicap_lock_stream( unicap_handle_t handle )
{
   unicap_status_t status = STATUS_SUCCESS;
   key_t key;
   int sem_id;
   char uid[2048];
   
#ifndef UNICAP_THREAD_LOCKING
   handle->lock->have_stream_lock = 1;
   return STATUS_SUCCESS;
#endif
   
   sprintf( uid, "%s", handle->device.identifier );
   key = uidtok( uid );

   if( !handle->lock->have_stream_lock ){
      struct semid_ds semds;
      union semun_linux semun;
      sem_id = semget( key, 1, IPC_CREAT | S_IRWXU | S_IRWXG | S_IRWXO);
      if( sem_id == -1 ){
	 return STATUS_FAILURE;
      }
		
      // check if the semaphore is used the first time -> initialize it
      semun.buf = &semds;
      semds.sem_otime = 0;
      semctl( sem_id, 0, IPC_STAT, semun );
      if( semds.sem_otime == 0 ){
	 struct sembuf sops;
	 semun.val = 1;
	 semctl( sem_id, 0, SETVAL, semun );

	 sops.sem_num = 0;
	 sops.sem_op = -1;
	 sops.sem_flg = SEM_UNDO|IPC_NOWAIT;
		
	 _unicap_acquire_cpi_lock( handle->sem_id );
			
	 if( semop( sem_id, &sops, 1 ) < 0 ){
				
	    status = STATUS_PERMISSION_DENIED;
	 }else{
	    handle->lock->stream_sem_id = sem_id;
	    handle->lock->have_stream_lock = 1;
	 }
	 _unicap_release_cpi_lock( handle->sem_id );
      }else{
	 struct sembuf sops;
	 sops.sem_num = 0;
	 sops.sem_op = -1;
	 sops.sem_flg = SEM_UNDO|IPC_NOWAIT;
		
	 _unicap_acquire_cpi_lock( handle->sem_id );
			
	 if( semop( sem_id, &sops, 1 ) < 0 ){
	    status = STATUS_PERMISSION_DENIED;
	 }else{
	    handle->lock->stream_sem_id = sem_id;
	    handle->lock->have_stream_lock = 1;
	 }
	 _unicap_release_cpi_lock( handle->sem_id );
      }
   }

   TRACE( "LOCK STREAM >>>> %d key = %d\n", handle->lock->have_stream_lock, key );
	
   return status;
}

unicap_status_t unicap_unlock_stream( unicap_handle_t handle )
{
   unicap_status_t status = STATUS_SUCCESS;

#ifndef UNICAP_THREAD_LOCKING
   handle->lock->have_stream_lock = 0;
   return STATUS_SUCCESS;
#endif
	
   if( handle->lock->have_stream_lock ){
      struct sembuf sops;
      sops.sem_num = 0;
      sops.sem_op = 1;
      sops.sem_flg = SEM_UNDO;
		
      if( semop( handle->lock->stream_sem_id, &sops, 1 ) < 0 ){
	 status = STATUS_FAILURE;
      }else{
	 handle->lock->have_stream_lock = 0;
      }
		
   }else{
      status = STATUS_PERMISSION_DENIED;
   }
	
   TRACE( "UNLOCK STREAM >>>> %d\n", handle->lock->have_stream_lock );

   return status;
}


unicap_status_t unicap_lock_properties( unicap_handle_t handle )
{
   unicap_status_t status = STATUS_FAILURE;
   return status;
}

unicap_status_t unicap_unlock_properties( unicap_handle_t handle )
{
   unicap_status_t status = STATUS_SUCCESS;
   return status;
}

unicap_status_t unicap_register_callback( unicap_handle_t handle, 
					  unicap_event_t event, 
					  unicap_callback_t func, 
					  void *user_ptr )
{
   unicap_status_t status = STATUS_FAILURE;

   TRACE( "register callback: %p\n", func );

   if( ( event < UNICAP_EVENT_FIRST ) || 
       ( event >= UNICAP_EVENT_LAST ) ){
      return STATUS_INVALID_PARAMETER;
   }
	
   _unicap_acquire_cpi_lock( handle->sem_id );

   if( !handle->cb_info || ( handle->cb_info[event].func != NULL ) ){
      status = STATUS_FAILURE;
   }else{
      handle->cb_info[event].func = func;
      handle->cb_info[event].user_ptr = user_ptr;
      status = STATUS_SUCCESS;
   }
	
   _unicap_release_cpi_lock( handle->sem_id );
	
   return status;
}

unicap_status_t unicap_unregister_callback( unicap_handle_t handle, 
					    unicap_event_t event )
{
   unicap_status_t status = STATUS_FAILURE;


   if( ( event < UNICAP_EVENT_FIRST ) || 
       ( event >= UNICAP_EVENT_LAST ) ){
      return STATUS_INVALID_PARAMETER;
   }
	
   _unicap_acquire_cpi_lock( handle->sem_id );

   if( handle->cb_info[event].func == NULL ){
      status = STATUS_FAILURE;
   }else{
      handle->cb_info[event].func = NULL;
      handle->cb_info[event].user_ptr = NULL;
      status = STATUS_SUCCESS;
   }
	
   _unicap_release_cpi_lock( handle->sem_id );
	
   return status;
}
	
int unicap_get_ref_count( unicap_handle_t handle )
{
   return *handle->ref_count;
}

unicap_data_buffer_t *unicap_data_buffer_new( unicap_format_t *format )
{
   unicap_data_buffer_t *buffer;
   buffer = malloc( sizeof( unicap_data_buffer_t) );
   memset( buffer, 0x0, sizeof( unicap_data_buffer_t ) );
   buffer->buffer_size = buffer->format.buffer_size;
   buffer->data = malloc( buffer->buffer_size );   
   unicap_copy_format( &buffer->format, format );
   buffer->private = NULL;

   return buffer;
}

void unicap_data_buffer_init( unicap_data_buffer_t *buffer, unicap_format_t *format, unicap_data_buffer_init_data_t *init_data )
{
   unicap_copy_format( &buffer->format, format );
   buffer->private = malloc( sizeof( unicap_data_buffer_private_t ) );
   sem_init( &buffer->private->lock, 0, 1 );   
   buffer->private->ref_count = 0;
   buffer->private->free_func = init_data->free_func;
   buffer->private->free_func_data = init_data->free_func_data;
   buffer->private->ref_func = init_data->ref_func;
   buffer->private->ref_func_data = init_data->ref_func_data;
   buffer->private->unref_func = init_data->unref_func;
   buffer->private->unref_func_data = init_data->unref_func_data;
}

void unicap_data_buffer_free( unicap_data_buffer_t *buffer )
{
   sem_wait( &buffer->private->lock );
   if( buffer->private->ref_count > 0 ){
      TRACE( "freeing a buffer with refcount = %d!!!\n", buffer->private.refcount );
   }
   if( buffer->private->free_func ){
      buffer->private->free_func( buffer, buffer->private->free_func_data );
   }
   
   sem_destroy( &buffer->private->lock );
   if (buffer->data)
      free( buffer->data );
   free( buffer );
}

unicap_status_t unicap_data_buffer_ref( unicap_data_buffer_t *buffer )
{
   sem_wait( &buffer->private->lock );
   buffer->private->ref_count++;
   sem_post( &buffer->private->lock );

   return STATUS_SUCCESS;
}

unicap_status_t unicap_data_buffer_unref( unicap_data_buffer_t *buffer )
{
   unicap_status_t status = STATUS_SUCCESS;
   sem_wait( &buffer->private->lock );
   if( buffer->private->ref_count > 0 ){
      buffer->private->ref_count--;
      if (buffer->private->unref_func){
	 buffer->private->unref_func (buffer, buffer->private->unref_func_data);
      }
      if (buffer->private->ref_count == 0 ){
	 unicap_data_buffer_free( buffer );
      }
   }else{
      TRACE( "unref of a buffer with refcount <= 0!" );
      status = STATUS_FAILURE;
   }
   sem_post (&buffer->private->lock);
   return status;
}

unsigned int unicap_data_buffer_get_refcount( unicap_data_buffer_t *buffer )
{
   return buffer->private->ref_count;
}

void *unicap_data_buffer_set_user_data( unicap_data_buffer_t *buffer, void *data )
{
   void *old_data = buffer->private->user_data;
   buffer->private->user_data = data;
   return old_data;
}

void *unicap_data_buffer_get_user_data( unicap_data_buffer_t *buffer )
{
   return buffer->private->user_data;
}
