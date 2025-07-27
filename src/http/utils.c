

#include <string.h>

#include "http.h"
#include "utils.h"

http_method_t string_to_http_method(const char *method)
{
        if (NULL == method) {
                return _HTTP_UNKNOWN;
        }

        if (strcmp(method, "GET") == 0) {
                return HTTP_GET;
        } else if (strcmp(method, "POST") == 0) {
                return HTTP_POST;
        } else if (strcmp(method, "PUT") == 0) {
                return HTTP_PUT;
        } else if (strcmp(method, "DELETE") == 0) {
                return HTTP_DELETE;
        } else if (strcmp(method, "PATCH") == 0) {
                return HTTP_PATCH;
        } else if (strcmp(method, "HEAD") == 0) {
                return HTTP_HEAD;
        } else if (strcmp(method, "OPTIONS") == 0) {
                return HTTP_OPTIONS;
        } else if (strcmp(method, "TRACE") == 0) {
                return HTTP_TRACE;
        }

        return _HTTP_UNKNOWN;
}

const char *http_method_to_string(http_method_t method)
{
        static const char *method_map[] = {
                [HTTP_GET] = "GET",         [HTTP_POST] = "POST",   [HTTP_PUT] = "PUT",
                [HTTP_DELETE] = "DELETE",   [HTTP_PATCH] = "PATCH", [HTTP_HEAD] = "HEAD",
                [HTTP_OPTIONS] = "OPTIONS", [HTTP_TRACE] = "TRACE", [_HTTP_UNKNOWN] = "UNKNOWN"
        };

        if (method < 0 || method >= HTTP_METHOD_COUNT) {
                return "UNKNOWN";
        }
        return method_map[method];
}
