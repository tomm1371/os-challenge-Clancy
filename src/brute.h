#ifndef BRUTE_H
#define BRUTE_H

#include <stdint.h>
#include "messages.h"



// Returns the matching candidate, or 0 if no match was found.
uint64_t brute_force(const uint8_t hash[SHA256_DIGEST_LENGTH],
                     uint64_t start,
                     uint64_t end);

#endif