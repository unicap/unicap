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

#ifndef __EUVCCAM_CAPTURE_H__
#define __EUVCCAM_CAPTURE_H__

#include "euvccam_cpi.h"

unicap_status_t euvccam_capture_start_capture( euvccam_handle_t handle );
unicap_status_t euvccam_capture_stop_capture( euvccam_handle_t handle );


#endif//__EUVCCAM_CAPTURE_H__
