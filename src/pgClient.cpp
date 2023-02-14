#include "pgClient.hpp"
#include <optional>
#include <string>
#include <vector>
#include "libpq-fe.h"
#include "workers.hpp"

static const char *CONNECTION_STRING_PARAMS = "connect_timeout=10";

PGResponse::PGResponse(PGresult *_result) : result(_result) {
    tuples = PQntuples(result);
    fields = PQnfields(result);

    array = (char **) malloc(sizeof(char *) * tuples * fields);
    for (int i = 0; i < tuples * fields; i++) array[i] = PQgetvalue(result, i / fields, i % fields);
}

char *PGResponse::get(int tuple, int field) {
    if (tuple >= tuples) return nullptr;
    return array[tuple * fields + field];
};

void PGResponse::clear() {
    free(array);
    PQclear(result);
}

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
    if (PQresultStatus(response) > PGRES_TUPLES_OK) {
        PQclear(response);
        return std::nullopt;
    }

    return PGResponse(response);
}

void PGClient::disconnect() {
    if (connection) PQfinish(connection);
    connection = nullptr;
}