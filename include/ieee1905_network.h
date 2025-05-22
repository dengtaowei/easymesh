#ifndef __IEEEE1905_NETWORK_H__
#define __IEEEE1905_NETWORK_H__
#include <linux/if_ether.h>
#include <event2/buffer.h>
#include <net/if.h>
#include "list.h"

typedef struct _NetworkInterface NetworkInterface;

typedef struct _interface_ops
{
    int (*create)(NetworkInterface *interface, const char *ifname);
    void (*release)(NetworkInterface *interface);
    int (*send_msg)(NetworkInterface *interface, void *buf, int size);
} interface_ops;

typedef struct _NetworkInterface
{
    int fd;
    unsigned char addr[ETH_ALEN];
    char ifname[IF_NAMESIZE];
    unsigned char br_addr[ETH_ALEN];
    unsigned char al_addr[ETH_ALEN];
    KamiListNode ifnode;
    KamiList nbr_1905;
    struct event_base *base;
    struct event *topo_timer;
    interface_ops *ops;
} NetworkInterface;

int if_create(NetworkInterface *interface, const char *ifname);
void if_release(NetworkInterface *interface);
int if_send(NetworkInterface *interface, void *buf, int size);

#endif