// minheap.c
#include <string.h>
#include "minheap.h"

#define min_heap_elem_greater(a, b) \
    ((a)->time > (b)->time)

int min_heap_left(min_heap_t *heap, int index)
{
    index = (index << 1) + 1;
    if (index >= heap->size)
    {
        return -1;
    }
    return index;
}

int min_heap_right(min_heap_t *heap, int index)
{
    index = (index << 1) + 2;
    if (index >= heap->size)
    {
        return -1;
    }
    return index;
}

void min_heap_create(min_heap_t *heap)
{
    heap->elements = 0;
    heap->size = 0;
    heap->capacity = 0;
}
void min_heap_destroy(min_heap_t *heap)
{
    if (heap->elements)
    {
        free(heap->elements);
    }
}
void min_heap_elem_init(timer_entry_t *elem) 
{ 
    memset(elem, 0, sizeof(timer_entry_t));
    elem->min_heap_idx = -1; 
}
int min_heap_is_empty(min_heap_t *heap) 
{ 
    return 0 == heap->size; 
}
unsigned min_heap_size(min_heap_t *heap) 
{ 
    return heap->size; 
}
timer_entry_t *min_heap_top(min_heap_t *heap) 
{
    if (heap->size)
    {
        return heap->elements[0];
    }
    return NULL;
}

int min_heap_push(min_heap_t *heap, timer_entry_t *elem)
{
    if (min_heap_ensure_capacity(heap, heap->size + 1))
    {
        return -1;
    }

    min_heap_shift_up(heap, heap->size, elem);
    heap->size++;
    return 0;
}

void min_heap_adjust(min_heap_t *heap, timer_entry_t *elem)
{
    min_heap_shift_down(heap, elem->min_heap_idx, elem);
}

timer_entry_t *min_heap_pop(min_heap_t *heap)
{
    if (heap->size <= 0)
    {
        return NULL;
    }

    timer_entry_t *root = heap->elements[0];
    timer_entry_t *last = heap->elements[heap->size - 1];
    heap->size--;
    heap->elements[heap->size] = NULL;
    min_heap_shift_down(heap, 0, last);
    root->min_heap_idx = -1;
    return root;
}

int min_heap_elt_is_top(const timer_entry_t *elem)
{
    return elem->min_heap_idx == 0;
}

int min_heap_delete(min_heap_t *heap, timer_entry_t *elem)
{
    if (heap->size <= 0)
    {
        return -1;
    }
    if (elem->min_heap_idx < 0)
    {
        return 0;
    }

    timer_entry_t *last = heap->elements[heap->size - 1];
    heap->size--;
    heap->elements[heap->size] = NULL;
    min_heap_shift_down(heap, elem->min_heap_idx, last);
    elem->min_heap_idx = -1;
    return 0;
}

int min_heap_ensure_capacity(min_heap_t *heap, unsigned n)
{
    if (heap->capacity < n)
    {
        timer_entry_t **p;
        unsigned a = heap->capacity ? heap->capacity * 2 : 8;
        if (a < n)
            a = n;
        if (!(p = (timer_entry_t **)realloc(heap->elements, a * sizeof *p)))
            return -1;
        heap->elements = p;
        heap->capacity = a;
    }
    return 0;
}

void min_heap_shift_up(min_heap_t *heap, uint32_t index, timer_entry_t *elem)
{

    while (index > 0)
    {
        uint32_t pindex = (index - 1) >> 1; // 父节点计算公式
        timer_entry_t *parent = heap->elements[pindex];
        if (parent->time <= elem->time) // 加等号让后面添加的在后面执行
        {
            break;
        }
        // 如果父节点比当前节点大，那么交换，将父节点放在index位置
        heap->elements[index] = parent;
        heap->elements[index]->min_heap_idx = index;

        index = pindex;
    }
    heap->elements[index] = elem;
    heap->elements[index]->min_heap_idx = index;

    return;
}

void min_heap_shift_down(min_heap_t *heap, unsigned int index, timer_entry_t *elem)
{
    int half = heap->size >> 1; // 叶子节点计算公式
    // 第一个叶子节点的索引 == 非叶子节点的数量
    // 第一个叶子节点之后的节点全部为叶子节点
    while (index < half)
    {
        // index 的子节点有两种情况，只有左节点和同时有两个子节点
        int left_index = (index << 1) + 1;
        int right_index = (index << 1) + 2;
        timer_entry_t *left = heap->elements[left_index];
        timer_entry_t *child = left;
        int child_index = left_index;
        if ((right_index < heap->size) && (heap->elements[right_index]->time < child->time))
        {
            child = heap->elements[right_index];
            child_index = right_index;
        }

        if (elem->time < child->time)
        {
            break;
        }

        // 将子节点存到index 的位置
        heap->elements[index] = child;
        heap->elements[index]->min_heap_idx = index;
        index = child_index;
    }
    heap->elements[index] = elem;
    heap->elements[index]->min_heap_idx = index;
}