/*
 * Scripting Subsystem - Host API: H.scoreboard and H.package.searcher
 *
 * Internal declaration for scripting_api_scoreboard.c, exposed for
 * Unity unit tests (NOT part of the stable public API).
 */

#ifndef HYDROGEN_SCRIPTING_API_SCOREBOARD_H
#define HYDROGEN_SCRIPTING_API_SCOREBOARD_H

// Forward-declare Lua types so we don't drag the Lua headers into
// every translation unit that includes this header.
typedef struct lua_State lua_State;

// Bytecode dump buffer used by bytecode_dump_writer. Defined privately
// in scripting_api_scoreboard.c; only an opaque pointer is needed here.
typedef struct BytecodeDumpBuffer BytecodeDumpBuffer;

int bytecode_dump_writer(lua_State* L, const void* p, size_t sz, void* ud);

#endif /* HYDROGEN_SCRIPTING_API_SCOREBOARD_H */
