#ifndef CLIENT_H
#define CLIENT_H

#include <arpa/inet.h>
#include <optional>
#include <string>
#include <vector>
#include "config.hpp"
#include "request.hpp"

class SelectResult {
private:
    std::vector<std::vector<char *>> storage;

public:
    std::vector<char *> memory;

    SelectResult(int tuples, int fields) : storage(tuples, std::vector<char *>(fields)) {}

    inline std::vector<char *> &operator[](int index) { return storage[index]; }
    inline size_t size() { return storage.size(); }
};

class Client {
private:
    int fd = -1;

    sockaddr_in createAddress(const char *ipAddress, uint16_t port);

public:
    Client(const char *ipAddress, uint16_t port);
    ~Client();

    void connect(const char *ipAddress, uint16_t port);
    void disconnect();

    ResourceId createResource(std::string connectionString, std::string schema, std::string table);

    std::optional<SelectResult> select(ResourceId resource, std::vector<std::string> columns);
    static void clearResult(SelectResult &select);

    int insert(ResourceId resource, std::vector<std::string> columns, std::vector<std::vector<std::string>> values);
};

#endif  // CLIENT_H