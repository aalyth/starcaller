
#include "http.h"

#include <stdlib.h>
#include <string.h>

#include "logger.h"

/// Returns the following status:
/// NULL - any failure
/// ptr - pointer to the end of the request line
static const char *parse_request_line(http_request_t *, const char *);
static http_method_t match_http_method(const char *);

/// Contrary to the common C standard - this function returns the following
/// status codes:
///     0 - failed to allocate headers array
/// n < 0 - failed to allocate n-th header
/// n > 0 - successfully parsed and allocated n headers
static int parse_headers(char **, const char *);
static int count_headers_between(const char *, const char *);
static void free_http_request(http_request_t *);

http_request_t *parse_http_request(const char *raw_request)
{
        if (NULL == raw_request) {
                log_trace("Received NULL HTTP request string");
                return NULL;
        }

        http_request_t *request = malloc(sizeof(http_request_t));
        if (NULL == request) {
                log_trace("Failed to allocate memory for HTTP request");
                return NULL;
        }

        const char *request_line_end = parse_request_line(request, raw_request);
        if (NULL == request_line_end) {
                goto free_request;
        }

        const char *headers_start = request_line_end + 2;
        int headers_parsed = parse_headers(request->headers, headers_start);
        if (0 == headers_parsed) {
                goto free_request_metafields;
        }
        if (headers_parsed < 0) {
                goto free_headers;
        }

        const char *body_start = strstr(raw_request, "\r\n\r\n");
        if (body_start != NULL) {
                // skip over the "\r\n\r\n"
                body_start += 4;
                if (*body_start != '\0') {
                        request->body = strdup(body_start);
                }
        }

        return request;

free_headers:
        headers_parsed -= 1;
        for (int i = 0; i < headers_parsed; ++i) {
                free(request->headers[i]);
        }
        free(request->headers);

free_request_metafields:
        free(request->method_str);
        free(request->path);
        free(request->version);

free_request:
        free(request);
error:
        return NULL;
}

static const char *parse_request_line(http_request_t *request, const char *request_line)
{
        const char *request_line_end = strstr(request_line, "\r\n");
        if (NULL == request_line_end) {
                return NULL;
        }

        const char *method_start = request_line;
        const char *method_end = strchr(method_start, ' ');
        if (NULL == method_end || method_end >= request_line_end) {
                log_trace("Invalid HTTP request: missing method");
                return NULL;
        }

        const char *path_start = method_end + 1;
        const char *path_end = strchr(path_start, ' ');
        if (NULL == path_end || path_end >= request_line_end) {
                log_trace("Invalid HTTP request: missing path");
                return NULL;
        }

        const char *version_start = path_end + 1;
        const char *version_end = request_line_end;

        request->method_str = strndup(method_start, method_end - method_start);
        request->path = strndup(path_start, path_end - path_start);
        request->version = strndup(version_start, version_end - version_start);

        if (NULL == request->method_str || NULL == request->path || NULL == request->version) {
                return NULL;
        }

        request->method = match_http_method(request->method_str);
        return request_line_end;
}

static http_method_t match_http_method(const char *method_str)
{
        if (NULL == method_str) {
                return _HTTP_UNKNOWN;
        }

        if (strcmp(method_str, "GET") == 0) {
                return HTTP_GET;
        } else if (strcmp(method_str, "POST") == 0) {
                return HTTP_POST;
        } else if (strcmp(method_str, "PUT") == 0) {
                return HTTP_PUT;
        } else if (strcmp(method_str, "DELETE") == 0) {
                return HTTP_DELETE;
        } else if (strcmp(method_str, "PATCH") == 0) {
                return HTTP_PATCH;
        } else if (strcmp(method_str, "HEAD") == 0) {
                return HTTP_HEAD;
        } else if (strcmp(method_str, "OPTIONS") == 0) {
                return HTTP_OPTIONS;
        } else if (strcmp(method_str, "TRACE") == 0) {
                return HTTP_TRACE;
        }

        return _HTTP_UNKNOWN;
}

static int parse_headers(char **headers, const char *headers_start)
{
        const char *headers_end = strstr(headers_start, "\r\n\r\n");
        if (!headers_end) {
                headers_end = headers_start + strlen(headers_start);
        }

        int header_count = count_headers_between(headers_start, headers_end);
        headers = calloc(header_count + 1, sizeof(char *));
        if (NULL == headers) {
                log_trace("Failed to allocate HTTP headers");
                return 0;
        }

        const char *line = headers_start;
        int header_idx = 0;

        while (line < headers_end && header_idx < header_count) {
                const char *header_line_end = strstr(line, "\r\n");
                if (NULL == header_line_end || header_line_end == line ||
                    header_line_end >= headers_end) {
                        break;
                }

                headers[header_idx] = strndup(line, header_line_end - line);
                if (NULL == headers[header_idx]) {
                        log_trace("Failed to allocate memory for HTTP header");
                        return -header_idx;
                }

                header_idx++;
                line = header_line_end + 2;
        }

        return header_idx;
}

static int count_headers_between(const char *start, const char *end)
{
        int count = 0;
        const char *line = start;

        while (line < end) {
                const char *line_end = strstr(line, "\r\n");
                if (NULL == line_end || line_end == line || line_end >= end) {
                        break;
                }

                count++;
                line = line_end + 2;
        }
        return count;
}

static void free_http_request(http_request_t *request)
{
        if (NULL == request) {
                log_trace("Received NULL HTTP request to free");
                return;
        }

        free(request->method_str);
        free(request->path);
        free(request->version);
        free(request->body);

        if (request->headers != NULL) {
                for (int i = 0; request->headers[i]; i++) {
                        free(request->headers[i]);
                }
                free(request->headers);
        }

        free(request);
}
