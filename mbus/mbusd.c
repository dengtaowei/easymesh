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
#include "group_manager.h"
#include "user_manager.h"
#include "bus_msg.h"
#include "tlv_parser.h"

static void _handle_heartbeat(msg_t *msg, io_buf_t *io)
{
    printf("heart beat ping\n");
    int nsend = io_write(io, (char *)msg, sizeof(msg_t) + msg->length);
    printf("send %d\n", nsend);

    usleep(10000000);
}

static void _handle_login_request(msg_t *msg, io_buf_t *io)
{
    printf("login request\n");
    client_t *cli = NULL;
    KamiTlv_S *pstRoot = Kami_Tlv_ParseObject(msg->body, msg->length, MBUS_TYPE_LEN);
    if (!pstRoot)
    {
        printf("group msg parse err\n");
        return;
    }
    KamiTlv_S *pstUserName = Kami_Tlv_GetObjectItem(pstRoot, TYPE_USERLOGIN_USERNAME, 0);
    if (!pstUserName)
    {
        Kami_Tlv_Delete(pstRoot);
        return;
    }
    cli = user_get_by_name(pstUserName->value);
    if (cli)
    {
        printf("user already existed\n");
        Kami_Tlv_Delete(pstRoot);
        return;
    }
    cli = user_create(pstUserName->value);
    if (!cli)
    {
        printf("user create error\n");
        Kami_Tlv_Delete(pstRoot);
        return;
    }
    io->arg = cli;
    cli->io = io;
    Kami_Tlv_Delete(pstRoot);
}

static void _handle_create_group(msg_t *msg, io_buf_t *io)
{
    KamiTlv_S *pstRoot = Kami_Tlv_ParseObject(msg->body, msg->length, MBUS_TYPE_LEN);
    if (!pstRoot)
    {
        printf("group msg parse err\n");
        return;
    }
    KamiTlv_S *pstGroupName = Kami_Tlv_GetObjectItem(pstRoot, TYPE_GROUPCREATE_GROUPNAME, 0);
    if (!pstGroupName)
    {
        Kami_Tlv_Delete(pstRoot);
        return;
    }
    group_t *group = search_group(pstGroupName->value);
    if (group)
    {
        printf("group already existed\n");
        Kami_Tlv_Delete(pstRoot);
        return;
    }
    group = group_create(pstGroupName->value);
    if (!group)
    {
        return;
    }
    printf("group %s created\n", group->group_name);
    Kami_Tlv_Delete(pstRoot);
    
}

static void _handle_join_group(msg_t *msg, io_buf_t *io)
{
    client_t *cli = (client_t *)io->arg;
    if (!cli)
    {
        printf("user not login\n");
        return;
    }
    
    KamiTlv_S *pstRoot = Kami_Tlv_ParseObject(msg->body, msg->length, MBUS_TYPE_LEN);
    if (!pstRoot)
    {
        printf("group msg parse err\n");
        Kami_Tlv_Delete(pstRoot);
        return;
    }
    KamiTlv_S *pstGroupName = Kami_Tlv_GetObjectItem(pstRoot, TYPE_GROUPJOIN_GROUPNAME, 0);
    if (!pstGroupName)
    {
        Kami_Tlv_Delete(pstRoot);
        return;
    }

    group_t *group = search_group(pstGroupName->value);
    if (!group)
    {
        printf("group not exist\n");
        Kami_Tlv_Delete(pstRoot);
        return;
    }

    join_group(cli->name, pstGroupName->value);
    printf("user %s join group %s\n", cli->name, group->group_name);
    Kami_Tlv_Delete(pstRoot);
}

static void _handle_leave_group(msg_t *msg, io_buf_t *io)
{
    client_t *cli = (client_t *)io->arg;
    if (!cli)
    {
        return;
    }
    leave_group_by_name(cli->name, msg_get_data(msg));
}

static void send_data_to_user(client_t *cli, char *data, int len)
{
    io_buf_t *io = cli->io;
    if (!io)
    {
        printf("error user %s not has io\n", cli->name);
        return;
    }
    int nsend = io_write(io, data, len);
    printf("send %d bytes to %s\n", nsend, cli->name);
}

typedef struct group_cb_arg_s
{
    char *from;
    char *data;
    int len;
} group_cb_arg_t;

static void group_cb(void *member, void *data)
{
    char *username = (char *)member;
    group_cb_arg_t *arg = (group_cb_arg_t *)data;
    if (0 == strcmp(username, arg->from))
    {
        return;
    }
    client_t *cli = user_get_by_name(username);
    if (cli)
    {
        send_data_to_user(cli, arg->data, arg->len);
    }
}

static void forward_msg_in_group(group_t *group, char *from, char *data, int len)
{
    group_cb_arg_t arg;
    arg.data = data;
    arg.len = len;
    arg.from = from;
    foreach_group_member(group->group_name, group_cb, &arg);
    // KamiListIterrator *iter = KamiListGetIter(&group->users, Iter_From_Head);
    // KamiListNode *tmp = NULL;
    // while ((tmp = KamiListNext(iter)) != NULL)
    // {
    //     char *user_name = (char *)tmp->data;
    //     if (0 == strcmp(user_name, from))
    //     {
    //         continue;
    //     }
    //     client_t *cli = user_get_by_name(user_name);
    //     if (cli)
    //     {
    //         send_data_to_user(cli, data, len);
    //     }
    // }
    // KamiListDelIterator(iter);
}

static void _handle_group_msg(msg_t *msg, io_buf_t *io)
{
    client_t *cli = (client_t *)io->arg;
    if (!cli)
    {
        printf("user not login\n");
        return;
    }

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

    printf("group msg from user %s in group %s, msg: %s, len: %d\n",
           cli->name, pstGroupName->value,
           pstGroupMsg->value, pstGroupMsg->length);

    if (pstGroupName->value[0] == '\0')
    {
        Kami_Tlv_Delete(pstRoot);
        return;
    }
    group_t *group = search_group(pstGroupName->value);
    if (!group)
    {
        printf("%s not existed\n", pstGroupName->value);
        Kami_Tlv_Delete(pstRoot);
        return;
    }

    forward_msg_in_group(group, cli->name, (char *)msg, sizeof(msg_t) + msg->length);
    Kami_Tlv_Delete(pstRoot);
}

static void _handle_private_msg(msg_t *msg, io_buf_t *io)
{
    client_t *cli = (client_t *)io->arg;
    if (!cli)
    {
        printf("user not login\n");
        return;
    }

    KamiTlv_S *pstRoot = Kami_Tlv_ParseObject(msg->body, msg->length, MBUS_TYPE_LEN);
    if (!pstRoot)
    {
        printf("group msg parse err\n");
        return;
    }
    KamiTlv_S *pstSenderName = Kami_Tlv_GetObjectItem(pstRoot, TYPE_PRIVATEMSG_SENDER_NAME, 0);
    KamiTlv_S *pstReciverName = Kami_Tlv_GetObjectItem(pstRoot, TYPE_PRIVATEMSG_RECEIVER_NAME, 0);
    KamiTlv_S *pstPrivateMsg = Kami_Tlv_GetObjectItem(pstRoot, TYPE_PRIVATEMSG_PRIVATE_MSG, 0);
    if (!pstSenderName || !pstSenderName || !pstPrivateMsg)
    {
        Kami_Tlv_Delete(pstRoot);
        return;
    }

    client_t *peer_cli = user_get_by_name(pstReciverName->value);
    if (!peer_cli)
    {
        printf("user %s not existed\n", pstReciverName->value);
        Kami_Tlv_Delete(pstRoot);
        return;
    }
    printf("forward msg from %s to %s\n", cli->name, peer_cli->name);
    send_data_to_user(peer_cli, (char *)msg, sizeof(msg_t) + msg->length);
    Kami_Tlv_Delete(pstRoot);
}

void handle_msg(msg_t *msg, io_buf_t *io)
{
    switch (msg_get_command_id(msg))
    {
    case CID_OTHER_HEARTBEAT:
        _handle_heartbeat(msg, io);
        break;
    case CID_LOGIN_REQ_USERLOGIN:
        _handle_login_request(msg, io);
        break;
    case CID_GROUP_CREATE_GROUP:
        _handle_create_group(msg, io);
        break;
    case CID_GROUP_JOIN_GROUP:
        _handle_join_group(msg, io);
        break;
    case CID_GROUP_LEAVE_GROUP:
        _handle_leave_group(msg, io);
        break;
    case CID_GROUP_MSG:
        _handle_group_msg(msg, io);
        break;
    case CID_PRIVATE_MSG:
        _handle_private_msg(msg, io);
        break;
    default:
        break;
    }
}

void handle_read(io_buf_t *io)
{
    msg_t *msg = (msg_t *)io_data(io);
    handle_msg(msg, io);
}

void handle_close(io_buf_t *io)
{
    printf("del user\n");
    client_t *cli = (client_t *)io->arg;
    if (!cli)
    {
        printf("del user user did not login\n");
        return;
    }
    leave_all_group(cli->name);
    user_delete(cli->name);
}

void handle_accept(io_buf_t *io)
{
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    int clientfd = accept(io->fd, (struct sockaddr *)&client_addr, &len);
    if (clientfd < 0)
    {
        return;
    }
    int i = 0;
    do
    {
        for (int i = 0; i < MAX_EPOLL_EVENTS; i++)
        {
            if (io->loop->ios[i].events == 0)
            {
                break;
            }
        }
        if (i == MAX_EPOLL_EVENTS)
        {
            printf("conn limit\n");
            break;
        }
        int flag = 0;
        if ((flag = fcntl(clientfd, F_SETFL, O_NONBLOCK)) < 0)
        {
            break;
        }
        eloop_set_event(&io->loop->ios[clientfd], clientfd, NULL);
        eloop_setcb_read(&io->loop->ios[clientfd], handle_read);
        eloop_setcb_close(&io->loop->ios[clientfd], handle_close);
        io_add_packer(&io->loop->ios[clientfd], sizeof(msg_t), 0);
        eloop_add_event(&io->loop->ios[clientfd], EPOLLIN);
    } while (0);

    return;
}

int main(int argc, const char *argv[])
{
    eloop_t *loop = eloop_create();
    int listenfd = eloop_tcp_server_init(8848);
    eloop_addlistener(loop, listenfd, handle_accept);
    eloop_run(loop);
    return 0;
}