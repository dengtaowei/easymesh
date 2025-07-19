#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include "eloop_event.h"
#include "list.h"
#include "bus_msg.h"
#include "mh-timer.h"
#include "msg.h"
#include "cmdu.h"
#include "tlv_parser.h"
#include "wsc.h"

typedef struct cfg_mgr_s cfg_mgr_t;

enum ConfigStatus
{
    UNCONFIGURED = 0,
    BACKHAUL_READY,
    SEND_M1,
    WAIT_M2,
    CONFIGURED
};

typedef struct radio_s radio_t;
struct radio_s
{
    unsigned char auto_conf_band;
    int conf_state;
};

typedef struct
{
    unsigned char *buffer;
    size_t length;
    size_t offset;
} m2buf_t;
struct cfg_mgr_s
{
    char bh_local_ifname[IF_NAMESIZE];
    char bh_local_ifaddr[ETH_ALEN];
    char bh_local_almac[ETH_ALEN];
    char bh_almac[ETH_ALEN];
    m2buf_t m2buf;
    io_buf_t *io;
    radio_t radios[2];
    unsigned short msgid;
    int nradio;
};

static const char *g_mesh_user_name = "cfgmgr";
static const char *g_mesh_group_name = "gmesh";

int cfg_mgr_init(cfg_mgr_t *cfgmgr)
{
    if (!cfgmgr)
    {
        return -1;
    }
    memset(cfgmgr, 0, sizeof(cfg_mgr_t));
    cfgmgr->m2buf.buffer = (unsigned char *)malloc(2048);
    cfgmgr->m2buf.length = 2048;
    cfgmgr->nradio = 2;
    cfgmgr->radios[0].auto_conf_band = 0;        // 2.4 G
    cfgmgr->radios[0].conf_state = UNCONFIGURED; // 2.4 G
    cfgmgr->radios[1].auto_conf_band = 1;        // 5 G
    cfgmgr->radios[1].conf_state = UNCONFIGURED; // 5 G
    return 0;
}

void cfg_mgr_destroy(cfg_mgr_t *cfgmgr)
{
    if (cfgmgr && cfgmgr->m2buf.buffer)
    {
        free(cfgmgr->m2buf.buffer);
    }
}

static unsigned char *cfg_mgr_ensure_m2buf(m2buf_t *const p, size_t needed)
{
    unsigned char *newbuffer = NULL;
    size_t newsize = 0;

    if ((p == NULL) || (p->buffer == NULL))
    {
        return NULL;
    }

    if ((p->length > 0) && (p->offset >= p->length))
    {
        return NULL;
    }

    if (needed > INT_MAX)
    {
        return NULL;
    }

    needed += p->offset + 1;
    if (needed <= p->length)
    {
        return p->buffer + p->offset;
    }

    if (needed > (INT_MAX / 2))
    {
        if (needed <= INT_MAX)
        {
            newsize = INT_MAX;
        }
        else
        {
            return NULL;
        }
    }
    else
    {
        newsize = needed * 2;
    }

    newbuffer = (unsigned char *)realloc(p->buffer, newsize);
    if (newbuffer == NULL)
    {
        free(p->buffer);
        p->length = 0;
        p->buffer = NULL;

        return NULL;
    }

    p->length = newsize;
    p->buffer = newbuffer;

    return newbuffer + p->offset;
}

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

void cfgmgr_send_wsc_m1(cfg_mgr_t *cfgmgr, char *al_addr, char *if_addr, char *ifname, char *dest, int msg_id, int band)
{
    char buffer[512] = {0};
    cmdu_raw_msg *raw_msg = (cmdu_raw_msg *)buffer;
    int offset = 0;

    memcpy(raw_msg->dest_addr, dest, ETH_ALEN);
    memcpy(raw_msg->src_addr, al_addr, ETH_ALEN);
    raw_msg->proto = htons(0x893a);
    raw_msg->msg_1905.hdr.msg_type = htons(MSG_AP_AUTOCONFIGURATION_WSC);
    raw_msg->msg_1905.hdr.Flags_last_fragment = 1;
    if (msg_id > 0)
    {
        raw_msg->msg_1905.hdr.msg_id = msg_id;
    }
    else
    {
        raw_msg->msg_1905.hdr.msg_id = htons(cfgmgr->msgid + 1);
    }

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
    mbus_sendmsg_in_group(cfgmgr->io, g_mesh_group_name, (char *)pkt, packet_length(pkt));
    packet_release(pkt);
    // return if_send(interface, raw_msg, offset);
}

int cfgmgr_parse_autoconfiguration_renew(cmdu_raw_msg *msg, int len)
{
    int ret = -1;
    KamiTlv_S *pstRenew = Kami_Tlv_ParseObject(msg->tlvs, len - sizeof(cmdu_raw_msg), 1);
    if (pstRenew)
    {
        KamiTlv_S *pstFreqBand = Kami_Tlv_GetObjectItem(pstRenew, 0x10, 0);
        if (pstFreqBand)
        {
            printf("radio %d\n", *((char *)pstFreqBand->value));
            if (*((char *)pstFreqBand->value) == SUP_FREQ_BAND_5G) // 5G
            {
                ret = SUP_FREQ_BAND_5G;
            }
            else if (*((char *)pstFreqBand->value) == SUP_FREQ_BAND_24G)
            {
                ret = SUP_FREQ_BAND_24G;
            }
        }
        Kami_Tlv_Delete(pstRenew);
    }
    return ret;
}

int cfgmgr_parse_autoconfiguration_response(cmdu_raw_msg *msg, int len)
{
    int ret = -1;
    KamiTlv_S *pstRenew = Kami_Tlv_ParseObject(msg->tlvs, len - sizeof(cmdu_raw_msg), 1);
    if (pstRenew)
    {
        KamiTlv_S *pstFreqBand = Kami_Tlv_GetObjectItem(pstRenew, 0x10, 0);
        if (pstFreqBand)
        {
            printf("radio %d\n", *((char *)pstFreqBand->value));
            if (*((char *)pstFreqBand->value) == SUP_FREQ_BAND_5G) // 5G
            {
                ret = SUP_FREQ_BAND_5G;
            }
            else if (*((char *)pstFreqBand->value) == SUP_FREQ_BAND_24G)
            {
                ret = SUP_FREQ_BAND_24G;
            }
        }
        Kami_Tlv_Delete(pstRenew);
    }
    return ret;
}

void cfgmgr_handle_autoconfiguration_renew(cfg_mgr_t *cfgmgr, mesh_packet_t *pkt)
{
    cmdu_raw_msg *msg = (cmdu_raw_msg *)pkt->packet;
    int freq_band = cfgmgr_parse_autoconfiguration_renew(msg, pkt->packet_len);
    if (freq_band < 0)
    {
        return;
    }
    cfgmgr_send_wsc_m1(cfgmgr, pkt->aladdr, pkt->ifaddr, pkt->ifname, (char *)msg->src_addr, msg->msg_1905.hdr.msg_id + 1, freq_band);
}

void cfgmgr_handle_autoconfiguration_response(cfg_mgr_t *cfgmgr, mesh_packet_t *pkt)
{
    cmdu_raw_msg *msg = (cmdu_raw_msg *)pkt->packet;
    int freq_band = cfgmgr_parse_autoconfiguration_response(msg, pkt->packet_len);
    if (freq_band < 0)
    {
        return;
    }
    cfgmgr_send_wsc_m1(cfgmgr, pkt->aladdr, pkt->ifaddr, pkt->ifname, (char *)msg->src_addr, 0, freq_band);
}

int cfgmgr_parse_autoconfiguration_wsc(cfg_mgr_t *cfgmgr, cmdu_raw_msg *msg, int len)
{
    int ret = -1;
    KamiTlv_S *pstM2 = Kami_Tlv_ParseObject(msg->tlvs, len - sizeof(cmdu_raw_msg), 1);
    if (pstM2)
    {
        for (int i = 0;; i++)
        {
            KamiTlv_S *pstWsc = Kami_Tlv_GetObjectItem(pstM2, 0x11, i);
            if (!pstWsc)
            {
                break;
            }
            KamiTlv_S *pstWscObj = Kami_Tlv_ParseObject(pstWsc->value, pstWsc->length, 2);
            if (!pstWscObj)
            {
                break;
            }
            KamiTlv_S *pstRfBands = Kami_Tlv_GetObjectItem(pstWscObj, 0x103c, 0);
            if (pstRfBands)
            {
                if (*((char *)pstRfBands->value) == 0x02) // 5G
                {
                    cfgmgr->radios[1].conf_state = CONFIGURED;
                }
                else if (*((char *)pstRfBands->value) == 0x01) // 2.4G
                {
                    cfgmgr->radios[0].conf_state = CONFIGURED;
                }
            }
            printf("received m2 wsc config %d, len=%d\n", i, pstWsc->length);
            Kami_Tlv_Delete(pstWscObj);
        }
        Kami_Tlv_Delete(pstM2);
    }
    return ret;
}

void cfgmgr_handle_autoconfiguration_wsc(cfg_mgr_t *cfgmgr, mesh_packet_t *pkt)
{
    cmdu_raw_msg *msg = (cmdu_raw_msg *)pkt->packet;
    if (!msg->msg_1905.hdr.Flags_last_fragment)
    {
        printf("this is not the last fragment pkt len: %d\n", pkt->packet_len);
        if (cfgmgr->m2buf.offset <= 0)
        {
            // no header in buffer
            unsigned char *pos = cfg_mgr_ensure_m2buf(&cfgmgr->m2buf, pkt->packet_len);
            if (!pos)
            {
                return;
            }
            memcpy(pos, pkt->packet, pkt->packet_len);
            cfgmgr->m2buf.offset += pkt->packet_len;
            // hex_dump("pkt", pkt->packet, pkt->packet_len);
            // hex_dump("buf", cfgmgr->m2buf.buffer, cfgmgr->m2buf.offset);
        }
        else
        {
            unsigned char *pos = cfg_mgr_ensure_m2buf(&cfgmgr->m2buf, pkt->packet_len - sizeof(cmdu_raw_msg));
            if (!pos)
            {
                return;
            }
            memcpy(pos, msg->tlvs, pkt->packet_len - sizeof(cmdu_raw_msg));
            cfgmgr->m2buf.offset += (pkt->packet_len - sizeof(cmdu_raw_msg));
        }
        return;
    }
    unsigned char *pos = cfg_mgr_ensure_m2buf(&cfgmgr->m2buf, pkt->packet_len - sizeof(cmdu_raw_msg));
    if (!pos)
    {
        return;
    }
    memcpy(pos, msg->tlvs, pkt->packet_len - sizeof(cmdu_raw_msg));
    cfgmgr->m2buf.offset += (pkt->packet_len - sizeof(cmdu_raw_msg));
    // hex_dump("pkt", pkt->packet, pkt->packet_len);
    // hex_dump("buf", cfgmgr->m2buf.buffer, cfgmgr->m2buf.offset);
    cfgmgr_parse_autoconfiguration_wsc(cfgmgr, (cmdu_raw_msg *)cfgmgr->m2buf.buffer, cfgmgr->m2buf.offset);
    // clean buffer to handle next wsc msg
    cfgmgr->m2buf.offset = 0;
}

void cfgmgr_handle_packet(cfg_mgr_t *cfgmgr, mesh_packet_t *pkt)
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
        // cfgmgr_handle_autoconfiguration_renew(cfgmgr, pkt);
        break;
    }
    case MSG_AP_AUTOCONFIGURATION_WSC:
    {
        cfgmgr_handle_autoconfiguration_wsc(cfgmgr, pkt);
        break;
    }
    case MSG_AP_AUTOCONFIGURATION_RESPONSE:
    {
        cfgmgr_handle_autoconfiguration_response(cfgmgr, pkt);
        break;
    }
    case MSG_TOPOLOGY_DISCOBERY:
    {
        snprintf(cfgmgr->bh_local_ifname, sizeof(cfgmgr->bh_local_ifname), "%s",
                 pkt->ifname);
        memcpy(cfgmgr->bh_local_ifaddr, pkt->ifaddr, ETH_ALEN);
        memcpy(cfgmgr->bh_local_almac, pkt->aladdr, ETH_ALEN);
        memcpy(cfgmgr->bh_almac, msg->src_addr, ETH_ALEN);
        if (cfgmgr->radios[0].conf_state != CONFIGURED)
        {
            cfgmgr->radios[0].conf_state = BACKHAUL_READY;
        }
        if (cfgmgr->radios[1].conf_state != CONFIGURED)
        {
            cfgmgr->radios[1].conf_state = BACKHAUL_READY;
        }
        break;
    }
    default:
        break;
    }
}

static void _cfgmgr_handle_group_msg(msg_t *msg, io_buf_t *io)
{
    cfg_mgr_t *cfgmgr = (cfg_mgr_t *)io->arg;
    if (!cfgmgr)
    {
        return;
    }
    cfgmgr->io = io;
    group_msg_t *group_msg = (group_msg_t *)msg->body;
    printf("group msg from group %s, msg: %s, len: %d\n",
           group_msg->group_name, group_msg->group_msg, group_msg->group_msg_len);
    if (0 == strcmp(group_msg->group_name, g_mesh_group_name))
    {
        cfgmgr_handle_packet(cfgmgr, (mesh_packet_t *)group_msg->group_msg);
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

#define TYPE_AL_MAC_ADDRESS 0x01
#define TYPE_SEARCH_ROLE 0x0d
#define TYPE_AUTOCONFIG_FREQ_BAND 0x0e
#define TYPE_SUPPORTED_SERVICE_INFORMATION 0x80
#define TYPE_SEARCHED_SERVICE_LIST 0x81

void cfgmgr_send_autoconfiguration_search(cfg_mgr_t *cfgmgr, radio_t *radio, char *ifname, char *if_addr, char *al_addr, char *dest)
{
    char buffer[512] = {0};
    cmdu_raw_msg *raw_msg = (cmdu_raw_msg *)buffer;
    int offset = 0;

    memcpy(raw_msg->dest_addr, dest, ETH_ALEN);
    memcpy(raw_msg->src_addr, al_addr, ETH_ALEN);
    raw_msg->proto = htons(0x893a);
    raw_msg->msg_1905.hdr.msg_type = htons(MSG_AP_AUTOCONFIGURATION_SEARCH);
    raw_msg->msg_1905.hdr.Flags_last_fragment = 1;
    raw_msg->msg_1905.hdr.msg_id = htons(cfgmgr->msgid++);

    offset += sizeof(cmdu_raw_msg);

    KamiTlv_S *pstRoot = Kami_Tlv_CreateObject(1);

    Kami_Tlv_AddTlvToObject(pstRoot, TYPE_AL_MAC_ADDRESS, ETH_ALEN, al_addr, 1);

    unsigned char search_role = 0x00;
    Kami_Tlv_AddTlvToObject(pstRoot, TYPE_SEARCH_ROLE, sizeof(search_role), &search_role, 1);

    Kami_Tlv_AddTlvToObject(pstRoot, TYPE_AUTOCONFIG_FREQ_BAND,
                            sizeof(radio->auto_conf_band), &radio->auto_conf_band, 1);

    unsigned char sup_serfice_info[] = {0x01, 0x01};
    Kami_Tlv_AddTlvToObject(pstRoot, TYPE_SUPPORTED_SERVICE_INFORMATION,
                            sizeof(sup_serfice_info), sup_serfice_info, 1);

    unsigned char search_serfice_info[] = {0x01, 0x00};
    Kami_Tlv_AddTlvToObject(pstRoot, TYPE_SEARCHED_SERVICE_LIST,
                            sizeof(search_serfice_info), search_serfice_info, 1);

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
    mbus_sendmsg_in_group(cfgmgr->io, g_mesh_group_name, (char *)pkt, packet_length(pkt));
    packet_release(pkt);
}

void cfgmgr_autoconf_search_timer_cb(timer_entry_t *te)
{
    cfg_mgr_t *cfgmgr = (cfg_mgr_t *)te->privdata;

    if (!cfgmgr)
    {
        reset_timer(&cfgmgr->io->loop->timer, te, 10000);
        return;
    }

    for (int i = 0; i < sizeof(cfgmgr->radios) / sizeof(cfgmgr->radios[0]); i++)
    {
        switch (cfgmgr->radios[i].conf_state)
        {
        case UNCONFIGURED:
        {
            break;
        }
        case BACKHAUL_READY:
        {
            cfgmgr_send_autoconfiguration_search(cfgmgr, &cfgmgr->radios[i], cfgmgr->bh_local_ifname, cfgmgr->bh_local_ifaddr,
                                                 cfgmgr->bh_local_almac, cfgmgr->bh_almac);
        }
        default:
            break;
        }
    }

    reset_timer(&cfgmgr->io->loop->timer, te, 10000);
    return;
}

int main(int argc, char *argv[])
{
    cfg_mgr_t cfg_mgr;
    int ret = cfg_mgr_init(&cfg_mgr);
    if (ret < 0)
    {
        printf("cfg mgr init err\n");
        return -1;
    }
    eloop_t *loop = eloop_create();
    int sockfd = eloop_tcp_connect(NULL, "127.0.0.1", "8848");
    if (sockfd < 0)
    {
        printf("connect error %s\n", strerror(errno));
        cfg_mgr_destroy(&cfg_mgr);
        return -1;
    }
    ret = fcntl(sockfd, F_SETFL, O_NONBLOCK | fcntl(sockfd, F_GETFL));
    if (ret < 0)
    {
        printf("set non block err\n");
        cfg_mgr_destroy(&cfg_mgr);
        return -1;
    }

    eloop_set_event(&loop->ios[sockfd], sockfd, &cfg_mgr);
    eloop_add_event(&loop->ios[sockfd], EPOLLIN);
    io_add_packer(&loop->ios[sockfd], sizeof(msg_t), 0);
    eloop_setcb_read(&loop->ios[sockfd], cfgmgr_handle_read);
    add_timer(&loop->timer, 1, cfgmgr_regmsg_timer_cb, &loop->ios[sockfd]);
    cfg_mgr.io = &loop->ios[sockfd];
    add_timer(&loop->timer, 10000, cfgmgr_autoconf_search_timer_cb, &cfg_mgr);
    eloop_run(loop);
    return 0;
}