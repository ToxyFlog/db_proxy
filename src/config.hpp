#ifndef CONFIG_H
#define CONFIG_H

static const int PORT = 5555;

static const int WORKER_THREADS = 2;

static const int BATCH_TIMEOUT_MS = 500;
static const int BATCH_MAX_SIZE = 25;

static const int CONNECTION_TIMEOUT_SECONDS = 5;
static const int RESPONSE_TIMEOUT_SECONDS = 60;

#endif // CONFIG_H