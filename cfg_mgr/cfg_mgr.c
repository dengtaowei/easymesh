#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include "eloop_event.h"
#include "list.h"
#include "bus_msg.h"
#include "mh-timer.h"
#include "msg.h"
#include "cmdu.h"
#include "tlv_parser.h"
#include "wsc.h"

static const char *g_mesh_user_name = "cfgmgr";
static const char *g_mesh_group_name = "gmesh";

static void _cfgmgr_handle_heartbeat(msg_t *msg, io_buf_t *io)
{
    printf("heart beat pong\n");
}

#define TLV_TYPE_RADIO_BASIC_CAPA 0x85
enum SupportedFreqBand
{
    SUP_FREQ_BAND_24G = 0x00,
    SUP_FREQ_BAND_5G = 0x01,
};

void cfgmgr_send_wsc_m1(io_buf_t *io, char *al_addr, char *if_addr, char *ifname, char *dest, int msg_id, int band)
{
    char buffer[512] = {0};
    cmdu_raw_msg *raw_msg = (cmdu_raw_msg *)buffer;
    int offset = 0;

    memcpy(raw_msg->dest_addr, dest, ETH_ALEN);
    memcpy(raw_msg->src_addr, al_addr, ETH_ALEN);
    raw_msg->proto = htons(0x893a);
    raw_msg->msg_1905.hdr.msg_type = htons(MSG_AP_AUTOCONFIGURATION_WSC);
    raw_msg->msg_1905.hdr.Flags_last_fragment = 1;
    raw_msg->msg_1905.hdr.msg_id = msg_id;

    offset += sizeof(cmdu_raw_msg);

    KamiTlv_S *pstRoot = Kami_Tlv_CreateObject(1);

    int len = 0;
    unsigned char *pcWscVal = NULL;
    if (band == SUP_FREQ_BAND_24G)
    {
        pcWscVal = wsc_m1_msg_create(if_addr, &len, 1);
    }
    else if (band == SUP_FREQ_BAND_5G)
    {
        pcWscVal = wsc_m1_msg_create(if_addr, &len, 2);
    }
    Kami_Tlv_AddTlvToObject(pstRoot, TLV_TYPE_WSC, len, pcWscVal, 1);
    

    unsigned char sup_classes[] = {00, 00, 00, 00, 01, 00, 01, 03, 0x51, 00, 00, 0x53, 00, 00, 0x54, 00, 00};
    Kami_Tlv_AddTlvToObject(pstRoot, TLV_TYPE_RADIO_BASIC_CAPA, sizeof(sup_classes), sup_classes, 1);

    Kami_Tlv_AddTlvToObject(pstRoot, 0x00, 0, NULL, 1);

    unsigned char *pr = Kami_Tlv_Print(pstRoot);
    memcpy(buffer + offset, pr, Kami_Tlv_ObjectLength(pstRoot));
    offset += Kami_Tlv_ObjectLength(pstRoot);

    free(pr);

    Kami_Tlv_Delete(pstRoot);

    mesh_packet_t *pkt = packet_create((char *)raw_msg, offset);
    if (!pkt)
    {
        return;
    }
    pkt->to = PKT_NETWORK;
    pkt->from = PKT_LOCAL;
    snprintf(pkt->ifname, sizeof(pkt->ifname), "%s", ifname);
    memcpy(pkt->aladdr, al_addr, ETH_ALEN);
    memcpy(pkt->ifaddr, if_addr, ETH_ALEN);
    mbus_sendmsg_in_group(io, g_mesh_group_name, (char *)pkt, packet_length(pkt));
    packet_release(pkt);
    // return if_send(interface, raw_msg, offset);
}

int cfgmgr_parse_autoconfiguration_renew(cmdu_raw_msg *msg, int len)
{
    int ret = -1;
    KamiTlv_S *pstRenew = Kami_Tlv_ParseObject(msg->tlvs, len - sizeof(cmdu_raw_msg), 1);
    if (pstRenew)
    {
        KamiTlv_S *pstFreqBand = Kami_Tlv_GetObjectItem(pstRenew, 0x10);
        if (pstFreqBand)
        {
            printf("radio %d\n", *((char *)pstFreqBand->value));
            if (*((char *)pstFreqBand->value) == SUP_FREQ_BAND_5G)  // 5G
            {
                ret = SUP_FREQ_BAND_5G;
            }
            else if (*((char *)pstFreqBand->value) == SUP_FREQ_BAND_24G)
            {
                ret = SUP_FREQ_BAND_24G;
            }
        }
    }
    return ret;
}

void cfgmgr_handle_autoconfiguration_renew(io_buf_t *io, mesh_packet_t *pkt)
{
    cmdu_raw_msg *msg = (cmdu_raw_msg *)pkt->packet;
    int freq_band = cfgmgr_parse_autoconfiguration_renew(msg, pkt->packet_len);
    if (freq_band < 0)
    {
        return;
    }
    cfgmgr_send_wsc_m1(io, pkt->aladdr, pkt->ifaddr, pkt->ifname, (char *)msg->src_addr, msg->msg_1905.hdr.msg_id, freq_band);
}

void cfgmgr_handle_packet(io_buf_t *io, mesh_packet_t *pkt)
{
    printf("from: %s, to: %s, ifname: %s, len: %d\n",
           packet_type(pkt->from), packet_type(pkt->to), pkt->ifname, pkt->packet_len);
    
    cmdu_raw_msg *msg = (cmdu_raw_msg *)pkt->packet;
    unsigned short type = ntohs(msg->msg_1905.hdr.msg_type);

    if (pkt->from == PKT_LOCAL && type == MSG_AP_AUTOCONFIGURATION_WSC)
    {
        printf("m1 pkt from local, drop it\n");
        return;
    }

    switch (type)
    {
    case MSG_AP_AUTOCONFIGURATION_RENEW:
    {
        cfgmgr_handle_autoconfiguration_renew(io, pkt);
        break;
    }
    default:
        break;
    }
}

static void _cfgmgr_handle_group_msg(msg_t *msg, io_buf_t *io)
{
    group_msg_t *group_msg = (group_msg_t *)msg->body;
    printf("group msg from group %s, msg: %s, len: %d\n",
           group_msg->group_name, group_msg->group_msg, group_msg->group_msg_len);
    if (0 == strcmp(group_msg->group_name, g_mesh_group_name))
    {
        cfgmgr_handle_packet(io, (mesh_packet_t *)group_msg->group_msg);
    }
}

void cfgmgr_handle_msg(msg_t *msg, io_buf_t *io)
{
    switch (msg_get_command_id(msg))
    {
    case CID_OTHER_HEARTBEAT:
        _cfgmgr_handle_heartbeat(msg, io);
        break;
    case CID_GROUP_MSG:
        _cfgmgr_handle_group_msg(msg, io);
        break;
    default:
        break;
    }
}

void cfgmgr_handle_read(io_buf_t *io)
{
    msg_t *msg = (msg_t *)io_data(io);
    cfgmgr_handle_msg(msg, io);
}

void cfgmgr_regmsg_timer_cb(timer_entry_t *te)
{
    io_buf_t *io = (io_buf_t *)te->privdata;

    // login
    mbus_register_object(io, g_mesh_user_name);
    mbus_create_group(io, g_mesh_group_name);
    mbus_join_group(io, g_mesh_group_name);

    // reset_timer(&io->loop->timer, te, 30);
    return;
}

int main(int argc, char *argv[])
{
    eloop_t *loop = eloop_create();
    int sockfd = eloop_tcp_connect(NULL, "127.0.0.1", "8848");
    if (sockfd < 0)
    {
        printf("connect error %s\n", strerror(errno));
        return -1;
    }
    int ret = fcntl(sockfd, F_SETFL, O_NONBLOCK | fcntl(sockfd, F_GETFL));
    if (ret < 0)
    {
        printf("set non block err\n");
        return -1;
    }

    eloop_set_event(&loop->ios[sockfd], sockfd, NULL);
    eloop_add_event(&loop->ios[sockfd], EPOLLIN);
    io_add_packer(&loop->ios[sockfd], sizeof(msg_t), 0);
    eloop_setcb_read(&loop->ios[sockfd], cfgmgr_handle_read);
    add_timer(&loop->timer, 1, cfgmgr_regmsg_timer_cb, &loop->ios[sockfd]);
    eloop_run(loop);
    return 0;
}