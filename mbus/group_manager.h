#ifndef __GROUP_MANAGER_H__
#define __GROUP_MANAGER_H__
#include "list.h"

typedef struct group_s group_t;

struct group_s
{
    char group_name[128];
    KamiList users;
    KamiListNode group_node;
};

group_t *group_create(const char *name);

group_t *search_group(const char *groupname);

int join_group(const char *username, const char *groupname);

int leave_group_by_name(const char *username, const char *groupname);

int leave_all_group(const char *username);

void foreach_group_member(const char *groupname, void (*cb)(void *, void *), void *arg);

#endif