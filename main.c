
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdbool.h>

#include <pthread.h>

#include "logger.h"
#include "threadpool.h"
#include "http.h"

#define PORT 8080
#define BUFFER_SIZE 1024

const int FAST_RESTART = true;
const unsigned MAX_CONNECTIONS = 100;

// Simple HTTP response
const char *http_response = "HTTP/1.1 200 OK\r\n"
                            "Content-Type: text/html\r\n"
                            // "Content-Length: 54\r\n"
                            "Connection: close\r\n"
                            "\r\n"
                            "<html><body><h1>Hello from C HTTP Server!</h1></body></html>";

void handle_client(int client_fd)
{
        char buffer[BUFFER_SIZE];

        // Read the HTTP request from client
        ssize_t bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);
        if (bytes_read < 0) {
                log_error("Failed to read client request from socket");
                return;
        }

        buffer[bytes_read] = '\0';
        printf("Received request:\n%s\n", buffer);

        http_request_t *request = parse_http_request(buffer);
        log_info("Parsed HTTP request: method=%s, path=%s, version=%s",
                 request->method ? request->method : "NULL", request->path ? request->path : "NULL",
                 request->version ? request->version : "NULL");
        for (int i = 0; request->headers && request->headers[i]; i++) {
                log_info("Header %d: %s", i, request->headers[i]);
        }

        // Send HTTP response
        ssize_t bytes_sent = write(client_fd, http_response, strlen(http_response));
        if (bytes_sent < 0) {
                log_error("Failed to send response to client");
                return;
        }
        log_info("Sent %zd bytes to client", bytes_sent);
}

void test_func(void *unused)
{
        log_info("Test function executed");
}

int main()
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

        const struct sockaddr_in server_address = {
                .sin_family = AF_INET, .sin_addr.s_addr = INADDR_ANY, .sin_port = htons(PORT)

        };

        result = bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address));
        if (result < 0) {
                close(server_fd);
                log_fatal(EXIT_FAILURE, "Failed to bind socket");
        }
        log_info("Socket bound to port %d", PORT);

        if (listen(server_fd, MAX_CONNECTIONS) < 0) {
                close(server_fd);
                log_fatal(EXIT_FAILURE, "Failed to listen on socket");
        }
        log_info("Server listening on port %d (backlog: %d)", PORT, MAX_CONNECTIONS);

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

                handle_client(client_fd);

                close(client_fd);
                log_debug("Client connection closed");
        }

        close(server_fd);

        /*
        threadpool_t *pool = threadpool_create(10);

        sleep(1);

        for (int i = 0; i < 1000; ++i) {
                threadpool_execute(pool, test_func, NULL);
        }

        sleep(1);

        threadpool_free(pool);
        */

        return 0;
}

