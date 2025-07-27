
#include "utils.h"

#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "http.h"

static const char *get_status_text(size_t);

static ssize_t write_string(int, const char *);
static ssize_t write_status_line(int, size_t);
static ssize_t write_headers(int, const http_response_t *);
static ssize_t write_body(int, const char *);

int write_http_response(int fd, const http_response_t *response)
{
        if (!response || fd < 0)
                return -1;

        if (write_status_line(fd, response->status_code) < 0)
                return -2;

        if (write_headers(fd, response) < 0)
                return -3;

        if (write_string(fd, "\r\n") < 0)
                return -4;

        if (write_body(fd, response->body) < 0)
                return -5;

        return 0;
}

static ssize_t write_string(int fd, const char *str)
{
        if (!str)
                return 0;

        return write(fd, str, strlen(str));
}

static ssize_t write_status_line(int fd, size_t status_code)
{
        char status_line[256];
        const char *status_text = get_status_text(status_code);

        int len = snprintf(status_line, sizeof(status_line), "HTTP/1.1 %lu %s\r\n", status_code,
                           status_text);
        if (len < 0)
                return -1;

        size_t unsigned_len = (size_t)len;
        if (unsigned_len >= sizeof(status_line))
                return -1;

        return write(fd, status_line, unsigned_len);
}

static ssize_t write_header(int fd, const char *header)
{
        if (write_string(fd, header) < 0)
                return -1;
        return write_string(fd, "\r\n");
}

static ssize_t write_content_length_header(int fd, size_t content_length)
{
        char header[128];
        int len = snprintf(header, sizeof(header), "Content-Length: %zu", content_length);

        if (len < 0 || (size_t)len >= sizeof(header))
                return -1;

        return write_header(fd, header);
}

static int write_custom_headers(int fd, char **headers)
{
        if (!headers)
                return 0;

        for (int i = 0; headers[i]; i++) {
                if (write_header(fd, headers[i]) < 0)
                        return -1;
        }

        return 0;
}

static bool header_exists(char **headers, const char *header_name)
{
        if (!headers || !header_name)
                return false;

        size_t name_len = strlen(header_name);

        for (const char *header = headers[0]; header; header++) {
                if (strncasecmp(header, header_name, name_len) == 0 && header[name_len] == ':')
                        return true;
        }

        return false;
}

static int write_default_headers(int fd, char **headers)
{
        if (!header_exists(headers, "Content-Type")) {
                if (write_header(fd, "Content-Type: text/html; charset=utf-8") < 0)
                        return -1;
        }

        if (!header_exists(headers, "Connection")) {
                if (write_header(fd, "Connection: close") < 0)
                        return -1;
        }

        return 0;
}

static ssize_t write_headers(int fd, const http_response_t *response)
{
        size_t body_len = response->body ? strlen(response->body) : 0;

        if (write_custom_headers(fd, response->headers) < 0)
                return -1;

        if (!header_exists(response->headers, "Content-Length")) {
                if (write_content_length_header(fd, body_len) < 0) {
                        return -1;
                }
        }

        if (write_default_headers(fd, response->headers) < 0)
                return -1;

        return 0;
}

static ssize_t write_body(int fd, const char *body)
{
        if (!body)
                return 0;

        return write_string(fd, body);
}

static const char *get_status_text(size_t status_code)
{
        switch (status_code) {
        case 200:
                return "OK";
        case 201:
                return "Created";
        case 204:
                return "No Content";
        case 301:
                return "Moved Permanently";
        case 302:
                return "Found";
        case 304:
                return "Not Modified";
        case 400:
                return "Bad Request";
        case 401:
                return "Unauthorized";
        case 403:
                return "Forbidden";
        case 404:
                return "Not Found";
        case 405:
                return "Method Not Allowed";
        case 409:
                return "Conflict";
        case 422:
                return "Unprocessable Entity";
        case 500:
                return "Internal Server Error";
        case 501:
                return "Not Implemented";
        case 502:
                return "Bad Gateway";
        case 503:
                return "Service Unavailable";
        default:
                return "Unknown";
        }
}
