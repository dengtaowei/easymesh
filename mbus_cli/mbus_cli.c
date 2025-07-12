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

static void _cli_handle_heartbeat(msg_t *msg, io_buf_t *io)
{
    printf("heart beat pong\n");
}

static void _cli_handle_group_msg(msg_t *msg, io_buf_t *io)
{
    group_msg_t *group_msg = (group_msg_t *)msg->body;
    printf("group msg from group %s, msg: %s, len: %d\n", 
        group_msg->group_name, group_msg->group_msg, group_msg->group_msg_len);
}

void cli_handle_msg(msg_t *msg, io_buf_t *io)
{
    switch (msg_get_command_id(msg))
    {
    case CID_OTHER_HEARTBEAT:
        _cli_handle_heartbeat(msg, io);
        break;
    case CID_GROUP_MSG:
        _cli_handle_group_msg(msg, io);
        break;
    default:
        break;
    }
}

void cli_handle_read(io_buf_t *io)
{
    msg_t *msg = (msg_t *)io_data(io);
    cli_handle_msg(msg, io);
}

const char *g_groupname = NULL;
const char *g_username = NULL;
const char *g_sendmsg = NULL;

void msg_timer_callback(timer_entry_t *te)
{
    if (!g_username)
    {
        printf("user name not found\n");
        return;
    }
    if (!g_groupname)
    {
        printf("group name not found\n");
        return;
    }
    io_buf_t *io = (io_buf_t *)te->privdata;
    char buffer[2048] = {0};
    msg_t *msg = (msg_t *)buffer;

    // login
    msg->command_id = CID_LOGIN_REQ_USERLOGIN;
    snprintf((char *)msg->body, 128, "%s", g_username);
    msg->length = strlen(g_username) + 1;
    int nsend = io_write(io, (char *)msg, sizeof(msg_t) + msg->length);
    printf("send %d\n", nsend);

    // make group CID_GROUP_MAKE_GROUP
    msg->command_id = CID_GROUP_MAKE_GROUP;
    snprintf((char *)msg->body, 128, "%s", g_groupname);
    msg->length = strlen(g_groupname) + 1;
    nsend = io_write(io, (char *)msg, sizeof(msg_t) + msg->length);
    printf("send %d\n", nsend);

    // join group CID_GROUP_MAKE_GROUP
    msg->command_id = CID_GROUP_JOIN_GROUP;
    snprintf((char *)msg->body, 128, "%s", g_groupname);
    msg->length = strlen(g_groupname) + 1;
    nsend = io_write(io, (char *)msg, sizeof(msg_t) + msg->length);
    printf("send %d\n", nsend);

    if (g_sendmsg)
    {
        memset(buffer, 0, sizeof(buffer));
        group_msg_t *group_msg = (group_msg_t *)msg->body;


        // join group CID_GROUP_MAKE_GROUP
        msg->command_id = CID_GROUP_MSG;  // send msg in group
        snprintf((char *)group_msg->group_name, 128, "%s", g_groupname);
        snprintf((char *)group_msg->group_msg, 128, "%s", g_sendmsg);
        group_msg->group_msg_len = strlen(g_sendmsg) + 1;
        msg->length = sizeof(group_msg_t) + group_msg->group_msg_len;
        nsend = io_write(io, (char *)msg, sizeof(msg_t) + msg->length);
        printf("send %d\n", nsend);
    }

    // reset_timer(&io->loop->timer, te, 300);
    return;
}

int parse_cmdline(int argc, char *argv[])
{
    int opt;
    while ((opt = getopt(argc, argv, "u:g:m:")) != -1)
    {
        switch (opt)
        {
        case 'u':
            g_username = optarg;
            printf("username: %s\n", g_username);
            break;
        case 'g':
            g_groupname = optarg;
            printf("groupname: %s\n", g_groupname);
            break;
        case 'm':
            g_sendmsg = optarg;
            break;
        default:
            break;
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    parse_cmdline(argc, argv);
    eloop_t *loop = eloop_create();
    int sockfd = eloop_tcp_connect(NULL, "127.0.0.1", "8848");
    if (sockfd < 0)
    {
        printf("connect error %s\n", strerror(errno));
        return -1;
    }
    eloop_set_event(&loop->ios[sockfd], sockfd, NULL);
    eloop_add_event(&loop->ios[sockfd], EPOLLIN);
    io_add_packer(&loop->ios[sockfd], sizeof(msg_t), 0);
    eloop_setcb_read(&loop->ios[sockfd], cli_handle_read);
    add_timer(&loop->timer, 1, msg_timer_callback, &loop->ios[sockfd]);
    eloop_run(loop);
    return 0;
}