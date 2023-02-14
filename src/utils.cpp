#include "utils.hpp"
#include <fcntl.h>
#include <chrono>
#include <cstdlib>
#include <string>
#include <vector>

void exitWithError(const char *message) {
    fprintf(stderr, "ERROR: %s\n", message);
    printf("errno: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
}

uint64_t unixTimeInMilliseconds() { return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count(); }

int setFlag(int fd, int flag, bool value) {
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1) return -1;
    int oldValue = flags & flag;

    int newFlags = value ? (flags | flag) : (flags & ~flag);
    if (fcntl(fd, F_SETFL, newFlags) == -1) return -1;

    return oldValue;
}