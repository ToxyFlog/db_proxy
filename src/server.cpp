#include <cerrno>
#include <string>
#include <vector>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "utils.hpp"
#include "config.hpp"
#include "server.hpp"

static const int BUFFER_LENGTH = 256;

Server::Server(RequestHandler _handler): handler(_handler) {}

sockaddr_in Server::createAddress(uint16_t port) {
    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    return address;
}

void Server::startListening(uint16_t port) {
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) exitWithError("Couldn't create a socket!");
    if (setFlag(fd, O_NONBLOCK, true) == -1) exitWithError("Error while setting a flag!");

    int value = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));
    sockaddr_in address = createAddress(port);
    if (bind(fd, (sockaddr*) &address, sizeof(address)) == -1) exitWithError("Couldn't bind to the address!");
    if (listen(fd, SOMAXCONN) == -1) exitWithError("Couldn't start listening!");
}

void Server::acceptConnections() {
    int connection;
    while ((connection = accept(fd, NULL, NULL)) != -1) connections.push_back({connection, "", 0});
    if (errno != EWOULDBLOCK) exitWithError("Couldn't accept connection!");
}

void Server::readRequests() {
    ssize_t numRead;
    char buffer[BUFFER_LENGTH];
    for (size_t i = 0;i < connections.size();i++) {
        Connection &connection = connections[i];
        bool error = false;

        if (connection.length == 0) {
            RequestLength length;
            numRead = read(connection.fd, &length, sizeof(length));
            if (numRead == -1 && errno == EWOULDBLOCK) continue;

            if (numRead != sizeof(length)) error = true;
            else length = ntohl(length);
            connection.length = length;
        }

        while ((numRead = read(connection.fd, buffer, min(connection.length, BUFFER_LENGTH))) > 0) {
            connection.length -= numRead;
            connection.request += std::string(buffer, buffer + numRead);

            if (connection.length == 0) {
                if (!handler(connection.fd, connection.request)) error = true;
                connection.request = "";
                break;
            }
        }

        if (error || (numRead == -1 && errno != EWOULDBLOCK)) {
            close(connection.fd);
            connections.erase(connections.begin() + i);
            i--;
        }
    }
}

void Server::waitForEvent() {
    if (!connections.empty()) return;

    pollfd pfd {fd, POLLIN, 0};
    poll(&pfd, 1, -1);
}

void Server::stopListening() {
    for (auto &connection : connections) close(connection.fd);
    close(fd);
}