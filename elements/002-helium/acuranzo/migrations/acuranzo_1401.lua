-- Migration: acuranzo_1401.lua
-- Orchestrator submits Build.ReceiveSuggested when a row is waiting
--
-- Does not edit acuranzo_1364.lua. Copies that body, keeps the
-- 15 second EnsureCanvasUser submit and the per-tick prune, and
-- adds a one-second suggestion check. Mail is not sent from this
-- loop. Reverse restores the 1364 body.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-23 - Orchestrator scans suggestions for one receipt

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1401"
cfg.GROUP_NAME = "Orchestrators"
cfg.SCRIPT_NAME = "Orchestrator"

local sql_old = [==[
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
local sql_new = [==[
-- Orchestrators.Orchestrator (500 Courses, receipt scan)
-- Each tick reclaims finished jobs, then looks for one suggestion
-- that still needs a receipt. A match submits Build.ReceiveSuggested.
-- An empty queue does not submit. EnsureCanvasUser stays on 15s.

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

local RECEIPT_JOB = "Build.ReceiveSuggested"
local RECEIPT_BACKOFF = 15
local receipt_backoff = 0

local function submit_receipt()
    if job_active(H.scoreboard.list(), RECEIPT_JOB) then
        return
    end
    local qr, err = H.query_sync([[
        SELECT s.suggestion_id
        FROM ${SCHEMA}course_suggestions s
        LEFT JOIN ${SCHEMA}course_build_runs r
          ON r.suggestion_id = s.suggestion_id
        WHERE r.run_id IS NULL
           OR (r.state = 'suggested' AND r.receipt_mail IS NULL)
        LIMIT 1
    ]])
    if err then
        H.log.warn("Orchestrator: suggestion receipt check failed: %s",
            tostring(err))
        receipt_backoff = RECEIPT_BACKOFF
        return
    end
    local found = qrows(qr)
    if not found or not found[1] then
        return
    end
    local src_qr, src_err = H.query_sync([[
        SELECT code FROM ${SCHEMA}scripts
        WHERE group_name = 'Build' AND script_name = 'ReceiveSuggested'
        LIMIT 1
    ]])
    local src_rows = qrows(src_qr)
    if src_err or not src_rows or not src_rows[1] then
        H.log.warn("Orchestrator: ReceiveSuggested source missing: %s",
            tostring(src_err or "no row"))
        receipt_backoff = RECEIPT_BACKOFF
        return
    end
    local src = src_rows[1].code or src_rows[1].CODE
    if not src or src == "" then
        H.log.warn("Orchestrator: ReceiveSuggested empty code")
        receipt_backoff = RECEIPT_BACKOFF
        return
    end
    local id = H.scoreboard.submit({
        script_name = RECEIPT_JOB,
        source = src,
    })
    if id then
        H.log.info("Orchestrator: submitted ReceiveSuggested job %s",
            tostring(id))
    else
        H.log.warn("Orchestrator: ReceiveSuggested submit failed")
        receipt_backoff = RECEIPT_BACKOFF
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
    if receipt_backoff > 0 then
        receipt_backoff = receipt_backoff - 1
    else
        local ok_r, err_r = pcall(submit_receipt)
        if not ok_r then
            H.log.warn("Orchestrator: receipt pcall err: %s", tostring(err_r))
            receipt_backoff = RECEIPT_BACKOFF
        end
    end
    local jobs = H.scoreboard.list()
    H.log.info("Orchestrator: tick %d, %d job(s)", iterations, #jobs)
    H.sleep(TICK_MS)
end

H.log.info("Orchestrator: shutdown requested, exiting after %d iteration(s)", iterations)
            ]==]

-- ----------------------------------------------------------------------------
-- Forward
-- ----------------------------------------------------------------------------
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
            UPDATE ${SCHEMA}scripts
            SET code = [==[
]] .. sql_new .. [[
            ]==]
            WHERE group_name = '${GROUP_NAME}'
              AND script_name = '${SCRIPT_NAME}';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Orchestrator scans suggestions for one receipt'                    AS name,
        [=[
            # Forward Migration ${MIGRATION}: suggestion receipt scan

            The orchestrator still submits EnsureCanvasUser every 15
            seconds and prunes finished jobs every tick. When a
            suggestion is still Suggested and receipt_mail is null,
            it submits Build.ReceiveSuggested. No row means no submit.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- ----------------------------------------------------------------------------
-- Reverse: restore the 1364 body
-- ----------------------------------------------------------------------------
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
            UPDATE ${SCHEMA}scripts
            SET code = [==[
]] .. sql_old .. [[
            ]==]
            WHERE group_name = '${GROUP_NAME}'
              AND script_name = '${SCRIPT_NAME}';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Restore 1364 orchestrator body'                                    AS name,
        [=[
            # Reverse Migration ${MIGRATION}: restore 1364 orchestrator

            Puts the acuranzo_1364.lua body back. The receipt scan goes
            away. EnsureCanvasUser and prune_terminal stay.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries end
