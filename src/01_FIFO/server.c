#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "threads.h"

static void server(int portno) {
    /* Count the number of available CPU cores, for worker threads */
    long worker_count = sysconf(_SC_NPROCESSORS_ONLN);
    if (worker_count < 1) {
        worker_count = 1;
    }
    printf("Number of worker threads: %ld\n", worker_count);

    /* Create the server socket */
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("ERROR opening socket");
        exit(EXIT_FAILURE);
    }

    /* Allow the socket to be reused, avoiding "Address already in use" errors */
    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("ERROR on setsockopt");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    /* Bind the socket to the specified port */
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons((uint16_t)portno);

    if (bind(server_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("ERROR on binding");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    /* Listen for incoming connections */
    if (listen(server_fd, 1024) < 0) {
        perror("ERROR on listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    /* Create worker threads to process requests */
    pthread_t *workers = calloc((size_t)worker_count, sizeof(*workers));
    if (workers == NULL) {
        perror("ERROR allocating worker threads");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    for (long i = 0; i < worker_count; i++) {
        int error = pthread_create(&workers[i], NULL, process_requests, NULL);
        if (error != 0) {
            fprintf(stderr, "ERROR creating worker thread: %s\n", strerror(error));
            close(server_fd);
            exit(EXIT_FAILURE);
        }
    }
    free(workers);

    /* Create a thread to accept incoming connections */
    pthread_t accepter_thread;
    int error = pthread_create(&accepter_thread, NULL, accept_requests, &server_fd);
    if (error != 0) {
        fprintf(stderr, "ERROR creating accepter thread: %s\n", strerror(error));
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Waiting for incoming connections...\n");
    error = pthread_join(accepter_thread, NULL);
    if (error != 0) {
        fprintf(stderr, "ERROR joining accepter thread: %s\n", strerror(error));
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    close(server_fd);
    close(server_fd);
}

int main(int argc, char *argv[]) {
    if (argc > 2) {
        fprintf(stderr, "Usage:\n  %s <port>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int port = 5003;
    if (argc == 2) {
        char *end = NULL;
        long parsed_port = strtol(argv[1], &end, 10);
        if (*argv[1] == '\0' || *end != '\0' || parsed_port < 1 || parsed_port > 65535) {
            fprintf(stderr, "Invalid port: %s\n", argv[1]);
            return EXIT_FAILURE;
        }
        port = (int)parsed_port;
    } else {
        printf("Using default port number: %d\n", port);
    }

    server(port);
    return EXIT_SUCCESS;
}
