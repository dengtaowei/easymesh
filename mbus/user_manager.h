#ifndef __USER_MANAGER_H__
#define __USER_MANAGER_H__
#include "list.h"
#include "eloop_event.h"

typedef struct client_s client_t;


struct client_s
{
    io_buf_t *io;
    char name[128];
    KamiListNode cli_node;
};

client_t *user_create(const char *name);

client_t *user_get_by_name(const char *name);

int user_delete(const char *name);

#endif