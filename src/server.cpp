#include <string>
#include <vector>
#include <csignal>
#include "tcp_server.hpp"
#include "work_queue.hpp"
#include "utils.hpp"
#include "config.hpp"

static const int MIN_MESSAGE_LENGTH = 5;

static volatile sig_atomic_t is_running = 1;

std::vector<Batch> batches;

void create_workers() {
}
void stop_workers() {
}
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

bool process_message(int fd, std::string message) {
    if (message.size() < MIN_MESSAGE_LENGTH) return false;
    int32_t resource = *(int32_t*) &message[1];
    if (resource >= (int32_t) resources.size()) return false;

    type_t type = *(type_t*) &message[0];
    Operation op {fd, NULL};

    if (type == CREATE_RESOURCE) {

    }

    return true;






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