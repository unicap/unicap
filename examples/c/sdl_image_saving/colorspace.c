#include <sys/types.h>
#include <linux/types.h>
#include <stdio.h>

size_t uyvy2rgb24( __u8 *dest, __u8 *source, size_t dest_size, size_t source_size )
{
	int i;
	int dest_offset = 0;
	
	if( dest_size < ( source_size + ( source_size >> 1 ) ) )
	{
		printf( "dest_size : %d, source_size: %d !!! ( FAIL ) \n", 
				dest_size, source_size );
		return 0;
	}

	for( i = 0; i < source_size; i+=4 )
	{
		__u8 *r, *b, *g;
		__u8 *y1, *y2, *u, *v;
		
		double fr, fg, fb;
		double fy1, fy2, fu, fv;

		r = dest + dest_offset;
		g = r + 1;
		b = g + 1;
		
		u = source + i;
		y1 = u + 1;
		v = y1 + 1;
		y2 = v + 1;

		fu = *u;
		fv = *v;
		fy1= *y1;
		fy2= *y2;

		fr = fy1 - 0.0009267 * ( fu - 128 ) + 1.4016868 * ( fv - 128 );
		fg = fy1 - 0.3436954 * ( fu - 128 ) - 0.7141690 * ( fv - 128 );
		fb = fy1 + 1.7721604 * ( fu - 128 ) + 0.0009902 * ( fv - 128 );

		*r = (__u8) ( fr > 255 ? 255 : ( fr < 0 ? 0 : fr ) );
		*g = (__u8) ( fg > 255 ? 255 : ( fg < 0 ? 0 : fg ) );
		*b = (__u8) ( fb > 255 ? 255 : ( fb < 0 ? 0 : fb ) );
		
		
		dest_offset += 3;
		
		r = dest + dest_offset;
		g = r + 1;
		b = g + 1;
		
		fr = fy2 - 0.0009267 * ( fu - 128 ) + 1.4016868 * ( fv - 128 );
		fg = fy2 - 0.3436954 * ( fu - 128 ) - 0.7141690 * ( fv - 128 );
		fb = fy2 + 1.7721604 * ( fu - 128 ) + 0.0009902 * ( fv - 128 );

		*r = (__u8) ( fr > 255 ? 255 : ( fr < 0 ? 0 : fr ) );
		*g = (__u8) ( fg > 255 ? 255 : ( fg < 0 ? 0 : fg ) );
		*b = (__u8) ( fb > 255 ? 255 : ( fb < 0 ? 0 : fb ) );

		dest_offset += 3;
	}

// From SciLab : This is the good one.
	//r = 1 * y -  0.0009267*(u-128)  + 1.4016868*(v-128);^M
	//g = 1 * y -  0.3436954*(u-128)  - 0.7141690*(v-128);^M
	//b = 1 * y +  1.7721604*(u-128)  + 0.0009902*(v-128);^M

	// YUV->RGB
	// r = 1.164 * (y-16) + 1.596*(v-128);
	// g = 1.164 * (y-16) + 0.813*(v-128) - 0.391*(u-128);
	// b = 1.164 * (y-16) + 2.018*(u-128);
	return source_size * 1.5;
}

size_t uyvy2rgb32( __u8 *dest, __u8 *source, size_t dest_size, size_t source_size )
{
	int i;
	int dest_offset = 0;
	
	if( dest_size < ( source_size * 2 ) )
	{
		return 0;
	}

	for( i = 0; i < source_size; i+=4 )
	{
		__u8 *r, *b, *g, *a;
		__u8 *y1, *y2, *u, *v;
		
		r = dest + dest_offset;
		g = r + 1;
		b = g + 1;
		a = b + 1;
		
		u = source + i;
		y1 = u + 1;
		v = y1 + 1;
		y1 = v + 1;

		*r = *y1 + ( 1.4075 * ( *v - 128 ) );
		*g = *y1 - ( 0.3455 * ( *u - 128 ) - ( 0.7169 * ( *v - 128 ) ) );
		*b = *y1 + ( 1.7790 * ( *u - 128 ) );
		*a = 0;
		
		dest_offset += 4;
		
		r = dest + dest_offset;
		g = r + 1;
		b = g + 1;
		a = b + 1;
		
		*r = *y2 + ( 1.4075 * ( *v - 128 ) );
		*g = *y2 - ( 0.3455 * ( *u - 128 ) - ( 0.7169 * ( *v - 128 ) ) );
		*b = *y2 + ( 1.7790 * ( *u - 128 ) );
		*a = 0;

		dest_offset += 4;
	}
	
	return source_size * 2;
}

size_t y4202rgb32( __u8 *dest, __u8 *source, int w, int h )
{
	int i, j;
	int dest_offset = 0;
//	int plane0_offset = h * w;
	int line_offset;
	
	for( j = 0; j < h; j++ )
	{
		line_offset = ( j / 2 ) * w;
		for( i = 0; i < w; i++ )
		{
			__u8 *r, *b, *g, *a;
			__u8 *y, *u, *v;
			
 			r = dest + dest_offset;
			g = r + 1;
			b = g + 1;
			a = b + 1;
			
			y = source + ( j * w ) + i;
			u = source + ( j * w ) + w + ( i >> 1 );//( line_offset + plane0_offset );
			v = source + ( j * w ) + w + ( w >> 1 ) + ( i>>1 );//( line_offset + plane1_offset );
			
			*r = *y + ( 1.4075 * ( *v - 128 ) );
			*g = *y - ( 0.3455 * ( *u - 128 ) - ( 0.7169 * ( *v - 128 ) ) );
			*b = *y + ( 1.7790 * ( *u - 128 ) );
			*a = 0;
		
			dest_offset += 4;		
		}
	}
	
	return 0;
}

size_t y8002rgb24( __u8 *dest, __u8 *source, size_t dest_size, size_t source_size )
{
	int i;

	int dest_offset = 0;
	
	if( dest_size < ( source_size * 3 ) )
	{
		return 0;
	}
	
	for( i = 0; i < source_size; i++ )
	{
		__u8 *y;
		__u8 *r, *b, *g;
		
		r = dest + dest_offset;
		g = r + 1;
		b = g + 1;
		
		y = source + i;
		
		*r = *y;
		*g = *y;
		*b = *y;
		
		dest_offset +=3;
	}
	
	return source_size * 3;
}

size_t y8002rgb32( __u8 *dest, __u8 *source, size_t dest_size, size_t source_size )
{
	int i;

	int dest_offset = 0;
	
	if( dest_size < ( source_size * 4 ) )
	{
		return 0;
	}
	
	for( i = 0; i < source_size; i++ )
	{
		__u8 *y;
		__u8 *r, *b, *g, *a;
		
		r = dest + dest_offset;
		g = r + 1;
		b = g + 1;
		a = b + 1;
		
		y = source + i;
		
		*r = *y;
		*g = *y;
		*b = *y;
		*a = 0;
		
		dest_offset +=4;
	}
	
	return source_size * 4;
}

