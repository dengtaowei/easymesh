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
#include "mesh_msg.h"

io_buf_t *g_mesh_io = NULL;
const char *g_mesh_user_name = "umesh";
const char *g_mesh_group_name = "gmesh";

static void forward_cmdu_to_interface(mesh_packet_t *pkt)
{
    NetworkInterface *interface = get_if_by_name(pkt->ifname);
    if (interface)
    {
        printf("send pkt len %d to if %s\n", pkt->packet_len, pkt->ifname);
        if_send(interface, pkt->packet, pkt->packet_len);
    }
}

static void forward_cmdu(mesh_packet_t *pkt)
{
    if (pkt->to == PKT_NETWORK)
    {
        forward_cmdu_to_interface(pkt);
    }
}

int cmdu_filter_func(NetworkInterface *interface, int op, void *data, int len)
{
    switch (op)
    {
    case IF_SEND:
    {
        printf("filter cmdu before send\n");
        mesh_packet_t *pkt = packet_create(data, len);
        if (!pkt)
        {
            printf("pkt create err\n");
            return -1;
        }
        pkt->from = PKT_LOCAL;
        pkt->to = PKT_NETWORK;
        snprintf(pkt->ifname, sizeof(pkt->ifname), "%s", interface->ifname);
        memcpy(pkt->ifaddr, interface->addr, ETH_ALEN);
        memcpy(pkt->aladdr, interface->al_addr, ETH_ALEN);
        mbus_sendmsg_in_group(g_mesh_io, g_mesh_user_name, g_mesh_group_name, (char *)pkt, packet_length(pkt));
        packet_release(pkt);
        break;
    }
    case IF_RECV:
    {
        printf("filter cmdu before recv\n");
        mesh_packet_t *pkt = packet_create(data, len);
        if (!pkt)
        {
            printf("pkt create err\n");
            return -1;
        }
        pkt->from = PKT_NETWORK;
        pkt->to = PKT_LOCAL;
        snprintf(pkt->ifname, sizeof(pkt->ifname), "%s", interface->ifname);
        memcpy(pkt->ifaddr, interface->addr, ETH_ALEN);
        memcpy(pkt->aladdr, interface->al_addr, ETH_ALEN);
        mbus_sendmsg_in_group(g_mesh_io, g_mesh_user_name, g_mesh_group_name, (char *)pkt, packet_length(pkt));
        packet_release(pkt);
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
    char data[2048] = {0};

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

static void _mesh_handle_heartbeat(mbus_t *mbus, char *from, char *to, void *data, int len)
{
    printf("heart beat pong\n");
}

void mesh_handle_packet(mesh_packet_t *pkt)
{
    forward_cmdu(pkt);
}

static void _mesh_handle_group_msg(mbus_t *mbus, char *from, char *to, void *data, int len)
{
    printf("group msg from %s to %s, len=%d\n", from, to, len);
    if (0 == strcmp(to, g_mesh_group_name))
    {
        mesh_handle_packet((mesh_packet_t *)data);
    }
}

static void _mesh_handle_private_msg(mbus_t *mbus, char *from, char *to, void *data, int len)
{
    printf("private msg from %s to %s, len=%d\n", from, to, len);
    if (0 != strcmp(to, g_mesh_user_name))
    {

        return;
    }

    mesh_msg_t *mesh_msg = (mesh_msg_t *)data;
    switch (mesh_msg->mesh_msg_id)
    {
    case MESH_MSGCMD_CONTROLLER_INFO_REQ:
    {
        char buffer[128] = {0};
        mesh_msg = (mesh_msg_t *)buffer;
        controller_info_t *info = (controller_info_t *)mesh_msg->mesh_msg_body;
        mesh_msg->mesh_msg_id = MESH_MSGCMD_CONTROLLER_INFO_RSP;
        nbr_1905dev *dev_controller = find_controller(info->local_ifaddr, info->local_ifname, info->my_al_mac);
        if (dev_controller)
        {
            memcpy(info->controller_al_mac, dev_controller->al_addr, ETH_ALEN);
            mbus_sendmsg_private(mbus->io, to, from, (char *)mesh_msg, sizeof(mesh_msg_t) + sizeof(controller_info_t));
        }
        break;
    }
    default:
        break;
    }
}

void mesh_handle_msg(mbus_t *mbus, int cmd, char *from, char *to, void *data, int len)
{
    switch (cmd)
    {
    case MBUS_CMD_HEARTBEAT:
        _mesh_handle_heartbeat(mbus, from, to, data, len);
        break;
    case MBUS_CMD_GROUP_MSG:
        _mesh_handle_group_msg(mbus, from, to, data, len);
        break;
    case MBUS_CMD_PRIVATE_MSG:
    {
        _mesh_handle_private_msg(mbus, from, to, data, len);
        break;
    }
    default:
        break;
    }
}

void mesh_obj_create_timer_cb(timer_entry_t *te)
{

    io_buf_t *io = (io_buf_t *)te->privdata;

    // login
    mbus_register_object(io, g_mesh_user_name);

    mbus_create_group(io, g_mesh_group_name);

    mbus_join_group(io, g_mesh_group_name);

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

    mbus_t mesh_mbus;
    memset(&mesh_mbus, 0, sizeof(mbus_t));
    mbus_io_init(&mesh_mbus, &loop->ios[sockfd], sockfd, NULL, mesh_handle_msg);

    add_timer(&loop->timer, 1, mesh_obj_create_timer_cb, &loop->ios[sockfd]);
    g_mesh_io = &loop->ios[sockfd];
    // 进入事件主循环
    eloop_run(loop);
    eloop_destroy(loop);
    if_release(&interface);

    return 0;
}