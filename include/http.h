#ifndef STARCALLER_HTTP_H
#define STARCALLER_HTTP_H

#include <stddef.h>

#include "threadpool.h"

typedef struct {
        char *method;
        char *path;
        char *version;
        char **headers;
        char *body;
} http_request_t;

http_request_t *parse_http_request(const char *);

typedef struct {
} http_router_t;

typedef struct {
        threadpool_t *request_handler_pool;
        unsigned int threadpool_size;

        unsigned int max_queue;

        unsigned short port;
        unsigned int address;

        http_router_t *router;
} server_t;

#endif
