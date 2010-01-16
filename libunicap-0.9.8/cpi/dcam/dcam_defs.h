#ifndef __DCAM_DEFS_H__
#define __DCAM_DEFS_H__

#define GETFLAG_MANUAL_INQ(x)    ( ( x>>24 ) & 1 )
#define SETFLAG_MANUAL_INQ(x,y)  ( x | ( (y&1)<<24 ) )
#define GETFLAG_AUTO_INQ(x)      ( ( x>>25 ) & 1 )
#define SETFLAG_AUTO_INQ(x,y)    ( x | ( (y&1)<<25 ) )
#define GETFLAG_ON_OFF_INQ(x)    ( ( x>>26 ) & 1 )
#define SETFLAG_ON_OFF_INQ(x,y)  ( x | ( (y&1)<<26 ) )
#define GETFLAG_READ_OUT_INQ(x)  ( ( x>>27 ) & 1 )
#define SETFLAG_READ_OUT_INQ(x,y)( x | ( (y&1)<<27 ) )
#define GETFLAG_ONE_PUSH_INQ(x)  ( ( x>>28 ) & 1 )
#define SETFLAG_ONE_PUSH_INQ(x,y)( x | ( (y&1)<<28 ) )
#define GETFLAG_ABS_CONTROL_INQ(x)  ( ( x>>30 ) & 1 )
#define SETFLAG_ABS_CONTROL_INQ(x,y)( x | ( (y&1)<<30 ) )
#define GETFLAG_PRESENCE_INQ(x)  ( ( x>>31 ) & 1 )
#define SETFLAG_PRESENCE_INQ(x,y)( x | ( (y&1)<<31 ) )

#define GETFLAG_AUTO(x)          ( ( x>>24 ) & 1 )
#define SETFLAG_AUTO(x,y)        ( ( x & ~(1<<24) ) | ( (y&1)<<24 ) )
#define GETFLAG_ON_OFF(x)        ( ( x>>25 ) & 1 )
#define SETFLAG_ON_OFF(x,y)      ( ( x & ~(1<<25) ) | ( (y&1)<<25 ) )
#define GETFLAG_ONE_PUSH(x)      ( ( x>>26 ) & 1 )
#define SETFLAG_ONE_PUSH(x,y)    ( ( x & ~(1<<26) ) | ( (y&1)<<26 ) )
#define GETFLAG_ABS_CONTROL(x)   ( ( x>>30 ) & 1 )
#define SETFLAG_ABS_CONTROL(x,y) ( ( x & ~(1<<30) ) | ( (y&1)<<30 ) )
#define GETFLAG_PRESENCE(x)      ( ( x>>31 ) & 1 )
#define SETFLAG_PRESENCE(x,y)    ( ( x & ~(1<<31) ) | ( (y&1)<<31 ) )

#define GETVAL_VALUE(x)          ( x & 0xfff )
#define SETVAL_VALUE(x,y)        ( ( x & ~0xfff ) | ( y&0xfff ) )
#define GETVAL_MIN_VALUE(x)      ( (x>>12) & 0xfff )
#define GETVAL_MAX_VALUE(x)      ( x & 0xfff )
#define GETVAL_V_VALUE(x)        ( x & 0xfff )
#define SETVAL_V_VALUE(x,y)      ( ( x & ~0xfff ) | ( y&0xfff ) )
#define GETVAL_U_VALUE(x)        ( (x>>12) & 0xfff )
#define SETVAL_U_VALUE(x,y)      ( ( x & ~( 0xfff<<12 ) ) | ( ( y&0xfff )<<12 ) )

#define GETVAL_FRAME_RATE(x) ( x >> 29 )
#define SETVAL_FRAME_RATE(x,y)   ( x | ( ( y&0xf ) << 29 ) )


#endif
