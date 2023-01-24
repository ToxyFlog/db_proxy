#ifndef WRITE_H
#define WRITE_H

#include <string>
#include <cstring>
#include <unistd.h>
#include "utils.hpp"

static const size_t WRITE_BUFFER_SIZE = 1024;
// TODO: write_buffer_size is assumed to be >= string_len_t max value

#define _flush() {                                                                            \
    while (_write_head != _write_buffer) {                                                    \
        int result = write(_write_fd, _write_buffer, WRITE_BUFFER_SIZE - _write_buffer_size); \
        if (result == -1) *_write_error_variable = true;                                      \
        _write_head -= result;                                                                \
        _write_buffer_size += result;                                                         \
    }                                                                                         \
}

#define _write_to_buffer(source, length) {    \
    if (length > _write_buffer_size) _flush() \
    std::memcpy(_write_head, source, length); \
    _write_head += length;                    \
    _write_buffer_size -= length;             \
}

#define begin_write(fd, error_variable)                         \
{                                                               \
    int _write_fd = fd;                                         \
    char _write_buffer[WRITE_BUFFER_SIZE];                      \
    char *_write_head = _write_buffer;                          \
    size_t _write_buffer_size = WRITE_BUFFER_SIZE;              \
    bool *_write_error_variable = error_variable;

#define write_variable_immediate(fd, type, value) { \
    type temp = value;                              \
    write(fd, &temp, sizeof(type));                 \
}

#define write_variable(type, value) {      \
    type temp = value;                     \
    _write_to_buffer(&temp, sizeof(temp)); \
}

#define write_c_str(str) {                              \
    string_len_t string_length = strlen(str);           \
    write_variable(string_len_t, htons(string_length)); \
    _write_to_buffer(str, string_length);               \
}

#define write_str(str) {                                 \
    string_len_t string_length = str.size();             \
    write_variable(string_len_t, htons(string_length));  \
    _write_to_buffer(str.c_str(), string_length);        \
}

#define end_write() \
    _flush()        \
}

#endif // WRITE_H