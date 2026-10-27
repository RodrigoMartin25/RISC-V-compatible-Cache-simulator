//Written by Eren Önel

//Eren: Used the code from my own Mini-Assembly homework for converting numbers but modified it to what is needed here

//Used it as own file to be able to read in both csvreader and parsing


#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include "number_converter.h"

// special chars that stop the conversion but does not show as an error

bool special_char (char to_test, const char array_of_chars[]) {
    while (*array_of_chars) {
        if (*array_of_chars == to_test) {
            return true;
        }
        array_of_chars++;
    }
    return false;
}


// Numbers can also be written with underscores but are not allowed at the beginning or end and also not consecutively 

bool char_to_number( char* num,  char** char_out, uint32_t max, uint32_t* out, const char* special_chars) {
    uint32_t number = 0;

    bool underscore_tester = false;
    bool expecting_digit = true;

    while (*num) {
        char c = *num;

        if (special_char(c, special_chars)) {
            break;
        }


        if (c >= '0' && c <= '9') {
            uint32_t digit = c - '0';
            
            if (number > (max - digit)/10){
                fprintf(stderr, "Integer overflow! Numbers can not be bigger than %#x or %u!\n",max, max);
                return false;
            }

            number = number * 10;
            number += digit;
            
            underscore_tester = false;
        }else if (c == '_') {
            if (expecting_digit) {
                fprintf(stderr, "There shouldn't be an underscore at the beginning of the number\n");
                return false;
            }

            if (underscore_tester) {
                fprintf(stderr, "Consecutive underscores are not allowed!\n");
                return false;
            }

            underscore_tester = true;
            num++;
            expecting_digit = false;
            continue;
        } else {
            fprintf(stderr, "There was a char that is not valid in the decimal system! Char '%c'\n", c);
            return false;
        }

        expecting_digit = false;
        num++;
    }

    if (expecting_digit) {
        fprintf(stderr, "There was no number written!\n");
        return false;
    }

    if (underscore_tester) {
        fprintf(stderr, "There shouldn't be an underscore at the end of the number\n");
        return false;
    }
    
    *out = number;
    if (char_out) *char_out = num;
    return true;
}

bool hex_to_num ( char* hex,  char** char_out, uint32_t max, uint32_t* out, const char* special_chars) {
    uint32_t num = 0;
    bool underscore_tester = false;
    bool expecting_digit = true;

    while (*hex) {
        char c = *hex;
        
        if (special_char(c, special_chars)) {
            break;
        }

        uint32_t digit;

        if (c >= '0' && c <= '9') {
            digit = c - '0';
            underscore_tester = false;
        }else if (c >= 'a' && c <= 'f') {
            digit = c - 'a' + 10;
            underscore_tester = false;
        }else if (c >= 'A' && c <= 'F') {
            digit = c - 'A' + 10;
            underscore_tester = false;
        }else if (c == '_') {
            if (expecting_digit) {
                fprintf(stderr, "There shouldn't be an underscore at the beginning of the number\n");
                return false;
            }

            if (underscore_tester) {
                fprintf(stderr, "Consecutive underscores are not allowed!\n");
                return false;
            }

            underscore_tester = true;
            expecting_digit = false;
            hex++;
            continue;
        } else {
            fprintf(stderr, "There was a char that is not valid in the hexadecimal system! Char '%c'\n", c);
            return false;
        }

        if (num > (max - digit) >> 4) {
            fprintf(stderr, "Integer overflow! Numbers can not be bigger than %#x or %u!\n",max, max);
            return false;
        }

        num = num << 4;
        num += digit;
        hex++;
        expecting_digit = false;
    }

    if (expecting_digit) {
        fprintf(stderr, "There was no number written!\n");
        return false;
    }

    if (underscore_tester) {
        fprintf(stderr, "There shouldn't be an underscore at the end of the number\n");
        return false;
    }
    
    *out = num;
    if (char_out) *char_out = hex;
    return true;
}

//default converting should always be uint32_t

bool convert_to_number( char* string_of_number,  char** char_out, uint32_t* out, const char* special_chars){
    if (!(string_of_number && *string_of_number)) {
        return false;
    }

    if (string_of_number[0] == '-') {
        fprintf(stderr, "Negative numbers are not allowed\n");
        return false;
    }
    
    if (string_of_number[0] == '0' && (string_of_number[1] == 'x' || string_of_number[1] == 'X')) {
        return hex_to_num(string_of_number + 2,char_out, 0xFFFFFFFFu, out, special_chars);
    }else {
        return char_to_number(string_of_number,char_out, 0xFFFFFFFFu, out, special_chars);
    }    
}


//here the maximum number can be given, as bit size

bool convert_to_number_with_maximum( char* string_of_number,  char** char_out, uint32_t max, uint32_t* out, const char* special_chars){
    if (!(string_of_number && *string_of_number)) {
        return false;
    }

    if (string_of_number[0] == '-') {
        fprintf(stderr, "Negative numbers are not allowed\n");
        return false;
    }

    if (max > 32) {
        fprintf(stderr, "Numbers are not allowed to be bigger than 32 bit!\n");
        return false;
    }
    
    uint32_t max_value = (max == 32) ? 0xFFFFFFFFu : (1u << max )- 1;

    
    if (string_of_number[0] == '0' && (string_of_number[1] == 'x' || string_of_number[1] == 'X')) {
        return hex_to_num(string_of_number + 2, char_out, max_value, out, special_chars);
    }else {
        return char_to_number(string_of_number, char_out, max_value, out, special_chars);
    }
}