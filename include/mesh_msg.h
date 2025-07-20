#ifndef __MESH_MSG_H__
#define __MESH_MSG_H__
#include <linux/if_ether.h>
#include <net/if.h>

typedef struct mesh_msg_s mesh_msg_t;
typedef struct controller_info_s controller_info_t;

enum MeshMsgCmd
{
    MESH_MSGCMD_CONTROLLER_INFO_REQ,
    MESH_MSGCMD_CONTROLLER_INFO_RSP,
};


struct controller_info_s
{
    char controller_al_mac[ETH_ALEN];
    char my_al_mac[ETH_ALEN];
    char local_ifaddr[ETH_ALEN];
    char local_ifname[IF_NAMESIZE];
};


struct mesh_msg_s
{
    unsigned short mesh_msg_id;
    char mesh_msg_body[0];
};


#endif