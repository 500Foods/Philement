-- Migration: courses_4123.lua
-- CATCHUP Phase 21: seed Build.Gate (not invokable)
--
-- Inserts Build.Gate with invokable 0. Module for the Phase 21 human-gate scripts. No diagram.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-27 - Seed Build.Gate (not invokable)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "4123"
cfg.GROUP_NAME = "Build"
cfg.SCRIPT_NAME = "Gate"
if engine == "mysql" or engine == "mariadb" then
    cfg.CAST_INTEGER = "signed"
    cfg.CAST_STATE = "char(50)"
else
    cfg.CAST_INTEGER = cfg.INTEGER
    cfg.CAST_STATE = cfg.VARCHAR_50
end
if engine == "db2" then
    cfg.FROM_ONE = "FROM SYSIBM.SYSDUMMY1"
elseif engine == "firebird" then
    cfg.FROM_ONE = "FROM RDB$DATABASE"
else
    cfg.FROM_ONE = ""
end
-- ----------------------------------------------------------------------------
-- Forward: seed Build.Gate (not invokable)
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
                invokable,
                ${COMMON_FIELDS}
            )
            VALUES (
                '${GROUP_NAME}',
                '${SCRIPT_NAME}',
                1,
                NULL, NULL, NULL, NULL,
                1,
                [==[
-- Build.Gate (CATCHUP Phase 21 / COURSEBUILDER CB-3 rule 2)
-- Module. require("Build.Gate").transition(params, spec)
-- Not JWT-invokable. Build.Accept, Build.Decline, Build.Approve, and
-- Build.Abandon call it. spec = { from = "...", to = "...", script = "..." }.
-- Staff gate first (Enroll.StaffGate). The state check and the update
-- are one conditional UPDATE, so a second click or a second operator
-- gets wrong_state instead of a second transition (per-run guard).
-- Writes one course_build_actions row named after the new state.
-- Failures: H.set_result_json({ ok=false, code=..., message=... }).
-- No mail here. CB-4 templates are named but not written yet.

local M = {}

local function qrows(res)
    if type(res) ~= "table" then return nil end
    if type(res.rows) == "table" then return res.rows end
    return res
end

local function pick(row, a, b)
    if not row then return nil end
    local v = row[a]
    if v == nil then v = row[b] end
    return v
end

local function fail(code, message, extra)
    local body = { ok = false, code = code, message = message or code }
    if type(extra) == "table" then
        for k, v in pairs(extra) do body[k] = v end
    end
    H.set_result_json(body)
    return 0
end
M.fail = fail

local function positive(value)
    local n = tonumber(value)
    if not n or n < 1 or n ~= math.floor(n) then return nil end
    return n
end

local function actor_of(params)
    local hydrogen = type(params._hydrogen) == "table" and params._hydrogen or {}
    local n = tonumber(hydrogen.user_id) or tonumber(params.user_id)
    if not n or n < 0 or n ~= math.floor(n) then return 0 end
    return n
end

local function staff_gate(params)
    local loaded, gate = pcall(require, "Enroll.StaffGate")
    if not loaded or type(gate) ~= "table" or type(gate.allow) ~= "function" then
        return fail("not_staff", "Staff check is unavailable")
    end
    local allowed, why = gate.allow(params)
    if allowed then return nil end
    if why == "lookup_error" then
        return fail("lookup_error", "Could not resolve staff access")
    end
    if why == "missing_identity" then
        return fail("missing_identity", "Sign in required")
    end
    return fail("not_staff", "Staff access is required")
end

local function current_state(run_id)
    local res, err = H.query_sync([[
        SELECT state
        FROM ${SCHEMA}course_build_runs
        WHERE run_id = CAST(:RUN_ID AS ${CAST_INTEGER})
    ]], { RUN_ID = run_id })
    if err then return nil, "lookup_error" end
    local rows = qrows(res)
    if not rows or not rows[1] then return nil, "not_found" end
    return tostring(pick(rows[1], "state", "STATE") or ""), nil
end

function M.transition(params, spec)
    local script = (spec and spec.script) or "Build.Gate"
    if type(params) ~= "table" then
        return fail("validation", "Missing params")
    end
    local denied = staff_gate(params)
    if denied then return denied end

    local run_id = positive(params.run_id)
    if not run_id then
        return fail("validation", "run_id is required")
    end
    local actor = actor_of(params)

    local res, err = H.query_sync([[
        UPDATE ${SCHEMA}course_build_runs
           SET state = CAST(:TO_STATE AS ${CAST_STATE}),
               updated_id = CAST(:USERID AS ${CAST_INTEGER}),
               updated_at = NOW()
         WHERE run_id = CAST(:RUN_ID AS ${CAST_INTEGER})
           AND state = CAST(:FROM_STATE AS ${CAST_STATE})
    ]], { TO_STATE = spec.to, USERID = actor, RUN_ID = run_id, FROM_STATE = spec.from })
    if err or type(res) ~= "table" then
        H.log.warn("%s: run %d update failed: %s", script, run_id, tostring(err))
        return fail("write_error", "Could not update the run")
    end
    if res.affected_rows ~= 1 then
        local state, why = current_state(run_id)
        if why == "not_found" then
            return fail("not_found", "Run not found", { run_id = run_id })
        end
        if why then
            return fail("lookup_error", "Could not load the run", { run_id = run_id })
        end
        return fail("wrong_state",
            string.format("Run is %s, expected %s", state, spec.from),
            { run_id = run_id, state = state, expected = spec.from })
    end

    local _, ledger_err = H.query_sync([[
        INSERT INTO ${SCHEMA}course_build_actions (
            action_id, run_id, actor_account_id, action_name,
            human_cost_cents, token_cost_cents, elapsed_ms, prompt_count,
            prompt, result_ref, valid_after, valid_until,
            created_id, created_at, updated_id, updated_at
        )
        SELECT
            COALESCE((SELECT MAX(action_id) FROM ${SCHEMA}course_build_actions), 0) + 1,
            src.run_id,
            src.user_id,
            src.action_name,
            NULL, NULL, NULL, NULL, NULL, NULL,
            NULL, NULL,
            src.user_id, NOW(), src.user_id, NOW()
        FROM (
            SELECT CAST(:RUN_ID AS ${CAST_INTEGER}) AS run_id,
                   CAST(:USERID AS ${CAST_INTEGER}) AS user_id,
                   CAST(:ACTION_NAME AS ${CAST_STATE}) AS action_name
            ${FROM_ONE}
        ) src
    ]], { RUN_ID = run_id, USERID = actor, ACTION_NAME = spec.to })
    local ledger_ok = not ledger_err
    if ledger_err then
        H.log.warn("%s: run %d ledger write failed: %s", script, run_id, tostring(ledger_err))
    end
    H.log.info("%s: run %d %s -> %s by %d", script, run_id, spec.from, spec.to, actor)

    H.set_result_json({
        ok = true,
        run_id = run_id,
        from = spec.from,
        state = spec.to,
        ledger_ok = ledger_ok,
    })
    return 0
end

return M
                ]==],
                'CATCHUP Phase 21: Course Builder gate transition module',
                0,
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed Build.Gate'                                              AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed Build.Gate

            Inserts Build.Gate with invokable 0. Module for the Phase 21 human-gate scripts. No diagram.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})
-- ----------------------------------------------------------------------------
-- Reverse
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
            DELETE FROM ${SCHEMA}scripts
            WHERE group_name = 'Build'
              AND script_name = 'Gate';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Build.Gate'                                            AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Build.Gate
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
