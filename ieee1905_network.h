#ifndef __IEEEE1905_NETWORK_H__
#define __IEEEE1905_NETWORK_H__
#include <linux/if_ether.h>
#include <net/if.h>

typedef struct _NetworkInterface
{
    int fd;
    unsigned char addr[ETH_ALEN];
    char ifname[IF_NAMESIZE];
    unsigned char br_addr[ETH_ALEN];
    unsigned char al_addr[ETH_ALEN];

} NetworkInterface;

int if_sock_create(NetworkInterface *interface);
void if_sock_destroy(NetworkInterface *interface);
int if_sock_send(NetworkInterface *interface, void *buf, int size);

#endif