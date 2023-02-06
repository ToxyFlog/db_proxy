#include <string>
#include <optional>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include "config.hpp"
#include "request.hpp"
#include "utils.hpp"
#include "write.hpp"
#include "client.hpp"

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

bool Client::waitForResponse() {
    pollfd pfd {fd, POLLIN, 0};
    if (!poll(&pfd, 1, RESPONSE_TIMEOUT_SECONDS * 1000) || pfd.revents & POLLHUP) return false;
    return true;
}

void Client::connect(const char *ipAddress, uint16_t port) {
    disconnect();

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) exitWithError("Couldn't create a socket!");

    sockaddr_in address = createAddress(ipAddress, port);
    if (setFlag(fd, O_NONBLOCK, true) == -1) exitWithError("Error while setting a flag!");
    if (::connect(fd, (sockaddr*) &address, sizeof(address)) == -1 && errno != EINPROGRESS) exitWithError("Couldn't connect to server!");
    if (setFlag(fd, O_NONBLOCK, false) == -1) exitWithError("Error while setting a flag!");

    pollfd pfd {fd, POLLOUT, 0};
    if (!poll(&pfd, 1, CONNECTION_TIMEOUT_SECONDS * 1000) || pfd.revents == POLLHUP) exitWithError("Couldn't connect to server!");
}

std::optional<ResourceId> Client::createResource(std::string connectionString, std::string schema, std::string table) {
    RequestLength length = sizeof(RequestType) + sizeof(ResourceId) + 3 * sizeof(RequestStringLength) + connectionString.size() + schema.size() + table.size();

    Write write(fd);
    write((RequestLength) htonl(length));
    write((RequestType) CREATE_RESOURCE);
    write((ResourceId) htonl(-1));
    write(connectionString);
    write(schema);
    write(table);
    if (!write.finish() || !waitForResponse()) return std::nullopt;

    ResourceId resource;
    if (read(fd, &resource, sizeof(resource)) == -1) return std::nullopt;
    return ntohl(resource);
}

std::optional<SelectResult> Client::select(ResourceId resource, std::vector<std::string> columns) {
    RequestLength length = sizeof(RequestType) + sizeof(ResourceId);
    for (auto &column : columns) length += sizeof(RequestStringLength) + column.size();

    Write write(fd);
    write((RequestLength) htonl(length));
    write((RequestType) SELECT);
    write((ResourceId) htonl(resource));
    for (auto &column : columns) write(column);
    if (!write.finish() || !waitForResponse()) return std::nullopt;

    int tuples, fields;
    read(fd, &tuples, sizeof(tuples));
    tuples = ntohl(tuples);
    read(fd, &fields, sizeof(fields));
    fields = htonl(fields);

    int bufferStringLengthOffset = 0;
    ssize_t numRead, offset;
    char buffer[READ_BUFFER_LENGTH];
    size_t index = -1;
    SelectResult result(tuples, std::vector<char*>(fields));
    RequestStringLength stringLength = 0, stringOffset = 0;
    while (waitForResponse()) {
        offset = 0;
        if ((numRead = read(fd, buffer + bufferStringLengthOffset, READ_BUFFER_LENGTH - bufferStringLengthOffset)) == -1) return std::nullopt;
        numRead += bufferStringLengthOffset;
        bufferStringLengthOffset = 0;

        while (offset < numRead) {
            if (stringOffset < stringLength) {
                size_t readLength = min(stringLength - stringOffset, numRead - offset);
                std::memcpy(result[index / fields][index % fields] + stringOffset, buffer + offset, readLength);
                stringOffset += readLength;
                offset += readLength;
                continue;
            }

            if (sizeof(RequestStringLength) + offset > numRead) {
                bufferStringLengthOffset = numRead - offset;
                std::memcpy(buffer, buffer + offset, bufferStringLengthOffset);
                offset += sizeof(RequestStringLength);
                break;
            }

            stringOffset = 0;
            stringLength = ntohs(*(RequestStringLength*) &buffer[offset]);
            offset += sizeof(RequestStringLength);
            if (stringLength == 0) return result;

            result[++index / fields][index % fields] = (char*) malloc(sizeof(char) * stringLength);
        }
    }

    return std::nullopt;
}

void Client::disconnect() {
    if (fd == -1) return;

    close(fd);
    fd = -1;
}

void Client::clear(SelectResult &tuples) {
    for (auto &tuple : tuples)
        for (auto field : tuple)
            free(field);
}