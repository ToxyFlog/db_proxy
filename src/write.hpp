#ifndef WRITE_H
#define WRITE_H

#include <cstdlib>
#include <string>

static const size_t WRITE_BUFFER_SIZE = 4096;

#define writeVariable(fd, type, value)  \
    {                                   \
        type temp = value;              \
        write(fd, &temp, sizeof(type)); \
    }

class Write {
private:
    int fd, oldBonBlockFlag;
    bool error = false, finished = false;

    char buffer[WRITE_BUFFER_SIZE];
    size_t offset = 0;

    void writeToBuffer(char *source, size_t length);
    inline void flush();

    inline void variable(auto value) { writeToBuffer((char *) &value, sizeof(value)); }
    void c_str(const char *str);
    void str(std::string str);

public:
    Write(int _fd);
    ~Write();

    bool operator()(std::string value) {
        str(value);
        return !error;
    }
    bool operator()(const char *value) {
        c_str(value);
        return !error;
    }
    bool operator()(char *value) {
        c_str(value);
        return !error;
    }
    bool operator()(auto value) {
        variable(value);
        return !error;
    }

    bool finish();
};

#endif  // WRITE_H