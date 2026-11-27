#ifndef THREADS_H
#define THREADS_H

#include <stdint.h>

#include "messages.h"

typedef struct Request {
    uint8_t hash[SHA256_DIGEST_LENGTH];
    uint64_t start;
    uint64_t end;
    uint8_t priority;
    int client_fd;
    struct Request *next;
} Request;

void *accept_requests(void *server_fd_arg);
void *process_requests(void *unused);
void request_queue_enqueue(Request *request);
Request *request_queue_dequeue(void);

#endif