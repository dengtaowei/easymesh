// minheap.h
#ifndef _MINHEAP_H
#define _MINHEAP_H

#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>


typedef struct timer_entry_s timer_entry_t;
typedef void (*timer_handler_pt)(timer_entry_t *ev);

#define TIMER_TIMEOUT (1 << 0)
#define TIMER_DELETE (1 << 1)
struct min_heap_s;
struct timer_entry_s
{
    struct min_heap *heap;
    uint32_t time;
    uint32_t interval;
    int min_heap_idx;
    timer_handler_pt handler;
    void *privdata;
};

typedef struct min_heap
{
    timer_entry_t **elements;
    uint32_t size;
    uint32_t capacity; // n 为实际元素个数  a 为容量
    pthread_mutex_t mtx;
} min_heap_t;

void min_heap_create(min_heap_t *heap);
void min_heap_destroy(min_heap_t *heap);
void min_heap_elem_init(timer_entry_t *elem);
int min_heap_elt_is_top(const timer_entry_t *elem);
int min_heap_is_empty(min_heap_t *heap);
unsigned min_heap_size(min_heap_t *heap);
timer_entry_t *min_heap_top(min_heap_t *heap);
int min_heap_ensure_capacity(min_heap_t *heap, unsigned n);
int min_heap_push(min_heap_t *heap, timer_entry_t *elem);
void min_heap_adjust(min_heap_t *heap, timer_entry_t *elem);
timer_entry_t *min_heap_pop(min_heap_t *heap);
int min_heap_delete(min_heap_t *heap, timer_entry_t *elem);
void min_heap_shift_up(min_heap_t *heap, unsigned int index, timer_entry_t *elem);
void min_heap_shift_down(min_heap_t *heap, unsigned int index, timer_entry_t *elem);

#endif // _MINHEAP_H