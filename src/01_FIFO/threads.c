#include "threads.h"

#include "brute.h"

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/* A simple queue for storing incoming requests */
static Request *queue_head;
static Request *queue_tail;
static pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t queue_not_empty = PTHREAD_COND_INITIALIZER;

static int receive_request(int client_fd, Request **request_out) {
    uint8_t buffer[PACKET_REQUEST_SIZE];
    size_t received = 0;

    /* Receive the entire request */
    while (received < sizeof(buffer)) {
        ssize_t count = recv(client_fd, buffer + received, sizeof(buffer) - received, 0);
        if (count < 0 && errno == EINTR) {
            continue;
        }
        if (count <= 0) {
            if (count < 0) {
                perror("ERROR receiving request");
            }
            close(client_fd);
            return -1;
        }
        received += (size_t)count;
    }

    /* Allocate a new Request structure and fill it with the received data */
    Request *request = malloc(sizeof(*request));
    if (request == NULL) {
        perror("ERROR allocating request");
        close(client_fd);
        return -1;
    }

    memcpy(request->hash, buffer + PACKET_REQUEST_HASH_OFFSET, sizeof(request->hash));

    uint64_t start_network;
    uint64_t difficulty_network;
    memcpy(&start_network, buffer + PACKET_REQUEST_START_OFFSET, sizeof(start_network));
    memcpy(&difficulty_network, buffer + PACKET_REQUEST_END_OFFSET, sizeof(difficulty_network));

    request->start = be64toh(start_network);
    request->end = request->start + be64toh(difficulty_network);
    request->priority = buffer[PACKET_REQUEST_PRIO_OFFSET];
    request->client_fd = client_fd;
    request->next = NULL;
    *request_out = request;
    return 0;
}

void request_queue_enqueue(Request *request) {
    pthread_mutex_lock(&queue_mutex);
    if (queue_tail == NULL) {
        queue_head = request;
    } else {
        queue_tail->next = request;
    }
    queue_tail = request;
    pthread_cond_signal(&queue_not_empty);
    pthread_mutex_unlock(&queue_mutex);
}

Request *request_queue_dequeue(void) {
    pthread_mutex_lock(&queue_mutex);
    while (queue_head == NULL) {
        pthread_cond_wait(&queue_not_empty, &queue_mutex);
    }

    Request *request = queue_head;
    queue_head = request->next;
    if (queue_head == NULL) {
        queue_tail = NULL;
    }
    pthread_mutex_unlock(&queue_mutex);

    request->next = NULL;
    return request;
}

static void process_request(Request *request) {
    uint64_t response = brute_force(request->hash, request->start, request->end);
    size_t sent = 0;

    while (sent < sizeof(response)) {
        ssize_t count = send(
            request->client_fd,
            (uint8_t *)&response + sent,
            sizeof(response) - sent,
            MSG_NOSIGNAL
        );
        if (count < 0 && errno == EINTR) {
            continue;
        }
        if (count <= 0) {
            if (count < 0) {
                perror("ERROR sending response");
            }
            break;
        }
        sent += (size_t)count;
    }

    close(request->client_fd);
    free(request);
}

void *accept_requests(void *server_fd_arg) {
    int server_fd = *(int *)server_fd_arg;

    for (;;) {
        int client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("ERROR on accept");
            return NULL;
        }

        Request *request = NULL;
        if (receive_request(client_fd, &request) == 0) {
            request_queue_enqueue(request);
        }
    }
}

void *process_requests(void *unused) {
    (void)unused;

    for (;;) {
        process_request(request_queue_dequeue());
    }
}
