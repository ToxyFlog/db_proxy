#include <csignal>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>
#include "config.hpp"
#include "server.hpp"
#include "utils.hpp"
#include "request.hpp"
#include "workers.hpp"
#include "workQueue.hpp"

static const int MIN_REQUEST_LENGTH = 5;

static volatile sig_atomic_t isRunning = 1;
void signalHandler(int signal) { if (signal != SIGPIPE) isRunning = 0; }

std::vector<Batch> batches;

void dispatchBatches() {
    for (size_t i = 0;i < batches.size();i++) {
        Batch batch = batches[i];
        if (unixTimeInMilliseconds() - batch.updatedAt >= BATCH_TIMEOUT_MS || batch.operations.size() >= BATCH_MAX_SIZE) {
            workQueue.push(batch);
            batches.erase(batches.begin() + i);
            i--;
        }
    }
}

inline std::string readString(std::string request) { return request.substr(sizeof(RequestStringLength), ntohs(*(RequestStringLength*) &request[0])); }

std::vector<std::string> readStringArray(std::string &request) {
    std::vector<std::string> result;

    for (size_t i = 0;i < request.size();) {
        std::string temp = readString(request.substr(i));
        i += sizeof(RequestStringLength) + temp.size();
        result.push_back(temp);
    }

    return result;
}

bool requestHandler(int fd, std::string &request) {
    if (request.size() < MIN_REQUEST_LENGTH) return false;

    ResourceId resourceId = ntohl(*(ResourceId*) &request[1]);
    if (resourceId >= (ResourceId) resources.size()) return false;

    RequestType type = *(RequestType*) &request[0];
    request = request.substr(5);

    if (type == CREATE_RESOURCE) {
        std::vector<std::string> values = readStringArray(request);
        if (values.size() != 3) return false;

        Resource resource {values[0], values[1], values[2], {}};
        Operation operation {fd, resource};
        Batch batch {CREATE_RESOURCE, -1, 0, {}, {operation}};
        workQueue.push(batch);
        return true;
    }

    if (type == SELECT) {
        std::vector<std::string> values = readStringArray(request);
        Columns columns(values.begin(), values.end());
        if (columns.size() == 0) return false;

        Operation operation {fd, columns};
        for (auto &batch : batches) {
            if (batch.type == type && batch.resourceId == resourceId) {
                for (auto &column : columns) batch.columns.insert(column);
                batch.operations.push_back(operation);
                return true;
            }
        }

        Batch batch {SELECT, resourceId, unixTimeInMilliseconds(), columns, {operation}};
        batches.push_back(batch);
        return true;
    } else if (type == INSERT) {

    }

    return false;
}

void setupSignalHandlers() {
    struct sigaction action;
    action.sa_handler = signalHandler;
    action.sa_flags = 0;
    action.sa_mask = 0;

    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
    sigaction(SIGPIPE, &action, NULL);
}

int main() {
    Server server(requestHandler);
    server.startListening(PORT);

    createWorkers();
    setupSignalHandlers();

    while (isRunning) {
        dispatchBatches();
        if (batches.empty()) server.waitForEvent();
        server.acceptConnections();
        server.readRequests();
    }

    server.stopListening();
    stopWorkers();

    return EXIT_SUCCESS;
};