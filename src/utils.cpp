#include <cstdlib>
#include <vector>
#include <string>
#include <chrono>
#include <fcntl.h>
#include "utils.hpp"

void exit_with_error(const char *msg) {
    fprintf(stderr, "ERROR: %s\n", msg);
    exit(EXIT_FAILURE);
}

uint64_t current_time_milliseconds() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

void set_fd_flag(int fd, int flag) {
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1) exit_with_error("Couldn't get flags!");
    if (fcntl(fd, F_SETFL, flags ^ flag) == -1) exit_with_error("Couldn't set flag!");
}

std::string join(std::vector<std::string> &strings, char delimiter) {
    std::string result = "";

    for (size_t i = 0;i < strings.size();i++) {
        if (i > 0) result.push_back(delimiter);
        result += strings[i];
    }

    return result;
}