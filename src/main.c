// Written by Eren Önel

#include <stdio.h>
#include <stdlib.h>
#include "CSV_reader.h"
#include "global_structs.h"
#include "parsing.h"
#include "main.h"



int main(int argc, char* argv[]){
    Parsing_result pr = read_from_terminal(argc, argv);
    


    if (pr.help) {
        return 0;
    }

    if (!pr.no_errors){
        fprintf(stderr, "Could not read from terminal\n");
        return 1;
    }


    uint32_t num_requests;

    Request* requests = get_request_from_csv_file(pr.csv_file, &num_requests);

    if (!requests){
        fprintf(stderr, "There was an error in reading the CSV-file\n");
        return 1;
    }
    

    // run C++ method
    
    Result result = run_simulation( pr.cycles ,
                    pr.tf_file ,
                    pr.num_cache_levels ,
                    pr.cacheline_size ,
                    pr.num_lines_l1 ,
                    pr.num_lines_l2 ,
                    pr.num_lines_l3 ,
                    pr.latency_cache_l1 ,
                    pr.latency_cache_l2 ,
                    pr.latency_cache_l3 ,
                    pr.mapping_strategy ,
                    num_requests ,
                    requests);

    printf("======= Result =======\n");
    printf("Cycles : %10u\n", result.cycles);
    printf("Misses : %10u\n", result.misses);
    printf("Hits   : %10u\n", result.hits);
    printf("======================\n");
    

    

    free(requests);

    return 0;
}