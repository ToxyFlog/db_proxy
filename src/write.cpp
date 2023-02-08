#include <cstdlib>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include "config.hpp"
#include "request.hpp"
#include "utils.hpp"
#include "write.hpp"

Write::Write(int _fd): fd(_fd) {
    oldBonBlockFlag = setFlag(fd, O_NONBLOCK, false);

    timeval value {WRITE_TIMEOUT_SECONDS, 0};
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &value, sizeof(timeval));
}
Write::~Write() { finish(); }

inline void Write::flush() {
    if (write(fd, buffer, offset) == -1) error = true;
    offset = 0;
}

void Write::writeToBuffer(char *source, size_t length) {
    if (length > WRITE_BUFFER_SIZE) {
        flush();
        if (write(fd, source, length) == -1) error = true;
        return;
    }

    size_t sourceOffset = 0;
    while (sourceOffset < length) {
        size_t bufferSpace = min(WRITE_BUFFER_SIZE - offset, length - sourceOffset);

        std::memcpy(buffer + offset, source + sourceOffset, bufferSpace);
        sourceOffset += bufferSpace;

        offset += bufferSpace;
        if (offset == WRITE_BUFFER_SIZE) flush();
    }
}

void Write::c_str(const char *string) {
    RequestStringLength stringLength = strlen(string);
    variable<RequestStringLength>(htons(stringLength));
    writeToBuffer((char*) string, stringLength);
}

void Write::str(std::string string) {
    RequestStringLength stringLength = string.size();
    variable<RequestStringLength>(htons(stringLength));
    writeToBuffer((char*) string.c_str(), stringLength);
}

bool Write::finish() {
    if (finished) return !error;
    finished = true;

    flush();
    timeval value {0, 0};
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &value, sizeof(timeval));
    setFlag(fd, O_NONBLOCK, oldBonBlockFlag);

    return !error;
}