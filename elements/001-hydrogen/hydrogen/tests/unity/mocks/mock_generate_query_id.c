#include <stdlib.h>
#include <string.h>
#include "mock_generate_query_id.h"

static const char* mock_query_id_result = NULL;

char* mock_generate_query_id(void) {
    if (mock_query_id_result) {
        return strdup(mock_query_id_result);
    }
    return NULL; // Simulate failure
}

void mock_generate_query_id_set_result(const char* result) {
    mock_query_id_result = result;
}

void mock_generate_query_id_reset(void) {
    mock_query_id_result = NULL;
}