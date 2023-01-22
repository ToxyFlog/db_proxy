#ifndef PG_CLIENT_H
#define PG_CLIENT_H

#include <vector>
#include <string>
#include <optional>
#include "libpq-fe.h"

typedef std::vector<std::vector<char*>> PGResponse;

class PGClient {
private:
    PGconn *connection = nullptr;
public:
    bool connect(std::string &connection_string);
    void disconnect();

   std::optional<PGResponse> query(const char *sql);
};

#endif // PG_CLIENT_H