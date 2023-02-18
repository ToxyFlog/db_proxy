#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <arpa/inet.h>
#include <string>
#include <vector>
#include "utils.hpp"

typedef std::function<bool(int, std::vector<std::string>&)> RequestHandler;

struct Connection {
    int fd;
    FieldLength length;
    std::vector<std::string> fields;
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