// mh-timer.h///////////////////////// 锁未完善，另一个线程删除定时器的时候如果定时器已经执行了，te 被释放了会有问题//////////////////////////////////////////////////////////////////
#ifndef _MINHEAP_TIMER_H
#define _MINHEAP_TIMER_H

#include <time.h>

#include "minheap.h"

void init_timer(min_heap_t *min_heap);

void deinit_timer(min_heap_t *min_heap);

timer_entry_t *add_timer(min_heap_t *min_heap, uint32_t msec, timer_handler_pt callback, void *privdata);

void reset_timer(min_heap_t *min_heap, timer_entry_t *te);

int del_timer(min_heap_t *min_heap, timer_entry_t *elem);

int find_nearest_expire_timer(min_heap_t *min_heap);

void expire_timer(min_heap_t *min_heap);

#endif // _MINHEAP_TIMER_H