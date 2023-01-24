#include <unordered_set>
#include <string>
#include <variant>
#include <vector>
#include <csignal>
#include "tcp_server.hpp"
#include "work_queue.hpp"
#include "workers.hpp"
#include "utils.hpp"
#include "config.hpp"

static const int MIN_MESSAGE_LENGTH = 5;

std::vector<Batch> batches;

static volatile sig_atomic_t is_running = 1;
void signal_handler() { is_running = 0; }

void push_batches() {
    for (size_t i = 0;i < batches.size();i++) {
        Batch batch = batches[i];
        if (current_time_milliseconds() - batch.updated_at >= BATCH_TIMEOUT_MS || batch.operations.size() >= BATCH_MAX_SIZE) {
            work_queue.push(batch);
            batches.erase(batches.begin() + i);
            i--;
        }
    }
}

inline std::string read_string(std::string message) { return message.substr(sizeof(string_len_t), ntohs(*(string_len_t*) &message[0])); }

std::vector<std::string> read_string_array(std::string message) {
    std::vector<std::string> result;

    for (size_t i = 0;i < message.size();) {
        std::string temp = read_string(message.substr(i));
        i += sizeof(string_len_t) + temp.size();
        result.push_back(temp);
    }

    return result;
}

bool process_message(int fd, std::string message) {
    if (message.size() < MIN_MESSAGE_LENGTH) return false;

    int32_t resource_id = ntohl(*(int32_t*) &message[1]);
    if (resource_id >= (int32_t) resources.size()) return false;

    type_t type = *(type_t*) &message[0];
    message = message.substr(5);

    if (type == CREATE_RESOURCE) {
        std::vector<std::string> values = read_string_array(message);
        if (values.size() != 3) return false;

        Resource resource {values[0], values[1], values[2], {}};
        Operation operation {fd, resource};
        Batch batch {CREATE_RESOURCE, -1, 0, operation, {}};
        work_queue.push(batch);
        return true;
    }

    if (type == SELECT) {
        std::vector<std::string> values = read_string_array(message);
        columns_t columns(values.begin(), values.end());
        if (columns.size() == 0) return false;

        Operation operation {fd, columns};
        for (auto &batch : batches) {
            if (batch.type == type && batch.resource_id == resource_id) {
                columns_t shared_data = std::get<columns_t>(batch.shared_data);
                for (auto &column : columns) shared_data.insert(column);
                batch.shared_data = shared_data;
                batch.operations.push_back(operation);
                return true;
            }
        }
        Batch batch {SELECT, resource_id, current_time_milliseconds(), columns, {operation}};
        batches.push_back(batch);
        return true;
    } else if (type == INSERT) {

    }

    return false;
}

int main(void) {
    TCPServer server(process_message);

    server.start_listening(PORT);
    create_workers();

    struct sigaction action;
    action.sa_handler = (void (*)(int)) signal_handler;
    action.sa_flags = 0;
    action.sa_mask = 0;
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGTERM, &action, NULL);

    while (is_running) {
        push_batches();
        if (batches.empty()) server.wait_for_event();
        server.accept_connections();
        server.read_messages();
    }

    server.stop_listening();
    stop_workers();

    return EXIT_SUCCESS;
};