#ifndef __CORE_H__
#define __CORE_H__
#include <linux/if_ether.h>
#include "list.h"

typedef struct _nbr_1905dev
{
    unsigned char al_addr[ETH_ALEN];
    KamiListNode node;
} nbr_1905dev;

int register_interface(NetworkInterface *interface);

void unregister_interface(NetworkInterface *interface);

NetworkInterface *get_if_by_name(char *ifname);

void add_1905_nbr(NetworkInterface *interface, nbr_1905dev *nbr);

nbr_1905dev *search_1905_nbr(NetworkInterface *interface, const char *almac);

#endif