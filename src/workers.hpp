#ifndef WORKERS_H
#define WORKERS_H

#include "pgClient.hpp"
#include "workQueue.hpp"

extern std::vector<Resource> resources;
extern WorkQueue workQueue;

void createWorkers();
void stopWorkers();

#endif  // WORKERS_H