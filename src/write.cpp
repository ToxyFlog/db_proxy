#include <cstdlib>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include "request.hpp"
#include "utils.hpp"
#include "write.hpp"

Write::Write(int _fd): fd(_fd) {}
Write::~Write() { flush(); }

void Write::flush() {
    int oldFlag = setFlag(fd, O_NONBLOCK, false);
    if (oldFlag == -1) error = true;

    // TODO: set timer during which next write has to complete, or error occurs OR wait 'till it's writeable and write with non-blocking IO 
    if (write(fd, buffer, offset) == -1) error = true;
    offset = 0;

    if (setFlag(fd, O_NONBLOCK, oldFlag)) error = true;
}

void Write::writeToBuffer(char *source, size_t length) {
    if (length > WRITE_BUFFER_SIZE) {
        flush();
        // TODO: this write might get stuck + it can be either blocking or non-blocking as no asserts/sets are made
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
    flush();
    return !error;
}