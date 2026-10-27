//Written by Eren Önel

#ifndef CSV_READER_H
#define CSV_READER_H

#include "global_structs.h"

Request* get_request_from_csv_file(const char* csv_path, uint32_t* size_out);

#endif
