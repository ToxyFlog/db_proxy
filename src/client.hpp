#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <optional>
#include <arpa/inet.h>
#include "config.hpp"
#include "request.hpp"

typedef std::vector<std::vector<char*>> SelectResult;

class Client {
private:
    int fd = -1;

    sockaddr_in createAddress(const char *ipAddress, uint16_t port);
    bool waitForResponse();
public:
    Client(const char *ipAddress, uint16_t port);
    ~Client();

    void connect(const char *ipAddress, uint16_t port);
    void disconnect();

    std::optional<ResourceId> createResource(std::string connectionString, std::string schema, std::string table);
    std::optional<SelectResult> select(ResourceId resource, std::vector<std::string> columns);

    static void clear(SelectResult &select);
};

#endif // CLIENT_H