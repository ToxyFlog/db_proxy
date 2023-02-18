#include "client.hpp"
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <optional>
#include <string>
#include "config.hpp"
#include "utils.hpp"
#include "write.hpp"

static const int MEMORY_BLOCK_SIZE = 128;
static const size_t READ_BUFFER_LENGTH = 1024;

Client::Client(const char *ipAddress, uint16_t port) { connect(ipAddress, port); }
Client::~Client() { disconnect(); }

sockaddr_in Client::createAddress(const char *ipAddress, uint16_t port) {
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    if (inet_pton(AF_INET, ipAddress, &address.sin_addr) == 0) exitWithError("Invalid IP address!");
    return address;
}

void Client::connect(const char *ipAddress, uint16_t port) {
    disconnect();

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) exitWithError("Couldn't create a socket!");

    sockaddr_in address = createAddress(ipAddress, port);
    if (setFlag(fd, O_NONBLOCK, true) == -1) exitWithError("Error while setting a flag!");
    if (::connect(fd, (sockaddr *) &address, sizeof(address)) == -1 && errno != EINPROGRESS) exitWithError("Couldn't connect to server!");
    if (setFlag(fd, O_NONBLOCK, false) == -1) exitWithError("Error while setting a flag!");

    pollfd pfd{fd, POLLOUT, 0};
    if (!poll(&pfd, 1, CONNECT_TIMEOUT_SECONDS * 1000) || pfd.revents == POLLHUP) exitWithError("Couldn't connect to server!");

    timeval value{READ_TIMEOUT_SECONDS, 0};
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &value, sizeof(timeval));
}

ResourceId Client::createResource(std::string connectionString, std::string schema, std::string table) {
    Write write(fd);
    write((RequestType) CREATE_RESOURCE);
    write((ResourceId) htons(-1));
    write(connectionString);
    write(schema);
    write(table);
    if (!write.finish()) return -1;

    ResourceId resource;
    if (read(fd, &resource, sizeof(resource)) == -1) return -1;
    return ntohs(resource);
}

std::optional<SelectResult> Client::select(ResourceId resource, std::vector<std::string> columns) {
    Write write(fd);
    write((RequestType) SELECT);
    write((ResourceId) htons(resource));
    for (auto &column : columns) write(column);
    if (!write.finish()) return std::nullopt;

    FieldLength length;
    int tuples, fields;
    if (read(fd, &length, sizeof(length)) == -1) return std::nullopt;
    if (read(fd, &tuples, sizeof(tuples)) == -1) return std::nullopt;
    if (read(fd, &length, sizeof(length)) == -1) return std::nullopt;
    if (read(fd, &fields, sizeof(fields)) == -1) return std::nullopt;
    tuples = ntohl(tuples);
    fields = ntohl(fields);

    char *memoryBlock;
    int memoryOffset = MEMORY_BLOCK_SIZE, bufferReadOffset = 0, index = -1;
    char buffer[READ_BUFFER_LENGTH];
    ssize_t numRead, offset;
    SelectResult result(tuples, fields);
    FieldLength stringLength = 0, stringOffset = 0;
    while ((numRead = read(fd, buffer + bufferReadOffset, READ_BUFFER_LENGTH - bufferReadOffset)) > 0) {
        numRead += bufferReadOffset;
        bufferReadOffset = 0;

        for (offset = 0; offset < numRead;) {
            if (stringOffset < stringLength) {
                size_t readLength = min(stringLength - stringOffset, numRead - offset);
                std::memcpy(result[index / fields][index % fields] + stringOffset, buffer + offset, readLength);
                stringOffset += readLength;
                offset += readLength;
                continue;
            }

            if (offset + sizeof(FieldLength) > (size_t) numRead) {
                bufferReadOffset = numRead - offset;
                std::memcpy(buffer, buffer + offset, bufferReadOffset);
                break;
            }

            stringLength = ntohs(*(FieldLength *) (buffer + offset));
            if (stringLength == 0) return result;

            stringOffset = 0;
            offset += sizeof(FieldLength);

            if (memoryOffset + stringLength > MEMORY_BLOCK_SIZE) {
                memoryOffset = 0;
                memoryBlock = (char *) malloc(sizeof(char) * MEMORY_BLOCK_SIZE);
                result.memory.push_back(memoryBlock);
            }

            result[++index / fields][index % fields] = memoryBlock + memoryOffset;
            memoryOffset += stringLength + 1;
        }
    }

    while (index > 0) free(result[index / fields][index-- % fields]);
    return std::nullopt;
}

int Client::insert(ResourceId resource, std::vector<std::string> columns, std::vector<std::vector<std::string>> tuples) {
    Write write(fd);
    write((RequestType) INSERT);
    write((ResourceId) htons(resource));
    write((FieldLength) htons(columns.size()));
    for (auto &column : columns) write(column);
    for (auto &tuple : tuples)
        for (auto &value : tuple) write(value);
    if (!write.finish()) return -1;

    int32_t response;
    if (read(fd, &response, sizeof(response)) == -1) return -1;
    return ntohl(response);
}

void Client::disconnect() {
    if (fd == -1) return;

    close(fd);
    fd = -1;
}

void Client::clearResult(SelectResult &tuples) {
    for (auto memory : tuples.memory) free(memory);
}