#ifndef CONFIG_H
#define CONFIG_H

static const int PORT = 5555;

static const int WORKER_THREADS = 2;

static const int BATCH_TIMEOUT_MS = 333;
static const int BATCH_MAX_SIZE = 10;

static const int CONNECT_TIMEOUT_SECONDS = 5;
static const int WRITE_TIMEOUT_SECONDS = 15;
static const int READ_TIMEOUT_SECONDS = 30;

#endif  // CONFIG_H