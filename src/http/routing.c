#include "http.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "logger.h"
#include "utils.h"

static const size_t DEFAULT_ROUTE_CAPACITY = 8;

static int url_router_init(url_router_t *);
static void url_router_free(url_router_t *);
static int url_router_add_entry(url_router_t *, const char *, http_handler_t);
static http_handler_t *url_router_get_handler(url_router_t *, const char *);

static http_response_t *default_404_handler(const http_request_t *);
static http_response_t *default_405_handler(const http_request_t *);

http_router_t *http_router_new(void)
{
        http_router_t *router = malloc(sizeof(http_router_t));
        if (!router) {
                log_trace("Failed allocating HTTP router");
                return NULL;
        }

        router->not_found_handler = default_404_handler;
        router->method_not_allowed_handler = default_405_handler;

        size_t method = 0;
        for (; method < HTTP_METHOD_COUNT; ++method) {
                url_router_t *current_router = &router->methods[method];
                if (url_router_init(current_router) != 0) {
                        goto error;
                }
        }

        return router;

error:
        for (size_t i = 0; i < method; i++) {
                url_router_free(&router->methods[i]);
        }
        free(router);
        return NULL;
}

void http_router_free(http_router_t *router)
{
        if (!router) {
                log_trace("Freeing a NULL HTTP router");
                return;
        }

        for (size_t i = 0; i < HTTP_METHOD_COUNT; i++) {
                url_router_free(&router->methods[i]);
        }
        free(router);
}

int http_router_add_route(http_router_t *router, http_method_t method, const char *path,
                          http_handler_t handler)
{
        if (!router || !path || !handler) {
                log_trace("Invalid arguments to router_add_route");
                return -1;
        }

        url_router_t *method_routes = &router->methods[method];

        if (url_router_add_entry(method_routes, path, handler) != 0)
                return -1;

        return 0;
}

void http_router_set_404_handler(http_router_t *router, http_handler_t handler)
{
        if (router)
                router->not_found_handler = handler;
}

void http_router_set_405_handler(http_router_t *router, http_handler_t handler)
{
        if (router)
                router->method_not_allowed_handler = handler;
}

http_response_t *create_response(size_t status_code, const char *body)
{
        http_response_t *response = malloc(sizeof(http_response_t));
        if (!response) {
                log_trace("Failled allocating HTTP response");
                return NULL;
        }

        response->status_code = status_code;
        response->body = body ? strdup(body) : NULL;
        response->headers = NULL;

        return response;
}

http_handler_t http_router_get_handler(http_router_t *router, http_method_t method,
                                       const char *path)
{
        if (!router || !path) {
                log_trace("Invalid arguments to http_router_get_handler");
                return default_404_handler;
        }

        if (method < 0 || method >= HTTP_METHOD_COUNT) {
                log_trace("Invalid HTTP method: %u", method);
                return default_405_handler;
        }

        url_router_t *url_router = &router->methods[method];
        http_handler_t *handler = url_router_get_handler(url_router, path);
        if (!handler) {
                // Technically, it is possible to go over all the existing routes in
                // order to find if such path exists, but under a different HTTP method,
                // so we can return 405, but with the current design this would be
                // highly inefficient

                return router->not_found_handler;
        }
        return *handler;
}

void http_response_free(http_response_t *response)
{
        if (!response)
                return;

        free(response->body);

        if (response->headers) {
                for (int i = 0; response->headers[i]; i++) {
                        free(response->headers[i]);
                }
                free(response->headers);
        }

        free(response);
}
static int url_router_init(url_router_t *router)
{
        if (!router) {
                log_trace("Initializing a NULL URL router");
                return -1;
        }

        router->count = 0;
        router->capacity = DEFAULT_ROUTE_CAPACITY;
        router->routes = calloc(DEFAULT_ROUTE_CAPACITY, sizeof(url_route_entry_t));
        if (!router->routes) {
                log_trace("Failed to allocate URL router routes");
                return -3;
        }
        return 0;
}

static void url_router_free(url_router_t *router)
{
        if (!router) {
                log_trace("Freeing a NULL URL router");
                return;
        }

        for (size_t i = 0; i < router->count; i++) {
                free(router->routes[i].path);
        }
        free(router->routes);

        router->routes = NULL;
        router->count = 0;
        router->capacity = 0;
}

static int url_router_add_entry(url_router_t *router, const char *url, http_handler_t handler)
{
        if (!router || !url || !handler) {
                log_trace("Invalid arguments to url_router_add_entry()");
                return -1;
        }

        if (router->count >= router->capacity) {
                size_t new_capacity = router->capacity * 2;
                url_route_entry_t *new_routes =
                        realloc(router->routes, new_capacity * sizeof(url_route_entry_t));

                if (!new_routes) {
                        log_trace("Failed reallocating URL router");
                        return -2;
                }

                router->routes = new_routes;
                router->capacity = new_capacity;

                memset(&router->routes[router->count], 0,
                       (new_capacity - router->count) * sizeof(url_route_entry_t));
        }

        url_route_entry_t *entry = &router->routes[router->count++];
        entry->path = strdup(url);
        entry->handler = handler;

        if (!entry->path) {
                log_trace("Failed allocating URL route entry");
                return -3;
        }

        return 0;
}

static http_handler_t *url_router_get_handler(url_router_t *router, const char *url)
{
        for (size_t i = 0; i < router->count; ++i) {
                url_route_entry_t *route = &router->routes[i];
                if (strcmp(route->path, url) == 0) {
                        return &route->handler;
                }
        }
        return NULL;
}

static http_response_t *default_404_handler(__unused const http_request_t *request)
{
        return create_response(404, "Page not Found");
}

static http_response_t *default_405_handler(__unused const http_request_t *request)
{
        return create_response(405, "Method not allowed");
}
