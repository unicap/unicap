/*
** ucil_audio.c
 *
 * Copyright (C) 2004-2008 Arne Caspari ( arne@unicap-imaging.org )
 *
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

#if HAVE_ALSA
#include <ucil_alsa.h>
#endif

#include <glib.h>
#include "ucil_audio.h"

GList *ucil_audio_list_cards( void )
{
#if HAVE_ALSA
   return ucil_alsa_list_cards();
#else
   return NULL;
#endif
}
