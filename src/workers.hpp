#ifndef WORKERS_H
#define WORKERS_H

#include "utils.hpp"
#include "work_queue.hpp"

struct Resource {
    std::string connection_string;
    columns_t columns;
};

extern std::vector<Resource> resources;
extern WorkQueue work_queue;

void create_workers();
void stop_workers();

#endif // WORKERS_H