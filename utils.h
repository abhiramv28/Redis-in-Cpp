#ifndef utils_h
#define utils_h

#include <cstddef>
#include <cstdint>
#include <stddef.h>
#include <stdint.h>
#include <vector>

extern uint32_t k_max_msg;

struct Buffer {
	uint8_t *buffer_begin;
	uint8_t *buffer_end;
	uint8_t *data_begin;
	uint8_t *data_end;
};

void die(const char *msg);

void msg(const char *text);

int32_t read_full(int fd, uint8_t *buf, size_t n);

int32_t write_all(int fd, const uint8_t *buf, size_t n);

void buf_append(Buffer *buf, const uint8_t *data, size_t len);

void buf_consume(Buffer *buf, size_t n);

size_t buf_size(Buffer *buf);

Buffer *new_buffer(size_t n);

#endif