#include <queue>
#include <mutex>
#include <condition_variable>
#include "work_queue.hpp"

Batch WorkQueue::pop() {
    std::unique_lock lock(mutex);

    cv.wait(lock, [this](){ return !queue.empty() || quit; });
    if (quit) return Batch();

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

void WorkQueue::stop_waiting() {
    quit = true;
    cv.notify_all();
}