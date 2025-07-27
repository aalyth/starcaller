
#include "http.h"

#include <stdlib.h>
#include <string.h>

#include "logger.h"
#include "utils.h"

/// Returns the following status:
/// NULL - any failure
/// ptr - pointer to the end of the request line
static const char *parse_request_line(http_request_t *, const char *);

/// Contrary to the common C standard - this function returns the following
/// status codes:
///     0 - failed to allocate headers array
/// n < 0 - failed to allocate n-th header
/// n > 0 - successfully parsed and allocated n headers
static long parse_headers(char **, const char *);
static unsigned count_headers_between(const char *, const char *);

http_request_t *parse_http_request(const char *raw_request)
{
        if (!raw_request) {
                log_trace("Received NULL HTTP request string");
                return NULL;
        }

        http_request_t *request = malloc(sizeof(http_request_t));
        if (!request) {
                log_trace("Failed to allocate memory for HTTP request");
                return NULL;
        }

        const char *request_line_end = parse_request_line(request, raw_request);
        if (!request_line_end)
                goto free_request;

        const char *headers_start = request_line_end + 2;
        long headers_parsed = parse_headers(request->headers, headers_start);
        if (0 == headers_parsed)
                goto free_request_metafields;

        if (headers_parsed < 0) {
                headers_parsed *= -1;
                goto free_headers;
        }

        const char *body_start = strstr(raw_request, "\r\n\r\n");
        if (body_start != NULL) {
                // skip over the "\r\n\r\n"
                body_start += 4;
                if (*body_start != '\0')
                        request->body = strdup(body_start);
        }

        return request;

free_headers:
        for (int i = 0; i < headers_parsed; ++i)
                free(request->headers[i]);

        free(request->headers);

free_request_metafields:
        free(request->method_str);
        free(request->path);
        free(request->version);

free_request:
        free(request);
        return NULL;
}

static const char *parse_request_line(http_request_t *request, const char *request_line)
{
        const char *request_line_end = strstr(request_line, "\r\n");
        if (!request_line_end)
                return NULL;

        const char *method_start = request_line;
        const char *method_end = strchr(method_start, ' ');
        if (!method_end || method_end >= request_line_end) {
                log_trace("Invalid HTTP request: missing method");
                return NULL;
        }

        const char *path_start = method_end + 1;
        const char *path_end = strchr(path_start, ' ');
        if (!path_end || path_end >= request_line_end) {
                log_trace("Invalid HTTP request: missing path");
                return NULL;
        }

        const char *version_start = path_end + 1;
        const char *version_end = request_line_end;

        size_t method_length = (size_t)(method_end - method_start);
        size_t path_length = (size_t)(path_end - path_start);
        size_t version_length = (size_t)(version_end - version_start);

        request->method_str = strndup(method_start, method_length);
        request->path = strndup(path_start, path_length);
        request->version = strndup(version_start, version_length);

        if (!request->method_str || !request->path || !request->version)
                return NULL;

        request->method = string_to_http_method(request->method_str);
        return request_line_end;
}

static long parse_headers(char **headers, const char *headers_start)
{
        const char *headers_end = strstr(headers_start, "\r\n\r\n");
        if (!headers_end)
                headers_end = headers_start + strlen(headers_start);

        long header_count = count_headers_between(headers_start, headers_end);
        headers = calloc((size_t)(header_count + 1), sizeof(char *));
        if (!headers) {
                log_trace("Failed to allocate HTTP headers");
                return 0;
        }

        const char *line = headers_start;
        long header_idx = 0;

        while (line < headers_end && header_idx < header_count) {
                const char *header_line_end = strstr(line, "\r\n");
                if (!header_line_end || header_line_end == line || header_line_end >= headers_end)
                        break;

                headers[header_idx] = strndup(line, (size_t)(header_line_end - line));
                if (!headers[header_idx]) {
                        log_trace("Failed to allocate memory for HTTP header");
                        return -header_idx;
                }

                header_idx++;
                line = header_line_end + 2;
        }

        return header_idx;
}

static unsigned count_headers_between(const char *start, const char *end)
{
        unsigned count = 0;
        const char *line = start;

        while (line < end) {
                const char *line_end = strstr(line, "\r\n");
                if (!line_end || line_end == line || line_end >= end)
                        break;

                count++;
                line = line_end + 2;
        }
        return count;
}

void free_http_request(http_request_t *request)
{
        if (!request) {
                log_trace("Received NULL HTTP request to free");
                return;
        }

        free(request->method_str);
        free(request->path);
        free(request->version);
        free(request->body);

        if (request->headers != NULL) {
                for (int i = 0; request->headers[i]; ++i)
                        free(request->headers[i]);
                free(request->headers);
        }

        free(request);
}
