#include "bus_msg.h"

uint16_t msg_get_command_id(msg_t *msg)
{
    return msg->command_id;
}

char *msg_get_data(msg_t *msg)
{
    return (char *)msg->body;
}