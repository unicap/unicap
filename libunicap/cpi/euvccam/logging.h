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

#ifndef __LOGGING__
#define __LOGGING__

#define LOG_MODULE_CAPTURE 2
#define LOG_MODULE_USB     3
#define LOG_MODULE_DEVICE  4
#define LOG_MODULE_ANY     1

void log_init( void );
void log_close( void );
void log_message( int module, int log_level, const char *format, ... );

#endif//__LOGGING__
