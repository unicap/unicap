#include <stdlib.h>
#include <linux/types.h>

#include <stdio.h>

#include "xv.h"

#include <sys/ipc.h>
#include <sys/shm.h>

#include <unicap.h>


static unsigned int
xv_mode_id(char * fourcc)
{
  return ((((unsigned int)(fourcc[0])<<0)|
	   ((unsigned int)(fourcc[1])<<8)|
	   ((unsigned int)(fourcc[2])<<16)|
	   ((unsigned int)(fourcc[3])<<24)));
}
#define YUY2 xv_mode_id("YUY2") /* YUYV (packed, 16 bits) */
#define UYVY xv_mode_id("UYVY") /* UYVY (packed, 16 bits) */



int xv_init( xv_handle_t handle, Display *dpy, __u32 fourcc, int width, int height, int bpp )
{
	unsigned int version, release;
	unsigned int request_base, event_base, error_base;
	
	int i;

	if( XvQueryExtension( dpy, &version, &release, &request_base, &event_base, &error_base ) != Success )
	{
#ifdef DEBUG
		fprintf( stderr, "XvQueryExtension failed\n" );
#endif
		return -1;
	}
	
	if( XvQueryAdaptors( dpy, DefaultRootWindow( dpy ), &handle->num_adaptors, &handle->p_adaptor_info ) != Success )
	{
#ifdef DEBUG
		fprintf( stderr, "XvQueryAdaptors failed\n" );
#endif
		return -1;
	}

/* 	printf( "%d adaptors found\n", handle->num_adaptors ); */
	
	if( handle->num_adaptors == 0 )
	{
		return -2;
	}
	

	for( i = 0; i < handle->num_adaptors; i++ )
	{
/* 		int format; */
		unsigned int num_encodings;
		XvEncodingInfo *p_encoding_info;
		int encoding;

		XvAttribute *at;
		unsigned int num_attributes;
		int attribute;

		XvImageFormatValues *xvimage_formats;
		unsigned int num_xvimage_formats;
		int format;
		

#ifdef DEBUG
		printf( "Adaptor: %d\n", i );
		printf( "Name: %s\n", handle->p_adaptor_info[i].name );
		printf( "Ports: %lu\n", handle->p_adaptor_info[i].num_ports );
		printf( "Formats: %lu\n", handle->p_adaptor_info[i].num_formats );
		
		for( format = 0; format < handle->p_adaptor_info[i].num_formats; format++ )
		{
			printf( "+Format: %d\n", format );
			printf( " +Depth: %d\n", handle->p_adaptor_info[i].formats[format].depth );
			printf( " +VisualID: %lu\n", handle->p_adaptor_info[i].formats[format].visual_id );
		}

			
		if( XvQueryEncodings( dpy, handle->p_adaptor_info[i].base_id, &num_encodings, &p_encoding_info ) != Success )
		{
			fprintf( stderr, "XvQueryEncodings failed\n" );
		}
		printf( " +num_encodings: %d\n", num_encodings );
		
		for( encoding = 0; encoding < num_encodings; encoding++ )
		{
			printf( "  +Encoding: %d\n", encoding );
			printf( "  +Name: %s\n", p_encoding_info[encoding].name );
			printf( "  +Resolution: %lu x %lu\n", 
					p_encoding_info[encoding].width, 
					p_encoding_info[encoding].height );
		}
#endif //DEBUG

		at = XvQueryPortAttributes( dpy, handle->p_adaptor_info[i].base_id, &num_attributes );
#ifdef DEBUG
		printf( "num_attributes: %d\n", num_attributes );
#endif
		for( attribute = 0; attribute < num_attributes; attribute++ )
		{
			int val;
			Atom atom;
			
			if( !strcmp( at[attribute].name, "XV_COLORKEY" ) )
			{
#ifdef DEBUG
				printf( "attribute: %d\n", attribute );
				printf( "name: %s\n", at[attribute].name );
#endif
				atom = (Atom)XInternAtom( dpy, at[attribute].name, 0 );
#ifdef DEBUG
				printf( "atom: %p\n", atom );
#endif
				XvGetPortAttribute( dpy, 
									handle->p_adaptor_info[i].base_id, 
									atom, 
									&val );
#ifdef DEBUG
				printf( "Attribute: %d\n", attribute );
				printf( "Name: %s\n", at[attribute].name );
				printf( "min: %x\n", at[attribute].min_value );
				printf( "max: %x\n", at[attribute].max_value );
				printf( "value: %x\n", val );
#endif
				handle->atom_colorkey = XInternAtom( dpy, at[attribute].name, 0 );
			}
			if( !strcmp( at[attribute].name, "XV_BRIGHTNESS" ) )
			{
				handle->atom_brightness = XInternAtom( dpy, at[attribute].name, 0 );
				handle->brightness_min = at[attribute].min_value;
				handle->brightness_max = at[attribute].max_value;
			}
			if( !strcmp( at[attribute].name, "XV_HUE" ) )
			{
				handle->atom_hue = XInternAtom( dpy, at[attribute].name, 0 );
				handle->hue_min = at[attribute].min_value;
				handle->hue_max = at[attribute].max_value;
			}
			if( !strcmp( at[attribute].name, "XV_CONTRAST" ) )
			{
				handle->atom_contrast = XInternAtom( dpy, at[attribute].name, 0 );
				handle->contrast_min = at[attribute].min_value;
				handle->contrast_max = at[attribute].max_value;
			}
			if( !strcmp( at[attribute].name, "XV_DOUBLE_BUFFER" ) )
			{
				Atom _atom;
				_atom = XInternAtom( dpy, at[attribute].name, 0 );
				XvSetPortAttribute( dpy, handle->p_adaptor_info[i].base_id, _atom, 1 );
#ifdef DEBUG
				printf( "Xv: DOUBLE_BUFFER available\n" );
#endif
			}	
		}

		xvimage_formats = XvListImageFormats( dpy, handle->p_adaptor_info[i].base_id, &num_xvimage_formats );
/* 		printf( "num_xvimage_formats: %d\n", num_xvimage_formats ); */
		for( format = 0; format < num_xvimage_formats; format++ )
		{
			char imageName[5] = {0, 0, 0, 0, 0};
			memcpy(imageName, &(xvimage_formats[format].id), 4);
#ifdef DEBUG
			fprintf(stdout, "      id: 0x%x", xvimage_formats[format].id);
#endif
			if( isprint( imageName[0]) && isprint(imageName[1]) &&
			    isprint( imageName[2]) && isprint(imageName[3])) 
			{
#ifdef DEBUG
				fprintf(stdout, " (%s)\n", imageName);
#endif
				if( xvimage_formats[format].id == fourcc )
				{
					handle->xv_mode_id = fourcc;
					break;
				}
			} else {
#ifdef DEBUG
				fprintf(stdout, "\n");
#endif
			}
		}

		if( handle->xv_mode_id != fourcc )
		{
			return -3;
		}

		if( XvGrabPort( dpy, handle->p_adaptor_info[i].base_id, CurrentTime ) != Success )
		{
/* 			fprintf( stderr, "Failed to grab port!\n" ); */
			return -1;
		}
	}

	handle->use_shm = 1;

	if( handle->use_shm )
	{
		memset( &handle->shminfo, 0, sizeof( XShmSegmentInfo ) );
		handle->image = XvShmCreateImage( dpy, 
						  handle->p_adaptor_info[0].base_id, 
						  handle->xv_mode_id, 
						  (char*)NULL, 
						  width,
						  height, 
						  &handle->shminfo );

		if( handle->image )
		{
			handle->shminfo.shmid = shmget( IPC_PRIVATE, handle->image->data_size, IPC_CREAT | 0777 );
			if( handle->shminfo.shmid == -1 )
			{
/* 				fprintf( stderr, "shmget failed\n" ); */
				return -1;
			}
			handle->shminfo.shmaddr = handle->image->data = shmat( handle->shminfo.shmid, 0, 0 );
			shmctl(handle->shminfo.shmid, IPC_RMID, 0);
			/* destroy when we terminate, now if shmat failed */

			if( handle->shminfo.shmaddr == ( void * ) -1 )
			{
/* 				fprintf( stderr, "shmat failed\n" ); */
				return -1;
			}
			
			handle->shminfo.readOnly = False;
			if( !XShmAttach( dpy, &handle->shminfo ) )
			{
/* 				fprintf( stderr, "XShmAttach failed\n" ); */
				shmdt( handle->shminfo.shmaddr );
				XFree( handle->image );
				return -1;
			}
		}
		else
		{
/* 			fprintf( stderr, "XvShmCreateImage failed\n" ); */
			return -1;
		}
	}
	else
	{
		char * data = (char*) malloc( width*height*(bpp/8));
		handle->image = XvCreateImage( dpy, handle->p_adaptor_info[0].base_id, handle->xv_mode_id, data, width, height );
	}	

	handle->display = dpy;

	return 0;
}

int xv_test_fourcc( Display *dpy, __u32 fourcc )
{
	unsigned int version, release;
	unsigned int request_base, event_base, error_base;
	
	int i;

	int num_adaptors;
	XvAdaptorInfo *p_adaptor_info;

	if( XvQueryExtension( dpy, &version, &release, &request_base, &event_base, &error_base ) != Success )
	{
#ifdef DEBUG
		fprintf( stderr, "XvQueryExtension failed\n" );
#endif
		return -1;
	}
	
	if( XvQueryAdaptors( dpy, DefaultRootWindow( dpy ), &num_adaptors, &p_adaptor_info ) != Success )
	{
#ifdef DEBUG
		fprintf( stderr, "XvQueryAdaptors failed\n" );
#endif
		return -1;
	}

	if( num_adaptors == 0 )
	{
		return -2;
	}
	

	for( i = 0; i < num_adaptors; i++ )
	{
		unsigned int num_encodings;
		XvEncodingInfo *p_encoding_info;
		int encoding;

		XvAttribute *at;
		unsigned int num_attributes;
		int attribute;

		XvImageFormatValues *xvimage_formats;
		unsigned int num_xvimage_formats;
		int format;
		

#ifdef DEBUG
		if( XvQueryEncodings( dpy, p_adaptor_info[i].base_id, &num_encodings, &p_encoding_info ) != Success )
		{
			fprintf( stderr, "XvQueryEncodings failed\n" );
		}
#endif


		xvimage_formats = XvListImageFormats( dpy, p_adaptor_info[i].base_id, &num_xvimage_formats );
		for( format = 0; format < num_xvimage_formats; format++ )
		{
			if( xvimage_formats[format].id == fourcc )
			{
				return 1;
			}
		}

		return 0;
	}

	return 0;
}

void xv_update_image( xv_handle_t handle, __u8 *image_data, size_t image_data_size, int w, int h )
{
	int y;
	static int val = 0;
	
/* 	memcpy( handle->image->data, image_data, image_data_size ); */
	
	if( handle->use_shm )
	{
		XvShmPutImage( handle->display,//GDK_DISPLAY(),
			       handle->p_adaptor_info[0].base_id,
			       handle->drawable,//GDK_WINDOW_XWINDOW( widget->window ),
			       handle->gc,//GDK_GC_XGC( widget->style->fg_gc[GTK_WIDGET_STATE (widget)] ),
			       handle->image,
			       0,
			       0,
			       handle->image->width,
			       handle->image->height,
			       0,
			       0,
			       w,
			       h,
			       1 );
	}
	else
	{
		XvPutImage( handle->display,//GDK_DISPLAY(), 
			    handle->p_adaptor_info[0].base_id, 
			    handle->drawable,//GDK_WINDOW_XWINDOW( widget->window ), 
			    handle->gc,//GDK_GC_XGC( widget->style->fg_gc[GTK_WIDGET_STATE (widget)] ), 
			    handle->image,
			    0, 0, handle->image->width, handle->image->height,
			    0, 0, w, h );
	}
}

