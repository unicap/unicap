#include <unicap.h>
#include <stdlib.h>
#include <string.h>

#include "aravis_properties.h"

#define N_(x) x
#define N_ELEMENTS(a) (sizeof( a )/ sizeof(( a )[0] ))


static unicap_status_t aravis_property_set_exposure(ArvCamera *cam, unicap_property_t *property);
static unicap_status_t aravis_property_get_exposure(ArvCamera *cam, unicap_property_t *property);
static unicap_status_t aravis_property_query_exposure(ArvCamera *cam, unicap_property_t *property);
static unicap_status_t aravis_property_set_gain(ArvCamera *cam, unicap_property_t *property);
static unicap_status_t aravis_property_get_gain(ArvCamera *cam, unicap_property_t *property);
static unicap_status_t aravis_property_query_gain(ArvCamera *cam, unicap_property_t *property);




static struct aravis_property aravis_properties[] = 
{
	{
		{
		  identifier: N_("Shutter"),
		  category: N_("Exposure"),
		  unit: N_("s"),
		  relations: NULL, 
		  relations_count: 0,
		  {value: 0},
		  {range: {min: 0, max: 0}},
		  stepping: 0.0001,
		  type: UNICAP_PROPERTY_TYPE_RANGE,
		  flags: UNICAP_FLAGS_MANUAL, 
		  flags_mask: UNICAP_FLAGS_MANUAL | UNICAP_FLAGS_AUTO,
		  property_data: NULL, 
		  property_data_size: 0,
		},
	
		aravis_property_set_exposure, 
		aravis_property_get_exposure, 
		aravis_property_query_exposure
	},
	{
		{
		  identifier: N_("Gain"),
		  category: N_("Exposure"),
		  unit: "",
		  relations: NULL, 
		  relations_count: 0,
		  {value: 0},
		  {range: {min: 0, max: 0}},
		  stepping: 1.0,
		  type: UNICAP_PROPERTY_TYPE_RANGE,
		  flags: UNICAP_FLAGS_MANUAL, 
		  flags_mask: UNICAP_FLAGS_MANUAL | UNICAP_FLAGS_AUTO,
		  property_data: NULL, 
		  property_data_size: 0,
		},
	
		aravis_property_set_gain, 
		aravis_property_get_gain, 
		aravis_property_query_gain
	},	  
};

	
static unicap_status_t aravis_property_set_exposure(ArvCamera *cam, unicap_property_t *property)
{
	if (property->flags & UNICAP_FLAGS_ONE_PUSH){
		arv_camera_set_exposure_time_auto (cam, ARV_AUTO_ONCE);
	} else if (property->flags & UNICAP_FLAGS_AUTO){
		arv_camera_set_exposure_time_auto (cam, ARV_AUTO_CONTINUOUS);
	} else {
		arv_camera_set_exposure_time_auto (cam, ARV_AUTO_OFF);
		arv_camera_set_exposure_time (cam, property->value * 1000000.0);
	}
	
	return STATUS_SUCCESS;
}


static unicap_status_t aravis_property_get_exposure(ArvCamera *cam, unicap_property_t *property)
{
	switch (arv_camera_get_exposure_time_auto (cam)){
	default:
	case ARV_AUTO_OFF:
		property->flags = UNICAP_FLAGS_MANUAL;
		break;
	case ARV_AUTO_ONCE:
		property->flags = UNICAP_FLAGS_ONE_PUSH;
		break;
	case ARV_AUTO_CONTINUOUS:
		property->flags = UNICAP_FLAGS_AUTO;
		break;
	}
	
	property->value = arv_camera_get_exposure_time (cam) / 1000000.0;
	
	return STATUS_SUCCESS;
}

static unicap_status_t aravis_property_query_exposure(ArvCamera *cam, unicap_property_t *property)
{
	arv_camera_get_exposure_time_bounds (cam, &property->range.min, &property->range.max);
	property->range.min /= 1000000.0;
	property->range.max /= 1000000.0;
	aravis_property_get_exposure (cam, property);
	
	return STATUS_SUCCESS;
}

static unicap_status_t aravis_property_set_gain(ArvCamera *cam, unicap_property_t *property)
{
	if (property->flags & UNICAP_FLAGS_ONE_PUSH){
		arv_camera_set_gain_auto (cam, ARV_AUTO_ONCE);
	} else if (property->flags & UNICAP_FLAGS_AUTO){
		arv_camera_set_gain_auto (cam, ARV_AUTO_CONTINUOUS);
	} else {
		arv_camera_set_gain_auto (cam, ARV_AUTO_OFF);
		arv_camera_set_gain (cam, property->value);
	}
	
	return STATUS_SUCCESS;
}


static unicap_status_t aravis_property_get_gain(ArvCamera *cam, unicap_property_t *property)
{
	switch (arv_camera_get_gain_auto (cam)){
	default:
	case ARV_AUTO_OFF:
		property->flags = UNICAP_FLAGS_MANUAL;
		break;
	case ARV_AUTO_ONCE:
		property->flags = UNICAP_FLAGS_ONE_PUSH;
		break;
	case ARV_AUTO_CONTINUOUS:
		property->flags = UNICAP_FLAGS_AUTO;
		break;
	}
	
	property->value = arv_camera_get_gain (cam);
	
	return STATUS_SUCCESS;
}

static unicap_status_t aravis_property_query_gain(ArvCamera *cam, unicap_property_t *property)
{
	gint min,max;
	arv_camera_get_gain_bounds (cam, &min, &max);
	property->range.min = min;
	property->range.max = max;
	aravis_property_get_gain (cam, property);
	
	return STATUS_SUCCESS;
}

void
aravis_properties_create_table(ArvCamera *cam, struct aravis_property **properties, unsigned int *n_properties)
{
	int i;
	int cnt = 0;
	struct aravis_property *propt = NULL;
	
	propt = malloc(sizeof(struct aravis_property) * N_ELEMENTS(aravis_properties));
	
	for (i=0; i < N_ELEMENTS(aravis_properties); i++){
		memcpy (propt + cnt, aravis_properties + i, sizeof (struct aravis_property));
		if(SUCCESS( propt[cnt].query(cam, &propt[cnt].property) ))
			cnt++;
	}
	
	*n_properties = cnt;

	*properties = propt;
}
