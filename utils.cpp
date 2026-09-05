#include "utils.h"
#include <assert.h>
#include <cstdint>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// 32 MB
uint32_t k_max_msg = 32 << 20;

void die(const char *msg) {
	int err = errno;
	fprintf(stderr, "[%d] %s\n", err, msg);
	abort();
}

void msg(const char *msg) { fprintf(stderr, "%s\n", msg); }

int32_t read_full(int fd, uint8_t *buf, size_t n) {
	while (n > 0) {
		// 0 for EOF and -1 for error
		ssize_t rv = read(fd, buf, n);
		if (rv <= 0) {
			// If the read buffer is empty
			if (errno == EINTR) {
				continue;
			}
			return -1;
		}

		assert((size_t)rv <= n);
		n -= (size_t)rv;
		buf += rv;
	}

	return 0;
}

int32_t write_all(int fd, const uint8_t *buf, size_t n) {
	while (n > 0) {
		ssize_t rv = write(fd, buf, n);
		if (rv <= 0) {
			return -1;
		}

		assert((size_t)rv <= n);
		n -= (size_t)rv;
		buf += rv;
	}

	return 0;
}

void buf_append(std::vector<uint8_t> &buf, const uint8_t *data, size_t len) {
	buf.insert(buf.end(), data, data + len);
}

void buf_consume(std::vector<uint8_t> &buf, size_t n) {
	buf.erase(buf.begin(), buf.begin() + n);
}