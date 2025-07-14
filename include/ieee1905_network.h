#ifndef __IEEEE1905_NETWORK_H__
#define __IEEEE1905_NETWORK_H__
#include <linux/if_ether.h>
#include <net/if.h>
#include "list.h"
#include "minheap.h"

typedef struct _NetworkInterface NetworkInterface;

typedef struct _interface_ops
{
    int (*create)(NetworkInterface *interface, const char *ifname);
    void (*release)(NetworkInterface *interface);
    int (*send_msg)(NetworkInterface *interface, void *buf, int size);
    int (*recv_msg)(NetworkInterface *interface, void *buf, int size);
    int (*get_mac_addr)(NetworkInterface *interface, unsigned char *mac);
} interface_ops;


enum CmduFilterOp
{
    IF_SEND = 0x00,
    IF_RECV = 0x01,
};

enum CmduFilterHandle
{
    IF_ACCEPT = 0x00,
    IF_DROP = 0x01,
};

typedef int (*cmdu_filter_cb)(NetworkInterface *interface, int op, void *data, int len);

typedef struct cmdu_filter_s cmdu_filter_t;
struct cmdu_filter_s
{
    KamiListNode node;
    cmdu_filter_cb cb;
};


typedef struct _NetworkInterface
{
    int fd;
    unsigned char addr[ETH_ALEN];
    char ifname[IF_NAMESIZE];
    unsigned char br_addr[ETH_ALEN];
    unsigned char al_addr[ETH_ALEN];
    KamiListNode ifnode;
    KamiList nbr_1905;
    timer_entry_t *topo_timer;
    interface_ops *ops;
    void *priv_data;
    KamiList filters;
} NetworkInterface;

int if_create(NetworkInterface *interface, const char *ifname, void *priv_data);
void if_release(NetworkInterface *interface);
int if_send(NetworkInterface *interface, void *buf, int size);
int if_recv(NetworkInterface *interface, void *buf, int size);

int if_register_filter(NetworkInterface *interface, cmdu_filter_cb cb);

#endif