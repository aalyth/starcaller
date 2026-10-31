#include "http.h"

#include <stddef.h>
#include <string.h>
#include <strings.h>

const char *HEADER_DELIMITER = "\r\n\r\n";

enum http_parse_result { HTTP_PARSE_OK = 0, HTTP_PARSE_INCOMPLETE = -1, HTTP_PARSE_BAD = -2 };

static char *cut(char **, const char *);
static enum http_parse_result parse_http_request(char *, struct http_request *);
static enum http_method parse_http_method(const char *);
static enum http_version parse_http_version(const char *);

static char *cut(char **s, const char *delim)
{
	char *start = *s;
	if (!start)
		return NULL;

	char *end = strstr(start, delim);
	if (!end) {
		*s = NULL;
	} else {
		*end = '\0';
		*s = end + strlen(delim);
	}
	return start;
}

enum http_parse_result parse_http_request(char *raw, struct http_request *req)
{
	if (!raw || !req)
		return HTTP_PARSE_BAD;
	if (!strstr(raw, HEADER_DELIMITER))
		return HTTP_PARSE_INCOMPLETE;

	req->method = parse_http_method(cut(&raw, " "));
	req->url = cut(&raw, " ");
	req->version = parse_http_version(cut(&raw, "\r\n"));

	char *headers = cut(&raw, HEADER_DELIMITER);
	for (; headers != NULL; headers = cut(&headers, "\r\n")) {
		const char *header = cut(&headers, ":");
		const char *value = cut(&headers + 2, "\r\n");
	}
	char *path = strchr(req, ' ');
	if (path == NULL)
		return NULL;
	path += 1;

	return NULL;
}

static enum http_method parse_http_method(const char *method)
{
	if (!method)
		return __HTTP_METHOD_UNKNOWN;

	if (0 == strcmp(method, "GET"))
		return HTTP_METHOD_GET;
	else if (0 == strcmp(method, "POST"))
		return HTTP_METHOD_POST;
	else if (0 == strcmp(method, "PUT"))
		return HTTP_METHOD_PUT;
	else if (0 == strcmp(method, "DELETE"))
		return HTTP_METHOD_DELETE;
	return __HTTP_METHOD_UNKNOWN;
}

static enum http_version parse_http_version(const char *ver)
{
	if (!ver)
		return __HTTP_VERSION_UNKNOWN;

	if (0 == strcasecmp(ver, "HTTP/0.9"))
		return HTTP_0_9;
	else if (0 == strcasecmp(ver, "HTTP/1.0"))
		return HTTP_1_0;
	else if (0 == strcasecmp(ver, "HTTP/1.1"))
		return HTTP_1_1;
	else if (0 == strcasecmp(ver, "HTTP/2"))
		return HTTP_2;
	else if (0 == strcasecmp(ver, "HTTP/3"))
		return HTTP_3;
	return __HTTP_VERSION_UNKNOWN;
}
