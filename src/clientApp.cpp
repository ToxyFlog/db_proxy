#include <chrono>
#include <string>
#include <thread>
#include <vector>
#include "client.hpp"
#include "request.hpp"
#include "utils.hpp"
#include "config.hpp"

static const std::vector<std::string> columns = {"string", "test", "num"};

std::mutex mutex;
std::condition_variable cv;
int counter = 0;

void threadFunction(int threadNumber, ResourceId resource, int columnsNumber) {
    Client client("127.0.0.1", PORT);

    std::optional<SelectResult> response = client.select(resource, std::vector(columns.begin(), columns.begin() + columnsNumber));
    if (!response.has_value()) exitWithError("Couldn't select columns!");

    SelectResult data = response.value();
    for (size_t i = 0;i < data.size();i++) {
        printf("[%d] ", threadNumber);
        for (auto value : data[i])
            printf("%s ", value);
        printf("\n");
    }
    Client::clear(data);

    std::lock_guard lock(mutex);
    if (--counter == 0) cv.notify_all();
}

void createThread(ResourceId resource, int columnsNumber) {
    std::thread thread(threadFunction, counter++, resource, columnsNumber);
    thread.detach();
}

int main() {
    Client client("127.0.0.1", PORT);

    std::string connectionString = "postgresql://root@127.0.0.1:5432/postgres?connect_timeout=5";
    std::string schema = "public";
    std::string table = "test_table";
    std::optional<ResourceId> response = client.createResource(connectionString, schema, table);

    if (!response.has_value()) exitWithError("Couldn't create resource!");
    client.disconnect();

    ResourceId resource = response.value();
    for (int i = 0;i < 1;i++) createThread(resource, 1);

    std::unique_lock lock(mutex);
    cv.wait(lock, [](){ return counter == 0; });

    return EXIT_SUCCESS;
}