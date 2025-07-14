#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "list.h"

KamiListIterrator *KamiListGetIter(KamiList *list, int direction)
{
    KamiListIterrator *iter = (KamiListIterrator *)malloc(sizeof(KamiListIterrator));

    if (!iter)
    {
        return NULL;
    }

    iter->direction = direction;

    if (direction == Iter_From_Head)
    {
        iter->next = list->head;
    }
    else
    {
        iter->next = list->tail;
    }
    return iter;
}

int KamiListIterInit(KamiList *list, KamiListIterrator *iter, int direction)
{
    if (!iter || !list)
    {
        return -1;
    }
    iter->direction = direction;

    if (direction == Iter_From_Head)
    {
        iter->next = list->head;
    }
    else
    {
        iter->next = list->tail;
    }
    return 0;
}

KamiListNode *KamiListNext(KamiListIterrator *iter)
{
    KamiListNode *cur = iter->next;
    if (cur == NULL)
    {
        return NULL;
    }
    if (iter->direction == Iter_From_Head)
    {
        iter->next = iter->next->next;
    }
    else
    {
        iter->next = iter->next->prev;
    }

    return cur;
}

void KamiListRewind(KamiList *list, KamiListIterrator *iter)
{
    if (iter->direction == Iter_From_Head)
    {
        iter->next = list->head;
    }
    else
    {
        iter->next = list->tail;
    }
}

void KamiListDelIterator(KamiListIterrator *iter)
{
    if (iter)
        free(iter);
}

// 找到返回 1，未找到返回 0
int KamiListContains(KamiList *list, KamiListNode *item)
{
    if (!list || !item || list->size == 0)
    {
        return 0;
    }
    int ret = 0;
    KamiListIterrator *iter = KamiListGetIter(list, Iter_From_Head);
    KamiListNode *node = NULL;
    while ((node = KamiListNext(iter)) != NULL)
    {
        if (node == item)
        {
            ret = 1;
            break;
        }
    }
    KamiListDelIterator(iter);
    return ret;
}

int KamiListAddHead(KamiList *list, KamiListNode *item)
{
    if (!list || !item || item->list == list)
        return -1;
    if (list->size == 0)
    {
        item->prev = item->next = NULL;
        list->head = list->tail = item;
        list->size++;
        item->list = list;
        return 0;
    }
    item->prev = NULL;
    item->next = list->head;
    list->head->prev = item;
    list->head = item;
    list->size++;
    item->list = list;
    return 0;
}

int KamiListAddTail(KamiList *list, KamiListNode *item)
{
    if (!list || !item || item->list == list)
        return -1;
    if (list->size == 0)
    {
        item->prev = item->next = NULL;
        list->head = list->tail = item;
        list->size++;
        item->list = list;
        return 0;
    }

    item->next = NULL;
    item->prev = list->tail;
    list->tail->next = item;
    list->tail = item;
    list->size++;
    item->list = list;
    return 0;
}

int KamiListAddAfter(KamiList *list, KamiListNode *prev, KamiListNode *item)
{
    if (!list || !prev || !item || prev->list != list ||item->list == list)
    {
        return -1;
    }

    item->list = list;
    item->prev = prev;
    item->next = prev->next;
    if (prev->next)
        prev->next->prev = item;
    prev->next = item;
    if (prev == list->tail)
    {
        list->tail = item;
    }
    list->size++;

    return 0;
}

int KamiListAddBefore(KamiList *list, KamiListNode *next, KamiListNode *item)
{
    if (!list || !next || !item || next->list != list || item->list == list)
    {
        return -1;
    }

    item->list = list;
    item->next = next;
    item->prev = next->prev;
    if (next->prev)
        next->prev->next = item;
    next->prev = item;
    if (next == list->head)
    {
        list->head = item;
    }
    list->size++;
    
    return 0;
}

//  0  1  2 list->size = 3
// -3 -2 -1
// i < -list->size || i >= list->size
KamiListNode *KamiListAt(KamiList *list, int i)
{
    if (!list || i < -list->size || i >= list->size)
        return NULL;
    if (i < 0)
    {
        i = list->size + i;
    }
    KamiListNode *tmp = list->head;
    while (tmp && i)
    {
        tmp = tmp->next;
        --i;
    }
    return tmp;
}

int KamiListInsertAt(KamiList *list, KamiListNode *item, int i)
{
    if (!list || !item || item->list == list)
    {
        return -1;
    }
    if (i == list->size)
    {
        KamiListAddTail(list, item);
        return 0;
    }
    KamiListNode *next = KamiListAt(list, i);
    if (!next)
    {
        return -1;
    }
    KamiListNode *prev = next->prev;
    item->next = next;
    item->prev = prev;
    item->list = list;
    if (prev)
        prev->next = item;
    if (next)
        next->prev = item;
    if (next == list->head)
    {
        list->head = item;
    }
    list->size++;
    return 0;
}

KamiListNode *KamiListHead(KamiList *list)
{
    if (!list)
        return NULL;
    return list->head;
}

KamiListNode *KamiListTail(KamiList *list)
{
    if (!list)
        return NULL;
    return list->tail;
}

int KamiListDel(KamiList *list, KamiListNode *item)
{
    if (!list || !item || list->size == 0 || item->list != list)
        return -1;
    if (list->size == 1)
    {
        list->head = list->tail = NULL;
        list->size = 0;
        item->list = NULL;
        return 0;
    }
    if (item == list->head)
    {
        list->head = list->head->next;
        if (list->head)
            list->head->prev = NULL;
        item->prev = item->next = NULL;
        list->size--;
        item->list = NULL;
        return 0;
    }
    if (item == list->tail)
    {
        list->tail = list->tail->prev;
        if (list->tail)
            list->tail->next = NULL;
        item->prev = item->next = NULL;
        list->size--;
        item->list = NULL;
        return 0;
    }
    if (item->prev)
        item->prev->next = item->next;
    if (item->next)
        item->next->prev = item->prev;
    item->prev = item->next = NULL;
    list->size--;
    item->list = NULL;
    return 0;
}

int KamiListSize(KamiList *list)
{
    if (!list)
        return -1;
    return list->size;
}

KamiListNode * KamiListDelByIndex(KamiList *list, int i)
{
    if (!list || i < -list->size || i >= list->size)
        return NULL;

    KamiListNode *tmp = KamiListAt(list, i);
    if (tmp)
    {
        KamiListDel(list, tmp);
        return tmp;
    }
    else
    {
        return NULL;
    }
}

int KamiListDelHead(KamiList *list)
{
    if (!list || list->size == 0)
    {
        return -1;
    }
    list->head->list = NULL;
    if (list->size <= 1)
    {
        list->head = list->tail = NULL;
        list->size--;
        return 0;
    }

    list->head = list->head->next;
    list->head->prev = NULL;
    list->size--;
    list->head->list = NULL;

    return 0;
}

int KamiListDelTail(KamiList *list)
{
    if (!list || list->size == 0)
    {
        return -1;
    }
    list->tail->list = NULL;
    if (list->size <= 1)
    {
        list->head = list->tail = NULL;
        list->size--;
        return 0;
    }

    list->tail = list->tail->prev;
    list->tail->next = NULL;
    list->size--;

    return 0;
}

KamiListNode *KamiListPopHead(KamiList *list)
{
    KamiListNode *tmp = KamiListHead(list);
    KamiListDelHead(list);
    return tmp;
}

KamiListNode *KamiListPopTail(KamiList *list)
{
    KamiListNode *tmp = KamiListTail(list);
    KamiListDelTail(list);
    return tmp;
}
