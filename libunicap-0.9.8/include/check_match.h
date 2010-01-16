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

#ifndef __CHECK_MATCH_H__
#define __CHECK_MATCH_H__

#include "unicap.h"

int _check_device_match( unicap_device_t *specifier, unicap_device_t *device );
int _check_format_match( unicap_format_t *specifier, unicap_format_t *format );
int _check_property_match( unicap_property_t *specifier, unicap_property_t *property );



#endif
