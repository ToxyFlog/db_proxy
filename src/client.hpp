#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <optional>
#include <arpa/inet.h>
#include "config.hpp"
#include "request.hpp"

class Client {
private:
    int fd;

    sockaddr_in createAddress(const char *ipAddress, uint16_t port);
    bool waitForResponse();
public:
    Client();
    Client(const char *ipAddress, uint16_t port);
    ~Client();

    void connect(const char *ipAddress, uint16_t port);
    void disconnect();

    std::optional<ResourceId> createResource(std::string connectionString, std::string schema, std::string table);
    std::optional<std::vector<std::vector<std::string>>> select(ResourceId resource, std::vector<std::string> columns);
};

#endif // CLIENT_H