// Written by Eren Önel

#ifndef PARSING_H
#define PARSING_H


#include <stddef.h>
#include <stdint.h>

typedef struct Parsing_result{
    char* csv_file;
    char* tf_file;
    uint32_t cycles;
    uint8_t help;
    uint8_t num_cache_levels;
    uint32_t cacheline_size;
    uint32_t num_lines_l1;
    uint32_t num_lines_l2;
    uint32_t num_lines_l3;
    uint32_t latency_cache_l1;
    uint32_t latency_cache_l2;
    uint32_t latency_cache_l3;
    uint8_t mapping_strategy;
    uint8_t no_errors;
}Parsing_result;

Parsing_result read_from_terminal(int argc, char *argv[]);

#endif