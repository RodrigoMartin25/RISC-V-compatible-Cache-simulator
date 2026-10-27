//Written by Eren Önel
//Used a modified version of the homework: P4 Mini Projekt Assembler

#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include "number_converter.h"
#include "parsing.h"




Parsing_result read_from_terminal(int argc, char* argv[]){

    //Currently the defaults are just set randomly, should be changed later on

    //Defaults are from general size of the caches

    char* tf_file = NULL;
    //-1 would be the max in uint32_t
    uint32_t cycles = 0xFFFFFFFFu;
    uint8_t num_cache_levels = 3;
    uint32_t cacheline_size = 64;
    uint32_t num_lines_l1 = 256;
    uint32_t num_lines_l2 = 2048;
    uint32_t num_lines_l3 = 32768;
    uint32_t latency_cache_l1 = 2;
    uint32_t latency_cache_l2 = 10;
    uint32_t latency_cache_l3 = 30;
    uint8_t mapping_strategy = 0;

    /*
    I am not sure if there is a possibility to do this without the shortcuts, seems a bit weird for some of them.
    Have to look into it
    */

    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"cycles", required_argument, 0, 'c'},
        {"tf", required_argument, 0, 't'},
        {"num-cache-levels", required_argument, 0, 'n'},
        {"cacheline-size", required_argument, 0, 's'},
        {"num-lines-l1", required_argument, 0, 'a'},
        {"num-lines-l2", required_argument, 0, 'b'},
        {"num-lines-l3", required_argument, 0, 'd'},
        {"latency-cache-l1", required_argument, 0, 'x'},
        {"latency-cache-l2", required_argument, 0, 'y'},
        {"latency-cache-l3", required_argument, 0, 'z'},
        {"mapping-strategy", required_argument, 0, 'm'},
        {0, 0, 0, 0}
    };

    int opt;
    Parsing_result result;
    result.no_errors = 1;
    result.help = 0;

    while ((opt = getopt_long(argc, argv, "hc:t:n:s:a:b:d:x:y:z:m:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'h':
                printf("Usage: %s [options] <input_file>\n", argv[0]);
                printf("Numbers can be given as hexadecimal or as decimal\n");
                printf("Options:\n");
                printf("  -c, --cycles <uint32_t>               Number of cycles to be simulated\n");
                printf("  -t, --tf <file>                       The path to tracefile. Without this argument no tracefile will be written\n");
                printf("  -n, --num-cache-levels <uint8_t>      Number of cache levels\n");
                printf("  -s, --cacheline-size <uint32_t>       The size of the one cacheline in bytes\n");
                printf("  -a, --num-lines-l1 <uint32_t>         Number of cachelines in L1-cache\n");
                printf("  -b, --num-lines-l2 <uint32_t>         Number of cachelines in L2-cache\n");
                printf("  -d, --num-lines-l3 <uint32_t>         Number of cachelines in L3-cache\n");
                printf("  -x, --latency-cache-l1 <uint32_t>     Latency of L1-cache in clock cycles\n");
                printf("  -y, --latency-cache-l2 <uint32_t>     Latency of L2-cache in clock cycles\n");
                printf("  -z, --latency-cache-l3 <uint32_t>     Latency of L3-cache in clock cycles\n");
                printf("  -m, --mapping-strategy <uint8_t>      Choose your mapping strategy: 0 = direct-mapped, 1 = fully associative\n");
                printf("  -h, --help                            Show this help message\n");
                result.help = 1;
                return result;
            case 'c':{
                char* end_ptr;
                if (!convert_to_number(optarg, &end_ptr, &cycles, "")) {
                    result.no_errors = 0;
                    fprintf(stderr, "Could not read number for cycles\n");
                    return result;
                }
                break;
            }
            case 'n': {
                uint32_t to_write_to = 0;
                char* end_ptr;
                if (!convert_to_number_with_maximum(optarg, &end_ptr, 8, &to_write_to, "")) {
                    result.no_errors = 0;
                    fprintf(stderr, "Could not read number for cache-levels\n");
                    return result;
                }
                num_cache_levels = (uint8_t) to_write_to;

                if (num_cache_levels > 3 || num_cache_levels == 0) {
                    result.no_errors = 0;
                    fprintf(stderr, "There has to be at least one or at most three cache levels\n");
                    return result;
                }
                
                break;
            }
            case 's':{
                char* end_ptr;
                if (!convert_to_number(optarg, &end_ptr, &cacheline_size, "")) {
                    result.no_errors = 0;
                    fprintf(stderr, "Could not read number for cacheline_size\n");
                    return result;
                }

                break;
            }
            case 'a':{
                char* end_ptr;
                if (!convert_to_number(optarg, &end_ptr, &num_lines_l1, "")) {
                    result.no_errors = 0;
                    fprintf(stderr, "Could not read number for num_lines_l1\n");
                    return result;
                }

                break;
            }
            case 'b':{
                char* end_ptr;
                if (!convert_to_number(optarg, &end_ptr, &num_lines_l2, "")) {
                    result.no_errors = 0;
                    fprintf(stderr, "Could not read number for num_lines_l2\n");
                    return result;
                }
                break;
            }
            case 'd':{
                char* end_ptr;
                if (!convert_to_number(optarg, &end_ptr, &num_lines_l3, "")) {
                    result.no_errors = 0;
                    fprintf(stderr, "Could not read number for num_lines_l3\n");
                    return result;
                }
                break;
            }
            case 'x':{
                char* end_ptr;
                if (!convert_to_number(optarg, &end_ptr, &latency_cache_l1,"")) {
                    result.no_errors = 0;
                    fprintf(stderr, "Could not read number for latency_cache_l1\n");
                    return result;
                }
                break;
            }
            case 'y':{
                char* end_ptr;
                if (!convert_to_number(optarg, &end_ptr, &latency_cache_l2, "")) {
                    result.no_errors = 0;
                    fprintf(stderr, "Could not read number for latency_cache_l2\n");
                    return result;
                }
                break;
            }
            case 'z':{
                char* end_ptr;
                if (!convert_to_number(optarg, &end_ptr, &latency_cache_l3, "")) {
                    result.no_errors = 0;
                    fprintf(stderr, "Could not read number for latency_cache_l3\n");
                    return result;
                }
                break;
            }
            case 'm': {
                uint32_t to_write_to = 0;
                char* end_ptr;
                if (!convert_to_number_with_maximum(optarg, &end_ptr, 8, &to_write_to, "")) {
                    result.no_errors = 0;
                    fprintf(stderr, "Could not read number for mapping_strategy\n");
                    return result;
                }
                mapping_strategy = (uint8_t) to_write_to;

                if (mapping_strategy > 1) {
                    result.no_errors = 0;
                    fprintf(stderr, "There are only two mapping strategies with values 0 and 1. Look at help for more details\n");
                    return result;
                }
                break;
                }
            case 't':
                tf_file = optarg;
                break;
            case '?':
                result.no_errors = 0;
                fprintf(stderr, "Unknown option: -%c\n", optopt);
                return result;
            default:
                result.no_errors = 0;
                fprintf(stderr, "Unexpected error while parsing options.\n");
                return result;
        }
    }

    if (optind == argc - 1) {
        result.csv_file = argv[optind];
    }else if (optind >= argc){
        result.no_errors = 0;
        fprintf(stderr, "Too many arguments were given. Should only be one\n");
        return result;
    } else {
        result.no_errors = 0;
        fprintf(stderr, "No csv_file was given. Is needed for the simulation\n");
        return result;
    }


    result.cacheline_size = cacheline_size;
    result.cycles = cycles;
    result.latency_cache_l1 = latency_cache_l1;
    result.latency_cache_l2 = latency_cache_l2;
    result.latency_cache_l3 = latency_cache_l3;
    result.num_cache_levels = num_cache_levels;
    result.num_lines_l1 = num_lines_l1;
    result.num_lines_l2 = num_lines_l2;
    result.num_lines_l3 = num_lines_l3;
    result.tf_file = tf_file;
    result.mapping_strategy = mapping_strategy;

    return result;
}
