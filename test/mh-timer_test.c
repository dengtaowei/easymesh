// mh-timer.c

#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>
#include "mh-timer.h"

uint32_t count = 1;

void hello_world(timer_entry_t *te)
{
    if (count == 1)
    {
        count = te->time;
    }
    if (te->time < count)
    {
        printf("hello world time = %u, count = %d\n", te->time, (int)te->privdata);
        printf("dtwdebug >>>>>>>>>>>>>>>> error\n");
        exit(1);
    }
    printf("hello world time = %u, count = %d\n", te->time, (int)te->privdata);
    count = te->time;
    // add_timer(te->heap, 1000, hello_world, te->privdata);
    // del_timer(te->min_heap_idx, te);
    return;
}

int main()
{
    min_heap_t min_heap;
    init_timer(&min_heap);

    // timer_entry_t *te1 = add_timer(&min_heap, 100, hello_world, 100);
    // timer_entry_t *te2 = add_timer(&min_heap, 5000, hello_world, 5000);
    // timer_entry_t *te3 = add_timer(&min_heap, 20000, hello_world, 20000);
    // timer_entry_t *te4 = add_timer(&min_heap, 5000, hello_world, 5000);
    // timer_entry_t *te5 = add_timer(&min_heap, 6000, hello_world, 6000);
    // timer_entry_t *te6 = add_timer(&min_heap, 4000, hello_world, 4000);
    // timer_entry_t *te7 = add_timer(&min_heap, 3000, hello_world, 3000);
    // timer_entry_t *te8 = add_timer(&min_heap, 2500, hello_world, 2500);
    // timer_entry_t *te9 = add_timer(&min_heap, 150, hello_world, 160);
    // timer_entry_t *te10 = add_timer(&min_heap, 770, hello_world, 770);
    // timer_entry_t *te11 = add_timer(&min_heap, 5689, hello_world, 5689);
    // timer_entry_t *te12 = add_timer(&min_heap, 5800, hello_world, 5800);
    // timer_entry_t *te13 = add_timer(&min_heap, 60000, hello_world, 60000);
    // timer_entry_t *te14 = add_timer(&min_heap, 30000, hello_world, 30000);
    // timer_entry_t *te15 = add_timer(&min_heap, 25000, hello_world, 25000);
    // timer_entry_t *te16 = add_timer(&min_heap, 800, hello_world, 800);

    // 设置种子为当前时间
    srand((unsigned int)time(NULL));

    for (int i = 0; i < 2000; i++)
    {
        // 生成100到25000之间的随机数
        int random_number = 1000 + rand() % (25000 - 1000 + 1);
        add_timer(&min_heap, random_number, hello_world, (void *)random_number);
    }

    int epfd = epoll_create(1);
    struct epoll_event events[512];

    for (;;)
    {
        int nearest = find_nearest_expire_timer(&min_heap);
        int n = epoll_wait(epfd, events, 512, nearest);
        for (int i = 0; i < n; i++)
        {
            //
        }
        expire_timer(&min_heap);
    }
    return 0;
}

// gcc mh-timer.c minheap.c -o mh -I./