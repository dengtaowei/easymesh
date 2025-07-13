#ifndef _ELOOP_EVENT_H_
#define _ELOOP_EVENT_H_
#include "minheap.h"
#include "list.h"

typedef struct io_buf_s io_buf_t;
typedef struct eloop_s eloop_t;
typedef struct io_pack_s io_pack_t;
typedef struct io_write_queue_s io_write_queue_t;

typedef void (*cb_accept)(io_buf_t *io);
typedef void (*cb_read)(io_buf_t *io);
typedef void (*cb_write)(io_buf_t *io);
typedef void (*cb_event)(io_buf_t *io);
typedef void (*cb_close)(io_buf_t *io);

#define IO_BUF_SIZE 8192
#define MAX_EPOLL_EVENTS 1024
#define MAX_WMEM_SIZE 16384

struct io_pack_s
{
    int nheader;
    int len_offset;
};

struct io_write_queue_s
{
    char *data;
    int len;
    int offset;
    KamiListNode node;
};

struct io_buf_s
{
    int fd;
    char *buf;
    int len;
    int head;
    int tail;
    cb_accept on_accept;
    cb_read on_read;
    cb_write on_write;
    cb_close on_close;
    // cb_event handler;
    eloop_t *loop;
    void *arg;
    unsigned int events;
    unsigned int what;
    int is_accept;
    io_pack_t pack;
    int has_packer;
    KamiList write_queue;
    int wmem_max_size;
    int wmem_size;
};

struct eloop_s
{
    int epfd;
    io_buf_t *ios;
    min_heap_t timer;
};


typedef struct eloop_tcp_cli_s eloop_tcp_cli_t;

struct eloop_tcp_cli_s
{
    /* data */
};


const char *io_data(io_buf_t *io);

void io_add_packer(io_buf_t *io, int nheader, int body_offset);

int io_write(io_buf_t *io, char *data, int len);

eloop_t* eloop_create();

void eloop_set_event(io_buf_t *io, int fd, void *arg);

void eloop_reset_io(io_buf_t *io);

void eloop_add_event(io_buf_t *io, int events);

void eloop_del_event(io_buf_t *io, int events);

void eloop_setcb_accept(io_buf_t *io, cb_accept on_accept);

void eloop_setcb_read(io_buf_t *io, cb_read on_read);

void eloop_setcb_write(io_buf_t *io, cb_write on_write);

void eloop_setcb_close(io_buf_t *io, cb_close on_close);

void eloop_destroy(eloop_t *loop);

int eloop_run(eloop_t *loop);

int eloop_tcp_server_init(int port);

eloop_tcp_cli_t *eloop_tcp_client_init();

void eloop_tcp_client_destroy(eloop_tcp_cli_t *cli);

int eloop_tcp_connect(eloop_tcp_cli_t *cli, const char *host, const char *port);

int eloop_addlistener(eloop_t *loop, int fd, cb_event cb);

#endif
