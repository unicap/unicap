/*
  unicap euvccam plugin

  Copyright (C) 2009  Arne Caspari

  Compilation, installation or use of this program requires a written license. 

  By compilling, installing or using this software you agree to accept the terms 
  of the license as specified in the COPYING file in the root folder of this 
  software package.
 */

#ifndef __EUVCCAM_CAPTURE_H__
#define __EUVCCAM_CAPTURE_H__

#include "euvccam_cpi.h"

unicap_status_t euvccam_capture_start_capture( euvccam_handle_t handle );
unicap_status_t euvccam_capture_stop_capture( euvccam_handle_t handle );


#endif//__EUVCCAM_CAPTURE_H__
