//This was written by Eren Önel


#include <stdio.h>
#include <stdlib.h>
#include "CSV_reader.h"
#include "file_handler.h"
#include "global_structs.h"
#include "number_converter.h"


// Here I included the ability to jump over spaces and tabs

char* skip_whitespace(char* cursor){
    while (*cursor) {
        if (*cursor != ' ' && *cursor != '\t') {
            return cursor;
        }
        cursor++;
    }
    return cursor;
}


const char special_chars[] = {',', '\n', '\r', ' ', '\t', '\0'};


bool get_request_from_line(char* cursor, uint32_t line, char** out, Request* out_request){

    char type = *cursor;

    /*
    Both lower and uppercase letters of write and read are valid, others are not
    */

    if(type != 'W' && type != 'R' && type != 'w' && type != 'r'){
        fprintf(stderr, "On the line %d there was a the wrong type! Must be W/w or R/r!\n", line);
        return false;
    }
    cursor++;
    
    cursor = skip_whitespace(cursor);

    if (*cursor != ','){
        fprintf(stderr, "Type should only be one letter long! Are you missing a comma? Line: %u\n", line);
        return false;
    }
    

    cursor++;

    cursor = skip_whitespace(cursor);

    uint32_t address_num;


    if (!(convert_to_number(cursor, &cursor, &address_num, special_chars))) {
        fprintf(stderr, "Could not convert address to a number! Line %u\n", line);
        return false;
    }

    //It should not be possible to read or write from the address of the last three bytes, because there would not be enough memory

    if (address_num > 0xFFFFFFFF - 4) {
        fprintf(stderr, "When read or written from memory 4 bytes of space are needed. At the given address there are not enough space to read or write! Line %d\n", line);
        return false;
    }
    

    /*
    The size_of_address is exactly as long as what is written for address, 
    which, when added to the cursor, jumps to the next char after the length of address
    */

    cursor = skip_whitespace(cursor);

    if (*cursor != ','){
        fprintf(stderr, "Missing comma to differentiate between address and data! Line: %u\n", line);
        return false;
    }

    cursor++;

    uint8_t w;

    if (type == 'R' || type == 'r'){
        w = 0;
    }else {
        w = 1;
    }

    uint32_t data_num = 0;

    /*
    differentiating between write and read because there should not be any data for read
    */

    cursor = skip_whitespace(cursor);

    if (w){

        /*
        data follows the same logic as address
        */
        

        if (!convert_to_number(cursor, &cursor, &data_num, special_chars)) {
            fprintf(stderr, "Could not convert data to number! Line %u\n", line);
            return false;
        }

        cursor = skip_whitespace(cursor);

        if (*cursor == ','){
            fprintf(stderr, "Too many arguments! Should only be 3 arguments. Line: %u\n", line);
            return false;
        }

    } else {
        if (*cursor == ','){
            fprintf(stderr, "Too many arguments! Should only be 3 arguments, while the third should be empty. Line: %u\n", line);
            return false;
        }

        if (*cursor != '\0' && *cursor != '\n' && *cursor != '\r') {
            fprintf(stderr, "There is something written in data even though it is of type read! Line: %u\n", line);
            return false;
        }
    }


    out_request->w = w;
    out_request->addr = address_num;
    out_request->data = data_num;

    *out = cursor;
    
    return true;
}


// This method jumps over all the empty lines, it also counts the amount of empty lines so it can be debugged later on if there is an error

char* jump_over_empty_lines(char* cursor, uint32_t* line_counter) {
    while (true) {
        cursor = skip_whitespace(cursor);

        if (*cursor == '\n' || *cursor == '\r') {
            (*line_counter)++;
            cursor++;
            while (*cursor == '\n' || *cursor == '\r') {
                (*line_counter)++;
                cursor++;
            }
        }else {
            return cursor;
        }
    }
}


Request* get_request_from_csv_file(const char* csv_path, uint32_t* num_requests_ptr) {

    size_t remaining_chars = 0;
    char* contents = read_file(csv_path, &remaining_chars);
    
    
    /*
    There was an error in reading the file or the file was empty
    */
   if (!contents) {
       fprintf(stderr, "There was an error reading the csv file or it was empty!\n");
       return NULL;
    }
    
    uint8_t count_commas = 0;
    char* cursor = contents;
    
    /*
    Here the header line is skipped all the while checking if there are three headers
    */
   
   while (*cursor && *cursor != '\n' && *cursor != '\r') {
       if (*cursor == ','){
           count_commas++;
        }
        cursor++;
    }
    
    uint32_t line_counter = 1;
    
    cursor = jump_over_empty_lines(cursor, &line_counter);

    
    Request* requests = NULL;
    
    /*
    This checks if there was any other data given.
    */
   
   if (!*cursor) {
       fprintf(stderr, "Only headers were given. The rest was empty.\n");
       goto cleanup;
    }
    
    if (count_commas != 2) {
        fprintf(stderr, "Not the right amount of headers in the CSV file\n");
        goto cleanup;
    }
    
    
    
    // Use dynamic allocating by doubling the space everytime 
    
    

    
    if (!(requests = malloc(sizeof(Request) * 4))) {
        fprintf(stderr, "Not enough memory for storing the requests\n");
        goto cleanup;
    }

    

    uint32_t counter = 0;

    uint32_t current_size = 4;


    
    /*
    This goes through all the lines of the csv file and returns the request of the next line
    If there were something wrong in the file than it returns a NULL pointer
    */


    while (*cursor) {
        counter++;


        // This condition looks up if the array needs to reallocate
        if (counter > current_size) {
            // Double the size
            current_size <<= 1;
            
            //reallocate more space
            Request* tmp = realloc(requests, sizeof(Request) * current_size);
            if (!tmp) {
                fprintf(stderr, "Not enough memory for storing the requests\n");
                free(requests);
                requests = NULL;
                goto cleanup;
            }

            requests = tmp;
        }

        Request request;
        if (!get_request_from_line(cursor, line_counter, &cursor, &request) ) {
            fprintf(stderr, "There was a problem retrieving the data from csv\n");
            free(requests);
            requests = NULL;
            goto cleanup;
        }

        cursor = jump_over_empty_lines(cursor, &line_counter);        
        requests[counter - 1] = request;
    }


    // If the reallocation has done too much it cuts it off here


    if (counter < current_size) {
        Request* tmp = realloc(requests, sizeof(Request) * counter);
        if (!tmp) {
            fprintf(stderr, "Not enough memory for storing the requests\n");
            free(requests);
            requests = NULL;
            goto cleanup;
        }

        requests = tmp;
    }



    *num_requests_ptr = counter;

    cleanup:
    free(contents);
    contents = NULL;

    return requests;
}