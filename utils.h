#ifndef utils_h
#define utils_h

#include <cstdint>
#include <stddef.h>
#include <stdint.h>
#include <vector>

extern uint32_t k_max_msg;

void die(const char *msg);

void msg(const char *text);

int32_t read_full(int fd, uint8_t *buf, size_t n);

int32_t write_all(int fd, const uint8_t *buf, size_t n);

void buf_append(std::vector<uint8_t> &buf, const uint8_t *data, size_t len);

void buf_consume(std::vector<uint8_t> &buf, size_t n);

#endif