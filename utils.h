#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdint.h>

extern uint32_t k_max_msg;

void die(const char *msg);

void msg(const char *text);

int32_t read_full(int fd, char *buf, size_t n);

int32_t write_all(int fd, const char *buf, size_t n);

#endif