#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
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
#include <event2/buffer.h>

#include <string.h>
#ifndef _WIN32
#include <signal.h>
#else
#endif

#include "ieee1905_network.h"
#include "cmdu.h"



// 错误，超时 （连接断开会进入）
void client_event_cb(struct bufferevent *be, short events, void *arg)
{
    // cout << "[client_E]" << flush;
    printf("client_E\n");
    // 读取超时时间发生后，数据读取停止
    if (events & BEV_EVENT_TIMEOUT && events & BEV_EVENT_READING)
    {
        // cout << "BEV_EVENT_READING BEV_EVENT_TIMEOUT" << endl;
        printf("BEV_EVENT_READING BEV_EVENT_TIMEOUT\n");
        // bufferevent_enable(be,EV_READ);
        bufferevent_free(be);
        return;
    }
    else if (events & BEV_EVENT_ERROR)
    {
        bufferevent_free(be);
        return;
    }
    // 服务端的关闭事件
    if (events & BEV_EVENT_EOF)
    {
        // cout << "BEV_EVENT_EOF" << endl;
        printf("BEV_EVENT_EOF\n");
        bufferevent_free(be);
    }
    if (events & BEV_EVENT_CONNECTED)
    {
        // cout << "BEV_EVENT_CONNECTED" << endl;
        printf("BEV_EVENT_CONNECTED\n");
        // 触发write
        bufferevent_trigger(be, EV_WRITE, 0);
    }
}
void client_write_cb(struct bufferevent *be, void *arg)
{
    // cout << "[client_W]" << flush;
    printf("client_W\n");
    FILE *fp = (FILE *)arg;
    char data[1024] = {0};
    int len = fread(data, 1, sizeof(data) - 1, fp);
    if (len <= 0)
    {
        // 读到结尾或者文件出错
        fclose(fp);
        // 立刻清理，可能会造成缓冲数据没有发送结束
        // bufferevent_free(be);
        bufferevent_disable(be, EV_WRITE);
        return;
    }
    // 写入buffer
    // bufferevent_write(be, data, len);
}

void client_read_cb(struct bufferevent *bev, void *arg)
{
    // cout << "[client_R]" << flush;
    printf("client_R\n");
    struct evbuffer *input = bufferevent_get_input(bev);
    struct evbuffer *output = bufferevent_get_output(bev);
    size_t len = evbuffer_get_length(input);
    char *data;
    data = (char *)malloc(len);
    evbuffer_remove(input, data, len);
    cmdu_raw_msg *msg = (cmdu_raw_msg *)data;
    unsigned short type = ntohs(msg->msg_1905.hdr.msg_type);
    if (type == MSG_TOPOLOGY_DISCOBERY)
    {
        printf("topo discovery received from %02x:%02x:%02x:%02x:%02x:%02x\n",
            msg->src_addr[0], msg->src_addr[1], msg->src_addr[2], msg->src_addr[3], 
            msg->src_addr[4], msg->src_addr[5]);
            send_topology_query((NetworkInterface *)arg, msg->src_addr, msg->msg_1905.hdr.msg_id);
    }
    else if (type == MSG_TOPOLOGY_QUERY)
    {
        printf("topo query received from %02x:%02x:%02x:%02x:%02x:%02x\n",
            msg->src_addr[0], msg->src_addr[1], msg->src_addr[2], msg->src_addr[3], 
            msg->src_addr[4], msg->src_addr[5]);
        send_topology_response((NetworkInterface *)arg, msg->src_addr, msg->msg_1905.hdr.msg_id);
    }
    free(data);
}
struct event* timer_ev = NULL;
void send_discovery_perodic(int sockfd, short what, void* arg) {

    int ret = send_topology_discovery((NetworkInterface *)arg);
    if (ret < 0)
    {
        printf("sk send error\n");
        return;
    }
    struct timeval t1 = { 45, 0 };	// 1秒0毫秒
    if (!evtimer_pending(timer_ev, &t1)) {
		evtimer_del(timer_ev);
		evtimer_add(timer_ev, &t1);
	}
    return;
}
int main(int argc, char *argv[])
{
    // 忽略管道信号，发送数据给已关闭的socket
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR)
        return 1;

    struct event_base *base = event_base_new();
    // 创建网络服务器

    int ret = 0;
    NetworkInterface interface;
    memset(&interface, 0, sizeof(interface));
    interface.addr[0] = 0x00;   // 00:0c:29:09:78:b7
    interface.addr[1] = 0x0c;   // 00:0c:29:09:78:b7
    interface.addr[2] = 0x29;   // 00:0c:29:09:78:b7
    interface.addr[3] = 0x09;   // 00:0c:29:09:78:b7
    interface.addr[4] = 0x78;   // 00:0c:29:09:78:b7
    interface.addr[5] = 0xb7;   // 00:0c:29:09:78:b7
    snprintf(interface.ifname, sizeof(interface.ifname), "%s", "ens32");
    ret = if_sock_create(&interface);
    if (ret)
    {
        printf("sk create error\n");
        return -1;
    }


    //定时器，非持久事件
	timer_ev = evtimer_new(base, send_discovery_perodic, &interface);
	if (!timer_ev) {
		printf("timer error\n");
		return 1;
	}
	struct timeval t1 = { 1, 0 };	// 1秒0毫秒
	evtimer_add(timer_ev, &t1); // 插入性能 O(logn)

    

    struct bufferevent *bev = bufferevent_socket_new(base, interface.fd, 0);
    if (!bev)
    {
        perror("bufferevent_new");
        return 1;
    }

    bufferevent_setcb(bev, client_read_cb, client_write_cb, client_event_cb, &interface);
    bufferevent_enable(bev, EV_READ);

    // 进入事件主循环
    event_base_dispatch(base);
    event_base_free(base);
    if_sock_destroy(&interface);


    return 0;
}