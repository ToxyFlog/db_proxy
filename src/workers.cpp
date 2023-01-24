#include <string>
#include <unordered_set>
#include <variant>
#include <optional>
#include <thread>
#include <mutex>
#include <fcntl.h>
#include <unistd.h>
#include "work_queue.hpp"
#include "pg_client.hpp"
#include "write.hpp"
#include "utils.hpp"
#include "workers.hpp"

WorkQueue work_queue;

std::vector<Resource> resources;
std::mutex resources_mutex;

std::queue<std::thread> workers;
bool is_running = true;

inline std::string select_columns_from_table(std::vector<std::string> &columns, std::string &table) { return "SELECT " + join(columns, ',') + " FROM " + table; }
inline std::string select_column_information(std::string table) {
    return "SELECT column_name, data_type, is_nullable, column_default FROM information_schema.columns WHERE table_name = '" + table + "'";
}

static const int NO_RESOURCE = -1;
void create_resource(PGClient &client, Batch batch) {
    Operation operation = std::get<Operation>(batch.shared_data);
    Resource resource = std::get<Resource>(operation.data);

    if (!client.connect(resource)) {
        write_variable_immediate(operation.fd, int32_t, htonl(NO_RESOURCE));
        return;
    }

    std::optional<PGResponse> response = client.query(select_column_information(resource.table));
    if (!response.has_value() || response.value().size() == 0) {
        write_variable_immediate(operation.fd, int32_t, htonl(NO_RESOURCE));
        return;
    }

    for (auto &tuple : response.value()) {
        bool is_nullable = tuple[2] == "YES" || tuple[3].size() > 0;
        Column column {tuple[0], tuple[1], is_nullable};
        resource.columns.push_back(column);
    }

    std::unique_lock resources_lock(resources_mutex);
    write_variable_immediate(operation.fd, int32_t, htonl(resources.size()));
    resources.push_back(resource);
    resources_lock.unlock();
}

void complete_select_operation(Operation &operation, std::vector<std::string> &columns, PGResponse &tuples) {
    set_fd_flag(operation.fd, O_NONBLOCK);
    bool error = false;
    begin_write(operation.fd, &error);

    columns_t cur_columns = std::get<columns_t>(operation.data);
    for (auto &tuple : tuples) {
        for (size_t j = 0;j < tuple.size();j++) {
            if (!cur_columns.contains(columns[j])) continue;

            write_str(tuple[j]);
            if (error) return;
        }
    }
    write_variable(string_len_t, 0);
    end_write();
    set_fd_flag(operation.fd, O_NONBLOCK);
}

void select(PGClient &client, Batch batch) {
    columns_t columns_set = std::get<columns_t>(batch.shared_data);
    std::vector<std::string> columns = std::vector<std::string>(columns_set.begin(), columns_set.end());

    std::optional<PGResponse> response = client.query(select_columns_from_table(columns, resources[batch.resource_id].table));
    if (!response.has_value()) return;

    uint64_t t = current_time_milliseconds();
    PGResponse tuples = response.value();
    for (auto &operation : batch.operations) complete_select_operation(operation, columns, tuples);
    printf("Time: %llu\n", current_time_milliseconds() - t);
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

        if (batch.type != CREATE_RESOURCE && !client.connect(resources[batch.resource_id])) continue;

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
            }
        } catch(std::bad_variant_access exception) {}

        client.disconnect();
    }
}

void create_workers() {
    for (int i = 0;i < WORKER_THREADS;i++) {
        std::thread worker(process_batches);
        worker.detach();
    }
}

void stop_workers() {
    is_running = false;
    work_queue.stop_waiting();
}