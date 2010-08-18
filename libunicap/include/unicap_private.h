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

#ifndef __UNICAP_PRIVATE_H__
#define __UNICAP_PRIVATE_H__

#include <unicap.h>
#include <unicap_cpi.h>

#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <semaphore.h>

#define SHM_REG_VERSION 1

#define DLLEXPORT __declspec(dllexport)



struct _unicap_callback_info
{
		unicap_callback_t func;
		void *user_ptr;
} unicap_callback_info;


struct _unicap_shm_reg
{
		int shm_reg_version;
		char device_identifier[128];
		int use_count;

		pid_t stream_owner;
		pid_t properties_owner;
} unicap_shm_reg;

struct unicap_device_lock
{
   int have_stream_lock;
   int temporary_stream_lock;
   int have_ppty_lock;
   int stream_lock_fd;
   int ppty_lock_fd;
   int stream_sem_id;
   int property_sem_id;
};   

struct _unicap_handle
{
   unicap_device_t device;      
   
   struct _unicap_cpi cpi;
   
   void *dlhandle;
   void *cpi_data;
      
   unsigned int cpi_flags;
   
   key_t sem_key;
   int   sem_id;   
		
   int *ref_count;

   struct unicap_device_lock *lock;
		
   struct _unicap_callback_info *cb_info;
} unicap_handle;

struct unicap_data_buffer_private
{
   unsigned int ref_count;
   unicap_data_buffer_func_t free_func;
   void *free_func_data;
   unicap_data_buffer_func_t ref_func;
   void *ref_func_data;
   unicap_data_buffer_func_t unref_func;
   void *unref_func_data;

   void *user_data;

   sem_t lock;
};


unicap_status_t unicap_real_enumerate_devices( int *count );


		

#endif //__UNICAP_PRIVATE_H__
