#include <string>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include "pgClient.hpp"
#include "request.hpp"
#include "utils.hpp"
#include "workQueue.hpp"
#include "write.hpp"
#include "workers.hpp"

static const ResourceId NO_RESOURCE = -1;

WorkQueue workQueue;
bool isRunning = true;

std::vector<Resource> resources;
std::mutex resourcesMutex;

inline std::string selectTableSchema(std::string &table) {
    return "SELECT column_name, data_type, is_nullable, column_default FROM information_schema.columns WHERE table_name = '" + table + "'";
}
inline std::string selectFrom(Resource &resource) {
    std::string columns = "";
    for (auto &[name, column] : resource.columns) columns += name + ",";
    columns.pop_back();

    return "SELECT " + columns + " FROM " + resource.table;
}
inline std::string insertInto(std::string &table, std::string &columns, std::string &values) {
    return "INSERT INTO " + table + "(" + columns + ") VALUES " + values;
}

void createResource(PGClient &client, Batch &batch) {
    int fd = batch.fd;
    Resource resource = batch.resource;

    if (!client.connect(resource)) {
        writeVariable(fd, ResourceId, htonl(NO_RESOURCE));
        return;
    }

    std::optional<PGResponse> response = client.query(selectTableSchema(resource.table));
    if (!response.has_value()) {
        writeVariable(fd, ResourceId, htonl(NO_RESOURCE));
        return;
    }

    PGResponse tuples = response.value();
    for (int tuple = 0;tuple < tuples.tuples;tuple++) {
        bool optional = strcmp(tuples.get(tuple, 2), "NO") == 0 && strlen(tuples.get(tuple, 3)) == 0;
        if (!optional) resource.requiredColumns++;

        Column column {tuples.get(tuple, 0), tuples.get(tuple, 1), optional};
        resource.columns[column.name] = column;
    }
    tuples.clear();

    std::unique_lock resourcesLock(resourcesMutex);
    writeVariable(fd, ResourceId, htonl(resources.size()));
    resources.push_back(resource);
    resourcesLock.unlock();
}

void completeSelectOperation(Select &select, std::vector<std::string> &columns, PGResponse &response) {
    if (setFlag(select.fd, O_NONBLOCK, false) == -1) return;

    std::vector<int> fields;
    std::unordered_set<std::string> currentColumns = select.columns;
    for (int field = 0;field < response.fields;field++)
        if (currentColumns.contains(columns[field]))
            fields.push_back(field);

    Write write(select.fd);
    write(htonl(response.tuples));
    write(htonl(fields.size()));
    for (int tuple = 0;tuple < response.tuples;tuple++)
        for (auto field : fields)
            if (!write(response.get(tuple, field))) return;
    write((RequestStringLength) 0);
    write.finish();

    if (setFlag(select.fd, O_NONBLOCK, true) == -1) return;
}

void select(PGClient &client, Batch &batch) {
    std::vector<std::string> columns(batch.columns.begin(), batch.columns.end());
    std::optional<PGResponse> response = client.query(selectFrom(resources[batch.resourceId]));
    if (!response.has_value()) return;

    PGResponse tuples = response.value();
    for (auto &operation : batch.selects) completeSelectOperation(operation, columns, tuples);
    tuples.clear();
}

void insert(PGClient &client, Batch &batch) {
    Resource &resource = resources[batch.resourceId];

    size_t length = 0;
    std::unordered_map<std::string, std::vector<std::string>> columnValues;
    for (auto &insert : batch.inserts) {
        for (size_t i = 0;i < insert.columns.size();i++) {
            std::string columnName = insert.columns[i];
            std::vector<std::string> &column = columnValues[columnName];

            if (!columnValues.contains(columnName)) columnValues[columnName] = {};
            while (column.size() < length) column.push_back("DEFAULT");

            std::string type = resource.columns[columnName].type;
            bool isString = type == "text" || type == "character varying";

            for (size_t j = i;j < insert.values.size();j += insert.columns.size()) {
                std::string value = insert.values[j];
                if (isString) value = "'" + value + "'";
                column.push_back(value);
            }
        }

        length += insert.values.size() / insert.columns.size();
    }

    std::string columns = "";
    std::vector<std::string> values(length, "(");
    for (auto &[name, column] : columnValues) {
        columns += name + ",";
        while (column.size() < length) column.push_back("DEFAULT");
        for (size_t i = 0;i < length;i++) values[i] += column[i] + ",";
    }
    columns.pop_back();

    std::string valuesStr = "";
    for (auto &value : values) {
        value.pop_back();
        valuesStr += value + "),";
    }
    valuesStr.pop_back();

    std::optional<PGResponse> response = client.query(insertInto(resource.table, columns, valuesStr));

    int32_t result = response.has_value() ? 0 : -1;
    for (auto &insert : batch.inserts) writeVariable(insert.fd, int32_t, htonl(result));
}

void processBatches() {
    PGClient client;
    while (isRunning) {
        std::optional<Batch> element = workQueue.pop();
        if (!isRunning) return;

        Batch batch = element.value();
        if (batch.type != CREATE_RESOURCE && !client.connect(resources[batch.resourceId])) continue;

        try {
            switch (batch.type) {
                case CREATE_RESOURCE:
                    createResource(client, batch);
                    break;
                case SELECT:
                    select(client, batch);
                    break;
                case INSERT:
                    insert(client, batch);
                    break;
            }
        } catch(std::bad_variant_access exception) {}

        client.disconnect();
    }
}

void createWorkers() {
    for (int i = 0;i < WORKER_THREADS;i++) {
        std::thread worker(processBatches);
        worker.detach();
    }
}

void stopWorkers() {
    isRunning = false;
    workQueue.stopWaiting();
}