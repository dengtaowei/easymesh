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

char *g_groupname = NULL;
char *g_username = NULL;
char *g_sendmsg = NULL;

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

    // login
    mbus_register_object(io, g_username);

    mbus_create_group(io, g_groupname);

    mbus_join_group(io, g_groupname);

    if (g_sendmsg)
    {
        mbus_sendmsg_in_group(io, g_groupname, g_sendmsg, strlen(g_sendmsg) + 1);
    }

    // reset_timer(&io->loop->timer, te, 30);
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
    int ret = fcntl(sockfd, F_SETFL, O_NONBLOCK | fcntl(sockfd, F_GETFL));
    if (ret < 0)
    {
        printf("set non block err\n");
        return -1;
    }
#define TEST_WRITE_QUEUE 1
#if defined(TEST_WRITE_QUEUE)
    int sndbuf = 4096;
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sndbuf, sizeof(sndbuf)) < 0)
    {
        printf("set snd buf err\n");
        return -1;
    }

#endif
    eloop_set_event(&loop->ios[sockfd], sockfd, NULL);
    eloop_add_event(&loop->ios[sockfd], EPOLLIN);
    io_add_packer(&loop->ios[sockfd], sizeof(msg_t), 0);
    eloop_setcb_read(&loop->ios[sockfd], cli_handle_read);
    add_timer(&loop->timer, 1, msg_timer_callback, &loop->ios[sockfd]);
    eloop_run(loop);
    return 0;
}