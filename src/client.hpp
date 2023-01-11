#ifndef CLIENT_H
#define CLIENT_H

#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include "config.hpp"
#include "utils.hpp"

class Client {
private:
    int fd = -1;

    sockaddr_in create_address(const char *ip_address, uint16_t port);

    bool wait_for_response();
public:
    void connect_to(const char *ip_address, uint16_t port);

    int create_resource(std::string connection_string, columns_t columns);
    std::vector<std::vector<std::string>> select(int resource, std::vector<std::string> columns);
};

#endif // CLIENT_H