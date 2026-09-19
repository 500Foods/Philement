-- Migration: acuranzo_1302.lua
-- Phase 53: seed Account.GetSettings (invokable) for Reception settings read
--
-- JWT POST /api/conduit/script. Identity from params._hydrogen only.
-- One payload: accounts + user_registration_meta + user_preferences.
-- No Hydrogen C. Data-only seed (no diagram).

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-08-13 - Seed Account.GetSettings invokable=1 (FINISHLINE FL-53)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1302"
cfg.GROUP_NAME = "Account"
cfg.SCRIPT_NAME = "GetSettings"
-- ----------------------------------------------------------------------------
-- Forward: seed Account.GetSettings (invokable)
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
-- Account.GetSettings (Phase 53)
-- JWT POST /api/conduit/script. Identity from params._hydrogen.
-- Returns one settings payload (accounts + meta + prefs). Never accepts
-- a client account id. Missing prefs row → FL-51 COALESCE defaults.

local function qrows(res)
    if type(res) ~= "table" then return nil end
    if type(res.rows) == "table" then return res.rows end
    return res
end

local function fail(code, message)
    H.set_result_json({ ok = false, code = code, message = message or code })
    return 0
end

local function pick(row, a, b)
    if not row then return nil end
    local v = row[a]
    if v == nil then v = row[b] end
    return v
end

local function as01(v, default)
    if v == true or v == 1 or v == "1" then return 1 end
    if v == false or v == 0 or v == "0" then return 0 end
    return default
end

if type(params) ~= "table" then
    return fail("validation", "Missing params")
end

local h = params._hydrogen
if type(h) ~= "table" then
    return fail("missing_identity", "Sign in required")
end

local account_id = tonumber(h.user_id) or tonumber(h.sub)
if not account_id then
    return fail("missing_identity", "Sign in required")
end

local _qr, qerr = H.query_sync([[
    SELECT
        a.account_id,
        a.name,
        a.first_name,
        a.last_name,
        urm.currency,
        urm.preferred_language,
        urm.referral_source,
        urm.learner_type,
        urm.country,
        urm.age_band,
        up.notify_new_course,
        up.notify_expiring,
        up.notify_expired,
        up.notify_weekly_summary
    FROM ${SCHEMA}accounts a
    LEFT JOIN ${SCHEMA}user_registration_meta urm
      ON urm.account_id = a.account_id
    LEFT JOIN ${SCHEMA}user_preferences up
      ON up.account_id = a.account_id
    WHERE a.account_id = :ACCOUNTID
]], { ACCOUNTID = account_id })
if qerr then
    H.log.warn("GetSettings: lookup err: %s", tostring(qerr))
    return fail("lookup_error", "Could not load settings")
end

local rows = qrows(_qr)
if not rows or not rows[1] then
    return fail("not_found", "Account not found")
end

local row = rows[1]
H.set_result_json({
    ok = true,
    account_id = tonumber(pick(row, "account_id", "ACCOUNT_ID")),
    name = pick(row, "name", "NAME"),
    first_name = pick(row, "first_name", "FIRST_NAME"),
    last_name = pick(row, "last_name", "LAST_NAME"),
    currency = pick(row, "currency", "CURRENCY") or "CAD",
    preferred_language = pick(row, "preferred_language", "PREFERRED_LANGUAGE") or "en",
    referral_source = pick(row, "referral_source", "REFERRAL_SOURCE"),
    learner_type = pick(row, "learner_type", "LEARNER_TYPE"),
    country = pick(row, "country", "COUNTRY"),
    age_band = pick(row, "age_band", "AGE_BAND"),
    notify_new_course = as01(pick(row, "notify_new_course", "NOTIFY_NEW_COURSE"), 0),
    notify_expiring = as01(pick(row, "notify_expiring", "NOTIFY_EXPIRING"), 1),
    notify_expired = as01(pick(row, "notify_expired", "NOTIFY_EXPIRED"), 1),
    notify_weekly_summary = as01(pick(row, "notify_weekly_summary", "NOTIFY_WEEKLY_SUMMARY"), 1),
})
return 0
                ]==],
                'Phase 53: invokable settings read (own row only)',
                1,
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed Account.GetSettings invokable script'                         AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed Account.GetSettings

            Inserts `Account.GetSettings` with `invokable = 1` for Reception
            `POST /api/conduit/script` (FINISHLINE Phase 53).

            The worker reads `params._hydrogen` (`user_id` / `sub`) and
            returns one combined settings payload. Client-supplied account
            ids are ignored (never bound). Missing `user_preferences` uses
            FL-51 defaults via COALESCE in Lua. No Keycloak write-back.
            No diagram (data seed only).
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
            WHERE group_name = 'Account'
              AND script_name = 'GetSettings';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Account.GetSettings script'                                 AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Account.GetSettings
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
