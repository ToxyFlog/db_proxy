#include <optional>
#include <string>
#include <vector>
#include "libpq-fe.h"
#include "workers.hpp"
#include "pgClient.hpp"

static const char *CONNECTION_STRING_PARAMS = "connect_timeout=10";

bool PGClient::connect(Resource &resource) {
    disconnect();

    std::string connectionString = resource.connectionString;
    connectionString += connectionString.find("?") == std::string::npos ? "?" : "&";
    connectionString += CONNECTION_STRING_PARAMS;

    connection = PQconnectdb(connectionString.c_str());

    if (PQstatus(connection) != CONNECTION_OK) {
        disconnect();
        return false;
    }

    query("SET search_path = '" + resource.schema + "';");
    return true;
}

std::optional<PGResponse> PGClient::query(std::string sql) { return query(sql.c_str()); };
std::optional<PGResponse> PGClient::query(const char *sql) {
    PGresult *response = PQexec(connection, sql);
    if (PQresultStatus(response) != PGRES_TUPLES_OK) {
        PQclear(response);
        return std::nullopt;
    }

    PGResponse result;
    for (int tuple = 0; tuple < PQntuples(response); tuple++) {
        result.push_back({});
        for (int field = 0; field < PQnfields(response); field++) {
            std::string value = PQgetvalue(response, tuple, field);
            result[tuple].push_back(value);
        }
    }

    PQclear(response);
    return result;
}

void PGClient::disconnect() {
    if (connection) PQfinish(connection);
    connection = nullptr;
}