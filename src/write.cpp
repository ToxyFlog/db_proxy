#include "write.hpp"
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstdlib>
#include <string>
#include "config.hpp"
#include "utils.hpp"

Write::Write(int _fd) : fd(_fd) {
    oldBonBlockFlag = setFlag(fd, O_NONBLOCK, false);

    timeval value{WRITE_TIMEOUT_SECONDS, 0};
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &value, sizeof(timeval));
}
Write::~Write() { finish(); }

void Write::flush() {
    if (write(fd, buffer, offset) == -1) error = true;
    offset = 0;
}

void Write::writeToBuffer(char *source, size_t length) {
    if (length >= WRITE_BUFFER_SIZE) {
        flush();
        if (write(fd, source, length) == -1) error = true;
        return;
    }

    if (offset + length <= WRITE_BUFFER_SIZE) {
        std::memcpy(buffer + offset, source, length);
        offset += length;
        if (offset == WRITE_BUFFER_SIZE) flush();
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

bool Write::operator()(std::string string) {
    FieldLength stringLength = string.size();
    variable<FieldLength>(htons(stringLength));
    writeToBuffer((char *) string.c_str(), stringLength);
    return !error;
}

bool Write::operator()(char *string) {
    FieldLength stringLength = strlen(string);
    variable<FieldLength>(htons(stringLength));
    writeToBuffer((char *) string, stringLength);
    return !error;
}

bool Write::finish() {
    FieldLength l = 0;
    writeToBuffer((char *) &l, sizeof(FieldLength));
    flush();

    timeval value{0, 0};
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &value, sizeof(timeval));
    setFlag(fd, O_NONBLOCK, oldBonBlockFlag);

    return !error;
}