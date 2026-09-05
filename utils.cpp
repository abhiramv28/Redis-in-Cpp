#include "utils.h"
#include <assert.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
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

// Implement handling of errors while allocation
void buf_append(Buffer *buf, const uint8_t *data, size_t len) {
	size_t capacity = buf->buffer_end - buf->buffer_begin;
	size_t data_size = buf->data_end - buf->data_begin;

	if (buf->buffer_begin != buf->data_begin) {
		memmove(buf->buffer_begin, buf->data_begin, data_size);
		buf->data_begin = buf->buffer_begin;
		buf->data_end = buf->data_begin + data_size;
	}

	if (len > capacity - data_size) {
		size_t extra = len - (capacity - data_size);
		buf->buffer_begin =
			(uint8_t *)realloc(buf->buffer_begin, capacity + extra);

		buf->buffer_end = buf->buffer_begin + capacity + extra;
		buf->data_begin = buf->buffer_begin;
		buf->data_end = buf->data_begin + data_size;
	}

	memcpy(buf->data_end, data, len);
	buf->data_end += len;
}

void buf_consume(Buffer *buf, size_t n) { buf->data_begin += n; }

size_t buf_size(Buffer *buf) { return buf->data_end - buf->data_begin; }

Buffer *new_buffer(size_t n) {
	Buffer *buf = (Buffer *)malloc(sizeof(struct Buffer));

	buf->buffer_begin = (uint8_t *)malloc(n * sizeof(uint8_t));
	buf->buffer_end = buf->buffer_begin + n;

	buf->data_begin = buf->buffer_begin;
	buf->data_end = buf->buffer_begin;

	return buf;
}