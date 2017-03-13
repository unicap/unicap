#include <unicap.h>
#include <stdlib.h>
#include <string.h>

#include "aravis_properties.h"

#define N_(x) x
#define N_ELEMENTS(a) (sizeof( a )/ sizeof(( a )[0] ))
#define ARAVIS_PROP_MAX_CATEGORIES 10

static unicap_status_t aravis_property_set_exposure(ArvCamera *cam, unicap_property_t *property);
static unicap_status_t aravis_property_get_exposure(ArvCamera *cam, unicap_property_t *property);
static unicap_status_t aravis_property_query_exposure(ArvCamera *cam, unicap_property_t *property);
static unicap_status_t aravis_property_set_gain(ArvCamera *cam, unicap_property_t *property);
static unicap_status_t aravis_property_get_gain(ArvCamera *cam, unicap_property_t *property);
static unicap_status_t aravis_property_query_gain(ArvCamera *cam, unicap_property_t *property);

static unicap_status_t aravis_property_set_fps(ArvCamera *cam, unicap_property_t *property);
static unicap_status_t aravis_property_get_fps(ArvCamera *cam, unicap_property_t *property);
static unicap_status_t aravis_property_query_fps(ArvCamera *cam, unicap_property_t *property);
// generic setter / getter /query functions
static unicap_status_t aravis_property_set_enum(ArvCamera *cam, unicap_property_t *property);
static unicap_status_t aravis_property_get_enum(ArvCamera *cam, unicap_property_t *property);
static unicap_status_t aravis_property_query_enum(ArvCamera *cam, unicap_property_t *property);

static unicap_status_t aravis_property_set_int(ArvCamera *cam, unicap_property_t *property);
static unicap_status_t aravis_property_get_int(ArvCamera *cam, unicap_property_t *property);
static unicap_status_t aravis_property_query_int(ArvCamera *cam, unicap_property_t *property);

static unicap_status_t aravis_property_set_float(ArvCamera *cam, unicap_property_t *property);
static unicap_status_t aravis_property_get_float(ArvCamera *cam, unicap_property_t *property);
static unicap_status_t aravis_property_query_float(ArvCamera *cam, unicap_property_t *property);


static void
arv_tool_list_features (ArvGc *genicam, const char *feature, gboolean show_description, int level);

static char *fps_menu[] =
{
   N_("3.75"),
   N_("7.5"),
   N_("15"),
   N_("20")
};
static int n_properties;

#define FPS_DEFAULT_ITEM "15"


static struct aravis_property aravis_properties[] =
{
	{
		{
		  identifier: N_("Shutter"),
		  category: N_("AcquisitionControl"),
		  unit: N_("s"),
		  relations: NULL,
		  relations_count: 0,
		  {value: 0},
		  {range: {min: 0, max: 0}},
		  stepping: 0.0001,
		  type: UNICAP_PROPERTY_TYPE_RANGE,
		  flags: UNICAP_FLAGS_MANUAL,
		  //flags_mask: UNICAP_FLAGS_MANUAL | UNICAP_FLAGS_AUTO,
		  flags_mask: UNICAP_FLAGS_MANUAL,
		  property_data: NULL,
		  property_data_size: 0,
		},

		aravis_property_set_exposure,
		aravis_property_get_exposure,
		aravis_property_query_exposure
	},
	{
		{
		  identifier: N_("FPS"),
		  category: N_("AcquisitionControl"),
		  unit: "",
		  relations: NULL,
		  relations_count: 0,

		  { menu_item: {FPS_DEFAULT_ITEM} },
			     { menu: { menu_items: fps_menu,
				       menu_item_count: sizeof( fps_menu ) / sizeof( char* ) } },
			     stepping: 0.0,
			     type: UNICAP_PROPERTY_TYPE_MENU,
			     flags: UNICAP_FLAGS_MANUAL,
			     flags_mask: UNICAP_FLAGS_MANUAL,
			     property_data: NULL,
			     property_data_size: 0
		},

		aravis_property_set_fps,
		aravis_property_get_fps,
		aravis_property_query_fps
	},
	{
		{
		  identifier: N_("Gain"),
		  category: N_("Exposure"),
		  unit: "",
		  relations: NULL,
		  relations_count: 0,
		  {value: 0},
		  {range: {min: 0, max: 10}},
		  stepping: 0.1,
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

static struct aravis_property *aravis_properties2;

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

static unicap_status_t aravis_property_set_fps(ArvCamera *cam, unicap_property_t *property)
{
    ArvDevice *device;

    device = arv_camera_get_device(cam);

    //printf("entro en set fps %s\n",property->menu_item);

    if (strcmp(property->menu_item ,N_("3.75")) == 0)  arv_device_set_string_feature_value (device,"FPS","FPS_3_75");
    if (strcmp(property->menu_item ,N_("7.5")) == 0)   arv_device_set_string_feature_value (device,"FPS","FPS_7_5");
    if (strcmp(property->menu_item ,N_("15"))  == 0)    arv_device_set_string_feature_value (device,"FPS","FPS_15");
    if (strcmp(property->menu_item , N_("20")) == 0)   arv_device_set_string_feature_value (device,"FPS","FPS_20");

/*  Enumeration: 'FPS'
    EnumEntry: 'FPS_3_75'
    EnumEntry: 'FPS_7_5'
    EnumEntry: 'FPS_15'
    EnumEntry: 'FPS_20'

 */

	return STATUS_SUCCESS;
}
static unicap_status_t aravis_property_get_fps(ArvCamera *cam, unicap_property_t *property)
{
    ArvDevice *device;
    const char *curval;
    device = arv_camera_get_device(cam);
    curval= arv_device_get_string_feature_value(device,"FPS");
   // printf("entro en get fps %s\n",curval);
    if (strcmp(curval ,"FPS_3_75") == 0)  strcpy(property->menu_item,N_("3.75"));
    if (strcmp(curval,"FPS_7_5") == 0)    strcpy(property->menu_item,N_("7.5"));
    if (strcmp(curval,"FPS_15")  == 0)    strcpy(property->menu_item,N_("15"));
    if (strcmp(curval,"FPS_20")  == 0)    strcpy(property->menu_item,N_("20"));



    //strcpy(property->menu_item,curval);

	return STATUS_SUCCESS;
}

static unicap_status_t aravis_property_query_fps(ArvCamera *cam, unicap_property_t *property)
{
	ArvDevice *device;
    const char *curval;
    device = arv_camera_get_device(cam);
    curval= arv_device_get_string_feature_value(device,"FPS");
    //printf("entro en get fps %s\n",curval);
    if (strcmp(curval ,"FPS_3_75") == 0)  strcpy(property->menu_item,N_("3.75"));
    if (strcmp(curval,"FPS_7_5") == 0)    strcpy(property->menu_item,N_("7.5"));
    if (strcmp(curval,"FPS_15")  == 0)    strcpy(property->menu_item,N_("15"));
    if (strcmp(curval,"FPS_20")  == 0)    strcpy(property->menu_item,N_("20"));



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
	double min,max;
	arv_camera_get_gain_bounds(cam, &min, &max);
	property->range.min = min;
	property->range.max = max;
	aravis_property_get_gain (cam, property);

	return STATUS_SUCCESS;
}

static unicap_status_t aravis_property_get_int(ArvCamera *cam, unicap_property_t *property)
{
    ArvDevice *device;
    device = arv_camera_get_device(cam);
    // cast due unicap property value is double
    property->value = (double) arv_device_get_integer_feature_value(device,property->identifier);
	return STATUS_SUCCESS;
}


static unicap_status_t aravis_property_set_int(ArvCamera *cam, unicap_property_t *property)
{
    ArvDevice *device;
    device = arv_camera_get_device(cam);
    // cast due unicap property value is double
    arv_device_set_integer_feature_value(device,property->identifier,(gint64) property->value);

	return STATUS_SUCCESS;
}

static unicap_status_t aravis_property_query_int(ArvCamera *cam, unicap_property_t *property)
{
    ArvDevice *device;
    gint64 min,max;
    device = arv_camera_get_device(cam);

    arv_device_get_integer_feature_bounds(device,property->identifier,&min,&max);
	property->range.min =(double)  min;
	property->range.max = (double) max;
	property->value = (double) arv_device_get_integer_feature_value(device,property->identifier);

	return STATUS_SUCCESS;
}

static unicap_status_t aravis_property_get_float(ArvCamera *cam, unicap_property_t *property)
{
    ArvDevice *device;
    device = arv_camera_get_device(cam);

    property->value =  arv_device_get_float_feature_value(device,property->identifier);
	return STATUS_SUCCESS;
}

static unicap_status_t aravis_property_set_float(ArvCamera *cam, unicap_property_t *property)
{
    ArvDevice *device;
    device = arv_camera_get_device(cam);

    arv_device_set_float_feature_value(device,property->identifier,property->value);

	return STATUS_SUCCESS;
}

static unicap_status_t aravis_property_query_float(ArvCamera *cam, unicap_property_t *property)
{
    ArvDevice *device;
    double min,max;
    device = arv_camera_get_device(cam);

    arv_device_get_float_feature_bounds(device,property->identifier,&min,&max);
	property->range.min = min;
	property->range.max = max;
	property->value = arv_device_get_float_feature_value(device,property->identifier);

	return STATUS_SUCCESS;
}


static unicap_status_t aravis_property_get_enum(ArvCamera *cam, unicap_property_t *property)
{
    ArvDevice *device;
    const char *curval;
    device = arv_camera_get_device(cam);
    curval = arv_device_get_string_feature_value(device,property->identifier);
    strcpy(property->menu_item,curval);
	return STATUS_SUCCESS;
}


static unicap_status_t aravis_property_set_enum(ArvCamera *cam, unicap_property_t *property)
{
    ArvDevice *device;
    device = arv_camera_get_device(cam);
    arv_device_set_string_feature_value(device,property->identifier,property->menu_item);
	return STATUS_SUCCESS;
}

static unicap_status_t aravis_property_query_enum(ArvCamera *cam, unicap_property_t *property)
{

	ArvDevice *device;
    const char *curval;
    device = arv_camera_get_device(cam);
    curval = arv_device_get_string_feature_value(device,property->identifier);
    strcpy(property->menu_item,curval);
	return STATUS_SUCCESS;
}

void
aravis_properties_create_table(ArvCamera *cam, struct aravis_property **properties, unsigned int *n_properties)
{
	int i;
	int cnt = 0;
	struct aravis_property *propt = NULL;
	struct aravis_property *propt2 = NULL;
	char *categories[ARAVIS_PROP_MAX_CATEGORIES];
	int ncats=0;
	int j=0;
	int totprops=0,propcnt=0;

    // willy
    // aqui se debe contar el numero de nodos genicam
    // y llenar la structura de properties de acuerdo con el n de propiedades
    // podemos colocar n+1 para mantener el caracter estatico del la propiedad shutter

    // contar todas las propiedades a mostar

    ArvGc *genicam;
    ArvDevice *device;
    ArvGcNode *node;

    device = arv_camera_get_device(cam);
    genicam = arv_device_get_genicam(device);

	node = arv_gc_get_node (genicam, "Root");
	if (ARV_IS_GC_FEATURE_NODE (node) &&
	    arv_gc_feature_node_is_implemented (ARV_GC_FEATURE_NODE (node), NULL))
    {

		//printf("for: %s\n",arv_dom_node_get_node_name (ARV_DOM_NODE (node)));
		if (ARV_IS_GC_CATEGORY (node))
		{
			const GSList *features;
			const GSList *iter;

			features = arv_gc_category_get_features (ARV_GC_CATEGORY (node));

			for (iter = features; iter != NULL; iter = iter->next){

				//arv_tool_list_features (genicam, iter->data, show_description, level + 1);
				//printf("Cat: %s\n",iter->data);
				//null terminated string ?
				categories[j]=malloc(sizeof(char *) * strlen(iter->data)+sizeof(char *) );
				memset(categories[j],0x0,sizeof(char *) * strlen(iter->data)+sizeof(char *) );
				strcpy(categories[j],iter->data);
				j++;
            }
			ncats=j;
		}
    }
    // contamos cuantas propiedades vamos a tener
    for (j=0;j<ncats;j++) {

        ArvGcNode *node_iter;
        ArvGcNode *node2;

        node_iter = arv_gc_get_node (genicam,categories[j]);
        if (ARV_IS_GC_CATEGORY (node_iter)) {
            const GSList *features;
            const GSList *iter;
            features = arv_gc_category_get_features (ARV_GC_CATEGORY (node_iter));

            for (iter = features; iter != NULL; iter = iter->next){
                node2 = arv_gc_get_node (genicam,iter->data);

                if (ARV_IS_GC_ENUMERATION(node2)) {
                   totprops++;
                }
                if (ARV_IS_GC_FLOAT(node2)) {
                    totprops++;
                }
                if (ARV_IS_GC_INTEGER(node2) && !ARV_IS_GC_ENUMERATION(node2)) {
                    totprops++;
                }
                /* if (ARV_IS_GC_COMMAND(node2)) {
                 *
                 *}
                 */
            }
        }
    }
    //totprops++;
    totprops++;
    propt2 = malloc(sizeof(struct aravis_property) * totprops);
    memset(propt2,0x0,sizeof(struct aravis_property) * totprops);
    //copiamo 1 propiedades estaticas !
    for (i=0; i < 1; i++)
		memcpy (propt2 + i, aravis_properties + i, sizeof (struct aravis_property));

    propcnt=1;
    for (j=0;j<ncats;j++) {

        ArvGcNode *node_iter;
        ArvGcNode *node2;

        //printf("Entrando en %s\n",categories[j]);
        node_iter = arv_gc_get_node (genicam,categories[j]);

        if (ARV_IS_GC_CATEGORY (node_iter)) {
              const GSList *features;
              const GSList *iter;
            //printf("nodo: %s\n",arv_dom_node_get_node_name(node_iter));

            features = arv_gc_category_get_features (ARV_GC_CATEGORY (node_iter));

            for (iter = features; iter != NULL; iter = iter->next){

                node2 = arv_gc_get_node (genicam,iter->data);
                strcpy(propt2[propcnt].property.category,categories[j]);
                strcpy(propt2[propcnt].property.identifier,iter->data);

                if (ARV_IS_GC_ENUMERATION(node2)) {
                    const GSList *enums;
                    const GSList *iter_enums;
                    int n_menus=0;
                    int k=0;
                    const char *mnu_entry;
		    int mnu_entry_len=0;	
                    char **menu_itms;

                    //printf("Category %s : %s is Enumeration \n",categories[j],iter->data);

                    enums = arv_gc_enumeration_get_entries (ARV_GC_ENUMERATION (node2));

                    for (iter_enums = enums; iter_enums != NULL; iter_enums = iter_enums->next){
                        n_menus++;
                    }

                    enums = arv_gc_enumeration_get_entries (ARV_GC_ENUMERATION (node2));

                    menu_itms = malloc(n_menus * sizeof(char *));
                    for (iter_enums = enums; iter_enums != NULL; iter_enums = iter_enums->next){
                        //printf("Category %s : EnumEntry '%s' \n",categories[j],  arv_gc_feature_node_get_name (iter_enums->data));
                        mnu_entry=arv_gc_feature_node_get_name (iter_enums->data);
			mnu_entry_len=strlen(mnu_entry);
			menu_itms[k]=malloc((mnu_entry_len+1) * sizeof(char *));			
                        memset(menu_itms[k],0x0,sizeof((mnu_entry_len+1) * sizeof(char *)));
                        strcpy(menu_itms[k],mnu_entry);
                        k++;
                    }

                    propt2[propcnt].property.menu.menu_items=menu_itms;
                    propt2[propcnt].property.menu.menu_item_count=n_menus;
                    propt2[propcnt].property.type = UNICAP_PROPERTY_TYPE_MENU;
                    propt2[propcnt].property.flags = UNICAP_FLAGS_MANUAL;

                    propt2[propcnt].set = aravis_property_set_enum;
                    propt2[propcnt].get =  aravis_property_get_enum;
                    propt2[propcnt].query = aravis_property_query_enum;

                    propcnt++;
                }
                if (ARV_IS_GC_FLOAT(node2)) {
                    //printf("Category %s : '%s' is float\n",categories[j],iter->data);

                    propt2[propcnt].property.type = UNICAP_PROPERTY_TYPE_RANGE;
                    propt2[propcnt].property.flags = UNICAP_FLAGS_MANUAL;
                    propt2[propcnt].property.stepping=0.1;
                    propt2[propcnt].set=aravis_property_set_float;
                    propt2[propcnt].get=aravis_property_get_float;
                    propt2[propcnt].query=aravis_property_query_float;

                    propcnt++;
                }
                if (ARV_IS_GC_INTEGER(node2) && !ARV_IS_GC_ENUMERATION(node2)) {
                   // printf("Category %s : '%s' is integer\n",categories[j],iter->data);
                    propt2[propcnt].property.type = UNICAP_PROPERTY_TYPE_RANGE;
                    propt2[propcnt].property.flags = UNICAP_FLAGS_MANUAL;
                    propt2[propcnt].property.stepping=1;
                    propt2[propcnt].set = aravis_property_set_int;
                    propt2[propcnt].get = aravis_property_get_int;
                    propt2[propcnt].query =  aravis_property_query_int;
                    propcnt++;
                }
                if (ARV_IS_GC_COMMAND(node2)) {
                    //printf("Category %s : '%s' is command \n",categories[j],iter->data);
                }

            } // element categories
        }

    } // categorie
    // fin


//  printf("tot categories: %d properties %d\n",ncats,totprops);
//	propt = malloc(sizeof(struct aravis_property) * N_ELEMENTS(aravis_properties));
//
//	for (i=0; i < N_ELEMENTS(aravis_properties); i++){
//		memcpy (propt + cnt, aravis_properties + i, sizeof (struct aravis_property));
//		if(SUCCESS( propt[cnt].query(cam, &propt[cnt].property) ))
//			cnt++;
//	}
//    for (i=0; i < totprops; i++){
//    printf("Property : %s : category : %s\n", propt2[i].property.identifier,propt2[i].property.category);
//    }
    cnt=0;
	for (i=0; i < totprops; i++){
		//memcpy (propt + cnt, aravis_properties + i, sizeof (struct aravis_property));
		//printf("query: prop id %d : identifier: %s\n",cnt,propt2[cnt].property.identifier);
		if(SUCCESS( propt2[cnt].query(cam, &propt2[cnt].property) ))
			cnt++;
	}


	*n_properties = cnt;

	*properties = propt2;
}
