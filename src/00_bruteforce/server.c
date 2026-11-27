#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include "messages.h"

#include <netdb.h>
#include <netinet/in.h>

#include <string.h>

#define DEBUG



#include "brute.h"

#ifdef DEBUG
int reqeust_count = 0;
#endif




static void handle_request(int newsockfd) {
    /* Accept incoming connections */
    char buffer[256];
    int n;
    if (newsockfd < 0)
    {
        perror("ERROR on accept");
        exit(1);
    }

    /* If a connection is established, process it */
    size_t received = 0;        
    while (received < PACKET_REQUEST_SIZE) {
        ssize_t count = recv(
            newsockfd,
            buffer + received,
            PACKET_REQUEST_SIZE - received,
            0
        );
    
        if (count <= 0) {
            // handle disconnect or error
            return;
        }
        received += (size_t)count;
    }




    // Extract the hash, start, and end values from the request
    // From the network they are in big-endian format, so we need to convert them to little-endian for processing
    uint64_t start_network, start, difficulty_network, difficulty, priority_network, priority;
    uint8_t hash_network[SHA256_DIGEST_LENGTH];
    memcpy(&hash_network, buffer + PACKET_REQUEST_HASH_OFFSET, SHA256_DIGEST_LENGTH);
    memcpy(&start_network, buffer + PACKET_REQUEST_START_OFFSET, sizeof(start_network));
    memcpy(&difficulty_network, buffer + PACKET_REQUEST_END_OFFSET, sizeof(difficulty_network));
    memcpy(&priority_network, buffer + PACKET_REQUEST_PRIO_OFFSET, sizeof(priority_network));

    start = htobe64(start_network);
    difficulty = htobe64(difficulty_network);
    priority = htobe64(priority_network);

    #ifdef DEBUG
        //printf("Received Hash: %s\n", (char *)buffer + PACKET_REQUEST_HASH_OFFSET);
        printf("[%d] start = %lu\n", ++reqeust_count, (start));
        printf("[%d]   end = %lu\n", reqeust_count, (start + difficulty));
    #endif


    // brute force the answer
    uint64_t candidate = brute_force(hash_network, start, (start + difficulty)); 


    // Send the response back to the client
    uint64_t response = candidate;

    size_t sent = 0;
    while (sent < sizeof(response)) {
        ssize_t count = send(
            newsockfd,
            (uint8_t *)&response + sent,
            sizeof(response) - sent,
            MSG_NOSIGNAL
        );

        if (count <= 0) {
            break;
        }

        sent += (size_t)count;
    }
    close(newsockfd);
}



void server (int portno) {
    long ncpu = sysconf(_SC_NPROCESSORS_ONLN);
    printf("Number of CPU cores: %ld\n", ncpu);




    int sockfd, newsockfd, clilen;
    struct sockaddr_in serv_addr, cli_addr;
    int reuse = 1;

    /* Initialize socket structure */
    /* Create a socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
    {
        perror("ERROR opening socket");
        exit(1);
    }

    /* Initialize socket structure */
    memset((char *) &serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    /* Create a socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
    {
        perror("ERROR opening socket");
        exit(1);
    }

    /* Set the socket to reuse the address */
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("ERROR on setsockopt");
        exit(1);
    }


    /* Bind the socket to the local address */
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
    {
        perror("ERROR on binding");
        exit(1);
    }

    /* Listen for incoming connections */
    listen(sockfd, 1024);
    clilen = sizeof(cli_addr);


    /* run the server */
    printf("Waiting for incoming connections...\n");

    
    while (1) {
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        handle_request(newsockfd);
    }
}



int main(int argc, char *argv[]){
    if (argc == 1) {
        printf("Using default port number: 5003\n");
        server(5003);
    }
    if (argc == 2) {
        printf("Using port number: %s\n", argv[1]);
        server(atoi(argv[1]));
    }
    if (argc > 2){
        fprintf(stderr, "Usage:\n  %s <port>\n ", argv[0]);
        exit(1);
    }


        
}