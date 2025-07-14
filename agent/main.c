#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include "eloop_event.h"
#include <sys/epoll.h>

#include <string.h>
#ifndef _WIN32
#include <signal.h>
#else
#endif

#include "ieee1905_network.h"
#include "cmdu.h"
#include "core.h"
#include "bus_msg.h"
#include "mh-timer.h"

io_buf_t *g_mesh_io = NULL;
const char *g_mesh_user_name = "umesh";
const char *g_mesh_group_name = "gmesh";

static void forward_cmdu(void *cmdu, int len)
{
    char buffer[2048] = { 0 };
    memset(buffer, 0, sizeof(buffer));
    msg_t *msg = (msg_t *)buffer;
    group_msg_t *group_msg = (group_msg_t *)msg->body;


    // join group CID_GROUP_MAKE_GROUP
    msg->command_id = CID_GROUP_MSG;  // send msg in group
    snprintf((char *)group_msg->group_name, 128, "%s", g_mesh_group_name);
    memmove(group_msg->group_msg, cmdu, len);
    group_msg->group_msg_len = len + 1;
    msg->length = sizeof(group_msg_t) + group_msg->group_msg_len;
    int nsend = io_write(g_mesh_io, (char *)msg, sizeof(msg_t) + msg->length);
    printf("send %d\n", nsend);
}

int cmdu_filter_func(NetworkInterface *interface, int op, void *data, int len)
{
    switch (op)
    {
    case IF_SEND:
    {
        printf("filter cmdu before send\n");
        forward_cmdu(data, len);
        break;
    }
    case IF_RECV:
    {
        printf("filter cmdu before recv\n");
        break;
    }
    default:
        break;
    }

    return IF_ACCEPT;
}

void client_read_cb(io_buf_t *io)
{
    // cout << "[client_R]" << flush;
    printf("client_R\n");
    char data[1024] = {0};

    NetworkInterface *interface = (NetworkInterface *)io->arg;
    int recv_b = if_recv(interface, data, sizeof(data));
    if (recv_b <= 0)
    {
        printf("recv err\n");
        return;
    }

    printf("=========== recv tytes %d===========\n", recv_b);

    cmdu_handle(interface, data, recv_b);
}

static void _mesh_handle_heartbeat(msg_t *msg, io_buf_t *io)
{
    printf("heart beat pong\n");
}

static void _mesh_handle_group_msg(msg_t *msg, io_buf_t *io)
{
    group_msg_t *group_msg = (group_msg_t *)msg->body;
    printf("group msg from group %s, msg: %s, len: %d\n",
           group_msg->group_name, group_msg->group_msg, group_msg->group_msg_len);
}

void mesh_handle_msg(msg_t *msg, io_buf_t *io)
{
    switch (msg_get_command_id(msg))
    {
    case CID_OTHER_HEARTBEAT:
        _mesh_handle_heartbeat(msg, io);
        break;
    case CID_GROUP_MSG:
        _mesh_handle_group_msg(msg, io);
        break;
    default:
        break;
    }
}

void mesh_handle_read(io_buf_t *io)
{
    msg_t *msg = (msg_t *)io_data(io);
    mesh_handle_msg(msg, io);
}

void mesh_obj_create_timer_cb(timer_entry_t *te)
{
    
    io_buf_t *io = (io_buf_t *)te->privdata;
    char buffer[2048] = {0};
    msg_t *msg = (msg_t *)buffer;

    // login
    msg->command_id = CID_LOGIN_REQ_USERLOGIN;
    snprintf((char *)msg->body, 128, "%s", g_mesh_user_name);
    msg->length = strlen(g_mesh_user_name) + 1;
    int nsend = io_write(io, (char *)msg, sizeof(msg_t) + msg->length);
    printf("send %d\n", nsend);

    // make group CID_GROUP_MAKE_GROUP
    msg->command_id = CID_GROUP_MAKE_GROUP;
    snprintf((char *)msg->body, 128, "%s", g_mesh_group_name);
    msg->length = strlen(g_mesh_group_name) + 1;
    nsend = io_write(io, (char *)msg, sizeof(msg_t) + msg->length);
    printf("send %d\n", nsend);

    // join group CID_GROUP_MAKE_GROUP
    msg->command_id = CID_GROUP_JOIN_GROUP;
    snprintf((char *)msg->body, 128, "%s", g_mesh_group_name);
    msg->length = strlen(g_mesh_group_name) + 1;
    nsend = io_write(io, (char *)msg, sizeof(msg_t) + msg->length);
    printf("send %d\n", nsend);

    // reset_timer(&io->loop->timer, te, 5000);
}

int main(int argc, char *argv[])
{
    // 忽略管道信号，发送数据给已关闭的socket
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
        return 1;

    eloop_t *loop = eloop_create();
    // 创建网络服务器

    int ret = 0;
    NetworkInterface interface;
    memset(&interface, 0, sizeof(interface));
    // 5a:9a:94:c0:42:57
    interface.al_addr[0] = 0x5a;
    interface.al_addr[1] = 0x9a;
    interface.al_addr[2] = 0x94;
    interface.al_addr[3] = 0xc0;
    interface.al_addr[4] = 0x42;
    interface.al_addr[5] = 0x57;

    ret = if_create(&interface, "ens32", (void *)loop);
    if (ret)
    {
        printf("sk create error\n");
        return -1;
    }

    register_interface(&interface);

    if_register_filter(&interface, cmdu_filter_func);

    eloop_set_event(&loop->ios[interface.fd], interface.fd, &interface);
    eloop_setcb_read(&loop->ios[interface.fd], client_read_cb);
    eloop_add_event(&loop->ios[interface.fd], EPOLLIN);

    int sockfd = eloop_tcp_connect(NULL, "127.0.0.1", "8848");
    if (sockfd < 0)
    {
        printf("connect error %s\n", strerror(errno));
        return -1;
    }
    ret = fcntl(sockfd, F_SETFL, O_NONBLOCK | fcntl(sockfd, F_GETFL));
    if (ret < 0)
    {
        printf("set non block err\n");
        return -1;
    }

    eloop_set_event(&loop->ios[sockfd], sockfd, NULL);
    eloop_add_event(&loop->ios[sockfd], EPOLLIN);
    io_add_packer(&loop->ios[sockfd], sizeof(msg_t), 0);
    eloop_setcb_read(&loop->ios[sockfd], mesh_handle_read);
    add_timer(&loop->timer, 1, mesh_obj_create_timer_cb, &loop->ios[sockfd]);
    g_mesh_io = &loop->ios[sockfd];
    // 进入事件主循环
    eloop_run(loop);
    eloop_destroy(loop);
    if_release(&interface);

    return 0;
}