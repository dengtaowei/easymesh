#ifndef __BUS_MSG_H__
#define __BUS_MSG_H__
#include <stdint.h>
#include "eloop_event.h"

#define MAX_USERNAME_LEN 256
#define MAX_GROUPNAME_LEN 256

enum MbusMsgCcmd
{
    MBUS_CMD_NONE,
    MBUS_CMD_HEARTBEAT,
    MBUS_CMD_PRIVATE_MSG,
    MBUS_CMD_GROUP_MSG,
};

typedef struct mbus_s mbus_t;
typedef void (*mbus_msg_cb)(mbus_t *mbus, int cmd, char *from, char *to, void *data, int len);
struct mbus_s
{
    io_buf_t *io;
    mbus_msg_cb msg_cb;
    void *arg;
};

typedef struct msg_create_group_s msg_create_group_t;
struct msg_create_group_s
{
    char groupname[MAX_GROUPNAME_LEN];
};

typedef struct msg_join_group_s msg_join_group_t;
struct msg_join_group_s
{
    char groupname[MAX_GROUPNAME_LEN];
};

typedef struct msg_s
{
    uint32_t length;
    uint16_t version;
    uint16_t flag;
    uint16_t service_id;
    uint16_t command_id;
    uint16_t seq_num;
    uint16_t reserved;
    uint8_t body[0];
} msg_t;

#define MBUS_TYPE_LEN 2
#define TYPE_GROUPMSG_SENDER_NAME 0x0001
#define TYPE_GROUPMSG_GROUP_NAME 0x0002
#define TYPE_GROUPMSG_GROUP_MSG 0x0003

#define TYPE_PRIVATEMSG_SENDER_NAME 0x0004
#define TYPE_PRIVATEMSG_RECEIVER_NAME 0x0005
#define TYPE_PRIVATEMSG_PRIVATE_MSG 0x0006

#define TYPE_USERLOGIN_USERNAME 0x0007

#define TYPE_GROUPCREATE_GROUPNAME 0x0008

#define TYPE_GROUPJOIN_GROUPNAME 0x0009

enum GroupCmdID
{
    CID_GROUP_NORMAL_LIST_REQUEST = 1025,
    CID_GROUP_CREATE_GROUP = 1026,
    CID_GROUP_JOIN_GROUP = 1027,
    CID_GROUP_LEAVE_GROUP = 1028,
    CID_GROUP_MSG = 1029,
};

enum PrivateCmdID
{
    CID_PRIVATE_MSG = 2000,
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

int mbus_io_init(mbus_t *mbus, io_buf_t *io, int fd, void *arg, mbus_msg_cb msg_cb);

int mbus_register_object(io_buf_t *io, const char *username);

int mbus_create_group(io_buf_t *io, const char *groupname);

int mbus_join_group(io_buf_t *io, const char *groupname);

int mbus_sendmsg_in_group(io_buf_t *io, const char *my_name, const char *groupname, char *data, int len);

int mbus_sendmsg_private(io_buf_t *io, const char *my_name, const char *peer_name, char *data, int len);

#endif