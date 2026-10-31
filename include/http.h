#ifndef STARCALLER_HTTP_H
#define STARCALLER_HTTP_H

enum http_method {
	HTTP_METHOD_GET,
	HTTP_METHOD_POST,
	HTTP_METHOD_PUT,
	HTTP_METHOD_DELETE,

	__HTTP_METHOD_UNKNOWN
};

enum http_version { HTTP_0_9, HTTP_1_0, HTTP_1_1, HTTP_2, HTTP_3, __HTTP_VERSION_UNKNOWN };

struct http_header {
	const char *header;
	const char *value;
};

struct http_param {
	const char *param;
	const char *value;
};

struct http_request {
	enum http_method method;
	const char *url;
	const struct http_param *query_params;
	const struct http_param *path_params;
	enum http_version version;
	const struct http_header *headers;
	const char *body;
};

enum http_response_status {
	HTTP_CONTINUE = 100,
	HTTP_SWITCHING_PROTOCOLS = 101,
	HTTP_PROCESSING = 102,
	HTTP_EARLY_HINTS = 103,

	HTTP_OK = 200,
	HTTP_CREATED = 201,
	HTTP_ACCEPTED = 202,
	HTTP_NON_AUTHORATIVE_INFORMATION = 203,
	HTTP_NO_CONTENT = 204,
	HTTP_RESET_CONTENT = 205,
	HTTP_PARTIAL_CONTENT = 206,

	HTTP_MULTIPLE_CHOICES = 300,
	HTTP_MOVED_PERMANENTLY = 301,
	HTTP_FOUND = 302,
	HTTP_SEE_OTHER = 303,
	HTTP_NOT_MODIFIED = 304,
	HTTP_TEMPORARY_REDIRECT = 307,
	HTTP_PERMANENT_REDIRECT = 308,

	HTTP_BAD_REQUEST = 400,
	HTTP_UNAUTHORIZED = 401,
	HTTP_PAYMENT_REQUIRED = 402,
	HTTP_FORBIDDEN = 403,
	HTTP_NOT_FOUND = 404,
	HTTP_METHOD_NOT_ALLOWED = 405,
	HTTP_NOT_ACCEPTABLE = 406,
	HTTP_PROXY_AUTHENTICATION_REQUIRED = 407,
	HTTP_REQUEST_TIMEOUT = 408,
	HTTP_CONFLICT = 409,
	HTTP_GONE = 410,
	HTTP_LENGHT_REQUIRED = 411,
	HTTP_PRECONDITION_FAILED = 412,
	HTTP_CONTENT_TOO_LARGE = 413,
	HTTP_URI_TOO_LONG = 414,
	HTTP_UNSUPPORTED_MEDIA_TYPE = 415,
	HTTP_RANGE_NOT_SATISFIABLE = 416,
	HTTP_EXPECTATION_FAILED = 417,
	HTTP_TOO_MANY_REQUESTS = 429,

	HTTP_INTERNAL_SERVER_ERROR = 500,
	HTTP_NOT_IMPLEMENTED = 501,
	HTTP_BAD_GATEWAY = 502,
	HTTP_SERVICE_UNAVAILABLE = 503,
	HTTP_GATEWAY_TIMEOUT = 504,
	HTTP_VERSION_NOT_SUPPORTED = 505,
	HTTP_NETWORK_AUTHENTICATION_REQUIRED = 511
};

struct http_response {
	enum http_version version;
	const struct http_header *headers;
	const char *body;
};

typedef struct http_response *(*http_handler_f)(struct http_request);

struct http_server_config {
	unsigned addr;
	unsigned short port;

	unsigned backlog;
};

struct http_route {
	enum http_method method;
	const char *url;
	http_handler_f handler;
};

struct http_server {};

int http_server_new(struct http_server *, struct http_server_config);
int http_server_run(struct http_server *);
int http_server_free(struct http_server *);

#endif
