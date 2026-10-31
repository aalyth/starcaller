#include <stdio.h>
#include <err.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>

#include <sys/socket.h>
#include <netinet/in.h>

#ifndef STAR_PORT
#define STAR_PORT 8080
#endif

#ifndef STAR_ADDR
#define STAR_ADDR 0L
#endif

#ifndef STAR_REQUEST_BACKLOG
#define STAR_REQUEST_BACKLOG 200
#endif

#ifndef STAR_BUF_SIZE
#define STAR_BUF_SIZE 4096
#endif

#define defer(f) __attribute__((cleanup(f)))

volatile sig_atomic_t terminated = 0;

const char *response = "HTTP/1.1 200 OK\n"
                       "Server: Star\n\n"
                       "<h1>calling stars</h1>";

void stop_http_server(const int sig)
{
	terminated = 1;
}

void close_socket(const int *sfd)
{
	if (!sfd)
		return;
	if (-1 == close(*sfd))
		dprintf(STDERR_FILENO, "cannot close socket(%d): %s\n", *sfd, strerror(errno));
}

int main()
{
	const int defer(close_socket) sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd == -1)
		err(-1, "failed to create socket");

	const int reuse = 1;
	if (-1 == setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)))
		err(-1, "cannot set SO_REUSEADDR");

	const struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(STAR_PORT),
		.sin_addr.s_addr = INADDR_ANY,
	};
	if (-1 == bind(sfd, (const struct sockaddr *)&addr, sizeof(addr)))
		err(-1, "cannot bind socket to %ld:%hd", STAR_ADDR, STAR_PORT);

	if (-1 == listen(sfd, STAR_REQUEST_BACKLOG))
		err(-1, "cannot listen on %ld:%hd", STAR_ADDR, STAR_PORT);
	printf("Server listening on %ld:%hd\n", STAR_ADDR, STAR_PORT);

	while (!terminated) {
		struct sockaddr_in client_addr;
		socklen_t client_addr_len;
		int reqfd = accept(sfd, (struct sockaddr *)&client_addr, &client_addr_len);
		if (reqfd == -1)
			err(-1, "cannot accept connection to %ld:%hd", STAR_ADDR, STAR_PORT);

		uint8_t reqbuf[STAR_BUF_SIZE];
		ssize_t bytes_read;
		if (-1 == (bytes_read = recv(reqfd, reqbuf, STAR_BUF_SIZE, 0)))
			err(-1, "reading from socket");
		reqbuf[bytes_read] = '\0';
		printf("Read %ld bytes request:\n%s\n", bytes_read, reqbuf);

		write(reqfd, response, strlen(response));
		close(reqfd);
	}

	return 0;
}
