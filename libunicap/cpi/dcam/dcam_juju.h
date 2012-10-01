#ifndef __DCAM_JUJU_H__
#define __DCAM_JUJU_H__

typedef struct dcam_juju_buffer dcam_juju_buffer_t;

struct dcam_juju_buffer
{
	size_t size;
	struct fw_cdev_iso_packet *packets;
};


int dcam_juju_probe (dcam_handle_t dcamhandle);
unicap_status_t __HIDDEN__
dcam_juju_setup (dcam_handle_t dcamhandle);
unicap_status_t
dcam_juju_shutdown (dcam_handle_t dcamhandle);
unicap_status_t
dcam_juju_prepare_iso (dcam_handle_t dcamhandle);
unicap_status_t
dcam_juju_start_iso (dcam_handle_t dcamhandle);
unicap_status_t
dcam_juju_queue_buffer (dcam_handle_t dcamhandle, int index);
unicap_status_t
dcam_juju_dequeue_buffer (dcam_handle_t dcamhandle,
			  int *index, 
			  uint32_t *cycle_time,
			  int *packet_discont);
void *dcam_juju_capture_thread( void *arg );




#endif//__DCAM_JUJU_H__
