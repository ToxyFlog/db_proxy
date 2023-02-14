#ifndef REQUEST_H
#define REQUEST_H

#include <cstdlib>

typedef uint32_t RequestLength;
typedef int32_t ResourceId;
typedef uint8_t RequestType;

enum Type : RequestType { CREATE_RESOURCE, SELECT, INSERT };

typedef uint16_t RequestStringLength;

#endif  // REQUEST_H