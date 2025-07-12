#include <string.h>
#include <stdio.h>
#include "user_manager.h"

static KamiList g_clients = {
    .head = NULL,
    .tail = NULL,
    .size = 0};

client_t *user_create(const char *name)
{
    client_t *cli = malloc(sizeof(client_t));
    if (!cli)
    {
        return NULL;
    }
    memset(cli, 0, sizeof(client_t));

    snprintf(cli->name, sizeof(cli->name), "%s", name);
    KamiListAddTail(&g_clients, &cli->cli_node);
    return cli;
}

client_t *user_get_by_name(const char *name)
{
    KamiListIterrator *iter = KamiListGetIter(&g_clients, Iter_From_Head);
    KamiListNode *tmp = NULL;
    while ((tmp = KamiListNext(iter)) != NULL)
    {
        client_t *cli = container_of(tmp, client_t, cli_node);
        if (0 == strcmp(cli->name, name))
        {
            KamiListDelIterator(iter);
            return cli;
        }
    }
    KamiListDelIterator(iter);
    return NULL;
}

int user_delete(const char *name)
{
    client_t *cli = user_get_by_name(name);
    if (!cli)
    {
        return 0;
    }
    KamiListDel(&g_clients, &cli->cli_node);
    printf("del user %s\n", cli->name);
    free(cli);
    return 0;
}