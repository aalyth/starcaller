#ifndef STARCALLER_THREADPOOL_H
#define STARCALLER_THREADPOOL_H

#include <pthread.h>

#include <stdbool.h>

typedef struct _ThreadpoolTask {
        void (*function)(void *);
        void *arg;
        struct _ThreadpoolTask *next;
} threadpool_task_t;

typedef struct {
        threadpool_task_t *head;
        threadpool_task_t *tail;
} threadpool_queue_t;

typedef struct {
        threadpool_queue_t task_queue;
        pthread_mutex_t mutex;
        pthread_cond_t notify;
        bool is_terminated;
} threadpool_scheduler_t;

typedef struct {
        size_t thread_count;
        pthread_t *threads;

        threadpool_scheduler_t *scheduler;
} threadpool_t;

threadpool_t *threadpool_create(size_t);
void threadpool_free(threadpool_t *);

void threadpool_execute(threadpool_t *, void (*)(void *), void *);

#endif
