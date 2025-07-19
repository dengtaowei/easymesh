#ifndef __CMDU_H__
#define __CMDU_H__
#include "msg.h"
#include "ieee1905_network.h"
int send_topology_discovery(NetworkInterface *interface);
int send_topology_query(NetworkInterface *interface, unsigned char *dest, unsigned short msg_id);
int send_topology_response(NetworkInterface *interface, unsigned char *dest, unsigned short msg_id);
int send_wsc_m1(NetworkInterface *interface, unsigned char *dest, unsigned short msg_id);
int send_ap_capa_report(NetworkInterface *interface, unsigned char *dest, unsigned short msg_id);
int cmdu_handle(NetworkInterface *interface, void *buf, int size);
mesh_packet_t *packet_create(char *data, int len);
void packet_release(mesh_packet_t *packet);
int packet_length(mesh_packet_t *packet);
const char *packet_type(int type);
#endif