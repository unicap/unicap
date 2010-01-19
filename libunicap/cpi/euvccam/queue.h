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

#ifndef __QUEUE_H__
#define __QUEUE_H__

#include <sys/time.h>
#include <semaphore.h>

struct _unicap_queue
{
		sem_t sema;
		sem_t *psema;

		struct timeval fill_start_time;
		struct timeval fill_end_time;

		void * data;
		struct _unicap_queue *next;
} unicap_queue;

typedef struct _unicap_queue unicap_queue_t;

void _init_queue( struct _unicap_queue *queue );
void _destroy_queue( struct _unicap_queue *queue );
void _insert_back_queue( struct _unicap_queue *queue, struct _unicap_queue *entry );
void _insert_front_queue( struct _unicap_queue *queue, struct _unicap_queue *entry );
struct _unicap_queue *_get_front_queue( struct _unicap_queue *queue );
void _move_to_queue( struct _unicap_queue *from_queue, struct _unicap_queue *to_queue );
int  _queue_get_size( struct _unicap_queue *queue );


#endif//__QUEUE_H__
