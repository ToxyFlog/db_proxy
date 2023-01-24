#ifndef WORKERS_H
#define WORKERS_H

#include "utils.hpp"
#include "pg_client.hpp"
#include "work_queue.hpp"

extern std::vector<Resource> resources;
extern WorkQueue work_queue;

void create_workers();
void stop_workers();

#endif // WORKERS_H