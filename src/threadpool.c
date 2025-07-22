
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

#include "logger.h"

#include "threadpool.h"

typedef struct {
        threadpool_scheduler_t *scheduler;
        const char *thread_name;
} thread_creation_args_t;

static void *threadpool_worker_function(void *);
static void free_threads(pthread_t *, size_t);

threadpool_task_t *threadpool_task_alloc(void (*function)(void *), void *arg)
{
        threadpool_task_t *task = malloc(sizeof(threadpool_task_t));
        if (NULL == task) {
                log_debug("Failed to allocate thread pool task");
                return NULL;
        }

        task->function = function;
        task->arg = arg;
        task->next = NULL;

        return task;
}

threadpool_task_queue_t threadpool_task_queue_create(void)
{
        threadpool_task_queue_t queue;
        queue.head = NULL;
        queue.tail = NULL;
        return queue;
}

void threadpool_task_queue_free(threadpool_task_queue_t *queue)
{
        if (NULL == queue) {
                log_debug("Trying to free a NULL task queue");
                return;
        }

        for (threadpool_task_t *task = queue->head; task != NULL;) {
                threadpool_task_t *next_task = task->next;
                free(task);
                task = next_task;
        }

        queue->head = NULL;
        queue->tail = NULL;
}

const threadpool_task_t *threadpool_task_queue_peek(const threadpool_task_queue_t *queue)
{
        if (NULL == queue) {
                log_debug("Trying to peek into a NULL task queue");
                return NULL;
        }
        return queue->head;
}

threadpool_task_t *threadpool_task_queue_pop_front(threadpool_task_queue_t *queue)
{
        if (NULL == queue || NULL == queue->head) {
                log_debug("Trying to pop from an empty or NULL task queue");
                return NULL;
        }

        threadpool_task_t *popped_task = queue->head;
        queue->head = popped_task->next;

        if (NULL == queue->head) {
                queue->tail = NULL;
        }

        return popped_task;
}

void threadpool_task_queue_push_back(threadpool_task_queue_t *queue, threadpool_task_t *task)
{
        if (NULL == queue || NULL == task) {
                log_debug("Trying to push a NULL task or into a NULL queue");
                return;
        }

        if (NULL == queue->tail) {
                queue->head = task;
                queue->tail = task;
        } else {
                queue->tail->next = task;
                queue->tail = task;
        }
}

threadpool_scheduler_t *threadpool_scheduler_create(void)
{
        threadpool_scheduler_t *scheduler = malloc(sizeof(threadpool_scheduler_t));
        if (NULL == scheduler) {
                log_debug("Failed to allocate thread pool scheduler");
                return NULL;
        }

        scheduler->task_queue = threadpool_task_queue_create();
        if (pthread_mutex_init(&scheduler->mutex, NULL) != 0) {
                log_error("Failed to initialize task mutex");
                free(scheduler);
                return NULL;
        }

        if (pthread_cond_init(&scheduler->notify, NULL) != 0) {
                log_error("Failed to initialize terminate condition variable");
                pthread_mutex_destroy(&scheduler->mutex);
                free(scheduler);
                return NULL;
        }

        scheduler->is_terminated = false;
        return scheduler;
}

void threadpool_scheduler_free(threadpool_scheduler_t *scheduler)
{
        if (NULL == scheduler) {
                log_debug("Trying to free a NULL thread pool scheduler");
                return;
        }

        pthread_mutex_lock(&scheduler->mutex);
        scheduler->is_terminated = true;
        pthread_cond_broadcast(&scheduler->notify);
        pthread_mutex_unlock(&scheduler->mutex);

        threadpool_task_queue_free(&scheduler->task_queue);

        pthread_mutex_destroy(&scheduler->mutex);
        pthread_cond_destroy(&scheduler->notify);

        scheduler->task_queue.head = NULL;
        scheduler->task_queue.tail = NULL;
}

threadpool_t *threadpool_create(unsigned threads_count)
{
        pthread_t *threads = malloc(threads_count * sizeof(pthread_t));
        if (NULL == threads) {
                log_warn("Failed to allocate thread pool threads");
                return NULL;
        }

        threadpool_scheduler_t *scheduler = threadpool_scheduler_create();
        if (NULL == scheduler) {
                log_error("Failed to create thread pool scheduler");
                free(threads);
                return NULL;
        }

        for (int i = 0; i < threads_count; ++i) {
                pthread_t *thread = &threads[i];

                constexpr size_t MAX_THREAD_NAME_LENGTH = 32;
                char *thread_name = malloc(MAX_THREAD_NAME_LENGTH);
                snprintf(thread_name, MAX_THREAD_NAME_LENGTH, "worker-thread-%u", i);

                thread_creation_args_t *args = malloc(sizeof(thread_creation_args_t));
                args->scheduler = scheduler;
                args->thread_name = thread_name;

                if (pthread_create(thread, NULL, threadpool_worker_function, args) != 0) {
                        log_error("Failed to create thread %u", i);
                        free_threads(threads, i);
                        return NULL;
                }
        }

        threadpool_t *pool = malloc(sizeof(threadpool_t));
        if (NULL == pool) {
                log_warn("Failed to allocate thread pool structure");
                return NULL;
        }

        *(unsigned *)&pool->thread_count = threads_count;
        pool->threads = threads;
        pool->scheduler = scheduler;

        return pool;
}

void threadpool_free(threadpool_t *pool)
{
        if (NULL == pool) {
                log_debug("Trying to free a NULL thread pool");
                return;
        }

        pthread_mutex_lock(&pool->scheduler->mutex);
        pool->scheduler->is_terminated = true;
        pthread_cond_broadcast(&pool->scheduler->notify);
        pthread_mutex_unlock(&pool->scheduler->mutex);

        threadpool_task_queue_free(&pool->scheduler->task_queue);

        pthread_mutex_destroy(&pool->scheduler->mutex);
        pthread_cond_destroy(&pool->scheduler->notify);

        threadpool_scheduler_free(pool->scheduler);
        free_threads(pool->threads, pool->thread_count);
        free(pool);
        pool = NULL;
}

void threadpool_add_task(threadpool_t *pool, void (*function)(void *), void *arg)
{
        if (NULL == pool || NULL == function) {
                log_debug("Trying to add a NULL task to a NULL thread pool");
                return;
        }

        threadpool_task_t *task = threadpool_task_alloc(function, arg);
        if (NULL == task) {
                log_error("Failed to allocate task for thread pool");
                return;
        }

        pthread_mutex_lock(&pool->scheduler->mutex);
        threadpool_task_queue_push_back(&pool->scheduler->task_queue, task);
        pthread_cond_signal(&pool->scheduler->notify);
        pthread_mutex_unlock(&pool->scheduler->mutex);
}

static void free_threads(pthread_t *threads, size_t threads_count)
{
        if (NULL == threads) {
                log_debug("Calling `clear_failed_threadpool()` with no threads");
                return;
        }

        for (int i = 0; i < threads_count; ++i) {
                pthread_t *thread = &threads[i];
                if (pthread_cancel(*thread) != 0) {
                        log_debug("Failed to cancel threadpool thread %d", i);
                }
        }

        for (int i = 0; i < threads_count; ++i) {
                pthread_t *thread = &threads[i];
                if (pthread_join(*thread, NULL) != 0) {
                        log_debug("Failed to join threadpool thread %d", i);
                }
        }

        free(threads);
        threads = NULL;
}

static void *threadpool_worker_function(void *arg)
{
        if (NULL == arg) {
                log_error("Threadpool worker function received NULL argument");
                return NULL;
        }

        thread_creation_args_t *args = (thread_creation_args_t *)arg;

        threadpool_scheduler_t *scheduler = args->scheduler;
        threadpool_task_queue_t *task_queue = &args->scheduler->task_queue;

        log_info("[%s] Started thread", args->thread_name);

        while (true) {
                pthread_mutex_lock(&args->scheduler->mutex);

                while (NULL == threadpool_task_queue_peek(task_queue) &&
                       !scheduler->is_terminated) {
                        pthread_cond_wait(&args->scheduler->notify, &args->scheduler->mutex);
                }

                if (scheduler->is_terminated) {
                        log_info("[%s] Thread is terminating", args->thread_name);
                        pthread_mutex_unlock(&args->scheduler->mutex);
                        pthread_exit(NULL);
                }

                threadpool_task_t *task = threadpool_task_queue_pop_front(task_queue);

                pthread_mutex_unlock(&args->scheduler->mutex);

                if (NULL == task) {
                        log_warn("[%s] Received NULL task for execution", args->thread_name);
                        continue;
                }
                log_debug("[%s] Executing task", args->thread_name);
                task->function(task->arg);
                free(task);
        }
        return NULL;
}
