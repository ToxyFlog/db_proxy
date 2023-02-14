#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

void exitWithError(const char *message);
uint64_t unixTimeInMilliseconds();
int setFlag(int fd, int flag, bool value);

#define min(x, y) (x < y ? x : y)
#define max(x, y) (x > y ? x : y)

#endif // UTILS_H