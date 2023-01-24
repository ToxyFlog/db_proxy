#ifndef WORK_QUEUE_H
#define WORK_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <unordered_set>
#include <variant>
#include "utils.hpp"
#include "pg_client.hpp"
#include "config.hpp"

typedef std::unordered_set<std::string> columns_t;

typedef std::variant<void*, columns_t, Resource> operation_data_t;
struct Operation {
    int fd;
    operation_data_t data;
};

typedef std::variant<void*, columns_t, Operation> shared_data_t;
struct Batch {
    Type type;
    int resource_id;
    uint64_t updated_at;
    shared_data_t shared_data;
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

    void stop_waiting();
};

#endif // WORK_QUEUE_H