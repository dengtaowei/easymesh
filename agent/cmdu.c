#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>

#include "msg.h"
#include "cmdu.h"
#include "ieee1905_network.h"
#include "tlv_parser.h"
#include "list.h"
#include "core.h"

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
    unsigned char al_mac[ETH_ALEN];
    unsigned char local_if_count;
    local_if_dev_info if_devs[16];
} tlv_type_1905_device_info;

typedef struct
{
    unsigned char tuples_count;
    unsigned char mac_addr_count;
    unsigned char mac_addr[ETH_ALEN];
} tlv_type_dev_bridging_capa;

typedef struct
{
    unsigned char if_addr[ETH_ALEN];
    unsigned char nbr_addr[ETH_ALEN];
    unsigned char nbr_flags;
} tlv_type_1905_nbr_dev;

typedef struct
{
    unsigned char sup_service_count;
    unsigned char sup_service;
} tlv_type_supported_service_info;

#pragma pack(pop)

int send_topology_discovery(NetworkInterface *interface)
{
    unsigned char dest_addr[ETH_ALEN] = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x13};
    char buffer[512] = {0};
    cmdu_raw_msg *raw_msg = (cmdu_raw_msg *)buffer;
    int offset = 0;

    memcpy(raw_msg->dest_addr, dest_addr, ETH_ALEN);
    memcpy(raw_msg->src_addr, interface->al_addr, ETH_ALEN);
    raw_msg->proto = htons(0x893a);
    raw_msg->msg_1905.hdr.Flags_last_fragment = 1;

    offset += sizeof(cmdu_raw_msg);

    KamiTlv_S *pstRoot = Kami_Tlv_CreateObject(1);
    Kami_Tlv_AddTlvToObject(pstRoot, 0x01, ETH_ALEN, interface->al_addr, 1);
    Kami_Tlv_AddTlvToObject(pstRoot, 0x02, ETH_ALEN, interface->addr, 1);
    static unsigned char ie[] = {0x00, 0x0c, 0xe7, 0xff, 0x00, 0x0c, 0xe7, 0x00};
    Kami_Tlv_AddTlvToObject(pstRoot, 0x0b, 8, ie, 1);
    Kami_Tlv_AddTlvToObject(pstRoot, 0x00, 0, NULL, 1);

    unsigned char *pr = Kami_Tlv_Print(pstRoot);
    memcpy(buffer + offset, pr, Kami_Tlv_ObjectLength(pstRoot));
    offset += Kami_Tlv_ObjectLength(pstRoot);

    free(pr);

    Kami_Tlv_Delete(pstRoot);

    return if_send(interface, raw_msg, offset);
}

int send_topology_query(NetworkInterface *interface, unsigned char *dest, unsigned short msg_id)
{
    char buffer[512] = {0};
    cmdu_raw_msg *raw_msg = (cmdu_raw_msg *)buffer;
    int offset = 0;

    memcpy(raw_msg->dest_addr, dest, ETH_ALEN);
    memcpy(raw_msg->src_addr, interface->addr, ETH_ALEN);
    raw_msg->proto = htons(0x893a);
    raw_msg->msg_1905.hdr.msg_type = htons(MSG_TOPOLOGY_QUERY);
    raw_msg->msg_1905.hdr.Flags_last_fragment = 1;
    raw_msg->msg_1905.hdr.msg_id = msg_id;

    offset += sizeof(cmdu_raw_msg);

    tlv_type_end *end = (tlv_type_end *)raw_msg->tlvs;

    end->type = 0;
    end->len = 0;
    offset += sizeof(tlv_type_end);
    return if_send(interface, raw_msg, offset);
}

int send_topology_response(NetworkInterface *interface, unsigned char *dest, unsigned short msg_id)
{
    char buffer[512] = {0};
    cmdu_raw_msg *raw_msg = (cmdu_raw_msg *)buffer;
    int offset = 0;

    memcpy(raw_msg->dest_addr, dest, ETH_ALEN);
    memcpy(raw_msg->src_addr, interface->al_addr, ETH_ALEN);
    raw_msg->proto = htons(0x893a);
    raw_msg->msg_1905.hdr.msg_type = htons(MSG_TOPOLOGY_RESPONSE);
    raw_msg->msg_1905.hdr.Flags_last_fragment = 1;
    raw_msg->msg_1905.hdr.msg_id = msg_id;

    offset += sizeof(cmdu_raw_msg);

    KamiTlv_S *pstRoot = Kami_Tlv_CreateObject(1);
    tlv_type_1905_device_info dev_info;
    memset(&dev_info, 0, sizeof(dev_info));
    memcpy(dev_info.al_mac, interface->al_addr, ETH_ALEN);
    dev_info.local_if_count = 1;
    memcpy(dev_info.if_devs[0].mac, interface->addr, ETH_ALEN);
    dev_info.if_devs[0].media_type = 0x0000;
    dev_info.if_devs[0].special_info_len = 0;
    Kami_Tlv_AddTlvToObject(pstRoot, 0x03,
                            ETH_ALEN + 1 + sizeof(local_if_dev_info) * dev_info.local_if_count, &dev_info, 1);

    tlv_type_dev_bridging_capa brg_capa;
    memset(&brg_capa, 0, sizeof(brg_capa));
    brg_capa.tuples_count = 1;
    brg_capa.mac_addr_count = 1;
    memcpy(brg_capa.mac_addr, interface->addr, ETH_ALEN);
    Kami_Tlv_AddTlvToObject(pstRoot, 0x04, sizeof(brg_capa), &brg_capa, 1);

    tlv_type_1905_nbr_dev ngr_dev;
    memset(&ngr_dev, 0, sizeof(ngr_dev));
    memcpy(ngr_dev.if_addr, interface->addr, ETH_ALEN);
    KamiListIterrator *iter = KamiListGetIter(&interface->nbr_1905, Iter_From_Head);
    KamiListNode *tmp = NULL;
    while ((tmp = KamiListNext(iter)) != NULL)
    {
        nbr_1905dev *dev = container_of(tmp, nbr_1905dev, node);
        memcpy(ngr_dev.nbr_addr, dev->al_addr, ETH_ALEN);
    }
    KamiListDelIterator(iter);
    ngr_dev.nbr_flags = 0x80;
    Kami_Tlv_AddTlvToObject(pstRoot, 0x07, sizeof(ngr_dev), &ngr_dev, 1);

    tlv_type_supported_service_info sup_info;
    memset(&sup_info, 0, sizeof(sup_info));
    sup_info.sup_service_count = 1;
    sup_info.sup_service = 1;
    Kami_Tlv_AddTlvToObject(pstRoot, 0x80, sizeof(sup_info), &sup_info, 1);

    Kami_Tlv_AddTlvToObject(pstRoot, 0x00, 0, NULL, 1);

    unsigned char *pr = Kami_Tlv_Print(pstRoot);
    memcpy(buffer + offset, pr, Kami_Tlv_ObjectLength(pstRoot));
    offset += Kami_Tlv_ObjectLength(pstRoot);

    free(pr);

    Kami_Tlv_Delete(pstRoot);

    return if_send(interface, raw_msg, offset);
}

#include "wsc.h"

#define TLV_TYPE_RADIO_BASIC_CAPA 0x85

int send_wsc_m1(NetworkInterface *interface, unsigned char *dest, unsigned short msg_id)
{
    char buffer[512] = {0};
    cmdu_raw_msg *raw_msg = (cmdu_raw_msg *)buffer;
    int offset = 0;

    memcpy(raw_msg->dest_addr, dest, ETH_ALEN);
    memcpy(raw_msg->src_addr, interface->al_addr, ETH_ALEN);
    raw_msg->proto = htons(0x893a);
    raw_msg->msg_1905.hdr.msg_type = htons(MSG_AP_AUTOCONFIGURATION_WSC);
    raw_msg->msg_1905.hdr.Flags_last_fragment = 1;
    raw_msg->msg_1905.hdr.msg_id = msg_id;

    offset += sizeof(cmdu_raw_msg);

    KamiTlv_S *pstRoot = Kami_Tlv_CreateObject(1);

    int len = 0;
    unsigned char *pcWscVal = wsc_m1_msg_create(interface, &len);
    Kami_Tlv_AddTlvToObject(pstRoot, TLV_TYPE_WSC, len, pcWscVal, 1);

    unsigned char sup_classes[] = {00, 00, 00, 00, 01, 00, 01, 03, 0x51, 00, 00, 0x53, 00, 00, 0x54, 00, 00};
    Kami_Tlv_AddTlvToObject(pstRoot, TLV_TYPE_RADIO_BASIC_CAPA, sizeof(sup_classes), sup_classes, 1);

    Kami_Tlv_AddTlvToObject(pstRoot, 0x00, 0, NULL, 1);

    unsigned char *pr = Kami_Tlv_Print(pstRoot);
    memcpy(buffer + offset, pr, Kami_Tlv_ObjectLength(pstRoot));
    offset += Kami_Tlv_ObjectLength(pstRoot);

    free(pr);

    Kami_Tlv_Delete(pstRoot);

    return if_send(interface, raw_msg, offset);
}

#define TLV_TYPE_AP_CAPABILITY 0xa1

int send_ap_capa_report(NetworkInterface *interface, unsigned char *dest, unsigned short msg_id)
{
    char buffer[512] = {0};
    cmdu_raw_msg *raw_msg = (cmdu_raw_msg *)buffer;
    int offset = 0;

    memcpy(raw_msg->dest_addr, dest, ETH_ALEN);
    memcpy(raw_msg->src_addr, interface->al_addr, ETH_ALEN);
    raw_msg->proto = htons(0x893a);
    raw_msg->msg_1905.hdr.msg_type = htons(MSG_AP_CAPABILITY_REPORT);
    raw_msg->msg_1905.hdr.Flags_last_fragment = 1;
    raw_msg->msg_1905.hdr.msg_id = msg_id;
    offset += sizeof(cmdu_raw_msg);

    unsigned char capa[] = {0xa1, 0x00, 0x01, 0x80, 0x85, 0x00, 0x20, 0x00, 
        0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x08, 0x73, 
        0x00, 0x00, 0x74, 0x00, 0x00, 0x76, 0x00, 0x00, 
        0x77, 0x00, 0x00, 0x7d, 0x00, 0x00, 0x7e, 0x00, 
        0x00, 0x80, 0x00, 0x00, 0x81, 0x00, 0x00, 0x85, 
        0x00, 0x11, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 
        0x01, 0x03, 0x51, 0x00, 0x00, 0x53, 0x00, 0x00, 
        0x54, 0x00, 0x00, 0x86, 0x00, 0x07, 0x00, 0x00, 
        0x00, 0x00, 0x01, 0x00, 0x5c, 0x86, 0x00, 0x07, 
        0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x5e, 0x87, 
        0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 
        0xff, 0xfa, 0xff, 0xfa, 0x07, 0x00, 0x87, 0x00, 
        0x0c, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0xff, 
        0xfa, 0xff, 0xfa, 0x27, 0x40, 0x88, 0x00, 0x0d, 
        0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x04, 0xfa, 
        0xff, 0xfa, 0xff, 0x24, 0x00, 0x88, 0x00, 0x11, 
        0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x08, 0xfa, 
        0xff, 0xfa, 0xff, 0xfa, 0xff, 0xfa, 0xff, 0x25, 
        0x34, 0x00, 0x00, 0x00};

    memcpy(buffer + offset, capa, sizeof(capa));
    offset += sizeof(capa);

    return if_send(interface, raw_msg, offset);
}

int cmdu_handle(NetworkInterface *interface, void *buf, int size)
{
    cmdu_raw_msg *msg = (cmdu_raw_msg *)buf;
    unsigned short type = ntohs(msg->msg_1905.hdr.msg_type);
    printf("type %x received\n", type);
    if (type == MSG_TOPOLOGY_DISCOBERY)
    {
        printf("topo discovery received from %02x:%02x:%02x:%02x:%02x:%02x\n",
               msg->src_addr[0], msg->src_addr[1], msg->src_addr[2], msg->src_addr[3],
               msg->src_addr[4], msg->src_addr[5]);
        if (memcmp(interface->al_addr, msg->src_addr, ETH_ALEN) != 0)
        {
            if (search_1905_nbr(interface, (const char *)msg->src_addr))
            {
                printf("dev existed\n");
                return 0;
            }
            nbr_1905dev *nbr = (nbr_1905dev *)malloc(sizeof(nbr_1905dev));
            if (nbr)
            {
                memset(nbr, 0, sizeof(nbr_1905dev));
                memcpy(nbr->al_addr, msg->src_addr, ETH_ALEN);
                add_1905_nbr(interface, nbr);
                send_topology_query(interface, msg->src_addr, msg->msg_1905.hdr.msg_id);
            }
        }
    }
    else if (type == MSG_TOPOLOGY_QUERY)
    {
        printf("topo query received from %02x:%02x:%02x:%02x:%02x:%02x\n",
               msg->src_addr[0], msg->src_addr[1], msg->src_addr[2], msg->src_addr[3],
               msg->src_addr[4], msg->src_addr[5]);
        if (memcmp(interface->al_addr, msg->src_addr, ETH_ALEN) != 0)
        {
            send_topology_response(interface, msg->src_addr, msg->msg_1905.hdr.msg_id);
        }
    }
    else if (type == MSG_AP_AUTOCONFIGURATION_RENEW)
    {

        printf("auto configuration renew received from %02x:%02x:%02x:%02x:%02x:%02x\n",
               msg->src_addr[0], msg->src_addr[1], msg->src_addr[2], msg->src_addr[3],
               msg->src_addr[4], msg->src_addr[5]);
        KamiTlv_S *pstRenew = Kami_Tlv_ParseObject(msg->tlvs, size - sizeof(cmdu_raw_msg), 1);
        if (pstRenew)
        {
            KamiTlv_S *pstFreqBand = Kami_Tlv_GetObjectItem(pstRenew, 0x10);
            if (pstFreqBand)
            {
                printf("radio %d\n", *((char *)pstFreqBand->value));
                if (*((char *)pstFreqBand->value) == 1)
                {
                    return 0;
                }
            }
            int i;
            i++;
        }
        if (memcmp(interface->al_addr, msg->src_addr, ETH_ALEN) != 0)
        {
            send_wsc_m1(interface, msg->src_addr, msg->msg_1905.hdr.msg_id);
        }
    }
    else if (type == MSG_AP_CAPABILITY_QUERY)
    {
        printf("ap capability query received from %02x:%02x:%02x:%02x:%02x:%02x\n",
               msg->src_addr[0], msg->src_addr[1], msg->src_addr[2], msg->src_addr[3],
               msg->src_addr[4], msg->src_addr[5]);
        if (memcmp(interface->al_addr, msg->src_addr, ETH_ALEN) != 0)
        {
            send_ap_capa_report(interface, msg->src_addr, msg->msg_1905.hdr.msg_id);
        }
    }

    return 0;
}
