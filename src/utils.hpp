#ifndef UTILS_H
#define UTILS_H

#include <vector>
#include <string>

typedef uint8_t type_t;
enum Type : type_t { CREATE_RESOURCE, SELECT, INSERT };

typedef uint16_t string_len_t;
#define string_size(str) sizeof(string_len_t) + str.size()

void exit_with_error(const char *msg);
uint64_t current_time_milliseconds();
void set_fd_flag(int fd, int flag);
std::string join(std::vector<std::string> &strings, char delimiter);

#define min(x, y) (x < y ? x : y)
#define max(x, y) (x > y ? x : y)

#endif // UTILS_H