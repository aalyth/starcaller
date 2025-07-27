#ifndef STARCALLER_HTTP_UTILS_H
#define STARCALLER_HTTP_UTILS_H

#include "http.h"

http_request_t *parse_http_request(const char *);
void free_http_request(http_request_t *);

http_method_t string_to_http_method(const char *);
const char *http_method_to_string(http_method_t);

http_router_t *http_router_new(void);
void http_router_free(http_router_t *);
int http_router_add_route(http_router_t *, http_method_t, const char *, http_handler_t);

http_handler_t http_router_get_handler(http_router_t *, http_method_t, const char *);
http_response_t *route_http_request(http_router_t *, const http_request_t *);

void http_router_set_404_handler(http_router_t *, http_handler_t);
void http_router_set_405_handler(http_router_t *, http_handler_t);

int write_http_response(int, const http_response_t *);
void http_response_free(http_response_t *);

#endif
