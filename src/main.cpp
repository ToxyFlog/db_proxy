#include <csignal>
#include <string>
#include <unordered_set>
#include <vector>
#include "config.hpp"
#include "server.hpp"
#include "utils.hpp"
#include "workQueue.hpp"
#include "workers.hpp"

static volatile sig_atomic_t isRunning = 1;
std::vector<Batch> batches;

void signalHandler(int signal) {
    if (signal == SIGPIPE) return;

    if (isRunning == 0) exit(EXIT_FAILURE);
    isRunning = 0;
}

void dispatchBatches() {
    for (size_t i = 0; i < batches.size(); i++) {
        Batch &batch = batches[i];
        if (unixTimeInMilliseconds() - batch.createdAt < BATCH_TIMEOUT_MS && batch.size() < BATCH_MAX_SIZE) continue;

        workQueue.push(batch);
        batches.erase(batches.begin() + i--);
    }
}

bool requestHandler(int fd, std::vector<std::string> &fields) {
    if (fields.size() < 3) return false;

    RequestType type = *(RequestType *) &fields[0];
    ResourceId resourceId = ntohs(*(ResourceId *) &fields[1]);
    if (resourceId >= (ResourceId) resources.size()) return false;

    if (type == CREATE_RESOURCE) {
        if (fields.size() != 5) return false;

        Resource resource{fields[2], fields[3], fields[4], 0, {}};
        Batch batch(fd, resource);
        workQueue.push(batch);

        return true;
    }

    if (type == SELECT) {
        std::unordered_set<std::string> columns(fields.begin() + 2, fields.end());
        Select select{fd, columns};

        for (auto &batch : batches) {
            if (batch.type == type && batch.resourceId == resourceId) {
                for (auto &column : columns) batch.columns.insert(column);
                batch.selects.push_back(select);
                return true;
            }
        }

        Batch batch(resourceId, columns, select);
        batches.push_back(batch);
        return true;
    } else if (type == INSERT) {
        FieldLength columnNumber = ntohs(*(FieldLength *) &fields[2]);
        if (columnNumber < 1) return false;
        if ((fields.size() - 3) % columnNumber != 0) return false;

        int requiredColumns = 0;
        Resource resource = resources[resourceId];
        std::vector<std::string> columns(fields.begin() + 3, fields.begin() + 3 + columnNumber);
        for (auto &column : columns) {
            if (!resource.columns.contains(column)) return false;
            if (!resource.columns[column].optional) requiredColumns++;
        }
        if (requiredColumns != resource.requiredColumns) return false;

        Insert insert{fd, columns, std::vector(fields.begin() + 3 + columnNumber, fields.end())};
        for (auto &batch : batches) {
            if (batch.type == type && batch.resourceId == resourceId) {
                batch.inserts.push_back(insert);
                return true;
            }
        }

        Batch batch(resourceId, insert);
        batches.push_back(batch);
        return true;
    }

    return false;
}

void setupSignalHandlers() {
    struct sigaction action;
    sigemptyset(&action.sa_mask);
    action.sa_handler = signalHandler;
    action.sa_flags = 0;

    if (sigaction(SIGINT, &action, NULL) == -1 || sigaction(SIGTERM, &action, NULL) == -1 || sigaction(SIGPIPE, &action, NULL) == -1)
        exitWithError("Couldn't set signal handlers!");
}

int main() {
    Server server(requestHandler);
    server.startListening(PORT);

    createWorkers();
    setupSignalHandlers();

    while (isRunning) {
        if (batches.empty()) server.waitForEvent();
        else dispatchBatches();
        server.acceptConnections();
        server.readRequests();
    }

    server.stopListening();
    stopWorkers();

    return EXIT_SUCCESS;
};