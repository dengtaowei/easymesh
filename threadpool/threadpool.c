#include <sys/time.h>
#include "threadpool.h"

static void *threadpool_thread(void *data)
{
    threadpool_t *pool = (threadpool_t *)data;
    struct timeval now;
    struct timespec outtime;
    int timeout = 50;

    pthread_mutex_lock(&pool->mtx);
    if (pool->queue.size <= 0)
    {
        while(pool->queue.size <= 0 && pool->quit == 0)
        {
            gettimeofday(&now, NULL);
            if (now.tv_usec + timeout > 1000)
            {
                outtime.tv_sec = now.tv_sec + 1;
                outtime.tv_nsec = ((now.tv_usec + timeout) % 1000) * 1000;
            }
            else
            {
                outtime.tv_sec = now.tv_sec;
                outtime.tv_nsec = (now.tv_usec + timeout) * 1000;
            }
            pthread_cond_timedwait(&pool->cond, &pool->mtx, &outtime);
        }
    }
    pthread_mutex_unlock(&pool->mtx);

}



int threadpool_init(threadpool_t *pool, int nthread, int nmax_task)
{
    if (!pool || nthread <= 0 || nmax_task <= 0)
    {
        return -1;
    }
    memset(pool, 0, sizeof(thread_task_t));

    pool->queue.tasks = (thread_task_t *)malloc(sizeof(thread_task_t));
    if (!pool->queue.tasks)
    {
        return -1;
    }
    memset(pool->queue.tasks, 0, sizeof(thread_task_t));

    pool->threads = malloc(nthread * sizeof(pthread_t));
    if (!pool->threads)
    {
        free(pool->queue.tasks);
        return -1;
    }

    pool->queue.head = pool->queue.tail = pool->queue.size = 0;

    int i = 0;
    for (i = 0; i < nthread; i++)
    {
        int ret = pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void *)pool);
        if (ret < 0)
        {
            break;
        }
        else
        {
            pool->nworker++;
        }
    }

    if (pool->nworker <= 0)
    {
        free(pool->queue.tasks);
        free(pool->threads);
        return 0;
    }

    pthread_mutex_init(&pool->mtx, NULL);
    pthread_cond_init(&pool->cond, NULL);

    return pool->nworker;
}

int threadpool_destroy(threadpool_t *pool)
{
    int ret = 0;
    if (pool->shutdown)
    {
        return 0;
    }
    pool->shutdown = 1;

    for (int i = 0; i < pool->nworker; i++)
    {
        if (pthread_join(pool->threads[i], NULL) != 0)
        {
            ret = -1;
        }
    }

    pthread_cond_destroy(&pool->cond);
    pthread_mutex_destroy(&pool->mtx);

    free(pool->queue.tasks);
    free(pool->threads);
    return ret;
}