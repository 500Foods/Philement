#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "mock_generate_query_id.h"

static const char* mock_query_id_result = NULL;
static int call_count = 0;

char* mock_generate_query_id(void) {
    if (mock_query_id_result) {
        char buffer[64];
        if (call_count == 0) {
            // First call returns the set result
            call_count++;
            return strdup(mock_query_id_result);
        } else {
            // Subsequent calls return unique IDs
            long current_time = (long)time(NULL) + call_count - 1;
            unsigned long long counter = 123ULL + (unsigned long long)call_count;
            call_count++;
            snprintf(buffer, sizeof(buffer), "conduit_%llu_%ld", counter, current_time);
            return strdup(buffer);
        }
    }
    return NULL; // Simulate failure
}

void mock_generate_query_id_set_result(const char* result) {
    mock_query_id_result = result;
}

void mock_generate_query_id_reset(void) {
    mock_query_id_result = NULL;
    call_count = 0;
}