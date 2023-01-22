#include <any>
#include <optional>
#include <thread>
#include <mutex>
#include "work_queue.hpp"
#include "pg_client.hpp"
#include "utils.hpp"
#include "workers.hpp"

static const int NO_RESOURCE = -1;

std::vector<Resource> resources;
std::mutex resources_mutex;

std::queue<std::thread> workers;
bool is_running = true;

WorkQueue work_queue;

void create_resource(PGClient &client, Batch batch) {
    Operation operation = std::any_cast<Operation>(batch.shared_data);
    Resource resource = std::any_cast<Resource>(operation.data);

    if (!client.connect(resource.connection_string)) {
        write_variable(operation.fd, int32_t, htonl(NO_RESOURCE));
        return;
    }

    std::optional<PGResponse> query_response = client.query("SELECT count(1) FROM pg_tables;");
    bool is_reachable = query_response.has_value() && query_response.value().size() && query_response.value()[0].size();
    if (!is_reachable) {
        write_variable(operation.fd, int32_t, htonl(NO_RESOURCE));
        return;
    }

    std::unique_lock resources_lock(resources_mutex);
    write_variable(operation.fd, int32_t, htonl(resources.size()));
    resources.push_back(resource);
    resources_lock.unlock();
}

void select(PGClient &client, Batch batch) {
    (void) client;
    // Something something = std::any_cast<Something>(batch.shared_data);

    for (auto &operation : batch.operations) {
    (void) operation;
        // Something something = std::any_cast<Something>(operation.data);
    }
}

void insert(PGClient &client, Batch batch) {
    (void) client;
    // Something something = std::any_cast<Something>(batch.shared_data);

    for (auto &operation : batch.operations) {
    (void) operation;
        // Something something = std::any_cast<Something>(operation.data);
    }
}

void process_batches() {
    PGClient client;
    while (is_running) {
        Batch batch = work_queue.pop();
        if (!is_running) return;

        if (batch.type != CREATE_RESOURCE) client.connect(resources[batch.resource_id].connection_string);

        bool error = true;
        try {
            switch (batch.type) {
                case CREATE_RESOURCE:
                    create_resource(client, batch);
                    break;
                case SELECT:
                    select(client, batch);
                    break;
                case INSERT:
                    insert(client, batch);
                    break;
                default:
                    error = true;
            }
        } catch(std::bad_any_cast exception) {
            error = true;
        }

        if (error) for (auto &operation : batch.operations) write_variable(operation.fd, int32_t, -1);

        client.disconnect();
    }
}

void create_workers() {
    //! threads can't be copied, so have to be either std::move()'d or instantiated inside of push
    while (workers.size() < WORKER_THREADS) workers.push(std::thread(process_batches));
}

void stop_workers() {
    is_running = false;
    work_queue.stop_waiting();

    while (!workers.empty()) {
        workers.front().join();
        workers.pop();
    }

    exit(EXIT_SUCCESS);
}