#include <chrono>
#include <string>
#include <thread>
#include <vector>
#include "client.hpp"
#include "config.hpp"
#include "request.hpp"
#include "utils.hpp"

static const Type action = SELECT;

static const std::vector<std::string> columns = {"string", "test", "num"};

std::mutex mutex;
std::condition_variable cv;
int counter = 0;

void threadFunction(int threadNumber, ResourceId resource) {
    Client client("127.0.0.1", PORT);

    if (action == SELECT) {
        std::optional<SelectResult> response = client.select(resource, columns);
        if (!response.has_value()) exitWithError("Select failed!");

        SelectResult data = response.value();
        // for (size_t i = 0; i < data.size(); i++) {
        //     printf("[%d] ", threadNumber);
        //     for (auto value : data[i]) printf("%s ", value);
        //     printf("\n");
        // }
        Client::clearResult(data);
    } else if (action == INSERT) {
        std::vector<std::vector<std::string>> values;
        for (int i = 0; i < 1; i++) values.push_back({"string", std::to_string(threadNumber), std::to_string(i)});

        int response = client.insert(resource, columns, values);
        if (response == -1) exitWithError("Insert failed!");
        printf("[%d] %d\n", threadNumber, response);
    }

    std::lock_guard lock(mutex);
    if (--counter == 0) cv.notify_all();
}

void createThread(ResourceId resource) {
    std::thread thread(threadFunction, counter++, resource);
    thread.detach();
}

int main() {
    Client client("127.0.0.1", PORT);

    std::string connectionString = "postgresql://root@127.0.0.1:5432/postgres?connect_timeout=5";
    std::string schema = "public";
    std::string table = "test_table";
    ResourceId resource = client.createResource(connectionString, schema, table);

    if (resource == -1) exitWithError("Couldn't create resource!");
    client.disconnect();

    for (int i = 0; i < 1; i++) createThread(resource);

    std::unique_lock lock(mutex);
    cv.wait(lock, []() { return counter == 0; });

    return EXIT_SUCCESS;
}