#ifndef _KAMILIST_H_
#define _KAMILIST_H_
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <stddef.h>

#ifndef container_of
#define container_of(ptr, type, member)					\
	({								\
		const __typeof__(((type *) NULL)->member) *__mptr = (ptr);	\
		(type *) ((char *) __mptr - offsetof(type, member));	\
	})
#endif

typedef struct _KamiListNode
{
    struct _KamiListNode *next;
    struct _KamiListNode *prev;
    struct _KamiList *list;
    void *data;
} KamiListNode;

typedef struct _KamiList
{
    KamiListNode *head;
    KamiListNode *tail;
    int size;
} KamiList;

#define Iter_From_Head 1
#define Iter_From_Tail -1;

typedef struct KamiListIterrator
{
    KamiListNode *next;
    int direction;
}KamiListIterrator;

KamiListIterrator *KamiListGetIter(KamiList *list, int direction);

KamiListNode *KamiListNext(KamiListIterrator *iter);

void KamiListRewind(KamiList *list, KamiListIterrator *iter);

void KamiListDelIterator(KamiListIterrator *iter);

// 找到返回 1，未找到返回 0
int KamiListContains(KamiList *list, KamiListNode *item);

int KamiListAddHead(KamiList *list, KamiListNode *item);

int KamiListAddTail(KamiList *list, KamiListNode *item);

int KamiListAddAfter(KamiList *list, KamiListNode *prev, KamiListNode *item);

int KamiListAddBefore(KamiList *list, KamiListNode *next, KamiListNode *item);

//  0  1  2 list->size = 3
// -3 -2 -1
// i < -list->size || i >= list->size
KamiListNode *KamiListAt(KamiList *list, int i);

int KamiListInsertAt(KamiList *list, KamiListNode *item, int i);

KamiListNode *KamiListHead(KamiList *list);

KamiListNode *KamiListTail(KamiList *list);

int KamiListDel(KamiList *list, KamiListNode *item);

int KamiListSize(KamiList *list);

KamiListNode * KamiListDelByIndex(KamiList *list, int i);

int KamiListDelHead(KamiList *list);

int KamiListDelTail(KamiList *list);

KamiListNode *KamiListPopHead(KamiList *list);

KamiListNode *KamiListPopTail(KamiList *list);
#endif
