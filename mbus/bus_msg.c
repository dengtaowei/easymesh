#include <stdio.h>
#include "bus_msg.h"
#include "eloop_event.h"

uint16_t msg_get_command_id(msg_t *msg)
{
    return msg->command_id;
}

char *msg_get_data(msg_t *msg)
{
    return (char *)msg->body;
}

int mbus_register_object(io_buf_t *io, const char *username)
{
    if (!io || !username || username[0] == '\0')
    {
        return -1;
    }
    // login
    char buffer[sizeof(msg_t) + sizeof(msg_login_t)] = {0};
    msg_t *msg = (msg_t *)buffer;
    msg_login_t *login_msg = (msg_login_t *)msg->body;
    msg->command_id = CID_LOGIN_REQ_USERLOGIN;
    snprintf((char *)login_msg->username, sizeof(login_msg->username), "%s", username);
    msg->length = sizeof(msg_login_t);
    int nsend = io_write(io, (char *)msg, sizeof(msg_t) + msg->length);
    printf("send %d\n", nsend);

    return 0;
}

int mbus_create_group(io_buf_t *io, const char *groupname)
{
    if (!io || !groupname || groupname[0] == '\0')
    {
        return -1;
    }
    char buffer[sizeof(msg_t) + sizeof(msg_create_group_t)] = {0};
    msg_t *msg = (msg_t *)buffer;
    msg_create_group_t *login_msg = (msg_create_group_t *)msg->body;
    msg->command_id = CID_GROUP_CREATE_GROUP;
    snprintf((char *)login_msg->groupname, sizeof(login_msg->groupname), "%s", groupname);
    msg->length = sizeof(msg_create_group_t);
    int nsend = io_write(io, (char *)msg, sizeof(msg_t) + msg->length);
    printf("send %d\n", nsend);

    return 0;
}

int mbus_join_group(io_buf_t *io, const char *groupname)
{
    if (!io || !groupname || groupname[0] == '\0')
    {
        return -1;
    }
    char buffer[sizeof(msg_t) + sizeof(msg_join_group_t)] = {0};
    msg_t *msg = (msg_t *)buffer;
    msg_join_group_t *login_msg = (msg_join_group_t *)msg->body;
    msg->command_id = CID_GROUP_JOIN_GROUP;
    snprintf((char *)login_msg->groupname, sizeof(login_msg->groupname), "%s", groupname);
    msg->length = sizeof(msg_join_group_t);
    int nsend = io_write(io, (char *)msg, sizeof(msg_t) + msg->length);
    printf("send %d\n", nsend);

    return 0;
}

static msg_t *group_msg_new(const char *groupname, void *data, int len)
{
    msg_t *msg = malloc(sizeof(msg_t) + sizeof(group_msg_t) + len);
    if (!msg)
    {
        return NULL;
    }
    memset(msg, 0, sizeof(msg_t) + sizeof(group_msg_t) + len);
    group_msg_t *group_msg = (group_msg_t *)msg->body;
    if (len)
    {
        memcpy(group_msg->group_msg, data, len);
    }
    snprintf(group_msg->group_name, sizeof(group_msg->group_name), "%s", groupname);
    group_msg->group_msg_len = len;
    msg->length = sizeof(group_msg_t) + group_msg->group_msg_len;
    return msg;
}

static void group_msg_delete(msg_t *msg)
{
    if (msg)
    {
        free(msg);
    }
}

int mbus_sendmsg_in_group(io_buf_t *io, const char *groupname, char *data, int len)
{
    if (!io || !groupname || len <= 0)
    {
        return -1;
    }
    msg_t *msg = group_msg_new(groupname, data, len);
    msg->command_id = CID_GROUP_MSG;
    if (!msg)
    {
        return -1;
    }

    int nsend = io_write(io, (char *)msg, sizeof(msg_t) + msg->length);
    printf("send %d\n", nsend);
    group_msg_delete(msg);
    return 0;
}