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
#include <arpa/inet.h>
#include "ieee1905_network.h"

static int linux_if_create(NetworkInterface *interface, const char *ifname);
static void linux_if_release(NetworkInterface *interface);
static int linux_if_send(NetworkInterface *interface, void *buf, int size);
static int linux_if_recv(NetworkInterface *interface, void *buf, int size);
static int linux_if_get_mac(NetworkInterface *interface, unsigned char *mac);

interface_ops if_ops = {
    .create = linux_if_create,
    .release = linux_if_release,
    .send_msg = linux_if_send,
    .recv_msg = linux_if_recv,
    .get_mac_addr = linux_if_get_mac,
};

static int attach_bpf_filters(NetworkInterface *interface)
{
    struct sock_filter zero_bytecode = BPF_STMT(BPF_RET | BPF_K, 0);
    struct sock_fprog zero = {1, &zero_bytecode};
    if (setsockopt(interface->fd, SOL_SOCKET, SO_ATTACH_FILTER, &zero, sizeof(zero)) == -1)
    {

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
    // struct sock_filter code[17] = {
    //     {0x28, 0, 0, 0x0000000c}, {0x15, 0, 8, 0x0000893a}, {0x20, 0, 0, 0x00000002},
    //     {0x15, 9, 0, 0xc2000013}, {0x15, 0, 2, 0x33445566}, // 4: replace with AL MAC Addr [2..5]
    //     {0x28, 0, 0, 0x00000000}, {0x15, 8, 9, 0x00001122}, // 6: replace with AL MAC Addr [0..1]
    //     {0x15, 0, 8, 0x99aabbcc},                           // 7: replace with IF MAC Addr [2..5]
    //     {0x28, 0, 0, 0x00000000}, {0x15, 5, 6, 0x00007788}, // 9: replace with IF MAC Addr [0..1]
    //     {0x15, 0, 5, 0x000088cc}, {0x20, 0, 0, 0x00000002}, {0x15, 0, 3, 0xc200000e},
    //     {0x28, 0, 0, 0x00000000}, {0x15, 0, 1, 0x00000180}, {0x6, 0, 0, 0x0000ffff},
    //     {0x6, 0, 0, 0x00000000},
    // };

    // unsigned char al_mac_addr_[6] = {0};

    struct sock_filter code[] = {
        {0x28, 0, 0, 0x0000000c},
        {0x15, 0, 11, 0x0000893a},
        {0x20, 0, 0, 0x00000002},
        {0x15, 0, 2, 0xc2000013},
        {0x28, 0, 0, 0x00000000},
        {0x15, 6, 7, 0x00000180},
        {0x15, 0, 2, 0x33445566}, // 6
        {0x28, 0, 0, 0x00000000},
        {0x15, 3, 4, 0x00001122}, // 8
        {0x15, 0, 3, 0x99aabbcc}, // 9
        {0x28, 0, 0, 0x00000000},
        {0x15, 0, 1, 0x00007788}, // 11
        {0x6, 0, 0, 0x00040000},
        {0x6, 0, 0, 0x00000000},
    };


    // Replace dummy values with AL MAC
    code[6].k = (((uint32_t)interface->al_addr[2]) << 24) | (((uint32_t)interface->al_addr[3]) << 16) |
                (((uint32_t)interface->al_addr[4]) << 8) | (((uint32_t)interface->al_addr[5]));
    printf("%02x:%02x:%02x:%02x:%02x:%02x\n", interface->al_addr[0], interface->al_addr[1], 
    interface->al_addr[2], interface->al_addr[3], interface->al_addr[4], interface->al_addr[5]);
    printf("%x\n", code[6].k);
    code[8].k = (((uint32_t)interface->al_addr[0]) << 8) | (((uint32_t)interface->al_addr[1]));

    // Replace dummy values with the Interface MAC
    code[9].k = (((uint32_t)interface->addr[2]) << 24) | (((uint32_t)interface->addr[3]) << 16) |
                (((uint32_t)interface->addr[4]) << 8) | (((uint32_t)interface->addr[5]));
    code[11].k = (((uint32_t)interface->addr[0]) << 8) | (((uint32_t)interface->addr[1]));

    // // Replace dummy values with AL MAC
    // code[4].k = (((uint32_t)al_mac_addr_[2]) << 24) | (((uint32_t)al_mac_addr_[3]) << 16) |
    //             (((uint32_t)al_mac_addr_[4]) << 8) | (((uint32_t)al_mac_addr_[5]) << 24);
    // code[6].k = (((uint32_t)al_mac_addr_[0]) << 8) | (((uint32_t)al_mac_addr_[1]) << 24);

    // // Replace dummy values with the Interface MAC
    // code[7].k = (((uint32_t)interface->addr[2]) << 24) | (((uint32_t)interface->addr[3]) << 16) |
    //             (((uint32_t)interface->addr[4]) << 8) | (((uint32_t)interface->addr[5]));
    // code[9].k = (((uint32_t)interface->addr[0]) << 8) | (((uint32_t)interface->addr[1]));

    // BPF filter structure
    struct sock_fprog bpf = {.len = (sizeof(code) / sizeof((code)[0])), .filter = code};

    // Attach the filter
    if (setsockopt(interface->fd, SOL_SOCKET, SO_ATTACH_FILTER, &bpf, sizeof(bpf)) == -1)
    {
        return -1;
    }
    return 0;
}

static int linux_if_create(NetworkInterface *interface, const char *ifname)
{
    int ret = 0;

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
    ret = attach_bpf_filters(interface);
    if (ret)
    {
        printf("attach error %s\n", strerror(errno));
        close(interface->fd);
        interface->fd = 0;
        return -1;
    }

    return 0;
}

int if_create(NetworkInterface *interface, const char *ifname, void *priv_data)
{
    if (!interface || !ifname || ifname[0] == '\0')
    {
        return -1;
    }
    if (interface->fd)
    {
        return -1;
    }

    interface->ops = &if_ops;

    snprintf(interface->ifname, sizeof(interface->ifname), "%s", ifname);
    interface->priv_data = priv_data;

    int ret = interface->ops->get_mac_addr(interface, interface->addr);
    if (ret < 0)
    {
        return -1;
    }

    return interface->ops->create(interface, ifname);
}

static void linux_if_release(NetworkInterface *interface)
{
    if (interface->fd)
    {
        close(interface->fd);
        return;
    }
    return;
}

void if_release(NetworkInterface *interface)
{
    if (!interface)
    {
        return;
    }

    interface->ops->release(interface);

    return;
}

static int linux_if_send(NetworkInterface *interface, void *buf, int size)
{
    if (!interface->fd)
    {
        return -1;
    }

    return send(interface->fd, buf, size, MSG_NOSIGNAL);
}

int if_send(NetworkInterface *interface, void *buf, int size)
{
    if (!interface)
    {
        return -1;
    }

    return interface->ops->send_msg(interface, buf, size);
}

static int linux_if_recv(NetworkInterface *interface, void *buf, int size)
{
    if (!interface->fd)
    {
        return -1;
    }

    return recv(interface->fd, buf, size, 0);
}

int if_recv(NetworkInterface *interface, void *buf, int size)
{
    if (!interface || !buf || size <= 0)
    {
        return -1;
    }
    return interface->ops->recv_msg(interface, buf, size);
}

static int linux_if_get_mac(NetworkInterface *interface, unsigned char *mac)
{
    int sockfd;
    struct ifreq ifr;

    // 创建UDP套接字（仅用于ioctl调用）
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        return -1;
    }

    // 设置要查询的网卡名称
    strncpy(ifr.ifr_name, interface->ifname, IF_NAMESIZE - 1);

    // 获取MAC地址
    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0)
    {
        close(sockfd);
        return -1;
    }

    // 打印MAC地址（6字节十六进制）
    memcpy(mac, ifr.ifr_ifru.ifru_hwaddr.sa_data, ETH_ALEN);

    close(sockfd);
    return 0;
}

int if_get_mac(NetworkInterface *interface, unsigned char *mac)
{
    if (!interface || !mac || interface->ifname[0] == '\0')
    {
        return -1;
    }
    return interface->ops->get_mac_addr(interface, mac);
}

int linux_sock_init()
{
    return 0;
}

void linux_sock_exit()
{
    return;
}
