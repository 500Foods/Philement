/*
 * Scripting Subsystem - Host API: H.llm (LLM Calls)
 *
 * Internal declarations for scripting_api_llm.c, exposed for Unity
 * unit tests (NOT part of the stable public API). The resolve/build
 * helpers are pure-ish functions that tests can drive directly.
 */

#ifndef HYDROGEN_SCRIPTING_API_LLM_H
#define HYDROGEN_SCRIPTING_API_LLM_H

#include <stdbool.h>

// Types used by the helpers.
#include <jansson.h>                               // json_t
#include <src/database/dbqueue/dbqueue.h>          // DatabaseQueue
#include <src/api/wschat/helpers/engine_cache.h>   // ChatEngineConfig

DatabaseQueue* resolve_llm_db_queue(const char* db_name, char** err_out);
ChatEngineConfig* resolve_llm_engine(const char* model_name, const char* db_name);
char* build_llm_request_json(const char* prompt, int max_tokens, double temperature);

#endif /* HYDROGEN_SCRIPTING_API_LLM_H */
