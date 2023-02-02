#ifndef WORK_QUEUE_H
#define WORK_QUEUE_H

#include <condition_variable>
#include <mutex>
#include <vector>
#include <variant>
#include <queue>
#include <unordered_set>
#include "config.hpp"
#include "pgClient.hpp"
#include "request.hpp"

typedef std::unordered_set<std::string> Columns;

typedef std::variant<void*, Columns, Resource> OperationData;
struct Operation {
    int fd;
    OperationData data;
};

typedef std::variant<void*, Columns, Operation> SharedData;
struct Batch {
    Type type;
    int resourceId;
    uint64_t updatedAt;
    SharedData sharedData;
    std::vector<Operation> operations;
};

class WorkQueue {
private:
    std::queue<Batch> queue;
    std::mutex mutex;

    std::condition_variable cv;
    bool quit = false;
public:
    Batch pop();
    void push(Batch value);
    void stopWaiting();
};

#endif // WORK_QUEUE_H