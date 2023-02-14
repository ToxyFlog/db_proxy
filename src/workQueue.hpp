#ifndef WORK_QUEUE_H
#define WORK_QUEUE_H

#include <condition_variable>
#include <mutex>
#include <vector>
#include <queue>
#include <unordered_set>
#include "config.hpp"
#include "utils.hpp"
#include "pgClient.hpp"
#include "request.hpp"

struct Select {
    int fd;
    std::unordered_set<std::string> columns;
};

struct Insert {
    int fd;
    std::vector<std::string> columns;
    std::vector<std::string> values;
};

struct Batch {
    Type type;
    ResourceId resourceId = -1;
    uint64_t updatedAt = 0;

    int fd;
    Resource resource;

    std::unordered_set<std::string> columns;
    std::vector<Select> selects;

    std::vector<Insert> inserts;

    Batch(int _fd, Resource _resource): type(CREATE_RESOURCE), fd(_fd), resource(_resource) {}
    Batch(ResourceId _resourceId, std::unordered_set<std::string> _columns, Select _select): type(SELECT), resourceId(_resourceId), columns(_columns), selects({_select}) {
        updatedAt = unixTimeInMilliseconds();
    }
    Batch(ResourceId _resourceId, Insert _insert): type(INSERT), resourceId(_resourceId), inserts({_insert}) {
        updatedAt = unixTimeInMilliseconds();
    }

    size_t size() {
        if (type == SELECT) return selects.size();
        if (type == INSERT) return inserts.size();
        return 1;
    }
};

class WorkQueue {
private:
    std::queue<Batch> queue;
    std::mutex mutex;

    std::condition_variable cv;
    bool quit = false;
public:
    std::optional<Batch> pop();
    void push(Batch value);
    void stopWaiting();
};

#endif // WORK_QUEUE_H