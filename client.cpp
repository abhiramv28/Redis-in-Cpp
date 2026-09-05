#include "utils.h"
#include <cstdint>
#include <errno.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

static int32_t send_req(int fd, const uint8_t *text, size_t len) {
	if (len > k_max_msg) {
		return -1;
	}

	// Send request
	std::vector<uint8_t> wbuf;
	buf_append(wbuf, (const uint8_t *)&len, 4);
	buf_append(wbuf, text, len);

	return write_all(fd, wbuf.data(), wbuf.size());
}

static int32_t read_res(int fd) {
	// 4 byte header
	std::vector<uint8_t> rbuf;
	rbuf.resize(4);
	errno = 0;

	int32_t err = read_full(fd, rbuf.data(), 4);
	if (err) {
		msg(errno == 0 ? "EOF" : "read() error");
		return err;
	}

	uint32_t len = 0;
	memcpy(&len, rbuf.data(), 4);
	if (len > k_max_msg) {
		msg("too long");
		return -1;
	}

	// Reply body
	rbuf.resize(4 + len);
	err = read_full(fd, &rbuf[4], len);
	if (err) {
		msg("read() error");
		return -1;
	}

	// Do something
	printf("len:%u data:%.*s\n", len, len < 100 ? len : 100, &rbuf[4]);
	return 0;
}

int main() {
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) {
		die("socket()");
	}

	struct sockaddr_in addr = {};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(1234);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
	if (rv) {
		die("connect()");
	}

	std::vector<std::string> query_list = {
		"hello1", "hello2", "hello3", std::string(k_max_msg, 'z'), "hello5"};

	for (const std::string &s : query_list) {
		int32_t err = send_req(fd, (uint8_t *)s.data(), s.size());
		if (err) {
			goto L_DONE;
		}
	}

	for (size_t i = 0; i < query_list.size(); i++) {
		int32_t err = read_res(fd);
		if (err) {
			goto L_DONE;
		}
	}

L_DONE:
	close(fd);

	return 0;
}