#include <string.h>
#include <stdio.h>
#include "group_manager.h"


static KamiList g_groups = {
    .head = NULL,
    .tail = NULL,
    .size = 0};

group_t *group_create(const char *name)
{
    group_t *group = malloc(sizeof(group_t));
    if (!group)
    {
        return NULL;
    }
    memset(group, 0, sizeof(group_t));

    snprintf(group->group_name, sizeof(group->group_name), "%s", name);
    KamiListAddTail(&g_groups, &group->group_node);
    return group;
}

group_t *search_group(const char *groupname)
{
    KamiListIterrator *iter = KamiListGetIter(&g_groups, Iter_From_Head);
    KamiListNode *tmp = NULL;
    while ((tmp = KamiListNext(iter)) != NULL)
    {
        group_t *group = container_of(tmp, group_t, group_node);
        if (0 == strcmp(groupname, group->group_name))
        {
            KamiListDelIterator(iter);
            return group;
        }
    }
    KamiListDelIterator(iter);
    return NULL;
}

KamiListNode *search_item_in_group(group_t *group, const char *username)
{
    KamiListIterrator *iter = KamiListGetIter(&group->users, Iter_From_Head);
    KamiListNode *tmp = NULL;
    while ((tmp = KamiListNext(iter)) != NULL)
    {
        char *user_name = (char *)tmp->data;
        if (0 == strcmp(user_name, username))
        {
            KamiListDelIterator(iter);
            return tmp;
        }
    }
    KamiListDelIterator(iter);
    return NULL;
}

int join_group(const char *username, const char *groupname)
{
    if (!username || !groupname)
    {
        return -1;
    }
    group_t *group = search_group(groupname);
    if (!group)
    {
        return -1;
    }

    KamiListNode *node = search_item_in_group(group, username);
    if (node)
    {
        printf("user %s already in the group %s\n", username, groupname);
        return -1;
    }

    node = malloc(sizeof(KamiListNode));
    if (!node)
    {
        return -1;
    }
    memset(node, 0, sizeof(KamiListNode));

    node->data = malloc(strlen(username) + 1);
    if (!node->data)
    {
        free(node);
        return -1;
    }
    snprintf(node->data, strlen(username) + 1, "%s", username);
    KamiListAddTail(&group->users, node);
    
    return 0;
}

int leave_group_by_name(const char *username, const char *groupname)
{
    if (!username || !groupname)
    {
        return -1;
    }

    group_t *group = search_group(groupname);
    if (!group)
    {
        return -1;
    }

    KamiListNode *node = search_item_in_group(group, username);
    if (!node)
    {
        printf("user %s not in the group %s\n", username, groupname);
        return -1;
    }

    KamiListDel(&group->users, node);
    free(node->data);
    free(node);
    
    return 0;
}

int leave_group(const char *username, group_t *group)
{
    if (!username || !group)
    {
        return -1;
    }

    KamiListNode *node = search_item_in_group(group, username);
    if (!node)
    {
        printf("user %s not in the group %s\n", username, group->group_name);
        return -1;
    }
    KamiListDel(&group->users, node);
    free(node->data);
    free(node);
    return 0;
}

int leave_all_group(const char *username)
{
    KamiListIterrator *iter = KamiListGetIter(&g_groups, Iter_From_Head);
    KamiListNode *tmp = NULL;
    while ((tmp = KamiListNext(iter)) != NULL)
    {
        group_t *group = container_of(tmp, group_t, group_node);
        int ret = leave_group(username, group);
        if (ret == 0)
        {
            printf("user %s leave group %s\n", username, group->group_name);
        }
    }
    KamiListDelIterator(iter);
    return 0;
}


void foreach_group_member(const char *groupname, void (*cb)(void *, void *), void *arg)
{
    if (!groupname || !cb)
    {
        return;
    }

    group_t *group = search_group(groupname);
    if (!group)
    {
        return;
    }

    KamiListIterrator *iter = KamiListGetIter(&group->users, Iter_From_Head);
    KamiListNode *tmp = NULL;
    while ((tmp = KamiListNext(iter)) != NULL)
    {
        char *user_name = (char *)tmp->data;
        cb(user_name, arg);
    }
    KamiListDelIterator(iter);
   
    return;
}