#include "client.hpp"
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <optional>
#include <string>
#include "config.hpp"
#include "request.hpp"
#include "utils.hpp"
#include "write.hpp"

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
    RequestLength length = sizeof(RequestType) + sizeof(ResourceId) + 3 * sizeof(RequestStringLength) + connectionString.size() + schema.size() + table.size();

    Write write(fd);
    write((RequestLength) htonl(length));
    write((RequestType) CREATE_RESOURCE);
    write((ResourceId) htonl(-1));
    write(connectionString);
    write(schema);
    write(table);
    if (!write.finish()) return -1;

    ResourceId resource;
    if (read(fd, &resource, sizeof(resource)) == -1) return -1;
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
    if (!write.finish()) return std::nullopt;

    int tuples, fields;
    if (read(fd, &tuples, sizeof(tuples)) == -1) return std::nullopt;
    tuples = ntohl(tuples);
    if (read(fd, &fields, sizeof(fields)) == -1) return std::nullopt;
    fields = htonl(fields);

    int bufferReadOffset = 0;
    ssize_t numRead;
    char buffer[READ_BUFFER_LENGTH];
    int index = -1;
    SelectResult result(tuples, std::vector<char *>(fields));
    RequestStringLength stringLength = 0, stringOffset = 0;
    while ((numRead = read(fd, buffer + bufferReadOffset, READ_BUFFER_LENGTH - bufferReadOffset)) > 0) {
        numRead += bufferReadOffset;
        bufferReadOffset = 0;

        for (ssize_t offset = 0; offset < numRead;) {
            if (stringOffset < stringLength) {
                size_t readLength = min(stringLength - stringOffset, numRead - offset);
                std::memcpy(result[index / fields][index % fields] + stringOffset, buffer + offset, readLength);
                stringOffset += readLength;
                offset += readLength;
                continue;
            }

            if (offset + sizeof(RequestStringLength) > (size_t) numRead) {
                bufferReadOffset = numRead - offset;
                std::memcpy(buffer, buffer + offset, bufferReadOffset);
                break;
            }

            stringOffset = 0;
            stringLength = ntohs(*(RequestStringLength *) &buffer[offset]);
            offset += sizeof(RequestStringLength);
            if (stringLength == 0) return result;

            result[++index / fields][index % fields] = (char *) malloc(sizeof(char) * stringLength);
        }
    }

    while (index > 0) free(result[index / fields][index-- % fields]);
    return std::nullopt;
}

int Client::insert(ResourceId resource, std::vector<std::string> columns, std::vector<std::vector<std::string>> tuples) {
    RequestLength length = sizeof(RequestType) + sizeof(ResourceId) + sizeof(RequestStringLength);
    for (auto &column : columns) length += column.size() + sizeof(RequestStringLength);
    for (auto &tuple : tuples)
        for (auto &value : tuple) length += value.size() + sizeof(RequestStringLength);

    Write write(fd);
    write((RequestLength) htonl(length));
    write((RequestType) INSERT);
    write((ResourceId) htonl(resource));
    write((RequestStringLength) htons(columns.size()));
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
    for (auto &tuple : tuples)
        for (auto field : tuple) free(field);
}