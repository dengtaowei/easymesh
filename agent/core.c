//
#include <event2/buffer.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include "list.h"
#include "ieee1905_network.h"
#include "core.h"
#include "cmdu.h"

KamiList interfaces = {
    .head = NULL,
    .tail = NULL,
    .size = 0,
};

void send_discovery_perodic(int sockfd, short what, void *arg)
{
    NetworkInterface *interface = (NetworkInterface *)arg;
    int ret = send_topology_discovery(interface);
    if (ret < 0)
    {
        // printf("sk send error\n");
        return;
    }
    struct timeval t1 = {45, 0}; // 1秒0毫秒
    if (!evtimer_pending(interface->topo_timer, &t1))
    {
        evtimer_del(interface->topo_timer);
        evtimer_add(interface->topo_timer, &t1);
    }
    return;
}

int register_interface(NetworkInterface *interface)
{
    memset(&interface->ifnode, 0, sizeof(interface->ifnode));
    interface->ifnode.data = (void *)interface;
    KamiListAddTail(&interfaces, &interface->ifnode);

    // 定时器，非持久事件
    interface->topo_timer = evtimer_new((struct event_base *)interface->priv_data,
                                        send_discovery_perodic, interface);
    if (!interface->topo_timer)
    {
        printf("timer error\n");
        return 1;
    }
    struct timeval t1 = {1, 0};              // 1秒0毫秒
    evtimer_add(interface->topo_timer, &t1); // 插入性能 O(logn)

    return 0;
}

void unregister_interface(NetworkInterface *interface)
{
    KamiListDel(&interfaces, &interface->ifnode);
    evtimer_del(interface->topo_timer);
    return;
}

void add_1905_nbr(NetworkInterface *interface, nbr_1905dev *nbr)
{
    KamiListAddTail(&interface->nbr_1905, &nbr->node);
    return;
}

void del_1905_nbr(NetworkInterface *interface, unsigned char *almac)
{
    KamiListIterrator *iter = KamiListGetIter(&interface->nbr_1905, Iter_From_Head);
    KamiListNode *tmp = NULL;
    while ((tmp = KamiListNext(iter)) != NULL)
    {
        nbr_1905dev *dev = container_of(tmp, nbr_1905dev, node);
        if (0 == memcmp(almac, dev->al_addr, ETH_ALEN))
        {
            KamiListDel(&interface->nbr_1905, &dev->node);
            free(dev);
        }
    }
    KamiListDelIterator(iter);
    return;
}