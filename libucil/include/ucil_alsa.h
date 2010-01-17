/*
** ucil_alsa.h
** 
** Made by Arne Caspari
** Login   <arne@localhost>
** 
** Started on  Wed Aug 22 18:37:39 2007 Arne Caspari
*/

/*
    unicap
    Copyright (C) 2004-2007  Arne Caspari

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

#ifndef   	UCIL_ALSA_H_
# define   	UCIL_ALSA_H_

#include <glib-2.0/glib.h>

void  *ucil_alsa_init( char *dev, unsigned int rate );
long   ucil_alsa_fill_audio_buffer( void *_data );
int    ucil_alsa_get_audio_buffer( void *_data, short **buffer );
void   ucil_alsa_close( void *_data );

GList *ucil_alsa_list_cards( void );

#endif 	    /* !UCIL_ALSA_H_ */
