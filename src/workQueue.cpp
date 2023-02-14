#include <condition_variable>
#include <optional>
#include <mutex>
#include <queue>
#include "workQueue.hpp"

std::optional<Batch> WorkQueue::pop() {
    std::unique_lock lock(mutex);

    cv.wait(lock, [this](){ return !queue.empty() || quit; });
    if (quit) return std::nullopt;

    Batch data = queue.front();
    queue.pop();
    lock.unlock();

    return data;
}

void WorkQueue::push(Batch value) {
    std::unique_lock lock(mutex);
    queue.push(value);
    lock.unlock();
    cv.notify_one();
}

void WorkQueue::stopWaiting() {
    quit = true;
    cv.notify_all();
}