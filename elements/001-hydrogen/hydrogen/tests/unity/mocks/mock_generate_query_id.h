#ifndef MOCK_GENERATE_QUERY_ID_H
#define MOCK_GENERATE_QUERY_ID_H

#ifdef USE_MOCK_GENERATE_QUERY_ID
#define generate_query_id mock_generate_query_id
#endif

char* mock_generate_query_id(void);
void mock_generate_query_id_set_result(const char* result);
void mock_generate_query_id_reset(void);

#endif