#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "utils.hpp"
#include "config.hpp"
#include "client.hpp"

static const int CONNECTION_TIMEOUT_MS = 5000;
static const int RESPONSE_TIMEOUT_MS = 20000;

sockaddr_in Client::create_address(const char *ip_address, uint16_t port) {
    sockaddr_in address;
    address.sin_family = AF_INET;
    if (inet_pton(AF_INET, ip_address, &address.sin_addr) == 0) exit_with_error("Invalid IP address!");
    address.sin_port = htons(port);
    return address;
}

bool Client::wait_for_response() {
    pollfd pfd {fd, POLLIN, 0};
    if (!poll(&pfd, 1, RESPONSE_TIMEOUT_MS) || pfd.revents & POLLHUP) return false;

    return true;
}

void Client::connect_to(const char *ip_address, uint16_t port) {
    if (fd != -1) close(fd);

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) exit_with_error("Couldn't create a socket!");

    sockaddr_in address = create_address(ip_address, port);
    set_fd_flag(fd, O_NONBLOCK);
    if (connect(fd, (sockaddr*) &address, sizeof(address)) == -1 && errno != EINPROGRESS) exit_with_error("Couldn't connect to server!");
    set_fd_flag(fd, O_NONBLOCK);

    pollfd pfd {fd, POLLOUT, 0};
    if (!poll(&pfd, 1, CONNECTION_TIMEOUT_MS) || pfd.revents == POLLHUP) exit_with_error("Couldn't connect to server!");
}

int Client::create_resource(std::string connection_string, columns_t columns) {
    uint32_t length = sizeof(type_t) + sizeof(int32_t) + net_string_size(connection_string);
    for (auto &[type, name] : columns) length += net_string_size(type) + net_string_size(name);
    write_variable(fd, uint32_t, htonl(length));

    write_variable(fd, type_t, CREATE_RESOURCE);
    write_variable(fd, int32_t, htonl(-1));
    write_str(fd, connection_string);
    for (auto &[type, name] : columns) {
        write_str(fd, type);
        write_str(fd, name);
    }

    if (!wait_for_response()) return -1;
    int32_t resource_id;
    int num_read = read(fd, &resource_id, sizeof(int32_t));
    return num_read == -1 ? -1 : ntohl(resource_id);
}

std::vector<std::vector<std::string>> Client::select(int resource, std::vector<std::string> columns) {
    return {};
}