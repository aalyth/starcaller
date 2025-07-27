
#include "http.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdbool.h>

#include "utils.h"
#include "logger.h"
#include "threadpool.h"

static const int FAST_RESTART = true;
#define BUFFER_SIZE 16384

static void handle_client(server_t *, int);

typedef struct {
        http_handler_t handler;
        http_request_t *request;
        int client_fd;
} http_handler_args_t;

static http_handler_args_t *http_handler_args_new(http_handler_t handler, http_request_t *request,
                                                  int client_fd)
{
        http_handler_args_t *args = malloc(sizeof(http_handler_args_t));
        if (!args) {
                log_trace("Failed allocating HTTP handler arguments");
                return NULL;
        }

        args->handler = handler;
        args->request = request;
        args->client_fd = client_fd;

        return args;
}

static void worker_handle_request(void *raw_args)
{
        http_handler_args_t *args = (http_handler_args_t *)raw_args;
        http_response_t *response = args->handler(args->request);
        if (!response) {
                log_warn("Handler returned NULL response");
                free(args->request);
                free(args);
                return;
        }

        int res = 0;
        if ((res = write_http_response(args->client_fd, response)) < 0) {
                log_error("Failed sending response to client - %d", res);
                return;
        }

        log_debug("Sent response with status code: %lu", response->status_code);
        free(args->request);
        http_response_free(response);
        close(args->client_fd);
        free(args);
}

static void handle_client(server_t *server, int client_fd)
{
        char buffer[BUFFER_SIZE];

        ssize_t bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);
        if (bytes_read < 0) {
                log_error("Failed to read client request from socket");
                return;
        }

        buffer[bytes_read] = '\0';
        printf("Received request:\n%s\n", buffer);

        http_request_t *request = parse_http_request(buffer);
        if (!request) {
                log_error("Failed to parse HTTP request");
                return;
        }

        http_handler_t handler =
                http_router_get_handler(server->router, request->method, request->path);

        threadpool_execute(server->threadpool, worker_handle_request,
                           http_handler_args_new(handler, request, client_fd));
}

server_t *server_new(server_config_t config)
{
        server_t *server = malloc(sizeof(server_t));
        if (!server) {
                log_trace("Failed allocating server");
                return NULL;
        }

        server->port = config.port;
        server->max_pending_requests = config.max_pending_requests;

        server->router = http_router_new();
        if (!server->router) {
                log_trace("Failed creating HTTP router");
                goto error_router;
        }

        server->threadpool = threadpool_create(config.threads);
        if (!server->threadpool) {
                log_trace("Failed creating thread pool for request handling");
                goto error_threadpool;
        }

        return server;

error_threadpool:
        http_router_free(server->router);

error_router:
        free(server->router);
        return NULL;
}

int server_add_route(server_t *server, http_method_t method, const char *url,
                     http_handler_t handler)
{
        if (!server || !url || !handler) {
                log_trace("Invalid arguments to server_add_route");
                return -1;
        }

        if (http_router_add_route(server->router, method, url, handler) != 0) {
                log_trace("Failed adding route for %s %s", http_method_to_string(method), url);
                return -2;
        }
        return 0;
}

void server_start(server_t *server)
{
        const int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
                log_fatal(EXIT_FAILURE, "Failed to create socket");
        }
        log_info("Created server socket");

        int result;

        result = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &FAST_RESTART,
                            sizeof(FAST_RESTART));
        if (result < 0) {
                perror("setsockopt");
                close(server_fd);
                exit(EXIT_FAILURE);
        }

        struct sockaddr_in server_address = { .sin_family = AF_INET,
                                              .sin_addr.s_addr = INADDR_ANY,
                                              .sin_port = htons(server->port)

        };

        result = bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address));
        if (result < 0) {
                close(server_fd);
                log_fatal(EXIT_FAILURE, "Failed to bind socket");
        }
        log_info("Socket bound to port %d", server->port);

        if (listen(server_fd, (int)server->max_pending_requests) < 0) {
                close(server_fd);
                log_fatal(EXIT_FAILURE, "Failed to listen on socket");
        }
        log_info("Server listening on port %d (backlog: %lu)", server->port,
                 server->max_pending_requests);

        int client_fd;
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        while (1) {
                log_debug("Waiting for connections...");

                client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
                if (client_fd < 0) {
                        log_error("Failed to accept client connection");
                        continue;
                }

                printf("Client connected from %s:%d (fd: %d)\n", inet_ntoa(client_addr.sin_addr),
                       ntohs(client_addr.sin_port), client_fd);

                handle_client(server, client_fd);

                log_debug("Client connection closed");
        }

        close(server_fd);
}

void server_free(server_t *server)
{
        if (!server) {
                log_trace("Trying to free a NULL http server");
                return;
        }

        threadpool_free(server->threadpool);
        http_router_free(server->router);
        free(server);
}
