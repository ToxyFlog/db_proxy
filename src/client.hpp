#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <arpa/inet.h>
#include "config.hpp"
#include "utils.hpp"

class Client {
private:
    int fd;

    sockaddr_in create_address(const char *ip_address, uint16_t port);

    std::string read();
public:
    Client();
    Client(const char *ip_address, uint16_t port);
    ~Client();

    void connect(const char *ip_address, uint16_t port);
    void disconnect();

    int create_resource(std::string connection_string, std::string schema, std::string table);
    std::vector<std::vector<std::string>> select(int resource, std::vector<std::string> columns);
};

#endif // CLIENT_H