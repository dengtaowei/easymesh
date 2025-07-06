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

typedef struct msg_s
{
    int type;
    int len;
    char value[];
} msg_t;


void handle_read(io_buf_t *io)
{
    msg_t *msg = (msg_t *)io_data(io);
    printf("type: %d, len: %d, value: %s\n", msg->type, msg->len, msg->value);
    usleep(100000);
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
        eloop_set_event(&io->loop->ios[clientfd], clientfd, io->loop);
        eloop_setcb_read(&io->loop->ios[clientfd], handle_read);
        io_add_packer(&io->loop->ios[clientfd], 8, 4);
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