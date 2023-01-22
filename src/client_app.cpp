#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include "client.hpp"
#include "utils.hpp"
#include "config.hpp"

int main() {
    Client client;
    client.connect_to("127.0.0.1", PORT);

    std::string connection_string = "postgresql://root@127.0.0.1:5432/postgres?connect_timeout=5";
    columns_t columns{{"serial", "test"}, {"number", "num"}, {"text", "string"}};

    int resource = client.create_resource(connection_string, columns);
    if (resource == -1) exit_with_error("Couldn't create resource!");
    printf("Resource #%d created!\n", resource);

    // std::vector<std::vector<std::string>> result = client.select(resource, {"test", "num"});

    return EXIT_SUCCESS;
}