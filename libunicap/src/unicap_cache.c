/*
** unicap_cache.c
** 
** Made by (Arne Caspari)
** Login   <arne@localhost>
** 
** Started on  Fri Aug 31 19:07:15 2007 Arne Caspari
*/

#include "config.h"
#include <string.h>

#define NUM_CACHE_ENTRIES 64

typedef struct
{
      char key[64];
      void *value;
      int refc;
} unicap_cache_entry_t;

static unicap_cache_entry_t g_cache[ NUM_CACHE_ENTRIES ];
static int g_use_cache = 0;


void unicap_cache_init( void )
{
   int i;
   
   if( !g_use_cache )
   {
      for( i = 0; i < NUM_CACHE_ENTRIES; i++ )
      {
	 g_cache[ i ].value = NULL;
	 g_cache[ i ].refc = 0;
      }
   }

   g_use_cache = 1;
}


int unicap_cache_add( char *key, void *value )
{
   int i;
   int ret = -1;
   
   if( g_use_cache )
   {
      for( i = 0; i < NUM_CACHE_ENTRIES; i++ )
      {
	 if( !strcpy( g_cache[ i ].key, key ) )
	 {
	    ret = 1;
	 }
      }
      
      if( ret < 0 )
      {
	 for( i = 0; i < NUM_CACHE_ENTRIES; i++ )
	 {
	    if( !g_cache[ i ].value )
	    {
	       ret = 0;
	       strcpy( g_cache[ i ].key, key );
	       g_cache[ i ].refc++;
	       g_cache[ i ].value = value;
	    }
	 }
      }
   }
   
   return ret;
}

void* unicap_cache_get( char *key )
{
   int i;
   
   if( g_use_cache )
   {
      for( i = 0; i < NUM_CACHE_ENTRIES; i++ )
      {
	 if( !strcmp( g_cache[ i ].key, key ) )
	 {
	    g_cache[ i ].refc++;
	    return g_cache[ i ].value;
	 }
      }
   }
   
   return NULL;
}

void *unicap_cache_unref( char *key )
{
   int i;
   void *ret = NULL;
   
   if( g_use_cache )
   {
      for( i = 0; i < NUM_CACHE_ENTRIES; i++ )
      {
	 if( !strcmp( g_cache[ i ].key, key ) )
	 {
	    g_cache[ i ].refc--;
	    
	    if( !g_cache[ i ].refc )
	    {
	       ret = g_cache[ i ].value;
	    }
	    g_cache[ i ].value = NULL;
	 }
      }
   }

   return ret;
}
