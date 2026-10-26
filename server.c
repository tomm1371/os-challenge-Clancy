#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include "messages.h"

#include <netdb.h>
#include <netinet/in.h>

#include <string.h>
#define LONESHA256_STATIC
#include "lonesha256.h"

//#define DEBUG 1
//#define DEBUG_FULL 1




/* Byte order functions

htons() - host to network short
htonl() - host to network long
ntohs() - network to host short
ntohl() - network to host long

*/

/* Socket functions */

/*
    create a socket, which specifies the communication domain, type, and protocol 
int socket(int family, int type, int protocol);

    used my client to connect to a server 
int connect(int sockfd, const struct sockaddr *addr, int addrlen);

    used by servers to associate a socket with a local address 
int bind(int sockfd, struct sockaddr *addr, int addrlen);

    used by servers to listen for incoming connections 
int listen(int sockfd, int backlog);

    used by servers to accept incoming connections made by clients 
int accept(int sockfd, struct sockaddr *addr, int *addrlen);

    used by clients and servers to send data over a socket 
int send(int sockfd, const void *msg, size_t len, int flags);

used by clients and servers to receive data over a socket 
int recv(int sockfd, void *buf, size_t len, int flags);
*/


/* desired flow

Server strats
    1. Create a socket
        socket(int family, int type, int protocol)
    2. Bind the socket to a local address
        bind(int sockfd, struct sockaddr *addr, int addrlen)
    3. Listen for (any) incoming connections
        listen(int sockfd, int backlog)
    4. Accept incoming connections
        accept(int sockfd, struct sockaddr *addr, int *addrlen)
    5. Receive data from the client
            recv(int sockfd, void *buf, size_t len, int flags)
        6. Process the received data
            (run though the algorithm, etc.)
        7. Send a response back to the client
            send(int sockfd, const void *msg, size_t len, int flags)
        8. Close the connection with the client
            close(int sockfd)
    9. Repeat steps 4-8 for any new incoming connections
    10. Shutdown the server
        shutdown(int sockfd, int how)

Client sends reqests
    1. Gereates request data
    2. Create a socket
    3. Connect to the server
        4. Send data to the server
        5. Wait for response from the server
        6. Process the response
    7. Repeat steps 4-6 according to cleint settings
    8. Generate final score


*/


void print_request(const uint8_t *buf) {
    printf("hash:  ");
    for (int i = 0; i < 32; i++) printf("%02x", buf[i]);
    printf("\nstart: ");
    for (int i = 32; i < 40; i++) printf("%02x", buf[i]);
    printf("\nend:   ");
    for (int i = 40; i < 48; i++) printf("%02x", buf[i]);
    printf("\nprio:  %02x\n", buf[48]);
}


uint64_t brute_force(uint8_t *target_hash, uint64_t start, uint64_t end) {
    for (uint64_t candidate = start; candidate < end; candidate++) {
        uint64_t candidate_le = htole64(candidate);
        unsigned char out[32];

        lonesha256(out, (uint8_t *)&candidate_le, sizeof(candidate_le));

        #ifdef DEBUG_FULL
            printf("Candidate: %llu \n", candidate);
            printf("Candidate Hash: ");
            for (int i = 0; i < 32; i++) printf("%02x", out[i]);
            printf("\n");
        #endif

        if (memcmp(out, (char *)target_hash, SHA256_DIGEST_LENGTH) == 0) {
            #ifdef DEBUG
                printf("Hashing succeeded for candidate: %02x\n", candidate);
                printf("\n");
            #endif
            return htobe64(candidate_le);
        }
        if (candidate == end - 1) {
            break;
        }
    }
    return 0;
}

void server (int portno) {
    int sockfd, newsockfd, clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n;

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

    /* Bind the socket to the local address */
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
    {
        perror("ERROR on binding");
        exit(1);
    }

    /* Listen for incoming connections */
    listen(sockfd, 100);
    clilen = sizeof(cli_addr);

    printf("Waiting for incoming connections...\n");
    while (1) {
        /* Accept incoming connections */
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0)
        {
            perror("ERROR on accept");
            exit(1);
        }

        /* If a connection is established, process it */
        memset(buffer, 0, 256);
        n = recv(newsockfd, buffer, 255, 0);
        if (n < 0)
        {
            perror("ERROR reading from socket");
            exit(1);
        }

        //print_request((uint8_t *)buffer);


        // Extract the hash, start, and end values from the request
        // From the network they are in big-endian format, so we need to convert them to little-endian for processing
        uint64_t start_network, end_network, start, end;
        uint8_t hash_network[SHA256_DIGEST_LENGTH];
        memcpy(&hash_network, buffer + PACKET_REQUEST_HASH_OFFSET, SHA256_DIGEST_LENGTH);
        memcpy(&start_network, buffer + PACKET_REQUEST_START_OFFSET, sizeof(start_network));
        memcpy(&end_network, buffer + PACKET_REQUEST_END_OFFSET, sizeof(end_network));

        start = htobe64(start_network);
        end = htobe64(end_network);

        #ifdef DEBUG
            printf("Received Hash: %02x\n", (char *)buffer + PACKET_REQUEST_HASH_OFFSET);
            printf("Received request: start=%llu, end=%llu\n", (start), (end));
        #endif


        // brute force the answer
        uint64_t candidate = brute_force(hash_network, start, (start + end));    



        n = send(newsockfd, (char *)&candidate, sizeof(candidate), 0);
        if (n < 0)
        {
            perror("ERROR writing to socket");
            exit(1);
        }

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


/*
    PLAN
    format of resonse to client:
    8 bytes, little endian, uint64_t, the answer to the request

    Algorithm to brute force the answer:
    1. Get the hash, start, end, and prio from the request
    2. For each number in the range [start, end]:
        a. Convert the number to a string
        b. Hash the string using SHA256
        c. Compare the hash to the requested hash
        d. If the hashes match, return the number as the answer
    
    Process multiple requests from the client

    
*/