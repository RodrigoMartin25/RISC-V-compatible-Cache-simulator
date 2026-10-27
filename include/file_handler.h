// version from the tutorial

#ifndef FILES_H
#define FILES_H


#include <stddef.h>

char* read_file(const char* path, size_t* number_of_chars);

void write_file(const char* path, const char* string);

#endif