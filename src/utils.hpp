#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>
#include <cstdlib>
#include <unistd.h>

typedef std::vector<std::pair<std::string, std::string>> columns_t;

enum Type { CREATE_RESOURCE, SELECT, INSERT };
typedef uint8_t type_t;
typedef uint8_t string_len_t;

void exit_with_error(const char *msg);
uint64_t current_time_milliseconds();
void set_fd_flag(int fd, int flag);

#define write_variable(fd, type, value) { \
    type temp = value;                    \
    write(fd, &temp, sizeof(type));       \
}

#define write_c_str(fd, c_str) \
    write(fd, c_str, strlen(c_str));

#define write_str(fd, str) {                                                 \
    string_len_t len = str.size();                                           \
    write(fd, &len, 1);                                                      \
    write(fd, (char *) str.c_str(), len);                                    \
}

#define net_string_size(str) \
    sizeof(string_len_t) + str.size()

#define min(x, y) \
    x < y ? x : y
#define max(x, y) \
    x > y ? x : y

#endif // UTILS_H