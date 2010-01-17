/*
** static_cpi.h
** 
** Made by Arne Caspari
** Login   <arne@localhost>
** 
** Started on  Wed Jun 13 07:56:40 2007 Arne Caspari
** Last update Wed Jun 13 07:56:40 2007 Arne Caspari
*/

#ifndef   	STATIC_CPI_H_
# define   	STATIC_CPI_H_

#include "config.h"
#include "unicap_cpi.h"

enum unicap_static_cpi_enum
{
   UNICAP_STATIC_CPI_INVALID = -1,
#if BUILD_DCAM
   UNICAP_STATIC_CPI_DCAM, 
#endif
#if BUILD_VID21394
   UNICAP_STATIC_CPI_VID21394,
#endif
#if BUILD_V4L
   UNICAP_STATIC_CPI_V4L,
#endif
#if BUILD_V4L2
   UNICAP_STATIC_CPI_V4L2,
#endif
   UNICAP_STATIC_CPI_COUNT
};


struct _unicap_cpi *static_cpis_s[UNICAP_STATIC_CPI_COUNT+1];

#endif 	    /* !STATIC_CPI_H_ */

