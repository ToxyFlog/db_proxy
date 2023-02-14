#ifndef PG_CLIENT_H
#define PG_CLIENT_H

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include "libpq-fe.h"

struct Column {
    std::string name;
    std::string type;
    bool optional;
};

struct Resource {
    std::string connectionString;
    std::string schema;
    std::string table;
    int requiredColumns;
    std::unordered_map<std::string, Column> columns;
};

class PGResponse {
private:
    PGresult *result;

public:
    int tuples, fields;
    char **array;

    PGResponse(PGresult *_result);

    char *get(int tuple, int field);
    void clear();
};

class PGClient {
private:
    PGconn *connection = nullptr;

public:
    bool connect(Resource &resource);
    void disconnect();

    std::optional<PGResponse> query(std::string sql);
    std::optional<PGResponse> query(const char *sql);
};

#endif  // PG_CLIENT_H