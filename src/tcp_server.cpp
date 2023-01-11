#include <string>
#include <vector>
#include <cerrno>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "utils.hpp"
#include "config.hpp"
#include "tcp_server.hpp"

static const int BUFFER_LENGTH = 5;

TCPServer::TCPServer(std::function<bool (int, std::string)> _process_message): process_message(_process_message) {}

sockaddr_in TCPServer::create_address(uint16_t port) {
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    return address;
}

void TCPServer::start_listening(uint16_t port) {
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) exit_with_error("Couldn't create a socket!");
    set_fd_flag(fd, O_NONBLOCK);

    int value = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));
    sockaddr_in address = create_address(port);
    if (bind(fd, (sockaddr*) &address, sizeof(address)) == -1) exit_with_error("Couldn't bind to the address!");
    if (listen(fd, SOMAXCONN) == -1) exit_with_error("Couldn't start listening!");
}

void TCPServer::accept_connections() {
    int connection;
    while ((connection = accept(fd, NULL, NULL)) != -1) connections.push_back(connection);
    if (errno != EWOULDBLOCK) exit_with_error("Couldn't accept connection!");
}

void TCPServer::read_messages() {
    while (messages.size() < connections.size()){
        messages.push_back("");
        message_lengths.push_back(0);
    }

    ssize_t num_read;
    char buffer[BUFFER_LENGTH];
    for (size_t i = 0;i < connections.size();i++) {
        bool error = false;

        if (message_lengths[i] == 0) {
            uint32_t length;
            num_read = read(connections[i], &length, sizeof(length));
            if (num_read == -1 && errno == EWOULDBLOCK) continue;

            if (num_read != sizeof(length)) error = true;
            else length = ntohl(length);
            message_lengths[i] = length;
        }

        while ((num_read = read(connections[i], buffer, min(message_lengths[i], BUFFER_LENGTH))) > 0) {
            message_lengths[i] -= num_read;
            messages[i] += std::string(buffer, buffer + num_read);

            if (message_lengths[i] == 0) {
                if (!process_message(connections[i], messages[i])) error = true;
                messages[i] = "";
                break;
            }
        }

        if (error || (num_read == -1 && errno != EWOULDBLOCK)) {
            close(connections[i]);
            connections.erase(connections.begin() + i);
            messages.erase(messages.begin() + i);
            message_lengths.erase(message_lengths.begin() + i);
            i--;
        }
    }
}

void TCPServer::wait_for_event() {
    if (!connections.empty()) return;

    pollfd pfd {fd, POLLIN, 0};
    poll(&pfd, 1, -1);
}

void TCPServer::stop_listening() {
    for (auto &connection : connections) close(connection);
    close(fd);
}