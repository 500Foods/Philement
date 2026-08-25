-- Migration: acuranzo_1364.lua
-- OOM fix: patch Orchestrators.Orchestrator to call H.scoreboard.prune_terminal() each tick
--
-- The Orchestrator calls H.scoreboard.list() every tick (1s) which snapshots
-- ALL entries including COMPLETED / FAILED / KILLED. With EnsureCanvasUser
-- submitting ~240 jobs/hr (every 15s), terminal entries accumulated unboundly,
-- each holding result_json / params_json / error strings (up to 1 MiB per
-- entry via result_json). RSS crept above the 128 MiB cgroup limit and the pod
-- OOMKilled after ~29.5h of uptime.
--
-- This migration patches the Orchestrator body to call
-- H.scoreboard.prune_terminal() before the per-tick list() snapshot so
-- finished jobs are reclaimed immediately. Reverse restores the 1343 body.
--
-- Based on acuranzo_1343.lua (the current production orchestrator).
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-08-24 - Orchestrator: call H.scoreboard.prune_terminal() each tick

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1364"
cfg.GROUP_NAME = "Orchestrators"
cfg.SCRIPT_NAME = "Orchestrator"
-- ----------------------------------------------------------------------------
-- Forward: replace Orchestrators.Orchestrator body (1343 baseline + prune_terminal)
-- ----------------------------------------------------------------------------
table.insert(queries,{sql=[====[

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
            UPDATE ${SCHEMA}scripts
            SET code = [==[
-- Orchestrators.Orchestrator (500 Courses — Phase 30 driver, 1365 prune_terminal OOM fix)
-- Periodic Provision.EnsureCanvasUser submit; per-iteration tick for lifecycle probes.
-- Calls H.scoreboard.prune_terminal() each tick before list() to bound memory.

local function qrows(res)
    if type(res) ~= "table" then return nil end
    if type(res.rows) == "table" then return res.rows end
    return res
end

local TICK_MS = 1000
local ENSURE_EVERY = 15
local ensure_countdown = 0

H.log.info("Orchestrator: started (500courses provision driver)")

local function job_active(jobs, name)
    for _, j in ipairs(jobs or {}) do
        local sn = j.script_name or j.SCRIPT_NAME or ""
        local st = string.lower(tostring(j.status or j.STATUS or ""))
        if sn == name and (st == "pending" or st == "running") then
            return true
        end
    end
    return false
end

local function submit_ensure()
    if job_active(H.scoreboard.list(), "Provision.EnsureCanvasUser") then
        H.log.info("Orchestrator: EnsureCanvasUser already active; skip")
        return
    end
    local _qr, err = H.query_sync([[
        SELECT code FROM ${SCHEMA}scripts
        WHERE group_name = 'Provision' AND script_name = 'EnsureCanvasUser'
        LIMIT 1
    ]])
    local rows = qrows(_qr)
    if err or not rows or not rows[1] then
        H.log.warn("Orchestrator: EnsureCanvasUser source missing: %s",
            tostring(err or "no row"))
        return
    end
    local src = rows[1].code or rows[1].CODE
    if not src or src == "" then
        H.log.warn("Orchestrator: EnsureCanvasUser empty code")
        return
    end
    local id = H.scoreboard.submit({
        script_name = "Provision.EnsureCanvasUser",
        source = src,
    })
    if id then
        H.log.info("Orchestrator: submitted EnsureCanvasUser job %s", tostring(id))
    else
        H.log.warn("Orchestrator: EnsureCanvasUser submit failed")
    end
end

local iterations = 0
-- Run once shortly after boot so first-login lag is bounded.
ensure_countdown = 5

while not H.shutdown_requested() do
    iterations = iterations + 1

    -- Reclaim finished jobs before the per-tick list() snapshot so the
    -- scoreboard does not grow without bound under sustained job throughput.
    local ok_p, pruned = pcall(H.scoreboard.prune_terminal)
    if ok_p and type(pruned) == "number" and pruned > 0 then
        H.log.info("Orchestrator: pruned %d terminal entries", pruned)
    end

    ensure_countdown = ensure_countdown - 1
    if ensure_countdown <= 0 then
        ensure_countdown = ENSURE_EVERY
        local ok, err = pcall(submit_ensure)
        if not ok then
            H.log.warn("Orchestrator: ensure pcall err: %s", tostring(err))
        end
    end
    local jobs = H.scoreboard.list()
    H.log.info("Orchestrator: tick %d, %d job(s)", iterations, #jobs)
    H.sleep(TICK_MS)
end

H.log.info("Orchestrator: shutdown requested, exiting after %d iteration(s)", iterations)
            ]==]
            WHERE group_name = 'Orchestrators'
              AND script_name = 'Orchestrator';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Orchestrator: add H.scoreboard.prune_terminal() each tick (1365)'   AS name,
        [=[
            # Forward Migration ${MIGRATION}: Orchestrator prune_terminal OOM fix

            PRIORITIZE 2.1. Adds H.scoreboard.prune_terminal() call at the top of
            each tick (before H.scoreboard.list()) so COMPLETED / FAILED /
            KILLED entries are reclaimed immediately. Without this, ~240
            EnsureCanvasUser jobs/hr accumulate indefinitely and the per-tick
            list() snapshot copies their owned strings (result_json up to 1 MiB)
            into Lua objects, driving RSS past the 128 MiB cgroup limit.

            Reverse restores the 1343 body (15s poll, no prune).
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

-- ----------------------------------------------------------------------------
-- Reverse: restore 1343 body (no prune)
-- ----------------------------------------------------------------------------
table.insert(queries,{sql=[====[

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
            UPDATE ${SCHEMA}scripts
            SET code = [==[
-- Orchestrators.Orchestrator (500 Courses — Phase 30 driver, 1343 15s poll)
-- Periodic Provision.EnsureCanvasUser submit; per-iteration tick for lifecycle probes.

local function qrows(res)
    if type(res) ~= "table" then return nil end
    if type(res.rows) == "table" then return res.rows end
    return res
end

local TICK_MS = 1000
local ENSURE_EVERY = 15
local ensure_countdown = 0

H.log.info("Orchestrator: started (500courses provision driver)")

local function job_active(jobs, name)
    for _, j in ipairs(jobs or {}) do
        local sn = j.script_name or j.SCRIPT_NAME or ""
        local st = string.lower(tostring(j.status or j.STATUS or ""))
        if sn == name and (st == "pending" or st == "running") then
            return true
        end
    end
    return false
end

local function submit_ensure()
    if job_active(H.scoreboard.list(), "Provision.EnsureCanvasUser") then
        H.log.info("Orchestrator: EnsureCanvasUser already active; skip")
        return
    end
    local _qr, err = H.query_sync([[
        SELECT code FROM ${SCHEMA}scripts
        WHERE group_name = 'Provision' AND script_name = 'EnsureCanvasUser'
        LIMIT 1
    ]])
    local rows = qrows(_qr)
    if err or not rows or not rows[1] then
        H.log.warn("Orchestrator: EnsureCanvasUser source missing: %s",
            tostring(err or "no row"))
        return
    end
    local src = rows[1].code or rows[1].CODE
    if not src or src == "" then
        H.log.warn("Orchestrator: EnsureCanvasUser empty code")
        return
    end
    local id = H.scoreboard.submit({
        script_name = "Provision.EnsureCanvasUser",
        source = src,
    })
    if id then
        H.log.info("Orchestrator: submitted EnsureCanvasUser job %s", tostring(id))
    else
        H.log.warn("Orchestrator: EnsureCanvasUser submit failed")
    end
end

local iterations = 0
-- Run once shortly after boot so first-login lag is bounded.
ensure_countdown = 5

while not H.shutdown_requested() do
    iterations = iterations + 1
    ensure_countdown = ensure_countdown - 1
    if ensure_countdown <= 0 then
        ensure_countdown = ENSURE_EVERY
        local ok, err = pcall(submit_ensure)
        if not ok then
            H.log.warn("Orchestrator: ensure pcall err: %s", tostring(err))
        end
    end
    local jobs = H.scoreboard.list()
    H.log.info("Orchestrator: tick %d, %d job(s)", iterations, #jobs)
    H.sleep(TICK_MS)
end

H.log.info("Orchestrator: shutdown requested, exiting after %d iteration(s)", iterations)
            ]==]
            WHERE group_name = 'Orchestrators'
              AND script_name = 'Orchestrator';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Restore pre-1365 Orchestrator body (no prune)'                      AS name,
        [=[
            # Reverse Migration ${MIGRATION}: restore 1343 orchestrator

            Removes the prune_terminal call added by 1365 forward, restoring
            the pre-1365 Orchestrator body.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
