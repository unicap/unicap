#include <unicap.h>
#include <arv.h>



#define N_ELEMENTS(a) (sizeof( a )/ sizeof(( a )[0] ))
#define FOURCC(a,b,c,d) (unsigned int)((((unsigned int)d)<<24)+(((unsigned int)c)<<16)+(((unsigned int)b)<<8)+a)


struct aravis_pixel_format
{
	ArvPixelFormat pixel_format;
	unsigned int fourcc;
	int bpp;
	char *desc;
};


static struct aravis_pixel_format pixel_fmt_desc[] = {
	/* Grey pixel formats */
	{
		ARV_PIXEL_FORMAT_MONO_8,
		FOURCC ('Y', '8', '0', '0'),
		8,
		"Y800"
	},
	{
		ARV_PIXEL_FORMAT_MONO_8_SIGNED,
		FOURCC ('S', 'Y', '8', 0),
		8,
		"Signed Mono8"
	},
	{
		ARV_PIXEL_FORMAT_MONO_10,
		FOURCC ('Y', '1', '0', 0),
		10,
		"Mono10"
	},
	{
		ARV_PIXEL_FORMAT_MONO_10_PACKED,
		FOURCC ('Y', '1', '0', 'P'),
		10,
		"Mono10 packed"
	},
	{
		ARV_PIXEL_FORMAT_MONO_12,
		FOURCC ('Y', '1', '2', 0),
		12,
		"Mono12"
	},
	{
		ARV_PIXEL_FORMAT_MONO_12_PACKED,
		FOURCC ('Y', '1', '2', 'P'),
		12,
		"Mono12 packed"
	},
	{
		ARV_PIXEL_FORMAT_MONO_16,
		FOURCC ('Y', '1', '6', ' '),
		16,
		"Mono16"
	},
	{
		ARV_PIXEL_FORMAT_BAYER_GB_8,
		//FOURCC ('B', 'Y', '8', 0),
		FOURCC('B','Y','8',' '),
		8,
		"Bayer GB 8"
	},



	/* { */
	/* 	ARV_PIXEL_FORMAT_BAYER_GR_8, */
	/* 	FOURCC ('B', 'Y', '8', 0), */
	/* 	"Bayer GR 8", */
	/* ARV_PIXEL_FORMAT_BAYER_RG_8		= 0x01080009, */
	/* ARV_PIXEL_FORMAT_BAYER_GB_8		= 0x0108000a, */
	/* ARV_PIXEL_FORMAT_BAYER_BG_8		= 0x0108000b, */

	/* ARV_PIXEL_FORMAT_BAYER_GR_10		= 0x0110000c, */
	/* ARV_PIXEL_FORMAT_BAYER_RG_10		= 0x0110000d, */
	/* ARV_PIXEL_FORMAT_BAYER_GB_10		= 0x0110000e, */
	/* ARV_PIXEL_FORMAT_BAYER_BG_10		= 0x0110000f, */

	/* ARV_PIXEL_FORMAT_BAYER_GR_12		= 0x01100010, */
	/* ARV_PIXEL_FORMAT_BAYER_RG_12		= 0x01100011, */
	/* ARV_PIXEL_FORMAT_BAYER_GB_12		= 0x01100012, */
	/* ARV_PIXEL_FORMAT_BAYER_BG_12		= 0x01100013, */

	/* /\* Color pixel formats *\/ */

	/* ARV_PIXEL_FORMAT_RGB_8_PACKED		= 0x02180014, */
	/* ARV_PIXEL_FORMAT_BGR_8_PACKED		= 0x02180015, */

	/* ARV_PIXEL_FORMAT_RGBA_8_PACKED		= 0x02200016, */
	/* ARV_PIXEL_FORMAT_BGRA_8_PACKED		= 0x02200017, */

	/* ARV_PIXEL_FORMAT_RGB_10_PACKED		= 0x02300018, */
	/* ARV_PIXEL_FORMAT_BGR_10_PACKED		= 0x02300019, */

	/* ARV_PIXEL_FORMAT_RGB_12_PACKED		= 0x0230001a, */
	/* ARV_PIXEL_FORMAT_BGR_12_PACKED		= 0x0230001b, */

	/* ARV_PIXEL_FORMAT_YUV_411_PACKED		= 0x020c001e, */
	/* ARV_PIXEL_FORMAT_YUV_422_PACKED		= 0x0210001f, */
	/* ARV_PIXEL_FORMAT_YUV_444_PACKED		= 0x02180020, */

	/* ARV_PIXEL_FORMAT_RGB_8_PLANAR		= 0x02180021, */
	/* ARV_PIXEL_FORMAT_RGB_10_PLANAR		= 0x02300022, */
	/* ARV_PIXEL_FORMAT_RGB_12_PLANAR		= 0x02300023, */
	/* ARV_PIXEL_FORMAT_RGB_16_PLANAR		= 0x02300024, */

	/* ARV_PIXEL_FORMAT_YUV_422_YUYV_PACKED 	= 0x02100032, */

	/* /\* Custom *\/ */

	/* ARV_PIXEL_FORMAT_CUSTOM_BAYER_GR_12_PACKED  	= 0x810c0001, */
	/* ARV_PIXEL_FORMAT_CUSTOM_BAYER_RG_12_PACKED  	= 0x810c0002, */
	/* ARV_PIXEL_FORMAT_CUSTOM_BAYER_GB_12_PACKED  	= 0x810c0003, */
	/* ARV_PIXEL_FORMAT_CUSTOM_BAYER_BG_12_PACKED  	= 0x810c0004, */
	/* ARV_PIXEL_FORMAT_CUSTOM_YUV_422_YUYV_PACKED 	= 0x82100005, */
	/* ARV_PIXEL_FORMAT_CUSTOM_BAYER_GR_16		= 0x81100006, */
	/* ARV_PIXEL_FORMAT_CUSTOM_BAYER_RG_16		= 0x81100007, */
	/* ARV_PIXEL_FORMAT_CUSTOM_BAYER_GB_16		= 0x81100008, */
	/* ARV_PIXEL_FORMAT_CUSTOM_BAYER_BG_16		= 0x81100009 */
};

unsigned int aravis_tools_get_fourcc (ArvPixelFormat fmt)
{
	int i;

	for (i = 0; i < N_ELEMENTS (pixel_fmt_desc); i++){
		if (pixel_fmt_desc [i].pixel_format == fmt)
			return pixel_fmt_desc [i].fourcc;
	}

	return 0;
}

const char *aravis_tools_get_pixel_format_string (ArvPixelFormat fmt)
{
	int i;

	for (i = 0; i < N_ELEMENTS (pixel_fmt_desc); i++){
		if (pixel_fmt_desc [i].pixel_format == fmt)
			return pixel_fmt_desc [i].desc;
	}

	return "Unknown";
}

int aravis_tools_get_bpp (ArvPixelFormat fmt)
{
	int i;

	for (i = 0; i < N_ELEMENTS (pixel_fmt_desc); i++){
		if (pixel_fmt_desc [i].pixel_format == fmt)
			return pixel_fmt_desc [i].bpp;
	}

	return 0;
}

ArvPixelFormat aravis_tools_get_pixel_format (unicap_format_t *format)
{
	ArvPixelFormat ret = 0;
	int i;

	for (i = 0; i < N_ELEMENTS (pixel_fmt_desc); i++){
		if ((pixel_fmt_desc[i].fourcc == format->fourcc) ||
		    (!strcmp (pixel_fmt_desc [i].desc, format->identifier))){
			ret = pixel_fmt_desc [i].pixel_format;
			break;
		}
	}

	return ret;
}

