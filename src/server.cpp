#include <string>
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

inline std::string read_string(std::string message) { return message.substr(sizeof(string_len_t), (string_len_t) message[0]); }

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
    
    int32_t resource = *(int32_t*) &message[1];
    if (resource >= (int32_t) resources.size()) return false;

    type_t type = *(type_t*) &message[0];
    message = message.substr(5);

    if (type == CREATE_RESOURCE) {
        std::vector<std::string> values = read_string_array(message);
        if (values.size() % 2 == 0) return false;
        
        Resource resource;
        resource.connection_string = values[0];
        for (size_t i = 1;i < values.size();i += 2) resource.columns.push_back({values[i], values[i + 1]});

        Batch batch;
        batch.type = CREATE_RESOURCE;
        Operation operation {fd, resource};
        batch.shared_data = operation;
        work_queue.push(batch);
    }

    return true;

    // std::vector<std::string> fields;
    // split_string(fields, message.substr(1), FIELD_SEPARATOR_CHAR);
    // if (fields.size() != NUMBER_OF_FIELDS) return false;

    // Operation op {fd, NULL};
    // type_t type = *(type_t*) &message[0];

    // if (type == CREATE_RESOURCE) {
    //     std::vector<std::string> values;
    //     split_string(values, fields[1], VALUE_SEPARATOR_CHAR);
    //     if (values.size() % 2 == 0) return false;

    //     Resource res {fields[0], {}};
    //     for (size_t i = 1;i < values.size();i += 2) res.columns.push_back({values[i - 1], values[i]});

    //     work_queue.push({CREATE_RESOURCE, 0, current_time_milliseconds(), NULL, {{fd, res}}});
    //     return true;
    // }

    // int resource_id = ntohl(*(int*) &fields[0]);
    // printf("resource id: %d\n", resource_id, (int) resources.size());
    // if (resource_id >= (int) resources.size()) return false;

    // // TODO:

    // if (type == SELECT) {

    // } else if (type == INSERT) {

    // } else return false;

    // // TODO: add to corresponding batch / create new

    // return true;
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