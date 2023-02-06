#include <string>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_set>
#include <variant>
#include <fcntl.h>
#include <unistd.h>
#include "pgClient.hpp"
#include "request.hpp"
#include "utils.hpp"
#include "workQueue.hpp"
#include "write.hpp"
#include "workers.hpp"

static const int NO_RESOURCE = -1;

WorkQueue workQueue;
bool isRunning = true;

std::vector<Resource> resources;
std::mutex resourcesMutex;

inline std::string selectColumns(std::vector<std::string> &columns, std::string &table) {
    return "SELECT " + join(columns, ',') + " FROM " + table;
}
inline std::string selectTableSchema(std::string &table) {
    return "SELECT column_name, data_type, is_nullable, column_default FROM information_schema.columns WHERE table_name = '" + table + "';";
}

void createResource(PGClient &client, Batch &batch) {
    Operation operation = batch.operations[0];
    Resource resource = std::get<Resource>(operation.data);

    if (!client.connect(resource)) {
        writeVariable(operation.fd, ResourceId, htonl(NO_RESOURCE));
        return;
    }

    std::optional<PGResponse> response = client.query(selectTableSchema(resource.table));
    if (!response.has_value()) {
        writeVariable(operation.fd, ResourceId, htonl(NO_RESOURCE));
        return;
    }

    PGResponse tuples = response.value();
    for (int tuple = 0;tuple < tuples.tuples;tuple++) {
        bool nullable = strcmp(tuples.get(tuple, 2), "YES") == 0 || strlen(tuples.get(tuple, 3)) > 0;
        Column column {tuples.get(tuple, 0), tuples.get(tuple, 1), nullable};
        resource.columns.push_back(column);
    }
    tuples.clear();

    std::unique_lock resourcesLock(resourcesMutex);
    writeVariable(operation.fd, ResourceId, htonl(resources.size()));
    resources.push_back(resource);
    resourcesLock.unlock();
}

void completeSelectOperation(Operation &operation, std::vector<std::string> &columns, PGResponse &response) {
    if (setFlag(operation.fd, O_NONBLOCK, false) == -1) return;

    std::vector<int> fields;
    Columns currentColumns = std::get<Columns>(operation.data);
    for (int field = 0;field < response.fields;field++)
        if (currentColumns.contains(columns[field]))
            fields.push_back(field);

    Write write(operation.fd);
    write(htonl(response.tuples));
    write(htonl(fields.size()));
    for (int tuple = 0;tuple < response.tuples;tuple++)
        for (auto field : fields)
            if (!write(response.get(tuple, field))) return;
    write((RequestStringLength) 0);
    write.finish();

    if (setFlag(operation.fd, O_NONBLOCK, true) == -1) return;
}

void select(PGClient &client, Batch &batch) {
    std::vector<std::string> columns = std::vector<std::string>(batch.columns.begin(), batch.columns.end());
    std::optional<PGResponse> response = client.query(selectColumns(columns, resources[batch.resourceId].table));
    if (!response.has_value()) return;

    PGResponse tuples = response.value();
    for (auto &operation : batch.operations) completeSelectOperation(operation, columns, tuples);
    tuples.clear();
}

void insert(PGClient &client, Batch &batch) {
    (void) client; (void) batch;
}

void processBatches() {
    PGClient client;
    while (isRunning) {
        Batch batch = workQueue.pop();
        if (!isRunning) return;

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