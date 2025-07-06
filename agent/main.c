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


void client_read_cb(io_buf_t *io)
{
    // cout << "[client_R]" << flush;
    printf("client_R\n");
    char data[1024] = {0};
    
    NetworkInterface *interface = (NetworkInterface *)io->arg;
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

    eloop_set_event(&loop->ios[interface.fd], interface.fd, &interface);
    eloop_setcb_read(&loop->ios[interface.fd], client_read_cb);
    eloop_add_event(&loop->ios[interface.fd], EPOLLIN);

    // 进入事件主循环
    eloop_run(loop);
    eloop_destroy(loop);
    if_release(&interface);

    return 0;
}