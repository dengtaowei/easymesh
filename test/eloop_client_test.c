#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

typedef struct msg_s
{
    int type;
    int len;
    char value[];
} msg_t;

static int
init_client_sock(char *ip, char *port)
{

    struct addrinfo hint, *result;
    int res, sfd;

    memset(&hint, 0, sizeof(hint));
    hint.ai_family = AF_INET;
    hint.ai_socktype = SOCK_STREAM;

    res = getaddrinfo(ip, port, &hint, &result);
    if (res == -1)
    {
        perror("error : cannot get socket address!\n");
        return -1;
    }

    sfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sfd == -1)
    {
        perror("error : cannot get socket file descriptor!\n");
        return -1;
    }

    res = connect(sfd, result->ai_addr, result->ai_addrlen);
    if (res == -1)
    {
        perror("error : cannot connect the server!\n");
        return -1;
    }

    return sfd;
}

int main(int argc, char *argv[])
{
    int clientfd = init_client_sock("127.0.0.1", "8848");
    if (clientfd < 0)
    {
        printf("connect error %s\n", strerror(errno));
    }
    char buffer[1024] = {0};
    int len = 0;
    msg_t *msg = (msg_t *)buffer;
    if (0 == strcmp(argv[1], "0.6"))
    {
        msg->type = 1;
        len += sizeof(msg_t);
        msg->len = 12;
        len += msg->len;
        snprintf(msg->value, msg->len, "%s", "hello world");
        while (1)
        {
            int sent = send(clientfd, buffer, len / 10 * 6, 0);
            printf("sent: %d\n", sent);
            usleep(500000);
            sent = send(clientfd, buffer + len / 10 * 6, len / 10 * 4, 0);
            printf("sent: %d\n", sent);
            
            msg->type++;
        }
    }
    if (0 == strcmp(argv[1], "1"))
    {
        msg->type = 1;
        len += sizeof(msg_t);
        msg->len = 12;
        len += msg->len;
        snprintf(msg->value, msg->len, "%s", "hello world");
        while (1)
        {
            int sent = send(clientfd, buffer, len, 0);
            msg->type++;
            printf("sent: %d\n", sent);
            sleep(1);
        }
    }
    if (0 == strcmp(argv[1], "2"))
    {

        msg->type = 1;
        len += sizeof(msg_t);
        msg->len = 12;
        len += msg->len;
        snprintf(msg->value, msg->len, "%s", "hello world");

        msg = (msg_t *)(buffer + len);
        msg->type = 2;
        len += sizeof(msg_t);
        msg->len = 13;
        len += msg->len;
        snprintf(msg->value, msg->len, "%s", "hello world2");
        while (1)
        {
            int sent = send(clientfd, buffer, len, 0);
            msg->type++;
            printf("sent: %d\n", sent);
            usleep(100000);
        }
    }
    return 0;
}