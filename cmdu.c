#include <string.h>

#include "msg.h"
#include "cmdu.h"
#include "ieee1905_network.h"

#pragma pack(push, 1)

typedef struct
{
    unsigned char type;
    unsigned short len;
    unsigned char al_mac[ETH_ALEN];
} tlv_type_al_mac;

typedef struct
{
    unsigned char type;
    unsigned short len;
    unsigned char mac[ETH_ALEN];
} tlv_type_mac;

typedef struct
{
    unsigned char type;
    unsigned short len;
} tlv_type_end;


typedef struct
{
    unsigned char mac[ETH_ALEN];
    unsigned short media_type;
    unsigned char special_info_len;
    unsigned char special_info[0];
} local_if_dev_info;


typedef struct
{
    unsigned char type;
    unsigned short len;
    unsigned char al_mac[ETH_ALEN];
    unsigned char local_if_count;
    local_if_dev_info if_devs[0];
} tlv_type_1905_device_info;

typedef struct
{
    unsigned char type;
    unsigned short len;
    unsigned char sup_service_count;
    unsigned char sup_service;
} tlv_type_supported_service_info;

#pragma pack(pop)


int send_topology_discovery(NetworkInterface *interface)
{
    unsigned char dest_addr[ETH_ALEN] = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x13};
    char buffer[512] = { 0 };
    cmdu_raw_msg *raw_msg = buffer;
    int offset = 0;

    memcpy(raw_msg->dest_addr, dest_addr, ETH_ALEN);
    memcpy(raw_msg->src_addr, interface->addr, ETH_ALEN);
    raw_msg->proto = htons(0x893a);
    raw_msg->msg_1905.hdr.Flags_last_fragment = 1;

    offset += sizeof(cmdu_raw_msg);

    tlv_type_al_mac *al_mac = raw_msg->tlvs;
    tlv_type_mac *mac = raw_msg->tlvs + sizeof(tlv_type_al_mac);
    tlv_type_end *end = (char *)mac + sizeof(tlv_type_mac);

    al_mac->type = 0x01;
    al_mac->len = htons(6);
    memcpy(al_mac->al_mac, interface->addr, ETH_ALEN);
    offset += sizeof(tlv_type_al_mac);

    mac->type = 0x02;
    mac->len = htons(6);
    memcpy(mac->mac, interface->addr, ETH_ALEN);
    offset += sizeof(tlv_type_mac);

    end->type = 0;
    end->len = 0;
    offset += sizeof(tlv_type_end);



    return if_sock_send(interface, raw_msg, offset);
}

int send_topology_query(NetworkInterface *interface, unsigned char *dest, unsigned short msg_id)
{
    char buffer[512] = { 0 };
    cmdu_raw_msg *raw_msg = buffer;
    int offset = 0;

    memcpy(raw_msg->dest_addr, dest, ETH_ALEN);
    memcpy(raw_msg->src_addr, interface->addr, ETH_ALEN);
    raw_msg->proto = htons(0x893a);
    raw_msg->msg_1905.hdr.msg_type = htons(MSG_TOPOLOGY_QUERY);
    raw_msg->msg_1905.hdr.Flags_last_fragment = 1;
    raw_msg->msg_1905.hdr.msg_id = msg_id;

    offset += sizeof(cmdu_raw_msg);

    tlv_type_end *end = raw_msg->tlvs;

    end->type = 0;
    end->len = 0;
    offset += sizeof(tlv_type_end);
    return if_sock_send(interface, raw_msg, offset);
}

int send_topology_response(NetworkInterface *interface, unsigned char *dest, unsigned short msg_id)
{
    char buffer[512] = { 0 };
    cmdu_raw_msg *raw_msg = buffer;
    int offset = 0;

    memcpy(raw_msg->dest_addr, dest, ETH_ALEN);
    memcpy(raw_msg->src_addr, interface->addr, ETH_ALEN);
    raw_msg->proto = htons(0x893a);
    raw_msg->msg_1905.hdr.msg_type = htons(MSG_TOPOLOGY_RESPONSE);
    raw_msg->msg_1905.hdr.Flags_last_fragment = 1;
    raw_msg->msg_1905.hdr.msg_id = msg_id;

    offset += sizeof(cmdu_raw_msg);

    tlv_type_1905_device_info *dev_info = raw_msg->tlvs;
    dev_info->type = 0x03;
    dev_info->len = 0;
    memcpy(dev_info->al_mac, interface->addr, ETH_ALEN);
    dev_info->len += ETH_ALEN;
    dev_info->local_if_count = 1;
    dev_info->len += 1;
    memcpy(dev_info->if_devs[0].mac, interface->addr, ETH_ALEN);
    dev_info->if_devs[0].media_type = 0;
    dev_info->if_devs[0].special_info_len = 0;
    offset += sizeof(tlv_type_1905_device_info);
    offset += sizeof(local_if_dev_info);
    dev_info->len += sizeof(local_if_dev_info);
    dev_info->len = htons(dev_info->len);

    tlv_type_supported_service_info *sup_service = dev_info->if_devs[0].special_info;
    sup_service->type = 0x80;
    sup_service->len = htons(2);
    sup_service->sup_service_count = 1;
    sup_service->sup_service = 1; // agent
    offset += sizeof(tlv_type_supported_service_info);

    tlv_type_end *end = buffer + offset;
    end->type = 0;
    end->len = 0;
    offset += sizeof(tlv_type_end);

    return if_sock_send(interface, raw_msg, offset);
}
