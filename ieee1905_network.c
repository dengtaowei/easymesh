#include <arpa/inet.h>
#include <linux/filter.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <netpacket/packet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <linux/filter.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <netpacket/packet.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <unistd.h>
#include <stdint.h>
#include "ieee1905_network.h"


static int attach_bpf_filters(NetworkInterface *interface)
{
    struct sock_filter zero_bytecode = BPF_STMT(BPF_RET | BPF_K, 0);
    struct sock_fprog zero           = {1, &zero_bytecode};
    if (setsockopt(interface->fd, SOL_SOCKET, SO_ATTACH_FILTER, &zero, sizeof(zero)) == -1) {
        
        return -1;
    }
    char drain[1];
    // here we choose to ignore the error code such as EINTR
    while (recv(interface->fd, &drain, sizeof(drain), MSG_DONTWAIT) >= 0)
        ;
    // This BPF is designed to accepts the following packets:
    // - IEEE1905 multicast packets (with IEEE1905 Multicast Address [01:80:c2:00:00:13] set as destination address)
    // - LLDP multicast packets (with LLDP Multicast Address as destination address)
    // - IEEE1905 unicast packets (with either this devices' AL MAC address or the interface's HW address set as destination address)
    //
    // BPF template is generated using the following command:
    // tcpdump -dd '(ether proto 0x893a and (ether dst 01:80:c2:00:00:13 or ether dst 11:22:33:44:55:66 or ether dst 77:88:99:aa:bb:cc)) or (ether proto 0x88cc and ether dst 01:80:c2:00:00:0e)'
    //
    // The two dummy addresses in this filter 11:22... and 77:88... will be replaced in runtime with the AL MAC address and the interface's HW address
    struct sock_filter code[17] = {
        {0x28, 0, 0, 0x0000000c}, {0x15, 0, 8, 0x0000893a}, {0x20, 0, 0, 0x00000002},
        {0x15, 9, 0, 0xc2000013}, {0x15, 0, 2, 0x33445566}, // 4: replace with AL MAC Addr [2..5]
        {0x28, 0, 0, 0x00000000}, {0x15, 8, 9, 0x00001122}, // 6: replace with AL MAC Addr [0..1]
        {0x15, 0, 8, 0x99aabbcc},                           // 7: replace with IF MAC Addr [2..5]
        {0x28, 0, 0, 0x00000000}, {0x15, 5, 6, 0x00007788}, // 9: replace with IF MAC Addr [0..1]
        {0x15, 0, 5, 0x000088cc}, {0x20, 0, 0, 0x00000002}, {0x15, 0, 3, 0xc200000e},
        {0x28, 0, 0, 0x00000000}, {0x15, 0, 1, 0x00000180}, {0x6, 0, 0, 0x0000ffff},
        {0x6, 0, 0, 0x00000000},
    };

    unsigned char al_mac_addr_[6] = { 0 };

    (((uint32_t)al_mac_addr_[2]) << 24);

    // Replace dummy values with AL MAC
    code[4].k = (((uint32_t)al_mac_addr_[2]) << 24) | (((uint32_t)al_mac_addr_[3]) << 16) |
                (((uint32_t)al_mac_addr_[4]) << 8) | (((uint32_t)al_mac_addr_[5]) << 24);
    code[6].k = (((uint32_t)al_mac_addr_[0]) << 8) | (((uint32_t)al_mac_addr_[1]) << 24);

    // Replace dummy values with the Interface MAC
    code[7].k = (((uint32_t)interface->addr[2]) << 24) | (((uint32_t)interface->addr[3]) << 16) |
                (((uint32_t)interface->addr[4]) << 8) | (((uint32_t)interface->addr[5]));
    code[9].k = (((uint32_t)interface->addr[0]) << 8) | (((uint32_t)interface->addr[1]));

    // BPF filter structure
    struct sock_fprog bpf = {.len = (sizeof(code) / sizeof((code)[0])), .filter = code};

    // Attach the filter
    if (setsockopt(interface->fd, SOL_SOCKET, SO_ATTACH_FILTER, &bpf, sizeof(bpf)) == -1) {
        return -1;
    }
    return 0;
}


int if_sock_create(NetworkInterface *interface)
{
    if (!interface)
    {
        return -1;
    }
    if (interface->fd)
    {
        return -1;
    }
    if (interface->ifname[0] == '\0')
    {
        return -1;
    }

    int sk = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sk < 0)
    {
        printf("sock error %s\n", strerror(errno));
        return -1;
    }

    struct sockaddr_ll addr;
    memset(&addr, 0, sizeof(addr));
    addr.sll_family = AF_PACKET;
    addr.sll_protocol = htons(ETH_P_ALL);
    addr.sll_ifindex = if_nametoindex(interface->ifname);
    if (bind(sk, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        printf("bind error %s\n", strerror(errno));
        close(sk);
        return -1;
    }
    interface->fd = sk;
    int ret = attach_bpf_filters(interface);
    if (ret)
    {
        printf("attach error %s\n", strerror(errno));
        close(interface->fd);
        interface->fd = 0;
        return -1;
    }
    
    return 0;
}

void if_sock_destroy(NetworkInterface *interface)
{
    if (!interface)
    {
        return;
    }
    if (interface->fd)
    {
        close(interface->fd);
        return;
    }
    return;
}

int if_sock_send(NetworkInterface *interface, void *buf, int size)
{
    if (!interface)
    {
        return -1;
    }
    if (!interface->fd)
    {
        return -1;
    }

    return send(interface->fd, buf, size, 0);
}
