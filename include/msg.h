#ifndef __MSG_H__
#define __MSG_H__
#include <linux/if_ether.h>
#include <net/if.h>


enum MessageType
{
    MSG_TOPOLOGY_DISCOBERY = 0x0000,
    MSG_TOPOLOGY_QUERY = 0x0002,
    MSG_TOPOLOGY_RESPONSE = 0x0003,
    MSG_AP_AUTOCONFIGURATION_SEARCH = 0X0007,
    MSG_AP_AUTOCONFIGURATION_RESPONSE = 0X0008,
    MSG_AP_AUTOCONFIGURATION_WSC = 0x0009,
    MSG_AP_AUTOCONFIGURATION_RENEW = 0x000a,
    MSG_AP_CAPABILITY_QUERY = 0x8001,
    MSG_AP_CAPABILITY_REPORT = 0x8002,
};


typedef struct _cmdu_hdr
{
    unsigned char msg_version;
    unsigned char msg_reserved;
    unsigned short msg_type;
    unsigned short msg_id;
    unsigned char fragment_id;
    unsigned char Flags_reserved : 6;
    unsigned char Flags_relay : 1;
    unsigned char Flags_last_fragment : 1;
} cmdu_dhr;

typedef struct _ieee1905_msg
{
    cmdu_dhr hdr;
} ieee1905_msg;

typedef struct _cmdu_raw_msg
{
    unsigned char dest_addr[ETH_ALEN];
    unsigned char src_addr[ETH_ALEN];
    unsigned short proto;
    ieee1905_msg msg_1905;
    unsigned char tlvs[0];
} cmdu_raw_msg;

typedef struct mesh_packet_s mesh_packet_t;

enum MeshPktType
{
    PKT_NONE = 0,
    PKT_NETWORK = 1,
    PKT_LOCAL = 2,
    PKT_MAX = 3
};
struct mesh_packet_s
{
    int from;
    int to;
    char ifname[IF_NAMESIZE];
    char ifaddr[ETH_ALEN];
    char aladdr[ETH_ALEN];
    int packet_len;
    char packet[0];
};

#endif