#ifndef STARCALLER_THREADPOOL_H
#define STARCALLER_THREADPOOL_H

#include <pthread.h>

#include <stdbool.h>

typedef struct _ThreadpoolTask {
        void (*function)(void *);
        void *arg;
        struct _ThreadpoolTask *next;
} __threadpool_task_t;

__threadpool_task_t *threadpool_task_alloc(void (*)(void *), void *);

typedef struct {
        __threadpool_task_t *head;
        __threadpool_task_t *tail;
} __threadpool_queue_t;

__threadpool_queue_t __threadpool_queue_create(void);
void __threadpool_queue_free(__threadpool_queue_t *);
const __threadpool_task_t *threadpool_task_queue_peek(const __threadpool_queue_t *);
__threadpool_task_t *__threadpool_queue_pop_front(__threadpool_queue_t *);
void __threadpool_queue_push_back(__threadpool_queue_t *, __threadpool_task_t *);

typedef struct {
        __threadpool_queue_t task_queue;
        pthread_mutex_t mutex;
        pthread_cond_t notify;
        bool is_terminated;
} __threadpool_scheduler_t;

__threadpool_scheduler_t *__threadpool_scheduler_create(void);
void __threadpool_scheduler_free(__threadpool_scheduler_t *);

__threadpool_task_t *__threadpool_scheduler_await_task(__threadpool_scheduler_t *);

typedef struct {
        const unsigned int thread_count;
        pthread_t *threads;

        __threadpool_scheduler_t *scheduler;
} threadpool_t;

threadpool_t *threadpool_create(unsigned);
void threadpool_free(threadpool_t *);

void threadpool_execute(threadpool_t *, void (*)(void *), void *);

#endif
