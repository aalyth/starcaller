#ifndef STARCALLER_HTTP_H
#define STARCALLER_HTTP_H

#include <stddef.h>

#include "threadpool.h"

typedef enum {
        HTTP_GET,
        HTTP_POST,
        HTTP_PUT,
        HTTP_DELETE,
        HTTP_PATCH,
        HTTP_HEAD,
        HTTP_OPTIONS,
        HTTP_TRACE,

        _HTTP_UNKNOWN,
} http_method_t;

typedef struct {
        http_method_t method;
        char *method_str;
        char *path;
        char *version;
        char **headers;
        char *body;
} http_request_t;

http_request_t *parse_http_request(const char *);

typedef struct {
        int status_code;
        char *body;
        char **headers;
} http_response_t;

typedef enum http_status_code {
        HTTP_OK = 200,
        HTTP_CREATED = 201,
        HTTP_ACCEPTED = 202,
        HTTP_NO_CONTENT = 204,
        HTTP_BAD_REQUEST = 400,
        HTTP_UNAUTHORIZED = 401,
        HTTP_FORBIDDEN = 403,
        HTTP_NOT_FOUND = 404,
        HTTP_INTERNAL_SERVER_ERROR = 500,
        HTTP_NOT_IMPLEMENTED = 501,
        HTTP_BAD_GATEWAY = 502,
        HTTP_SERVICE_UNAVAILABLE = 503
} http_status_code_t;

typedef http_response_t *(*route_handler_t)(const http_request_t *request);

typedef struct {
        char *path;
        route_handler_t handler;
} url_route_entry_t;

typedef struct {
        url_route_entry_t *routes;
        size_t count;
        size_t capacity;
} url_router_t;

typedef struct {
        url_router_t methods[_HTTP_UNKNOWN];
        route_handler_t not_found_handler;
        route_handler_t method_not_allowed_handler;
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
