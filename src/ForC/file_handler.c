//modified version of the tutorial


#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include "file_handler.h"


char *read_file(const char *path, size_t* number_of_chars) {
  char *string = NULL;
  FILE *file;

  if (!(file = fopen(path, "r"))) {
    perror("Error opening file");
    return NULL;
  }

  struct stat statbuf;
  if (fstat(fileno(file), &statbuf)) {
    fprintf(stderr, "Error retrieving file stats\n");
    goto cleanup;
  }
  if (!S_ISREG(statbuf.st_mode) || statbuf.st_size <= 0) {
    fprintf(stderr,
            "Error processing file: Not a regular file or invalid size\n");
    goto cleanup;
  }

  if (!(string = malloc(statbuf.st_size + 1))) {
    fprintf(stderr, "Error reading file: Could not allocate enough memory\n");
    goto cleanup;
  }

  if (fread(string, 1, statbuf.st_size, file) != (size_t)statbuf.st_size) {
    fprintf(stderr, "Error reading file");
    free(string);
    string = NULL;
    goto cleanup;
  }

  *number_of_chars = statbuf.st_size;

  string[statbuf.st_size] = '\0';

cleanup:
  if (file)
    fclose(file);

  return string;
}
