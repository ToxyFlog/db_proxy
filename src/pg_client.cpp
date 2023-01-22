#include <string>
#include <vector>
#include <optional>
#include "libpq-fe.h"
#include "pg_client.hpp"

static const char *CONNECTION_STRING_PARAMS = "connect_timeout=10";

bool PGClient::connect(std::string &connection_string) {
    disconnect();

    connection_string += connection_string.find("?") == std::string::npos ? "?" : "&";
    connection_string += CONNECTION_STRING_PARAMS;

    connection = PQconnectdb(connection_string.c_str());

    if (PQstatus(connection) != CONNECTION_OK) {
        disconnect();
        return false;
    }

    return true;
}

std::optional<PGResponse> PGClient::query(const char *sql) {
    PGresult *response = PQexec(connection, sql);
    if (PQresultStatus(response) != PGRES_TUPLES_OK) {
        PQclear(response);
        return std::nullopt;
    }

    PGResponse result(PQntuples(response));
    for (int tuple = 0; tuple < PQntuples(response); tuple++)
        for (int field = 0; field < PQnfields(response); field++)
            result[tuple].push_back(PQgetvalue(response, tuple, field));

    PQclear(response);
    return result;
}

void PGClient::disconnect() {
    if (connection) PQfinish(connection);
    connection = nullptr;
}