#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include "messages.h"
#include "brute.h"

#define LONESHA256_STATIC
#include "lonesha256.h"
//#define DEBUG


uint64_t brute_force(const uint8_t *target_hash, uint64_t start, uint64_t end) {
    uint32_t target_prefix =
        ((uint32_t)target_hash[0] << 24) |
        ((uint32_t)target_hash[1] << 16) |
        ((uint32_t)target_hash[2] << 8)  |
        (uint32_t)target_hash[3];
    unsigned char out[32];
    

    for (uint64_t candidate = start; candidate < end; candidate++) {
        uint64_t candidate_le = htole64(candidate);
        lonesha256(out, (uint8_t *)&candidate_le, sizeof(candidate_le));
        
        uint32_t out_prefix =
            ((uint32_t)out[0] << 24) |
            ((uint32_t)out[1] << 16) |
            ((uint32_t)out[2] << 8)  |
            (uint32_t)out[3];
        if (out_prefix == target_prefix && 
            memcmp(out, (char *)target_hash, SHA256_DIGEST_LENGTH) == 0) {
                #ifdef DEBUG
                    printf("[worker %lu] Success: %lu (processed: %lu)\n\n",
                            (unsigned long)pthread_self(),
                            candidate,
                            candidate - start + 1);
                #endif
                return htobe64(candidate_le);
        }
        if (candidate == end - 1) {
            #ifdef DEBUG
                printf("[worker %lu] Failed to find a match in the range %lu to %lu\n",
                        (unsigned long)pthread_self(),
                        start,
                        end);
            #endif
            break;
        }
    }
    return 0;
}