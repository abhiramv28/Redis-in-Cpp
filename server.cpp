#include "utils.h"
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

struct Conn {
	int fd = -1;

	// For event loop
	bool want_read = false;
	bool want_write = false;
	bool want_close = false;

	// Buffered input and output
	Buffer *incoming = new_buffer(64 * 1024);
	Buffer *outgoing = new_buffer(64 * 1024);
};

static void fd_set_nb(int fd) {
	errno = 0;
	int flags = fcntl(fd, F_GETFL, 0);
	if (errno) {
		die("fcntl error");
		return;
	}

	flags |= O_NONBLOCK;

	errno = 0;
	(void)fcntl(fd, F_SETFL, flags);
	if (errno) {
		die("fcntl error");
	}
}

static Conn *handle_accept(int fd) {
	struct sockaddr_in client_addr = {};
	socklen_t addrlen = sizeof(client_addr);
	int connfd = accept(fd, (struct sockaddr *)&client_addr, &addrlen);
	if (connfd < 0) {
		msg("accept error");
		return NULL;
	}

	uint32_t ip = client_addr.sin_addr.s_addr;
	fprintf(stderr, "new client from %u.%u.%u.%u:%u\n", ip & 255,
			(ip >> 8) & 255, (ip >> 16) & 255, ip >> 24,
			htons(client_addr.sin_port));

	// Set the connection to non-blocking mode
	fd_set_nb(connfd);

	Conn *conn = new Conn();
	conn->fd = connfd;
	conn->want_read = true;
	return conn;
}

static bool try_one_request(Conn *conn) {
	Buffer *rbuf = conn->incoming;
	if (buf_size(rbuf) < 4) {
		return false;
	}

	uint32_t len = 0;
	memcpy(&len, rbuf->data_begin, 4);
	if (len > k_max_msg) {
		msg("too long");
		conn->want_close = true;
		return false;
	}

	if (4 + len > buf_size(rbuf)) {
		return false;
	}

	const uint8_t *request = rbuf->data_begin + 4;
	printf("client says: len:%d data:%.*s\n", len, len < 100 ? len : 100,
		   request);

	// Response (echo)
	Buffer *wbuf = conn->outgoing;
	buf_append(wbuf, (const uint8_t *)&len, 4);
	buf_append(wbuf, request, len);

	buf_consume(rbuf, 4 + len);
	return true;
}

static void handle_write(Conn *conn) {
	Buffer *wbuf = conn->outgoing;
	assert(buf_size(wbuf) > 0);
	ssize_t rv = write(conn->fd, wbuf->data_begin, buf_size(wbuf));
	if (rv < 0 && errno == EAGAIN) {
		// Not ready some signal interrupted (EAGAIN generally does not occur in
		// non-blocking code)
		return;
	}

	if (rv < 0) {
		msg("write() error");
		conn->want_close = true;
		return;
	}

	buf_consume(wbuf, size_t(rv));

	if (buf_size(wbuf) == 0) {
		conn->want_read = true;
		conn->want_write = false;
	}
}

static void handle_read(Conn *conn) {
	uint8_t buf[64 * 1024];
	ssize_t rv = read(conn->fd, buf, sizeof(buf));
	if (rv < 0 && errno == EAGAIN) {
		return;
	}

	if (rv < 0) {
		msg("read() error");
		conn->want_close = true;
		return;
	}

	Buffer *rbuf = conn->incoming;
	buf_append(rbuf, buf, (size_t)rv);

	while (try_one_request(conn)) {
	}

	Buffer *wbuf = conn->outgoing;
	if (buf_size(wbuf) > 0) {
		conn->want_read = false;
		conn->want_write = true;
		handle_write(conn);
		return;
	}
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

	// Map of connections keyed by fd as linux assigns lowest available number
	// to fd
	std::vector<Conn *> fd2conn;
	// Event loop
	std::vector<struct pollfd> poll_args;
	while (true) {
		poll_args.clear();
		// Listening socket in the 1st position
		struct pollfd pfd = {fd, POLLIN, 0};
		poll_args.push_back(pfd);

		// Sockets
		for (Conn *conn : fd2conn) {
			if (!conn) {
				continue;
			}

			struct pollfd pfd = {conn->fd, POLLERR, 0};
			if (conn->want_read) {
				pfd.events |= POLLIN;
			}
			if (conn->want_write) {
				pfd.events |= POLLOUT;
			}
			poll_args.push_back(pfd);
		}

		int rv = poll(poll_args.data(), (nfds_t)poll_args.size(), -1);
		if (rv < 0 && errno == EINTR) {
			continue;
		}
		if (rv < 0) {
			die("poll");
		}

		// Listening socket
		if (poll_args[0].revents) {
			if (Conn *conn = handle_accept(fd)) {
				if (fd2conn.size() <= (size_t)conn->fd) {
					fd2conn.resize(conn->fd + 1);
				}
				assert(!fd2conn[conn->fd]);
				fd2conn[conn->fd] = conn;
			}
		}

		// Other sockets
		for (size_t i = 1; i < poll_args.size(); i++) {
			uint32_t ready = poll_args[i].revents;
			if (ready == 0) {
				continue;
			}

			Conn *conn = fd2conn[poll_args[i].fd];
			if (ready & POLLIN) {
				assert(conn->want_read);
				handle_read(conn);
			}
			if (ready & POLLOUT) {
				assert(conn->want_write);
				handle_write(conn);
			}

			if ((ready & POLLERR) || conn->want_close) {
				(void)close(conn->fd);
				fd2conn[conn->fd] = NULL;
				delete conn;
			}
		}
	}

	return 0;
}