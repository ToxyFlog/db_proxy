#include <chrono>
#include <string>
#include <thread>
#include <vector>
#include "client.hpp"
#include "request.hpp"
#include "utils.hpp"
#include "config.hpp"

static const std::vector<std::string> columns = {"string", "test", "num"};
void threadFunction(int threadNumber, ResourceId resource, int columnsNumber) {
    std::this_thread::sleep_for(std::chrono::milliseconds(arc4random() % (BATCH_TIMEOUT_MS / 2)));

    Client client("127.0.0.1", PORT);
    std::optional<std::vector<std::vector<std::string>>> response = client.select(resource, std::vector(columns.begin(), columns.begin() + columnsNumber));
    if (!response.has_value()) exitWithError("Couldn't select columns!");
    std::vector<std::vector<std::string>> data = response.value();

    for (size_t i = 0;i < min(3, data.size());i++) {
        printf("[%d] ", threadNumber);
        for (auto &value : data[i]) printf("%s ", value.c_str());
        printf("\n");
    }
}

static int i = 0;
static std::vector<std::thread> threads;
void createThread(ResourceId resource, int columnsNumber) { threads.push_back(std::thread(threadFunction, i++, resource, columnsNumber)); }
void joinThreads() { for (auto &thread : threads) thread.join(); }

int main() {
    Client client("127.0.0.1", PORT);
    std::string connectionString = "postgresql://root@127.0.0.1:5432/postgres?connect_timeout=5";
    std::string schema = "public";
    std::string table = "test_table";
    std::optional<ResourceId> response = client.createResource(connectionString, schema, table);
    if (!response.has_value()) exitWithError("Couldn't create resource!");
    ResourceId resource = response.value();
    client.disconnect();

    createThread(resource, 3);
    createThread(resource, 1);
    createThread(resource, 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(BATCH_TIMEOUT_MS));
    createThread(resource, 2);

    joinThreads();
    return EXIT_SUCCESS;
}