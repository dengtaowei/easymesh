#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if_ether.h>

#include "util.h"

int get_if_mac(const char *ifname, unsigned char *mac)
{
    int sockfd;
    struct ifreq ifr;

    // 创建UDP套接字（仅用于ioctl调用）
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
       return -1;
    }

    // 设置要查询的网卡名称
    strncpy(ifr.ifr_name, ifname, IF_NAMESIZE - 1);

    // 获取MAC地址
    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
        close(sockfd);
        return -1;
    }

    // 打印MAC地址（6字节十六进制）
    memcpy(mac, ifr.ifr_ifru.ifru_hwaddr.sa_data, ETH_ALEN);

    close(sockfd);
    return 0;
}