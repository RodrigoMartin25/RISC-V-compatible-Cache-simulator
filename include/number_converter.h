// Written by Eren Önel

#ifndef NUMBER_CONVERTER_H
#define NUMBER_CONVERTER_H


#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

bool convert_to_number(char* string_of_number, char** char_out, uint32_t* out, const char* special_chars);

bool convert_to_number_with_maximum(char* string_of_number, char** char_out, uint32_t max, uint32_t* out, const char* special_chars);

#endif