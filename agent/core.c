//
#include "list.h"
#include "ieee1905_network.h"
#include "core.h"

KamiList interfaces = {
    .head = NULL,
    .tail = NULL,
    .size = 0,
};

int register_interface(NetworkInterface *interface)
{
    memset(&interface->ifnode, 0, sizeof(interface->ifnode));
    interface->ifnode.data = (void *)interface;
    KamiListAddTail(&interfaces, &interface->ifnode);
    return 0;
}

void unregister_interface(NetworkInterface *interface)
{
    KamiListDel(&interfaces, &interface->ifnode);
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
    while((tmp = KamiListNext(iter)) != NULL)
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