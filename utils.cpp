#include "utils.h"
#include <assert.h>
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

uint32_t k_max_msg = 4096;

void die(const char *msg) {
	int err = errno;
	fprintf(stderr, "[%d] %s\n", err, msg);
	abort();
}

void msg(const char *msg) { fprintf(stderr, "%s\n", msg); }

int32_t read_full(int fd, char *buf, size_t n) {
	while (n > 0) {
		// 0 for EOF and -1 for error
		ssize_t rv = read(fd, buf, n);
		if (rv <= 0) {
			return -1;
		}

		assert((size_t)rv <= n);
		n -= (size_t)rv;
		buf += rv;
	}

	return 0;
}

int32_t write_all(int fd, const char *buf, size_t n) {
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