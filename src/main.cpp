#include <csignal>
#include <string>
#include <unordered_set>
#include <vector>
#include "config.hpp"
#include "server.hpp"
#include "utils.hpp"
#include "request.hpp"
#include "workers.hpp"
#include "workQueue.hpp"

static const int MIN_REQUEST_LENGTH = 5;

static volatile sig_atomic_t isRunning = 1;
std::vector<Batch> batches;

void signalHandler(int signal) {
    if (signal == SIGPIPE) return;

    if (isRunning == 0) exit(EXIT_FAILURE);
    isRunning = 0;
}

void dispatchBatches() {
    for (size_t i = 0;i < batches.size();i++) {
        Batch batch = batches[i];
        if (unixTimeInMilliseconds() - batch.updatedAt < BATCH_TIMEOUT_MS && batch.size() < BATCH_MAX_SIZE) continue;

        workQueue.push(batch);
        batches.erase(batches.begin() + i--);
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

        Resource resource {values[0], values[1], values[2], 0, {}};
        Batch batch(fd, resource);
        workQueue.push(batch);

        return true;
    }

    if (type == SELECT) {
        std::vector<std::string> values = readStringArray(request);
        if (values.size() == 0) return false;

        std::unordered_set<std::string> columns(values.begin(), values.end());
        Select select {fd, columns};

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
        RequestStringLength columnNumber = ntohs(*(RequestStringLength*) request.c_str());
        if (columnNumber < 1) return false;

        request = request.substr(sizeof(RequestStringLength));
        std::vector<std::string> values = readStringArray(request);
        if (values.size() % columnNumber != 0) return false;

        int requiredColumns = 0;
        Resource resource = resources[resourceId];
        std::vector<std::string> columns(values.begin(), values.begin() + columnNumber);
        for (auto &column : columns) {
            if (!resource.columns.contains(column)) return false;
            if (!resource.columns[column].optional) requiredColumns++;
        }
        if (requiredColumns != resource.requiredColumns) return false;

        Insert insert {fd, columns, std::vector(values.begin() + columnNumber, values.end())};
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

    if (sigaction(SIGINT,  &action, NULL) == -1 ||
        sigaction(SIGTERM, &action, NULL) == -1 ||
        sigaction(SIGPIPE, &action, NULL) == -1)
        exitWithError("Couldn't set signal handlers!");
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