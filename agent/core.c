#include <stdio.h>
#include "list.h"
#include "ieee1905_network.h"
#include "core.h"
#include "cmdu.h"
#include "eloop_event.h"
#include "mh-timer.h"

KamiList interfaces = {
    .head = NULL,
    .tail = NULL,
    .size = 0,
};

void send_discovery_perodic(timer_entry_t *te)
{
    static int count = 5;
    NetworkInterface *interface = (NetworkInterface *)te->privdata;
    int ret = send_topology_discovery(interface);
    if (ret < 0)
    {
        // printf("sk send error\n");
        return;
    }

    eloop_t *loop = (eloop_t *)interface->priv_data;
    if (!count--)
    {
        reset_timer(&loop->timer, te, 45000);
    }
    else
    {
        reset_timer(&loop->timer, te, 0);
    }
    return;
}

int register_interface(NetworkInterface *interface)
{
    memset(&interface->ifnode, 0, sizeof(interface->ifnode));
    interface->ifnode.data = (void *)interface;
    KamiListAddTail(&interfaces, &interface->ifnode);

    eloop_t *loop = (eloop_t *)interface->priv_data;
    interface->topo_timer = add_timer(&loop->timer, 5000, send_discovery_perodic, (void *)interface);
    if (!interface->topo_timer)
    {
        printf("timerr error\n");
        return -1;
    }

    return 0;
}

void unregister_interface(NetworkInterface *interface)
{
    KamiListDel(&interfaces, &interface->ifnode);
    eloop_t *loop = (eloop_t *)interface->priv_data;
    del_timer(&loop->timer, interface->topo_timer);
    return;
}

NetworkInterface *get_if_by_name(char *ifname)
{
    KamiListIterrator iter;
    KamiListIterInit(&interfaces, &iter, Iter_From_Head);
    KamiListNode *tmp = NULL;
    while ((tmp = KamiListNext(&iter)) != NULL)
    {
        NetworkInterface *interface = container_of(tmp, NetworkInterface, ifnode);
        if (interface && 0 == strcmp(ifname, interface->ifname))
        {
            return interface;
        }
    }
    return NULL;
}

void add_1905_nbr(NetworkInterface *interface, nbr_1905dev *nbr)
{
    KamiListAddTail(&interface->nbr_1905, &nbr->node);
    return;
}

nbr_1905dev *search_1905_nbr(NetworkInterface *interface, const char *almac)
{
    KamiListIterrator *iter = KamiListGetIter(&interface->nbr_1905, Iter_From_Head);
    KamiListNode *tmp = NULL;
    while ((tmp = KamiListNext(iter)) != NULL)
    {
        nbr_1905dev *dev = container_of(tmp, nbr_1905dev, node);
        if (0 == memcmp(almac, dev->al_addr, ETH_ALEN))
        {
            KamiListDelIterator(iter);
            return dev;
        }
    }
    KamiListDelIterator(iter);
    return NULL;
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

static nbr_1905dev *find_controller_interface(NetworkInterface *interface)
{
    KamiListIterrator iter;
    KamiListNode *tmp = NULL;
    KamiListIterInit(&interface->nbr_1905, &iter, Iter_From_Head);
    while ((tmp = KamiListNext(&iter)) != NULL)
    {
        nbr_1905dev *dev = container_of(tmp, nbr_1905dev, node);
        if (dev->sup_service == SUP_SERVICE_CONTROLLER)
        {
            return dev;
        }
    }
    return NULL;
}

nbr_1905dev *find_controller(char *local_ifaddr, char *local_ifname, char *my_almac)
{
    KamiListIterrator iter;
    KamiListIterInit(&interfaces, &iter, Iter_From_Head);
    KamiListNode *tmp = NULL;
    while ((tmp = KamiListNext(&iter)) != NULL)
    {
        NetworkInterface *interface = container_of(tmp, NetworkInterface, ifnode);
        nbr_1905dev *dev = find_controller_interface(interface);
        if (!dev)
        {
            continue;
        }
        memcpy(local_ifaddr, interface->addr, ETH_ALEN);
        memcpy(my_almac, interface->al_addr, ETH_ALEN);
        strncpy(local_ifname, interface->ifname, IF_NAMESIZE - 1);
        return dev;
    }
    return NULL;
}