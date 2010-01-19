/*
  unicap euvccam plugin

  Copyright (C) 2009  Arne Caspari

  Compilation, installation or use of this program requires a written license. 

  By compilling, installing or using this software you agree to accept the terms 
  of the license as specified in the COPYING file in the root folder of this 
  software package.
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
