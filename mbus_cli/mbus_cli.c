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

static void _cli_handle_heartbeat(mbus_t *mbus, char *from, char *to, void *data, int len)
{
    printf("heart beat pong\n");
}

static void _cli_handle_group_msg(mbus_t *mbus, char *from, char *to, void *data, int len)
{
    printf("group msg from %s to %s, len=%d\n", from, to, len);
}

static void _cli_handle_private_msg(mbus_t *mbus, char *from, char *to, void *data, int len)
{
    printf("private msg from %s to %s, len=%d, msg: %s\n", from, to, len, (char *)data);
}

void cli_handle_msg(mbus_t *mbus, int cmd, char *from, char *to, void *data, int len)
{
    switch (cmd)
    {
    case MBUS_CMD_HEARTBEAT:
        _cli_handle_heartbeat(mbus, from, to, data, len);
        break;
    case MBUS_CMD_GROUP_MSG:
        _cli_handle_group_msg(mbus, from, to, data, len);
        break;
    case MBUS_CMD_PRIVATE_MSG:
        _cli_handle_private_msg(mbus, from, to, data, len);
        break;
    default:
        break;
    }
}


char *g_groupname = NULL;
char *g_username = NULL;
char *g_sendmsg = NULL;
char *g_peername = NULL;

void msg_timer_callback(timer_entry_t *te)
{
    if (!g_username)
    {
        printf("user name not found\n");
        return;
    }
    if (!g_groupname && !g_peername)
    {
        printf("group name or peer user name not found\n");
        return;
    }
    io_buf_t *io = (io_buf_t *)te->privdata;
    // login
    mbus_register_object(io, g_username);
    if (g_groupname && g_sendmsg)
    {

        mbus_create_group(io, g_groupname);

        mbus_join_group(io, g_groupname);

        mbus_sendmsg_in_group(io, g_username, g_groupname, g_sendmsg, strlen(g_sendmsg) + 1);
    }

    if (g_peername && g_sendmsg)
    {
        mbus_sendmsg_private(io, g_username, g_peername, g_sendmsg, strlen(g_sendmsg) + 1);
    }

    // reset_timer(&io->loop->timer, te, 30);
    return;
}

int parse_cmdline(int argc, char *argv[])
{
    int opt;
    while ((opt = getopt(argc, argv, "u:g:m:p:")) != -1)
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
        case 'p':
            g_peername = optarg;
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

    mbus_t mbus_cli;
    memset(&mbus_cli, 0, sizeof(mbus_t));
    mbus_io_init(&mbus_cli, &loop->ios[sockfd], sockfd, NULL, cli_handle_msg);

    add_timer(&loop->timer, 1, msg_timer_callback, &loop->ios[sockfd]);
    eloop_run(loop);
    return 0;
}