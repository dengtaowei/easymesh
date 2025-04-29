#ifndef __CMDU_H__
#define __CMDU_H__
#include "msg.h"
#include "ieee1905_network.h"
int send_topology_discovery(NetworkInterface *interface);
int send_topology_query(NetworkInterface *interface, unsigned char *dest, unsigned short msg_id);
int send_topology_response(NetworkInterface *interface, unsigned char *dest, unsigned short msg_id);
#endif