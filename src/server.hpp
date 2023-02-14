#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <arpa/inet.h>
#include <string>
#include <vector>
#include "request.hpp"

typedef std::function<bool(int, std::string&)> RequestHandler;

struct Connection {
    int fd;
    std::string request;
    RequestLength length;
};

class Server {
private:
    int fd;
    RequestHandler handler;
    std::vector<Connection> connections;

    sockaddr_in createAddress(uint16_t port);

public:
    Server(RequestHandler _handler);

    void startListening(uint16_t port);
    void stopListening();

    void acceptConnections();
    void readRequests();

    void waitForEvent();
};

#endif  // TCP_SERVER_H