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

#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "ucutil.h"
#include <sys/time.h>
#include <semaphore.h>

struct _unicap_queue
{
      sem_t sema;
      sem_t *psema;

      void * data;
      struct _unicap_queue *next;
} unicap_queue;

typedef struct _unicap_queue unicap_queue_t;

__HIDDEN__ void ucutil_insert_back_queue( unicap_queue_t *queue, unicap_queue_t *entry );
__HIDDEN__ void ucutil_insert_front_queue( unicap_queue_t *queue, unicap_queue_t *entry );
__HIDDEN__ unicap_queue_t *ucutil_get_front_queue( unicap_queue_t *queue );
__HIDDEN__ void ucutil_move_to_queue( unicap_queue_t *from_queue, unicap_queue_t *to_queue );
__HIDDEN__ void ucutil_init_queue( unicap_queue_t * );
__HIDDEN__ unicap_queue_t *ucutil_queue_new( void );
__HIDDEN__ int ucutil_destroy_queue( unicap_queue_t *queue );
__HIDDEN__ int ucutil_queue_get_size( unicap_queue_t *queue );


#endif//__QUEUE_H__
