/*
 * Scripting Subsystem - State
 *
 * Phase 3b of the LUA_PLAN. Defines and owns the static state for the
 * Scripting subsystem. See scripting.h for the field contract.
 *
 * `scripting_threads` and `scripting_system_shutdown` are owned by
 * state.c (alongside every other subsystem's flag and ServiceThreads
 * handle) so they're visible to the wider system (state queries,
 * restart path, registry). This file owns only the scripting-specific
 * pieces: the long-lived Orchestrator lua_State and the init/cleanup
 * helpers.
 */

 // Project includes
#include <src/hydrogen.h>
#include <lua.h>

// Local includes
#include "scripting.h"
#include "lua_context.h"

// Long-lived Orchestrator state placeholder. NULL in Phase 3b - Phase 11
// will allocate and compile a real Orchestrator state here.
lua_State* scripting_orchestrator_state = NULL;

/*
 * Initialize the subsystem's static state. Safe to call multiple times.
 */
void scripting_init_state(void) {
    scripting_system_shutdown = 0;
    scripting_orchestrator_state = NULL;
    init_service_threads(&scripting_threads, SR_SCRIPTING);
}

/*
 * Tear down the subsystem's static state.
 *
 * Phase 3b has no Orchestrator running and no workers, so this is
 * mostly bookkeeping. Once Phase 11 lands, the Orchestrator state will
 * be lua_close'd here.
 */
void scripting_cleanup_state(void) {
    if (scripting_orchestrator_state) {
        H_lua_destroy_context(scripting_orchestrator_state);
        scripting_orchestrator_state = NULL;
    }
    scripting_system_shutdown = 1;
    init_service_threads(&scripting_threads, SR_SCRIPTING);
}
