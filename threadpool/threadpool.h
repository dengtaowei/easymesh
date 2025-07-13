#ifndef _ELOOP_EVENT_H_
#define _ELOOP_EVENT_H_
#include <pthread.h>

typedef struct threadpool_s threadpool_t;
typedef struct thread_task_s thread_task_t;
typedef struct thread_pool_queue_s thread_pool_queue_t;

struct thread_task_s
{
    void *data;
    void (*handler)(void *data);
};

struct thread_pool_queue_s
{
    thread_task_t *tasks;
    int head;
    int tail;
    int size;
    int capacity;

};


struct threadpool_s
{
    pthread_mutex_t mtx;
    pthread_cond_t cond;
    pthread_t *threads;
    thread_pool_queue_t queue;
    int shutdown;
    int quit;
    int nworker;

};

int threadpool_init(threadpool_t *pool, int nthread, int nmax_task);

int threadpool_destroy(threadpool_t *pool);


#endif