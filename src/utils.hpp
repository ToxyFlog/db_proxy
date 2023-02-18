#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

typedef uint16_t FieldLength;
typedef int16_t ResourceId;
typedef uint8_t RequestType;
enum Type : RequestType { CREATE_RESOURCE, SELECT, INSERT };

void exitWithError(const char *message);
uint64_t unixTimeInMilliseconds();
int setFlag(int fd, int flag, bool value);

#define min(x, y) (x < y ? x : y)
#define max(x, y) (x > y ? x : y)
#define toLowerCase(string) std::transform(string.begin(), string.end(), string.begin(), [](char c) { return std::tolower(c); });

#endif  // UTILS_H