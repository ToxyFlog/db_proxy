#ifndef CONFIG_H
#define CONFIG_H

static const int PORT = 5555;

static const int WORKER_THREADS = 2;

static const int BATCH_TIMEOUT_MS = 1000;
static const int BATCH_MAX_SIZE = 3;

static const int CONNECTION_TIMEOUT_MS = 5000;
static const int RESPONSE_TIMEOUT_MS = 20000;

#endif // CONFIG_H