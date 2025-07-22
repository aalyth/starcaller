#ifndef STARCALLER_THREADPOOL_H
#define STARCALLER_THREADPOOL_H

#include <pthread.h>

#include <stdbool.h>

typedef struct _ThreadpoolTask {
        void (*function)(void *);
        void *arg;
        struct _ThreadpoolTask *next;
} threadpool_task_t;

threadpool_task_t *threadpool_task_alloc(void (*)(void *), void *);

typedef struct {
        threadpool_task_t *head;
        threadpool_task_t *tail;
} threadpool_task_queue_t;

threadpool_task_queue_t threadpool_task_queue_create(void);
void threadpool_task_queue_free(threadpool_task_queue_t *);
const threadpool_task_t *threadpool_task_queue_peek(const threadpool_task_queue_t *);
threadpool_task_t *threadpool_task_queue_pop_front(threadpool_task_queue_t *);
void threadpool_task_queue_push_back(threadpool_task_queue_t *, threadpool_task_t *);

typedef struct {
        threadpool_task_queue_t task_queue;
        pthread_mutex_t mutex;
        pthread_cond_t notify;
        bool is_terminated;
} threadpool_scheduler_t;

threadpool_scheduler_t *threadpool_scheduler_create(void);
void threadpool_scheduler_free(threadpool_scheduler_t *);

threadpool_task_t *threadpool_scheduler_await_task(threadpool_scheduler_t *);

typedef struct {
        const unsigned int thread_count;
        pthread_t *threads;

        threadpool_scheduler_t *scheduler;
} threadpool_t;

threadpool_t *threadpool_create(unsigned);
void threadpool_free(threadpool_t *);

void threadpool_add_task(threadpool_t *, void (*)(void *), void *);

#endif
