#include <sys/epoll.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include "eloop_event.h"
#include "mh-timer.h"

void io_add_packer(io_buf_t *io, int nheader, int body_offset)
{
    io->pack.nheader = nheader;
    io->pack.len_offset = body_offset;
    io->has_packer = 1;
}

const char *io_data(io_buf_t *io)
{
    return io->buf + io->head;
}

static int __read_cb(io_buf_t *io, char *buf, int len)
{
    int nhandle = 0;
    while (len >= io->pack.nheader)  // 只要有包头可以读
    {
        int nbody = *((int *)(buf + io->pack.len_offset));
        if (len < nbody + io->pack.nheader)  // 不是一个完整的包
        {
            break;
        }
        io->on_read(io);  // 默认已经有一个完整的包被处理

        buf += nbody + io->pack.nheader;
        io->head += nbody + io->pack.nheader;
        io->len -= nbody + io->pack.nheader;
        len -= nbody + io->pack.nheader;
    }

    return nhandle;
}

static void handle_read(io_buf_t *io)
{
    void *buf = io->buf + io->tail;
    int len = IO_BUF_SIZE - io->tail;
    if (len <= 0)
    {
        printf("no more buf\n");
        eloop_reset_io(io);
        return;
    }
    int nget = recv(io->fd, buf, len, 0);
    if (nget < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return; 
        }
        else
        {
            printf("fatal error %s\n", strerror(errno));
            return;
        }
    }
    else if (nget == 0)
    {
        printf("peer close %d\n", io->fd);
        eloop_reset_io(io);
        return;
    }
    io->tail += nget;
    io->len += nget;

    printf("recv %d\n", nget);

    __read_cb(io, io->buf + io->head, io->len);

    if (io->tail >= IO_BUF_SIZE)
    {
        memmove(io->buf, io->buf + io->head, io->len);
        io->head = 0;
        io->tail = io->len;
    }

}

static void eloop_handle_data(io_buf_t *io)
{
    if (io->what & EPOLLIN)
    {
        if (io->is_accept)
        {
            io->on_accept(io);
        }
        else
        {
            if (io->has_packer)
            {
                handle_read(io);
            }
            else
            {
                io->on_read(io);
            }
        }
    }

    if (io->what & EPOLLOUT)
    {
        io->on_write(io);
    }
}

eloop_t *eloop_create()
{
    eloop_t *loop = malloc(sizeof(eloop_t));
    if (!loop)
    {
        return NULL;
    }
    memset(loop, 0, sizeof(eloop_t));
    loop->epfd = epoll_create(64);
    if (loop->epfd < 0)
    {
        free(loop);
        return NULL;
    }
    loop->ios = malloc(MAX_EPOLL_EVENTS * sizeof(io_buf_t));
    if (!loop->ios)
    {
        close(loop->epfd);
        free(loop);
        return NULL;
    }
    for (int i = 0; i < MAX_EPOLL_EVENTS; i++)
    {
        loop->ios[i].loop = loop;
    }
    init_timer(&loop->timer);
    return loop;
}

void eloop_set_event(io_buf_t *io, int fd, void *arg)
{
    io->fd = fd;
    io->arg = arg;
    if (!io->buf)
    {
        io->buf = malloc(IO_BUF_SIZE);
    }
}

void eloop_reset_io(io_buf_t *io)
{
    close(io->fd);
    if (io->buf)
    {
        free(io->buf);
    }
    eloop_t *loop = io->loop;
    memset(io, 0, sizeof(io_buf_t));
    io->loop = loop;
}

void eloop_add_event(io_buf_t *io, int events)
{
    struct epoll_event ee;
    memset(&ee, 0, sizeof(ee));
    ee.events = io->events;
    ee.events |= events;
    ee.data.ptr = io;
    if (0 == io->events)
    {
        epoll_ctl(io->loop->epfd, EPOLL_CTL_ADD, io->fd, &ee);
    }
    else
    {
        epoll_ctl(io->loop->epfd, EPOLL_CTL_MOD, io->fd, &ee);
    }
    io->events = ee.events;
}

void eloop_del_event(io_buf_t *io, int events)
{
    struct epoll_event ee;
    memset(&ee, 0, sizeof(ee));
    ee.events = io->events;
    ee.events &= (~events);
    if (0 == ee.events)
    {
        epoll_ctl(io->loop->epfd, EPOLL_CTL_DEL, io->fd, &ee);
    }
    else
    {
        epoll_ctl(io->loop->epfd, EPOLL_CTL_MOD, io->fd, &ee);
    }
    io->events = ee.events;
}

void eloop_setcb_accept(io_buf_t *io, cb_accept on_accept)
{
    io->on_accept = on_accept;
    io->is_accept = 1;
}

void eloop_setcb_read(io_buf_t *io, cb_read on_read)
{
    io->on_read = on_read;
}

void eloop_setcb_write(io_buf_t *io, cb_write on_write)
{
    io->on_write = on_write;
}

void eloop_destroy(eloop_t *loop)
{
    if (loop)
    {
        if (loop->epfd)
        {
            close(loop->epfd);
        }
        for (int i = 0; i < MAX_EPOLL_EVENTS; i++)
        {
            if (loop->ios[i].buf)
            {
                free(loop->ios[i].buf);
            }
        }
        if (loop->ios)
        {
            free(loop->ios);
        }
        deinit_timer(&loop->timer);
        free(loop);
    }

    return;
}

int eloop_run(eloop_t *loop)
{
    struct epoll_event events[MAX_EPOLL_EVENTS + 1];
    memset(events, 0, sizeof(events));
    while (1)
    {
        int nearest = find_nearest_expire_timer(&loop->timer);
        int nready = epoll_wait(loop->epfd, events, MAX_EPOLL_EVENTS, nearest);
        if (nready < 0)
        {
            continue;
        }

        for (int idx = 0; idx < nready; idx++)
        {
            io_buf_t *io = (io_buf_t *)events[idx].data.ptr;
            io->what = 0;
            if (events[idx].events & EPOLLIN)
            {
                io->what |= EPOLLIN;
            }
            if (events[idx].events & EPOLLOUT)
            {
                io->what |= EPOLLOUT;
            }
            eloop_handle_data(io);
        }
        expire_timer(&loop->timer);
    }
}

int eloop_tcp_server_init(int port)
{
    int opt = 1;
    int ret = 0;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(fd, F_SETFL, O_NONBLOCK);

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (ret < 0)
    {
        close(fd);
        return -1;
    }

    ret = bind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (ret < 0)
    {
        printf("%s\n", strerror(errno));
        close(fd);
        return -1;
    }

    if (listen(fd, 20) < 0)
    {
        close(fd);
        return -1;
    }
    return fd;
}

int eloop_addlistener(eloop_t *loop, int fd, cb_accept cb)
{
    eloop_set_event(&loop->ios[fd], fd, loop);
    eloop_setcb_accept(&loop->ios[fd], cb);
    eloop_add_event(&loop->ios[fd], EPOLLIN);
    return 0;
}