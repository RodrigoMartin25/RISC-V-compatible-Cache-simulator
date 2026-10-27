// Written by Eren Önel
// So these structs exist everywhere needed

#ifndef GLOBAL_STRUCTS_H
#define GLOBAL_STRUCTS_H

#include <stdint.h>

// C++ guard
#ifdef __cplusplus
extern "C" {
#endif

typedef struct Request{
    uint32_t addr;
    uint32_t data;
    uint8_t w;
} Request;

typedef struct Result {
    uint32_t cycles;
    uint32_t misses;
    uint32_t hits;
}Result;

// C++ guard end
#ifdef __cplusplus
}
#endif

#endif
