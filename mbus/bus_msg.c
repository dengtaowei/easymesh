#include <stdio.h>
#include "bus_msg.h"
#include <sys/epoll.h>
#include "eloop_event.h"
#include "tlv_parser.h"

uint16_t msg_get_command_id(msg_t *msg)
{
    return msg->command_id;
}

char *msg_get_data(msg_t *msg)
{
    return (char *)msg->body;
}

static void mbus_handle_heartbeat(msg_t *msg, io_buf_t *io)
{
    mbus_t *mbus = (mbus_t *)io->arg;
    mbus->msg_cb(mbus, MBUS_CMD_HEARTBEAT, NULL, NULL, msg->body, msg->length);
}

static void mbus_handle_group_msg(msg_t *msg, io_buf_t *io)
{
    mbus_t *mbus = (mbus_t *)io->arg;
    KamiTlv_S *pstRoot = Kami_Tlv_ParseObject(msg->body, msg->length, MBUS_TYPE_LEN);
    if (!pstRoot)
    {
        printf("group msg parse err\n");
        return;
    }
    KamiTlv_S *pstSenderName = Kami_Tlv_GetObjectItem(pstRoot, TYPE_GROUPMSG_SENDER_NAME, 0);
    KamiTlv_S *pstGroupName = Kami_Tlv_GetObjectItem(pstRoot, TYPE_GROUPMSG_GROUP_NAME, 0);
    KamiTlv_S *pstGroupMsg = Kami_Tlv_GetObjectItem(pstRoot, TYPE_GROUPMSG_GROUP_MSG, 0);
    if (!pstSenderName || !pstGroupMsg || !pstGroupName)
    {
        Kami_Tlv_Delete(pstRoot);
        return;
    }

    mbus->msg_cb(mbus, MBUS_CMD_GROUP_MSG, pstSenderName->value, pstGroupName->value,
                 (void *)pstGroupMsg->value, pstGroupMsg->length);
    Kami_Tlv_Delete(pstRoot);
}

static void mbus_handle_private_msg(msg_t *msg, io_buf_t *io)
{
    mbus_t *mbus = (mbus_t *)io->arg;
    KamiTlv_S *pstRoot = Kami_Tlv_ParseObject(msg->body, msg->length, MBUS_TYPE_LEN);
    if (!pstRoot)
    {
        printf("private msg parse err\n");
        return;
    }
    KamiTlv_S *pstSenderName = Kami_Tlv_GetObjectItem(pstRoot, TYPE_PRIVATEMSG_SENDER_NAME, 0);
    KamiTlv_S *pstReceiverName = Kami_Tlv_GetObjectItem(pstRoot, TYPE_PRIVATEMSG_RECEIVER_NAME, 0);
    KamiTlv_S *pstPrivateMsg = Kami_Tlv_GetObjectItem(pstRoot, TYPE_PRIVATEMSG_PRIVATE_MSG, 0);
    if (!pstSenderName || !pstReceiverName || !pstPrivateMsg)
    {
        Kami_Tlv_Delete(pstRoot);
        return;
    }

    mbus->msg_cb(mbus, MBUS_CMD_PRIVATE_MSG, pstSenderName->value, pstReceiverName->value,
                 pstPrivateMsg->value, pstPrivateMsg->length);
    Kami_Tlv_Delete(pstRoot);
}

static void mbus_handle_msg(msg_t *msg, io_buf_t *io)
{
    mbus_t *mbus = (mbus_t *)io->arg;

    if (!mbus || !mbus->msg_cb)
    {
        return;
    }

    switch (msg_get_command_id(msg))
    {
    case CID_OTHER_HEARTBEAT:
    {
        mbus_handle_heartbeat(msg, io);
        break;
    }
    case CID_GROUP_MSG:
    {
        mbus_handle_group_msg(msg, io);
        break;
    }
    case CID_PRIVATE_MSG:
    {
        mbus_handle_private_msg(msg, io);
        break;
    }
    default:
        break;
    }
}

static void mbus_handle_read(io_buf_t *io)
{
    msg_t *msg = (msg_t *)io_data(io);
    mbus_handle_msg(msg, io);
}

int mbus_io_init(mbus_t *mbus, io_buf_t *io, int fd, void *arg, mbus_msg_cb msg_cb)
{
    eloop_set_event(io, fd, arg);
    eloop_add_event(io, EPOLLIN);
    io_add_packer(io, sizeof(msg_t), 0);
    eloop_setcb_read(io, mbus_handle_read);
    mbus->io = io;
    mbus->arg = arg;
    mbus->msg_cb = msg_cb;
    io->arg = mbus;
    return 0;
}

int mbus_register_object(io_buf_t *io, const char *username)
{
    if (!io || !username || username[0] == '\0')
    {
        return -1;
    }

    KamiTlv_S *pstRoot = Kami_Tlv_CreateObject(MBUS_TYPE_LEN);
    if (!pstRoot)
    {
        return -1;
    }
    Kami_Tlv_AddTlvToObject(pstRoot, TYPE_USERLOGIN_USERNAME,
                            strlen(username) + 1, (void *)username, MBUS_TYPE_LEN);

    unsigned char *pr = Kami_Tlv_Print(pstRoot);
    int tlv_len = Kami_Tlv_ObjectLength(pstRoot);

    if (!pr)
    {
        Kami_Tlv_Delete(pstRoot);
        return -1;
    }

    msg_t *msg = malloc(sizeof(msg_t) + tlv_len);
    if (!msg)
    {
        free(pr);
        Kami_Tlv_Delete(pstRoot);
        return -1;
    }
    memset(msg, 0, sizeof(msg_t) + tlv_len);
    memcpy(msg->body, pr, tlv_len);
    msg->length = tlv_len;

    msg->command_id = CID_LOGIN_REQ_USERLOGIN;

    int nsend = io_write(io, (char *)msg, sizeof(msg_t) + msg->length);
    printf("send %d\n", nsend);

    free(pr);
    Kami_Tlv_Delete(pstRoot);
    free(msg);
    return 0;
}

int mbus_create_group(io_buf_t *io, const char *groupname)
{
    if (!io || !groupname || groupname[0] == '\0')
    {
        return -1;
    }
    KamiTlv_S *pstRoot = Kami_Tlv_CreateObject(MBUS_TYPE_LEN);
    if (!pstRoot)
    {
        return -1;
    }
    Kami_Tlv_AddTlvToObject(pstRoot, TYPE_GROUPCREATE_GROUPNAME,
                            strlen(groupname) + 1, (void *)groupname, MBUS_TYPE_LEN);

    unsigned char *pr = Kami_Tlv_Print(pstRoot);
    int tlv_len = Kami_Tlv_ObjectLength(pstRoot);

    if (!pr)
    {
        Kami_Tlv_Delete(pstRoot);
        return -1;
    }

    msg_t *msg = malloc(sizeof(msg_t) + tlv_len);
    if (!msg)
    {
        free(pr);
        Kami_Tlv_Delete(pstRoot);
        return -1;
    }
    memset(msg, 0, sizeof(msg_t) + tlv_len);
    memcpy(msg->body, pr, tlv_len);
    msg->length = tlv_len;

    msg->command_id = CID_GROUP_CREATE_GROUP;

    int nsend = io_write(io, (char *)msg, sizeof(msg_t) + msg->length);
    printf("send %d\n", nsend);

    free(pr);
    Kami_Tlv_Delete(pstRoot);
    free(msg);

    return 0;
}

int mbus_join_group(io_buf_t *io, const char *groupname)
{
    if (!io || !groupname || groupname[0] == '\0')
    {
        return -1;
    }
    KamiTlv_S *pstRoot = Kami_Tlv_CreateObject(MBUS_TYPE_LEN);
    if (!pstRoot)
    {
        return -1;
    }
    Kami_Tlv_AddTlvToObject(pstRoot, TYPE_GROUPJOIN_GROUPNAME,
                            strlen(groupname) + 1, (void *)groupname, MBUS_TYPE_LEN);

    unsigned char *pr = Kami_Tlv_Print(pstRoot);
    int tlv_len = Kami_Tlv_ObjectLength(pstRoot);

    if (!pr)
    {
        Kami_Tlv_Delete(pstRoot);
        return -1;
    }

    msg_t *msg = malloc(sizeof(msg_t) + tlv_len);
    if (!msg)
    {
        free(pr);
        Kami_Tlv_Delete(pstRoot);
        return -1;
    }
    memset(msg, 0, sizeof(msg_t) + tlv_len);
    memcpy(msg->body, pr, tlv_len);
    msg->length = tlv_len;

    msg->command_id = CID_GROUP_JOIN_GROUP;

    int nsend = io_write(io, (char *)msg, sizeof(msg_t) + msg->length);
    printf("send %d\n", nsend);

    free(pr);
    Kami_Tlv_Delete(pstRoot);
    free(msg);

    return 0;
}

static msg_t *group_msg_new(const char *my_name, const char *groupname, void *data, int len)
{
    KamiTlv_S *pstRoot = Kami_Tlv_CreateObject(MBUS_TYPE_LEN);
    if (!pstRoot)
    {
        return NULL;
    }
    Kami_Tlv_AddTlvToObject(pstRoot, TYPE_GROUPMSG_SENDER_NAME,
                            strlen(my_name) + 1, (void *)my_name, MBUS_TYPE_LEN);
    Kami_Tlv_AddTlvToObject(pstRoot, TYPE_GROUPMSG_GROUP_NAME,
                            strlen(groupname) + 1, (void *)groupname, MBUS_TYPE_LEN);
    if (data && len > 0)
    {
        Kami_Tlv_AddTlvToObject(pstRoot, TYPE_GROUPMSG_GROUP_MSG,
                                len, data, MBUS_TYPE_LEN);
    }

    unsigned char *pr = Kami_Tlv_Print(pstRoot);
    int tlv_len = Kami_Tlv_ObjectLength(pstRoot);

    if (!pr)
    {
        Kami_Tlv_Delete(pstRoot);
        return NULL;
    }

    msg_t *msg = malloc(sizeof(msg_t) + tlv_len);
    if (!msg)
    {
        free(pr);
        Kami_Tlv_Delete(pstRoot);
        return NULL;
    }
    memset(msg, 0, sizeof(msg_t) + tlv_len);
    memcpy(msg->body, pr, tlv_len);

    msg->length = tlv_len;

    free(pr);
    Kami_Tlv_Delete(pstRoot);
    return msg;
}

static msg_t *private_msg_new(const char *my_name, const char *peer_name, void *data, int len)
{
    KamiTlv_S *pstRoot = Kami_Tlv_CreateObject(MBUS_TYPE_LEN);
    if (!pstRoot)
    {
        return NULL;
    }
    Kami_Tlv_AddTlvToObject(pstRoot, TYPE_PRIVATEMSG_SENDER_NAME,
                            strlen(my_name) + 1, (void *)my_name, MBUS_TYPE_LEN);
    Kami_Tlv_AddTlvToObject(pstRoot, TYPE_PRIVATEMSG_RECEIVER_NAME,
                            strlen(peer_name) + 1, (void *)peer_name, MBUS_TYPE_LEN);
    if (data && len > 0)
    {
        Kami_Tlv_AddTlvToObject(pstRoot, TYPE_PRIVATEMSG_PRIVATE_MSG,
                                len, data, MBUS_TYPE_LEN);
    }

    unsigned char *pr = Kami_Tlv_Print(pstRoot);
    int tlv_len = Kami_Tlv_ObjectLength(pstRoot);

    if (!pr)
    {
        Kami_Tlv_Delete(pstRoot);
        return NULL;
    }

    msg_t *msg = malloc(sizeof(msg_t) + tlv_len);
    if (!msg)
    {
        free(pr);
        Kami_Tlv_Delete(pstRoot);
        return NULL;
    }
    memset(msg, 0, sizeof(msg_t) + tlv_len);
    memcpy(msg->body, pr, tlv_len);

    msg->length = tlv_len;

    free(pr);
    Kami_Tlv_Delete(pstRoot);
    return msg;
}

static void group_msg_delete(msg_t *msg)
{
    if (msg)
    {
        free(msg);
    }
}

static void private_msg_delete(msg_t *msg)
{
    if (msg)
    {
        free(msg);
    }
}

int mbus_sendmsg_in_group(io_buf_t *io, const char *my_name, const char *groupname, char *data, int len)
{
    if (!io || !my_name || !groupname || len <= 0)
    {
        return -1;
    }
    msg_t *msg = group_msg_new(my_name, groupname, data, len);
    msg->command_id = CID_GROUP_MSG;
    if (!msg)
    {
        return -1;
    }

    int nsend = io_write(io, (char *)msg, sizeof(msg_t) + msg->length);
    printf("send %d\n", nsend);
    group_msg_delete(msg);
    return 0;
}

int mbus_sendmsg_private(io_buf_t *io, const char *my_name, const char *peer_name, char *data, int len)
{
    if (!io || !my_name || !peer_name || len <= 0)
    {
        return -1;
    }
    msg_t *msg = private_msg_new(my_name, peer_name, data, len);
    msg->command_id = CID_PRIVATE_MSG;
    if (!msg)
    {
        return -1;
    }

    int nsend = io_write(io, (char *)msg, sizeof(msg_t) + msg->length);
    printf("send %d\n", nsend);
    private_msg_delete(msg);
    return 0;
}