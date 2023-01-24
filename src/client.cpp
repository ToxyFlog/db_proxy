#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "write.hpp"
#include "utils.hpp"
#include "config.hpp"
#include "client.hpp"

static const size_t READ_BUFFER_LENGTH = 1024;

Client::Client(): fd(-1) {}
Client::Client(const char *ip_address, uint16_t port) { connect(ip_address, port); }
Client::~Client() { disconnect(); }

sockaddr_in Client::create_address(const char *ip_address, uint16_t port) {
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    if (inet_pton(AF_INET, ip_address, &address.sin_addr) == 0) exit_with_error("Invalid IP address!");
    return address;
}

std::string Client::read() {
    pollfd pfd {fd, POLLIN, 0};
    if (!poll(&pfd, 1, RESPONSE_TIMEOUT_MS) || pfd.revents & POLLHUP) return "";

    ssize_t num_read;
    std::string result = "";
    char buffer[READ_BUFFER_LENGTH];

    set_fd_flag(fd, O_NONBLOCK);
    while ((num_read = ::read(fd, buffer, READ_BUFFER_LENGTH)) > 0) result += std::string(buffer, buffer + num_read);
    set_fd_flag(fd, O_NONBLOCK);

    if (num_read == -1 && errno != EWOULDBLOCK) return "";
    return result;
}

void Client::connect(const char *ip_address, uint16_t port) {
    disconnect();

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) exit_with_error("Couldn't create a socket!");

    sockaddr_in address = create_address(ip_address, port);
    set_fd_flag(fd, O_NONBLOCK);
    if (::connect(fd, (sockaddr*) &address, sizeof(address)) == -1 && errno != EINPROGRESS) exit_with_error("Couldn't connect to server!");
    set_fd_flag(fd, O_NONBLOCK);

    pollfd pfd {fd, POLLOUT, 0};
    if (!poll(&pfd, 1, CONNECTION_TIMEOUT_MS) || pfd.revents == POLLHUP) exit_with_error("Couldn't connect to server!");
}

int Client::create_resource(std::string connection_string, std::string schema, std::string table) {
    bool error = false;
    uint32_t length = sizeof(type_t) + sizeof(int32_t) + string_size(connection_string) + string_size(schema) + string_size(table);

    begin_write(fd, &error);
    write_variable(uint32_t, htonl(length));
    write_variable(type_t, CREATE_RESOURCE);
    write_variable(int32_t, htonl(-1));
    write_str(connection_string);
    write_str(schema);
    write_str(table);
    end_write();
    if (error) return -1;

    std::string response = read();
    if (response.size() == 0) return -1;

    return ntohl(*(int32_t*) response.c_str());
}

std::vector<std::vector<std::string>> Client::select(int32_t resource, std::vector<std::string> columns) {
    bool error = false;
    uint32_t length = sizeof(type_t) + sizeof(int32_t);
    for (auto &column : columns) length += string_size(column);

    begin_write(fd, &error);
    write_variable(uint32_t, htonl(length));
    write_variable(type_t, SELECT);
    write_variable(int32_t, htonl(resource));
    for (auto &column : columns) write_str(column);
    end_write();
    if (error) return {};

    bool a = false;
    string_len_t str_length = 0;
    std::vector<std::vector<std::string>> result = {{}};
    for (;;) {
        std::string response = read();
        size_t response_size = response.size();
        if (response_size == 0) return {};

        size_t offset = 0;
        while (offset < response_size) {
            if (str_length == 0) {
                str_length = ntohs(*(string_len_t*) &response[offset]);
                if (str_length == 0) return result;
                offset += sizeof(string_len_t);
                result.back().push_back("");
            }

            size_t cur = min(str_length, response.size() - offset);
            result.back().back() += response.substr(offset, cur);
            offset += cur;
            str_length -= cur;
            if (str_length == 0 && result.back().size() == columns.size()) result.push_back({});
        }
    }
}

void Client::disconnect() {
    if (fd == -1) return;

    close(fd);
    fd = -1;
}