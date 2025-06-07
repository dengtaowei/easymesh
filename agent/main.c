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
#include "core.h"

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
    char data[1024] = {0};
    
    NetworkInterface *interface = (NetworkInterface *)arg;
    int recv_b = if_recv(interface, data, sizeof(data));
    if(recv_b <= 0)
    {
        printf("recv err\n");
        return;
    }

    printf("=========== recv tytes %d===========\n", recv_b);
    
    cmdu_handle(interface, data, recv_b);
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
 	// 5a:9a:94:c0:42:57
    interface.al_addr[0] = 0x5a;
    interface.al_addr[1] = 0x9a;
    interface.al_addr[2] = 0x94;
    interface.al_addr[3] = 0xc0;
    interface.al_addr[4] = 0x42;
    interface.al_addr[5] = 0x57;
    
    ret = if_create(&interface, "ens32", (void *)base);
    if (ret)
    {
        printf("sk create error\n");
        return -1;
    }

    register_interface(&interface);

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
    if_release(&interface);

    return 0;
}