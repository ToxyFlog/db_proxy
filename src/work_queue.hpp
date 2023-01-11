#ifndef WORK_QUEUE_H
#define WORK_QUEUE_H

#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <any>
#include <unistd.h>
#include "utils.hpp"
#include "config.hpp"

struct Operation {
    int fd;
    std::any data;
};

struct Batch {
    Type type;
    int resource_id;
    uint64_t updated_at;
    std::any shared_data;
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