/*
 * UUID generation utility implementation.
 */

// Global includes
#include <src/hydrogen.h>

// Local includes
#include "utils_uuid.h"

// Generate collision-resistant file identifiers
void generate_uuid(char *uuid_str) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long long int time_in_usec = ((unsigned long long int)tv.tv_sec * 1000000ULL) + (unsigned long long int)tv.tv_usec;

    snprintf(uuid_str, UUID_STR_LEN, "%08llx-%04x-%04x-%04x-%08llx%04x",
             time_in_usec & 0xFFFFFFFFULL,
             (unsigned int)(rand() & 0xffff),
             (unsigned int)((rand() & 0xfff) | 0x4000),
             (unsigned int)((rand() & 0x3fff) | 0x8000),
             ((unsigned long long int)(unsigned int)rand() * (unsigned int)rand()) & 0xFFFFFFFFULL,
             (unsigned int)(rand() & 0xffff));
}
