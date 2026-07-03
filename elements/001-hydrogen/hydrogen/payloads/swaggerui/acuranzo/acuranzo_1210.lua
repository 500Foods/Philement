-- Migration: acuranzo_1210.lua
-- Seeds the default Orchestrator row in the scripts table for the Hydrogen Scripting Subsystem (Phase 11c)

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-07-01 - Initial creation for LUA_PLAN Phase 11c (seed Orchestrator)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1210"
cfg.GROUP_NAME = "Orchestrators"
cfg.SCRIPT_NAME = "Orchestrator"
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
table.insert(queries,{sql=[[

    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    WITH next_query_id AS (
        SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
        FROM ${SCHEMA}${QUERIES}
    )
    SELECT
        new_query_id                                                        AS query_id,
        ${MIGRATION}                                                        AS query_ref,
        ${STATUS_ACTIVE}                                                    AS query_status_a27,
        ${TYPE_FORWARD_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        [=[
            INSERT INTO ${SCHEMA}scripts (
                group_name,
                script_name,
                script_type,
                schedule,
                next_run,
                last_run_start,
                last_run_end,
                status,
                code,
                summary,
                ${COMMON_FIELDS}
            )
            VALUES (
                '${GROUP_NAME}',
                '${SCRIPT_NAME}',
                0,                              -- script_type = orchestrator (Lookup 061)
                NULL,                           -- schedule (orchestrators do not follow a schedule themselves)
                NULL,                           -- next_run
                NULL,                           -- last_run_start
                NULL,                           -- last_run_end
                1,                              -- status = enabled (Lookup 062)
                [==[
                    -- Hydrogen Scripting Subsystem - Reference Orchestrator
                    --
                    -- Phase 11c/f of the LUA_PLAN. Seeded into the `scripts` table by
                    -- migration 1210 (group_name='Orchestrators', script_name='Orchestrator').
                    -- The C-side orchestrator loader reads this source from the database
                    -- at the `READY FOR REQUESTS` hook (QueryRef #087) and runs it on a
                    -- dedicated pthread's lua_State. It is not a worker; it is the
                    -- subsystem's own long-running tier-2 context.
                    --
                    -- Contract:
                    --   - Cooperative shutdown: check H.shutdown_requested() on every
                    --     iteration; exit the loop cleanly when it returns true.
                    --   - Use H.sleep(ms) between iterations. H.sleep returns early
                    --     when shutdown is requested, so a 1-second H.sleep wakes up
                    --     within ~100 ms of landing.
                    --   - H.log.* is the right channel for status messages; H.set_current_state
                    --     is a no-op here (no scoreboard entry - the Orchestrator is not a job).
                    --   - H.scoreboard.list() / H.scoreboard.submit() reflect the in-memory
                    --     scoreboard. The Orchestrator is expected to drive the scoreboard;
                    --     the worker pool drains it.
                    --
                    -- This reference Orchestrator:
                    --   1. Logs that it has started.
                    --   2. Every TICK_MS, lists the in-memory scoreboard and logs a summary.
                    --   3. If the scoreboard is empty, submits a sample "tick" job to
                    --      exercise the worker pool end-to-end.
                    --   4. Exits cleanly when H.shutdown_requested() is true.

                    local TICK_MS = 1000

                    H.log.info("Orchestrator: started")

                    local iterations = 0
                    while not H.shutdown_requested() do
                        iterations = iterations + 1
                        local jobs = H.scoreboard.list()
                        H.log.info("Orchestrator: tick %d, %d job(s) in scoreboard",
                                    iterations, #jobs)

                        if #jobs == 0 then
                            local id = H.scoreboard.submit({
                                script_name = "orchestrator.tick",
                                source = "H.set_current_state('orchestrator tick ' .. tostring(os.time()))\n"
                                         .. "return 0\n",
                            })
                            if id then
                                H.log.info("Orchestrator: submitted tick job %s", id)
                            else
                                H.log.warn("Orchestrator: submit returned nil")
                            end
                        end

                        H.sleep(TICK_MS)
                    end

                    H.log.info("Orchestrator: shutdown requested, exiting after %d iteration(s)",
                                iterations)
                ]==],
                [==[
                    # Reference Orchestrator

                    Tier-2 long-running Lua script. The Hydrogen Scripting
                    Subsystem runs exactly one Orchestrator per process; it
                    is loaded from the `scripts` table at the
                    `READY FOR REQUESTS` hook and torn down at landing.

                    This is the seed implementation; production deployments
                    can replace it with a real scheduler (poller,
                    event-driven, batch) by editing this row or by
                    configuring `config->scripting.Orchestrator` to point
                    at a different `(group_name, script_name)` row.
                ]==],
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed default Orchestrator row in ${TABLE} table'                   AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed default Orchestrator row in ${TABLE} table

            Inserts the default `Orchestrators/Orchestrator.lua` row in the
            `scripts` table. The Orchestrator's code is the reference
            scheduling loop shipped under `src/scripting/orchestrator.lua` in
            the Hydrogen source tree. Production deployments may replace
            the code (or point `config->scripting.Orchestrator` at a
            different row) to use a different scheduler.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
-- Reverse: delete the seeded Orchestrator row
table.insert(queries,{sql=[[

    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    WITH next_query_id AS (
        SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
        FROM ${SCHEMA}${QUERIES}
    )
    SELECT
        new_query_id                                                        AS query_id,
        ${MIGRATION}                                                        AS query_ref,
        ${STATUS_ACTIVE}                                                    AS query_status_a27,
        ${TYPE_REVERSE_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        [=[
            DELETE FROM ${SCHEMA}${TABLE}
            WHERE group_name  = '${GROUP_NAME}'
              AND script_name = '${SCRIPT_NAME}';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove seed Orchestrator row from ${TABLE} table'                  AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove seed Orchestrator row from ${TABLE} table

            This is provided for completeness when testing the migration system.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
