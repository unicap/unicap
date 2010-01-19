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
