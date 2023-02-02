#ifndef PG_CLIENT_H
#define PG_CLIENT_H

#include <optional>
#include <string>
#include <vector>
#include "libpq-fe.h"

struct Column {
    std::string name;
    std::string type;
    bool nullable;
};

struct Resource {
    std::string connectionString;
    std::string schema;
    std::string table;
    std::vector<Column> columns;
};

typedef std::vector<std::vector<std::string>> PGResponse;

class PGClient {
private:
    PGconn *connection = nullptr;
public:
    bool connect(Resource &resource);
    void disconnect();

   std::optional<PGResponse> query(std::string sql);
   std::optional<PGResponse> query(const char *sql);
};

#endif // PG_CLIENT_H