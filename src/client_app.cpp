#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include "client.hpp"
#include "utils.hpp"
#include "config.hpp"

static const std::vector<std::string> columns = {"string", "test", "num"};
void thread_func(int thread_number, int columns_num) {
    std::this_thread::sleep_for(std::chrono::milliseconds(arc4random() % (int) (BATCH_TIMEOUT_MS * 0.5f)));

    Client client("127.0.0.1", PORT);
    std::vector<std::vector<std::string>> result = client.select(0, std::vector(columns.begin(), columns.begin() + columns_num));
    if (result.size() == 0) exit_with_error("Couldn't select columns!");

    for (size_t i = 0;i < result.size();i++) {
        for (auto &value : result[i]) {
            // something
        }
    }
}

int main() {
    Client client("127.0.0.1", PORT);

    std::string connection_string = "postgresql://root@127.0.0.1:5432/postgres?connect_timeout=5";
    std::string schema = "public";
    std::string table = "test_table";

    int32_t resource = client.create_resource(connection_string, schema, table);
    if (resource == -1) exit_with_error("Couldn't create resource!");
    client.disconnect();

    int i = 0;
    std::thread t1(thread_func, i++, 3), t2(thread_func, i++, 1), t3(thread_func, i++, 2);
    std::this_thread::sleep_for(std::chrono::milliseconds(BATCH_TIMEOUT_MS));
    std::thread t4(thread_func, i++, 2);
    t1.join(); t2.join(); t3.join(); t4.join();

    return EXIT_SUCCESS;
}