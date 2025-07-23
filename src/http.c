
#include "http.h"

#include <stdlib.h>
#include <string.h>

#include "logger.h"

static int count_headers(const char *start, const char *end)
{
        int count = 0;
        const char *line = start;

        while (line < end) {
                const char *line_end = strstr(line, "\r\n");
                if (!line_end || line_end >= end)
                        break;

                if (line_end == line)
                        break;
                count++;
                line = line_end + 2;
        }
        return count;
}

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

        const char *first_line_end = strstr(raw_request, "\r\n");
        if (!first_line_end) {
                goto free_request;
        }

        const char *method_start = raw_request;
        const char *method_end = strchr(method_start, ' ');
        if (!method_end || method_end >= first_line_end) {
                log_trace("Invalid HTTP request: missing method");
                goto free_request;
        }

        const char *path_start = method_end + 1;
        const char *path_end = strchr(path_start, ' ');
        if (!path_end || path_end >= first_line_end) {
                log_trace("Invalid HTTP request: missing path");
                goto free_request;
        }

        const char *version_start = path_end + 1;
        const char *version_end = first_line_end;

        request->method = strndup(method_start, method_end - method_start);
        request->path = strndup(path_start, path_end - path_start);
        request->version = strndup(version_start, version_end - version_start);

        if (!request->method || !request->path || !request->version) {
                goto free_request_metafields;
        }

        const char *headers_start = first_line_end + 2;

        const char *headers_end = strstr(headers_start, "\r\n\r\n");
        if (!headers_end) {
                headers_end = headers_start + strlen(headers_start);
        }

        int header_count = count_headers(headers_start, headers_end);
        request->headers = calloc(header_count + 1, sizeof(char *));
        if (!request->headers) {
                log_trace("Failed to allocate HTTP headers");
                goto free_request_metafields;
        }

        // Parse headers
        const char *line = headers_start;
        int header_idx = 0;

        while (line < headers_end && header_idx < header_count) {
                const char *header_line_end = strstr(line, "\r\n");
                if (!header_line_end || header_line_end >= headers_end)
                        break;

                if (header_line_end == line)
                        break;

                request->headers[header_idx] = strndup(line, header_line_end - line);
                if (!request->headers[header_idx]) {
                        log_trace("Failed to allocate memory for HTTP header");
                        goto free_headers;
                }

                header_idx++;
                line = header_line_end + 2;
        }

        const char *body_start = strstr(raw_request, "\r\n\r\n");
        if (body_start != NULL) {
                body_start += 4;
                if (*body_start != '\0') {
                        request->body = strdup(body_start);
                }
        }

        return request;

free_headers:
        for (int i = 0; i < header_idx; i++) {
                free(request->headers[i]);
        }
        free(request->headers);

free_request_metafields:
        free(request->method);
        free(request->path);
        free(request->version);

free_request:
        free(request);
error:
        return NULL;
}

void free_http_request(http_request_t *req)
{
        if (NULL == req) {
                log_trace("Received NULL HTTP request to free");
                return;
        }

        free(req->method);
        free(req->path);
        free(req->version);
        free(req->body);

        if (req->headers) {
                for (int i = 0; req->headers[i]; i++) {
                        free(req->headers[i]);
                }
                free(req->headers);
        }

        free(req);
}
