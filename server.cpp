#include "utils.h"
#include <errno.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static int one_request(int connfd) {
	// 4 bytes header
	char rbuf[4 + k_max_msg];
	errno = 0;

	int32_t err = read_full(connfd, rbuf, 4);
	if (err) {
		msg(errno == 0 ? "EOF" : "read() error");
		return err;
	}

	uint32_t len = 0;
	memcpy(&len, rbuf, 4);
	if (len > k_max_msg) {
		msg("too long");
		return -1;
	}

	// Request body
	err = read_full(connfd, &rbuf[4], len);
	if (err) {
		msg("read() error");
		return err;
	}

	// Do something
	printf("Client says: %.*s\n", len, &rbuf[4]);

	// Reply
	const char reply[] = "world";

	char wbuf[4 + sizeof(reply)];
	len = (uint32_t)strlen(reply);

	memcpy(wbuf, &len, 4);
	memcpy(&wbuf[4], reply, len);

	return write_all(connfd, wbuf, 4 + len);
}

int main() {
	// Setting up a socket
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		die("socker()");
	}
	int val = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));

	// Address for listening
	struct sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(1234);
	addr.sin_addr.s_addr = htonl(0);

	// Binding the socket
	int rv = bind(fd, (const struct sockaddr *)&addr, sizeof(addr));
	if (rv) {
		die("bind()");
	}

	rv = listen(fd, SOMAXCONN);
	if (rv) {
		die("listen()");
	}

	while (true) {
		struct sockaddr_in client_addr = {};
		socklen_t addrlen = sizeof(client_addr);

		int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);
		if (connfd < 0) {
			die("accept()");
		}

		while (true) {
			int32_t err = one_request(connfd);
			if (err) {
				break;
			}
		}
		close(connfd);
	}

	return 0;
}