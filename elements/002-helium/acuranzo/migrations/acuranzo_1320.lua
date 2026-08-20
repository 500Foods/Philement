-- Migration: acuranzo_1320.lua
-- PRIORITIZE 2.11: Account.UpdatePrefs empty-optional nil-bind fix
--
-- Hydrogen drops nil named binds (same class as Stripe.Checkout, 1317).
-- Migration 1303 ran empty_to_nil() on country / referral_source /
-- learner_type / age_band then passed those nils into UPDATE +
-- INSERT-if-missing, so a default Settings Save (all four empty)
-- hit write_error.
-- Bind '' and store NULL via NULLIF(:COL, ''). Do not restamp 1303.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-08-17 - Replace Account.UpdatePrefs body (nil optional binds)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1320"
cfg.GROUP_NAME = "Account"
cfg.SCRIPT_NAME = "UpdatePrefs"
-- ----------------------------------------------------------------------------
-- Forward: replace Account.UpdatePrefs body
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
-- Account.UpdatePrefs (Phase 53; 1320 empty-optional bind fix)
-- JWT POST /api/conduit/script. Identity from params._hydrogen.
-- Partial patch of known settings keys only. Own row. No Keycloak write-back.
-- Hydrogen drops nil named binds — optional columns bind '' and store
-- NULL via NULLIF(:COL, ''). Never pass a missing named bind.

local ALLOWED = {
    currency = true,
    preferred_language = true,
    referral_source = true,
    learner_type = true,
    country = true,
    age_band = true,
    notify_new_course = true,
    notify_expiring = true,
    notify_expired = true,
    notify_weekly_summary = true,
}

local CURRENCIES = { CAD = true, USD = true, EUR = true, GBP = true }
local LANGUAGES = { en = true }
local REFERRALS = {
    ["search engine"] = true,
    ["friend or colleague"] = true,
    ["social media"] = true,
    ["teacher or school"] = true,
    other = true,
}
local LEARNERS = {
    student = true,
    teacher = true,
    ["content creator"] = true,
    ["life-long learner"] = true,
}
local AGE_BANDS = {
    under_18 = true,
    ["18_24"] = true,
    ["25_34"] = true,
    ["35_49"] = true,
    ["50_plus"] = true,
}

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

local function empty_to_nil(v)
    if v == nil then return nil end
    if type(v) == "string" and v == "" then return nil end
    return v
end

-- Named binds must always be present. Empty string → SQL NULL via NULLIF.
local function bind_opt(v)
    if v == nil then return "" end
    return v
end

local function load_settings(account_id)
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
    if qerr then return nil, qerr end
    local rows = qrows(_qr)
    if not rows or not rows[1] then return nil, "not_found" end
    local row = rows[1]
    return {
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
    }, nil
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

if params.account_id ~= nil then
    return fail("unknown_key", "account_id is not writable")
end

for k, _ in pairs(params) do
    if k ~= "_hydrogen" and not ALLOWED[k] then
        return fail("unknown_key", "Unknown field: " .. tostring(k))
    end
end

local cur, lerr = load_settings(account_id)
if not cur then
    if lerr == "not_found" then
        return fail("not_found", "Account not found")
    end
    H.log.warn("UpdatePrefs: lookup err: %s", tostring(lerr))
    return fail("lookup_error", "Could not load settings")
end

if params.currency ~= nil then
    local c = tostring(params.currency):upper()
    if not CURRENCIES[c] then
        return fail("validation", "Invalid currency")
    end
    cur.currency = c
end

if params.preferred_language ~= nil then
    local lang = tostring(params.preferred_language)
    if not LANGUAGES[lang] then
        return fail("validation", "Invalid preferred_language")
    end
    cur.preferred_language = lang
end

if params.referral_source ~= nil then
    local v = empty_to_nil(params.referral_source)
    if v ~= nil and not REFERRALS[v] then
        return fail("validation", "Invalid referral_source")
    end
    cur.referral_source = v
end

if params.learner_type ~= nil then
    local v = empty_to_nil(params.learner_type)
    if v ~= nil and not LEARNERS[v] then
        return fail("validation", "Invalid learner_type")
    end
    cur.learner_type = v
end

if params.country ~= nil then
    local v = empty_to_nil(params.country)
    if v ~= nil then
        v = tostring(v)
        if #v > 100 then
            return fail("validation", "country is too long")
        end
    end
    cur.country = v
end

if params.age_band ~= nil then
    local v = empty_to_nil(params.age_band)
    if v ~= nil and not AGE_BANDS[v] then
        return fail("validation", "Invalid age_band")
    end
    cur.age_band = v
end

local function patch_bool(key)
    if params[key] == nil then return true end
    local n = as01(params[key], nil)
    if n == nil then
        return false
    end
    cur[key] = n
    return true
end

if not patch_bool("notify_new_course")
    or not patch_bool("notify_expiring")
    or not patch_bool("notify_expired")
    or not patch_bool("notify_weekly_summary") then
    return fail("validation", "Notification flags must be boolean")
end

local meta_binds = {
    ACCOUNTID = account_id,
    CURRENCY = cur.currency,
    PREFERREDLANGUAGE = cur.preferred_language,
    REFERRALSOURCE = bind_opt(cur.referral_source),
    LEARNERTYPE = bind_opt(cur.learner_type),
    COUNTRY = bind_opt(cur.country),
    AGEBAND = bind_opt(cur.age_band),
}

local _, uerr = H.query_sync([[
    UPDATE ${SCHEMA}user_registration_meta
       SET currency = :CURRENCY,
           preferred_language = :PREFERREDLANGUAGE,
           referral_source = NULLIF(:REFERRALSOURCE, ''),
           learner_type = NULLIF(:LEARNERTYPE, ''),
           country = NULLIF(:COUNTRY, ''),
           age_band = NULLIF(:AGEBAND, ''),
           updated_at = NOW()
     WHERE account_id = :ACCOUNTID
]], meta_binds)
if uerr then
    H.log.warn("UpdatePrefs: meta update err: %s", tostring(uerr))
    return fail("write_error", "Could not save settings")
end

local _, ierr = H.query_sync([[
    INSERT INTO ${SCHEMA}user_registration_meta (
        meta_id, account_id, currency, preferred_language,
        referral_source, learner_type, country, age_band,
        valid_after, valid_until, created_id, created_at, updated_id, updated_at
    )
    SELECT
        (SELECT COALESCE(MAX(meta_id), 0) + 1 FROM ${SCHEMA}user_registration_meta),
        a.account_id,
        :CURRENCY, :PREFERREDLANGUAGE,
        NULLIF(:REFERRALSOURCE, ''), NULLIF(:LEARNERTYPE, ''),
        NULLIF(:COUNTRY, ''), NULLIF(:AGEBAND, ''),
        '2025-01-01 00:00:00', '2035-01-01 00:00:00',
        0, NOW(), 0, NOW()
    FROM ${SCHEMA}accounts a
    WHERE a.account_id = :ACCOUNTID
      AND NOT EXISTS (
          SELECT 1 FROM ${SCHEMA}user_registration_meta urm
          WHERE urm.account_id = :ACCOUNTID
      )
]], meta_binds)
if ierr then
    H.log.warn("UpdatePrefs: meta insert err: %s", tostring(ierr))
    return fail("write_error", "Could not save settings")
end

local _, puerr = H.query_sync([[
    UPDATE ${SCHEMA}user_preferences
       SET notify_new_course = :NNC,
           notify_expiring = :NEX,
           notify_expired = :NED,
           notify_weekly_summary = :NWS,
           updated_at = NOW()
     WHERE account_id = :ACCOUNTID
]], {
    ACCOUNTID = account_id,
    NNC = cur.notify_new_course,
    NEX = cur.notify_expiring,
    NED = cur.notify_expired,
    NWS = cur.notify_weekly_summary,
})
if puerr then
    H.log.warn("UpdatePrefs: prefs update err: %s", tostring(puerr))
    return fail("write_error", "Could not save settings")
end

local _, pierr = H.query_sync([[
    INSERT INTO ${SCHEMA}user_preferences (
        pref_id, account_id,
        notify_new_course, notify_expiring, notify_expired, notify_weekly_summary,
        valid_after, valid_until, created_id, created_at, updated_id, updated_at
    )
    SELECT
        (SELECT COALESCE(MAX(pref_id), 0) + 1 FROM ${SCHEMA}user_preferences),
        a.account_id,
        :NNC, :NEX, :NED, :NWS,
        '2025-01-01 00:00:00', '2035-01-01 00:00:00',
        0, NOW(), 0, NOW()
    FROM ${SCHEMA}accounts a
    WHERE a.account_id = :ACCOUNTID
      AND NOT EXISTS (
          SELECT 1 FROM ${SCHEMA}user_preferences up
          WHERE up.account_id = :ACCOUNTID
      )
]], {
    ACCOUNTID = account_id,
    NNC = cur.notify_new_course,
    NEX = cur.notify_expiring,
    NED = cur.notify_expired,
    NWS = cur.notify_weekly_summary,
})
if pierr then
    H.log.warn("UpdatePrefs: prefs insert err: %s", tostring(pierr))
    return fail("write_error", "Could not save settings")
end

local out, rerr = load_settings(account_id)
if not out then
    H.log.warn("UpdatePrefs: reread err: %s", tostring(rerr))
    return fail("lookup_error", "Saved but could not reload settings")
end
out.ok = true
H.set_result_json(out)
return 0
            ]==]
            WHERE group_name = 'Account'
              AND script_name = 'UpdatePrefs';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Fix Account.UpdatePrefs nil optional binds'                        AS name,
        [=[
            # Forward Migration ${MIGRATION}: Fix Account.UpdatePrefs binds

            Hydrogen named binds omit nil values. 1303 converted empty
            optional About-you fields to nil, then bound those nils into
            both the `user_registration_meta` UPDATE and the
            INSERT-if-missing. A default Settings Save (empty country /
            referral / learner / age) therefore hit `write_error`.

            Bind `''` for those four columns and store NULL via
            `NULLIF(:COL, '')`. Do not restamp 1303. No diagram.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

-- ----------------------------------------------------------------------------
-- Reverse: restore 1303 body
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
-- Account.UpdatePrefs (Phase 53)
-- JWT POST /api/conduit/script. Identity from params._hydrogen.
-- Partial patch of known settings keys only. Own row. No Keycloak write-back.

local ALLOWED = {
    currency = true,
    preferred_language = true,
    referral_source = true,
    learner_type = true,
    country = true,
    age_band = true,
    notify_new_course = true,
    notify_expiring = true,
    notify_expired = true,
    notify_weekly_summary = true,
}

local CURRENCIES = { CAD = true, USD = true, EUR = true, GBP = true }
local LANGUAGES = { en = true }
local REFERRALS = {
    ["search engine"] = true,
    ["friend or colleague"] = true,
    ["social media"] = true,
    ["teacher or school"] = true,
    other = true,
}
local LEARNERS = {
    student = true,
    teacher = true,
    ["content creator"] = true,
    ["life-long learner"] = true,
}
local AGE_BANDS = {
    under_18 = true,
    ["18_24"] = true,
    ["25_34"] = true,
    ["35_49"] = true,
    ["50_plus"] = true,
}

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

local function empty_to_nil(v)
    if v == nil then return nil end
    if type(v) == "string" and v == "" then return nil end
    return v
end

local function load_settings(account_id)
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
    if qerr then return nil, qerr end
    local rows = qrows(_qr)
    if not rows or not rows[1] then return nil, "not_found" end
    local row = rows[1]
    return {
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
    }, nil
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

if params.account_id ~= nil then
    return fail("unknown_key", "account_id is not writable")
end

for k, _ in pairs(params) do
    if k ~= "_hydrogen" and not ALLOWED[k] then
        return fail("unknown_key", "Unknown field: " .. tostring(k))
    end
end

local cur, lerr = load_settings(account_id)
if not cur then
    if lerr == "not_found" then
        return fail("not_found", "Account not found")
    end
    H.log.warn("UpdatePrefs: lookup err: %s", tostring(lerr))
    return fail("lookup_error", "Could not load settings")
end

if params.currency ~= nil then
    local c = tostring(params.currency):upper()
    if not CURRENCIES[c] then
        return fail("validation", "Invalid currency")
    end
    cur.currency = c
end

if params.preferred_language ~= nil then
    local lang = tostring(params.preferred_language)
    if not LANGUAGES[lang] then
        return fail("validation", "Invalid preferred_language")
    end
    cur.preferred_language = lang
end

if params.referral_source ~= nil then
    local v = empty_to_nil(params.referral_source)
    if v ~= nil and not REFERRALS[v] then
        return fail("validation", "Invalid referral_source")
    end
    cur.referral_source = v
end

if params.learner_type ~= nil then
    local v = empty_to_nil(params.learner_type)
    if v ~= nil and not LEARNERS[v] then
        return fail("validation", "Invalid learner_type")
    end
    cur.learner_type = v
end

if params.country ~= nil then
    local v = empty_to_nil(params.country)
    if v ~= nil then
        v = tostring(v)
        if #v > 100 then
            return fail("validation", "country is too long")
        end
    end
    cur.country = v
end

if params.age_band ~= nil then
    local v = empty_to_nil(params.age_band)
    if v ~= nil and not AGE_BANDS[v] then
        return fail("validation", "Invalid age_band")
    end
    cur.age_band = v
end

local function patch_bool(key)
    if params[key] == nil then return true end
    local n = as01(params[key], nil)
    if n == nil then
        return false
    end
    cur[key] = n
    return true
end

if not patch_bool("notify_new_course")
    or not patch_bool("notify_expiring")
    or not patch_bool("notify_expired")
    or not patch_bool("notify_weekly_summary") then
    return fail("validation", "Notification flags must be boolean")
end

local _, uerr = H.query_sync([[
    UPDATE ${SCHEMA}user_registration_meta
       SET currency = :CURRENCY,
           preferred_language = :PREFERREDLANGUAGE,
           referral_source = :REFERRALSOURCE,
           learner_type = :LEARNERTYPE,
           country = :COUNTRY,
           age_band = :AGEBAND,
           updated_at = NOW()
     WHERE account_id = :ACCOUNTID
]], {
    ACCOUNTID = account_id,
    CURRENCY = cur.currency,
    PREFERREDLANGUAGE = cur.preferred_language,
    REFERRALSOURCE = cur.referral_source,
    LEARNERTYPE = cur.learner_type,
    COUNTRY = cur.country,
    AGEBAND = cur.age_band,
})
if uerr then
    H.log.warn("UpdatePrefs: meta update err: %s", tostring(uerr))
    return fail("write_error", "Could not save settings")
end

local _, ierr = H.query_sync([[
    INSERT INTO ${SCHEMA}user_registration_meta (
        meta_id, account_id, currency, preferred_language,
        referral_source, learner_type, country, age_band,
        valid_after, valid_until, created_id, created_at, updated_id, updated_at
    )
    SELECT
        (SELECT COALESCE(MAX(meta_id), 0) + 1 FROM ${SCHEMA}user_registration_meta),
        a.account_id,
        :CURRENCY, :PREFERREDLANGUAGE,
        :REFERRALSOURCE, :LEARNERTYPE, :COUNTRY, :AGEBAND,
        '2025-01-01 00:00:00', '2035-01-01 00:00:00',
        0, NOW(), 0, NOW()
    FROM ${SCHEMA}accounts a
    WHERE a.account_id = :ACCOUNTID
      AND NOT EXISTS (
          SELECT 1 FROM ${SCHEMA}user_registration_meta urm
          WHERE urm.account_id = :ACCOUNTID
      )
]], {
    ACCOUNTID = account_id,
    CURRENCY = cur.currency,
    PREFERREDLANGUAGE = cur.preferred_language,
    REFERRALSOURCE = cur.referral_source,
    LEARNERTYPE = cur.learner_type,
    COUNTRY = cur.country,
    AGEBAND = cur.age_band,
})
if ierr then
    H.log.warn("UpdatePrefs: meta insert err: %s", tostring(ierr))
    return fail("write_error", "Could not save settings")
end

local _, puerr = H.query_sync([[
    UPDATE ${SCHEMA}user_preferences
       SET notify_new_course = :NNC,
           notify_expiring = :NEX,
           notify_expired = :NED,
           notify_weekly_summary = :NWS,
           updated_at = NOW()
     WHERE account_id = :ACCOUNTID
]], {
    ACCOUNTID = account_id,
    NNC = cur.notify_new_course,
    NEX = cur.notify_expiring,
    NED = cur.notify_expired,
    NWS = cur.notify_weekly_summary,
})
if puerr then
    H.log.warn("UpdatePrefs: prefs update err: %s", tostring(puerr))
    return fail("write_error", "Could not save settings")
end

local _, pierr = H.query_sync([[
    INSERT INTO ${SCHEMA}user_preferences (
        pref_id, account_id,
        notify_new_course, notify_expiring, notify_expired, notify_weekly_summary,
        valid_after, valid_until, created_id, created_at, updated_id, updated_at
    )
    SELECT
        (SELECT COALESCE(MAX(pref_id), 0) + 1 FROM ${SCHEMA}user_preferences),
        a.account_id,
        :NNC, :NEX, :NED, :NWS,
        '2025-01-01 00:00:00', '2035-01-01 00:00:00',
        0, NOW(), 0, NOW()
    FROM ${SCHEMA}accounts a
    WHERE a.account_id = :ACCOUNTID
      AND NOT EXISTS (
          SELECT 1 FROM ${SCHEMA}user_preferences up
          WHERE up.account_id = :ACCOUNTID
      )
]], {
    ACCOUNTID = account_id,
    NNC = cur.notify_new_course,
    NEX = cur.notify_expiring,
    NED = cur.notify_expired,
    NWS = cur.notify_weekly_summary,
})
if pierr then
    H.log.warn("UpdatePrefs: prefs insert err: %s", tostring(pierr))
    return fail("write_error", "Could not save settings")
end

local out, rerr = load_settings(account_id)
if not out then
    H.log.warn("UpdatePrefs: reread err: %s", tostring(rerr))
    return fail("lookup_error", "Saved but could not reload settings")
end
out.ok = true
H.set_result_json(out)
return 0
            ]==]
            WHERE group_name = 'Account'
              AND script_name = 'UpdatePrefs';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Restore Account.UpdatePrefs 1303 body'                             AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Restore Account.UpdatePrefs 1303 body
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
