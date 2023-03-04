#ifndef WRITE_H
#define WRITE_H

#include <cstdlib>
#include <string>
#include "utils.hpp"

static const size_t WRITE_BUFFER_SIZE = 4096;

#define writeVariable(fd, type, value)  \
    {                                   \
        type temp = value;              \
        write(fd, &temp, sizeof(type)); \
    }

class Write {
private:
    int fd, oldFlag;
    bool error = false;

    char buffer[WRITE_BUFFER_SIZE];
    size_t offset = 0;

    void writeToBuffer(char *source, size_t length);
    void flush();

    inline void variable(auto value) { writeToBuffer((char *) &value, sizeof(value)); }

public:
    Write(int _fd);
    ~Write();

    bool operator()(std::string value);
    bool operator()(char *value);
    bool operator()(auto value) {
        variable<FieldLength>(htons(sizeof(value)));
        variable(value);
        return !error;
    }

    bool finish();
};

#endif  // WRITE_H