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
#include <unistd.h>
#include <string.h>
#ifdef DEBUG
#include <stdio.h>
#endif
#include "queue.h"
#include <semaphore.h>
#include <debug.h>

void _dump_queue( struct _unicap_queue *queue )
{
	int i = 0;
	printf( "## dump queue\n" );
	while( queue->next )
	{
		printf( "entry: %d, addr: %p\n", i++, queue );
		printf( "entry->next: %p\n", queue->next );
		printf( "data: %p\n", queue->data );
		printf( "sema: %p\n", queue->psema );
		queue = queue->next;
	}
	printf( "entry: %d, addr: %p\n", i, queue );
	printf( "entry->next: %p\n", queue->next );
	printf( "data: %p\n", queue->data );
	printf( "sema: %p\n\n", queue->psema );
}

		


void _insert_back_queue( struct _unicap_queue *queue, struct _unicap_queue *entry )
{
	struct _unicap_queue *tmp_queue;
	if( !entry )
	{
		return;
	}

	if( sem_wait( queue->psema ) )
	{
		TRACE( "FATAL: sem_wait failed\n" );
		return;
	}
	
	tmp_queue = queue;

	// run to the end of the queue
	while( tmp_queue->next )
	{
		tmp_queue = tmp_queue->next;
	}
	tmp_queue->next = entry;
	entry->psema = queue->psema;
	
	entry->next = 0;
	if( sem_post( entry->psema ) )
	{
		TRACE( "FATAL: sem_post failed\n" );
		return;
	}
}

void _insert_front_queue( struct _unicap_queue *queue, struct _unicap_queue *entry )
{
	if( !entry )
	{
		return;
	}
	
	if( sem_wait( queue->psema ) )
	{
		TRACE( "FATAL: sem_wait failed\n" );
		return;
	}
	
	entry->next = queue->next;
	entry->psema = queue->psema;
	queue->next = entry;

	if( sem_post( queue->psema ) )
	{
		TRACE( "FATAL: sem_post failed\n" );
	}
}

struct _unicap_queue *_get_front_queue( struct _unicap_queue *queue )
{
	struct _unicap_queue *entry;

	if( sem_wait( queue->psema ) )
	{
		TRACE( "sem_wait failed!\n" );
		return 0;
	}

	if( !queue->next )
	{
		entry = 0;
		sem_post( queue->psema );
		goto out;
	}
	
	entry = queue->next;

	if( queue->next )
	{
		queue->next = queue->next->next;
	}
	
	entry->psema = queue->psema;
	
	entry->next = 0;

	if( sem_post( entry->psema ) )
	{
		TRACE( "FATAL: sem_post failed\n" );
	}
	
out:

	return entry;
}

void _move_to_queue( struct _unicap_queue *from_queue, struct _unicap_queue *to_queue )
{
	struct _unicap_queue *tmp_to_queue;
	struct _unicap_queue *tmp_from_queue;
	struct _unicap_queue *entry;

	if( sem_wait( from_queue->psema ) )
	{
		TRACE( "FATAL: sem_wait failed\n" );
		return;
	}
	if( sem_wait( to_queue->psema ) ) 
	{
		TRACE( "FATAL: sem_wait failed\n" );
		return;
	}

	if( !from_queue->next )
	{
		entry = 0;
		goto out;
	}
	
	tmp_from_queue = from_queue;
	///////////////////////////////////////////////////////////////
	entry = from_queue->next;
	if( tmp_from_queue->next )
	{
		tmp_from_queue->next = tmp_from_queue->next->next;
	}
	entry->next = 0;
	
	////////////////////////////////////////////////////////////// 
	tmp_to_queue = to_queue;
	// run to the end of the queue
	while( tmp_to_queue->next )
	{
		tmp_to_queue = tmp_to_queue->next;
	}
	tmp_to_queue->next = entry;
	entry->psema = to_queue->psema;
out:
	if( sem_post( from_queue->psema ) )
	{
		TRACE( "FATAL: sem_post failed\n" );
	}

	if( sem_post( to_queue->psema ) )
	{
		TRACE( "FATAL: sem_post failed\n" );
	}
}


void _init_queue( struct _unicap_queue *queue )
{
	memset( queue, 0, sizeof( struct _unicap_queue ) );
	if( sem_init( &queue->sema, 0, 1 ) )
	{
		TRACE( "FATAL: sem_init failed\n" );
	}
	queue->psema = &queue->sema;
}

int  _queue_get_size( struct _unicap_queue *queue )
{
	int entries = 0;
	struct _unicap_queue *entry = queue;

/* 	TRACE( "entries: %d\n", entries ); */
	while( entry->next )
	{
/* 		TRACE( "entries: %d\n", entries ); */
		entries++;
		entry = entry->next;
	}

	return entries;
}
