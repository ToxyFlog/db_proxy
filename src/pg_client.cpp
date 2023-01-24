#include <string>
#include <vector>
#include <optional>
#include "libpq-fe.h"
#include "workers.hpp"
#include "pg_client.hpp"

static const char *CONNECTION_STRING_PARAMS = "connect_timeout=10";

bool PGClient::connect(Resource &resource) {
    disconnect();

    std::string connection_string = resource.connection_string;
    connection_string += connection_string.find("?") == std::string::npos ? "?" : "&";
    connection_string += CONNECTION_STRING_PARAMS;

    connection = PQconnectdb(connection_string.c_str());

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

    PGResponse result(PQntuples(response));
    for (int tuple = 0; tuple < PQntuples(response); tuple++)
        for (int field = 0; field < PQnfields(response); field++)
            result[tuple].push_back(std::string(PQgetvalue(response, tuple, field)));

    PQclear(response);
    return result;
}

void PGClient::disconnect() {
    if (connection) PQfinish(connection);
    connection = nullptr;
}