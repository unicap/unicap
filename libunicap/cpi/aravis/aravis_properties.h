#ifndef __ARAVIS_PROPERTIES_H__
#define __ARAVIS_PROPERTIES_H__

#include <arv.h>



typedef enum
{
	ARAVIS_PROPERTY_TYPE_INTEGER = 0,
	ARAVIS_PROPERTY_TYPE_FLOAT,
	ARAVIS_PROPERTY_TYPE_ENUM,    //aravis lib report string features as enums
	ARAVIS_PROPERTY_TYPE_COMMAND,

	ARAVIS_PROPERTY_TYPE_UNKNOWN
} aravis_property_type_enum_t;


typedef unicap_status_t (*aravis_property_func_t) (ArvCamera *camera, unicap_property_t *property);

struct aravis_property
{
	unicap_property_t property;
	aravis_property_func_t set;
	aravis_property_func_t get;
	aravis_property_func_t query;
};

void
aravis_properties_create_table(ArvCamera *cam, struct aravis_property **properties, unsigned int *n_properties);


#endif//__ARAVIS_PROPERTIES_H__
