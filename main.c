

#include <stdio.h>
#include <err.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>

#ifndef STAR_PORT
// #define STAR_PORT 5778
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

void socket_close(const int *sfd)
{
	printf("Closing socket %d\n", *sfd);
	if (-1 == close(*sfd))
		dprintf(STDERR_FILENO, "Failed to close socket %d: %s\n", *sfd, strerror(errno));
}

int main()
{
	const int defer(socket_close) sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd == -1)
		err(-1, "Failed to create socket");

	const struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = htons(STAR_PORT),
		.sin_addr.s_addr = INADDR_ANY,
	};
	if (-1 == bind(sfd, (const struct sockaddr *)&addr, sizeof(addr)))
		err(-1, "Failed to bind socket to %ld:%hd", STAR_ADDR, STAR_PORT);

	if (-1 == listen(sfd, STAR_REQUEST_BACKLOG))
		err(-1, "Failed to start listening on %ld:%hd", STAR_ADDR, STAR_PORT);
	printf("Server listening on %ld:%hd\n", STAR_ADDR, STAR_PORT);

	int reqfd;
	struct sockaddr_in client_addr;
	socklen_t client_addr_len;
	if (-1 == (reqfd = accept(sfd, (struct sockaddr *)&client_addr, &client_addr_len)))
		err(-1, "Failed to accept connection to %ld:%hd", STAR_ADDR, STAR_PORT);

	uint8_t request_buffer[STAR_BUF_SIZE];
	ssize_t bytes_read;
	if (-1 == (bytes_read = recv(reqfd, request_buffer, STAR_BUF_SIZE, 0)))
		err(-1, "Failed to read from socket");
	request_buffer[bytes_read] = '\0';

	printf("Read %ld bytes request: %s\n", bytes_read, request_buffer);

	printf("hello there \n");
	return 0;
}
