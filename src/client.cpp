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

Client::Client(): fd(-1) {}
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
    if (!poll(&pfd, 1, RESPONSE_TIMEOUT_MS) || pfd.revents & POLLHUP) return false;
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
    if (!poll(&pfd, 1, CONNECTION_TIMEOUT_MS) || pfd.revents == POLLHUP) exitWithError("Couldn't connect to server!");
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

std::optional<std::vector<std::vector<std::string>>> Client::select(ResourceId resource, std::vector<std::string> columns) {
    RequestLength length = sizeof(RequestType) + sizeof(ResourceId);
    for (auto &column : columns) length += sizeof(RequestStringLength) + column.size();

    Write write(fd);
    write((RequestLength) htonl(length));
    write((RequestType) SELECT);
    write((ResourceId) htonl(resource));
    for (auto &column : columns) write(column);
    if (!write.finish()) return std::nullopt;

    RequestStringLength stringLength = 0;
    char buffer[READ_BUFFER_LENGTH];
    std::vector<std::vector<std::string>> result = {{}};
    while (waitForResponse()) {
        ssize_t numRead, offset = 0;
        if ((numRead = read(fd, buffer, READ_BUFFER_LENGTH)) == -1) return std::nullopt;

        while (offset < numRead) {
            if (stringLength == 0) {
                stringLength = ntohs(*(RequestStringLength*) &buffer[offset]);
                offset += sizeof(RequestStringLength);
            }
            if (stringLength == 0) return result;

            if (result.back().size() == columns.size()) result.push_back({});

            if (offset < numRead) {
                size_t readLength = min(stringLength, numRead - offset);
                result.back().push_back(std::string(buffer + offset, buffer + offset + readLength));
                stringLength -= readLength;
                offset += readLength;
            }
        }
    }

    return std::nullopt;
}

void Client::disconnect() {
    if (fd == -1) return;

    close(fd);
    fd = -1;
}