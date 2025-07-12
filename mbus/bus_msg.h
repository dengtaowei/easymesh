#ifndef __BUS_MSG_H__
#define __BUS_MSG_H__
#include <stdint.h>

typedef struct group_msg_s group_msg_t;

typedef struct msg_s
{
    uint32_t length;
    uint16_t version;
    uint16_t flag;
    uint16_t service_id;
    uint16_t command_id;
    uint16_t seq_num;
    uint16_t reserved;
    uint8_t body[];
} msg_t;

struct group_msg_s
{
    char group_name[128];
    uint16_t group_msg_len;
    uint8_t group_msg[0];
};

enum GroupCmdID
{
    CID_GROUP_NORMAL_LIST_REQUEST = 1025,
    CID_GROUP_MAKE_GROUP = 1026,
    CID_GROUP_JOIN_GROUP = 1027,
    CID_GROUP_LEAVE_GROUP = 1028,
    CID_GROUP_MSG = 1029,
};

enum LoginCmdID
{
    CID_LOGIN_REQ_USERLOGIN = 259,
    CID_LOGIN_RES_USERLOGIN = 260,
    CID_LOGIN_REQ_LOGINOUT = 261,
    CID_LOGIN_RES_LOGINOUT = 262,
};

enum OtherCmdID
{
    CID_OTHER_HEARTBEAT = 1793,
    CID_OTHER_STOP_RECV_PACKET = 1794,
    CID_OTHER_VALIDATE_REQ = 1795,
    CID_OTHER_VALIDATE_RSP = 1796,
    CID_OTHER_GET_DEVICE_TOKEN_REQ = 1797,
    CID_OTHER_GET_DEVICE_TOKEN_RSP = 1798,
    CID_OTHER_ROLE_SET = 1799,
    CID_OTHER_ONLINE_USER_INFO = 1800,
    CID_OTHER_MSG_SERV_INFO = 1801,
    CID_OTHER_USER_STATUS_UPDATE = 1802,
    CID_OTHER_USER_CNT_UPDATE = 1803,
    CID_OTHER_SERVER_KICK_USER = 1805,
    CID_OTHER_LOGIN_STATUS_NOTIFY = 1806,
    CID_OTHER_PUSH_TO_USER_REQ = 1807,
    CID_OTHER_PUSH_TO_USER_RSP = 1808,
    CID_OTHER_GET_SHIELD_REQ = 1809,
    CID_OTHER_GET_SHIELD_RSP = 1810,
    CID_OTHER_FILE_TRANSFER_REQ = 1841,
    CID_OTHER_FILE_TRANSFER_RSP = 1842,
    CID_OTHER_FILE_SERVER_IP_REQ = 1843,
    CID_OTHER_FILE_SERVER_IP_RSP = 1844
};

uint16_t msg_get_command_id(msg_t *msg);

char *msg_get_data(msg_t *msg);

#endif