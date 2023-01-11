#ifndef CONFIG_H
#define CONFIG_H

#include <unistd.h>

static const int PORT = 5555;

static const int WORKER_THREADS = 2;

static const int BATCH_TIMEOUT_MS = 100;
static const int BATCH_MAX_SIZE = 3;

#endif // CONFIG_H