-- Migration: argent_2035.lua
-- Argent tags and attachments
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-07 - MCP tag links and attachment revisions
-- 1.0.1 - 2026-10-07 - Cast optional NULL, dates, and empty strings
-- 1.0.2 - 2026-10-07 - MySQL CAST targets; json parameters are cast before ingest

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "2035"
cfg.GROUP_NAME = "Argent"
if engine == "mysql" then
    cfg.CAST_INTEGER = "signed"
    cfg.CAST_TEXT = "char(255)"
else
    cfg.CAST_INTEGER = cfg.INTEGER
    cfg.CAST_TEXT = cfg.TEXT
end
-- ----------------------------------------------------------------------------
-- Forward
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
                mcp_access,
                mcp_schema,
                mcp_annotations,
                ${COMMON_FIELDS}
            )
            VALUES (
                '${GROUP_NAME}',
                'AddTags',
                1,
                NULL, NULL, NULL, NULL,
                1,
                [==[
-- Argent.AddTags
-- Each entry is tag_id, or name plus an optional organization_id.
-- organization_id omitted means a global tag. An existing link is returned.

local function qrows(res)
    if type(res) ~= "table" then return {} end
    if type(res.rows) == "table" then return res.rows end
    return res
end

local function pick(row, name)
    if type(row) ~= "table" then return nil end
    if row[name] ~= nil then return row[name] end
    local up = string.upper(name)
    if row[up] ~= nil then return row[up] end
    return nil
end

local function fail(code, message, extra)
    local body = extra or {}
    body.ok = false
    body.code = code
    body.message = message or code
    H.set_result_json(body)
    return 0
end

local function ok(body)
    body.ok = true
    H.set_result_json(body)
    return 0
end

local function num(v)
    if type(v) == "number" then return v end
    if type(v) == "string" and v ~= "" then return tonumber(v) end
    return nil
end

local function int(v)
    local n = num(v)
    if not n or n ~= n or n % 1 ~= 0 then return nil end
    return n
end

local function nonempty(v)
    if type(v) ~= "string" or v == "" then return nil end
    return v
end

local function actor_id()
    local h = {}
    if type(params) == "table" and type(params._hydrogen) == "table" then
        h = params._hydrogen
    end
    return tonumber(h.user_id) or tonumber(h.sub) or 0
end

if type(params) ~= "table" then
    return fail("params_required", "params must be an object")
end

local entity_type = int(params.entity_type)
local entity_id = int(params.entity_id)
if not entity_type or entity_type < 1 or entity_type > 9 then
    return fail("entity_type", "entity_type must be lookup 2009 key 1 through 9")
end
if not entity_id then return fail("entity_id_required", "entity_id is required") end
if type(params.tags) ~= "table" or #params.tags == 0 then
    return fail("tags_required", "tags must be a non-empty array")
end

local actor = actor_id()
local links = {}

local function link_tag(tag_id)
    local res, err = H.query_sync([[
        SELECT tag_link_id
        FROM ${SCHEMA}tag_links
        WHERE tag_id = :TAG_ID
          AND entity_type_a2009 = :ENTITY_TYPE
          AND entity_id = :ENTITY_ID
    ]], { TAG_ID = tag_id, ENTITY_TYPE = entity_type, ENTITY_ID = entity_id })
    if err then return nil, nil, err end
    local rows = qrows(res)
    if rows[1] then
        return tonumber(pick(rows[1], "tag_link_id")), false, nil
    end
    local nres, nerr = H.query_sync([[
        SELECT COALESCE(MAX(tag_link_id), 0) + 1 AS next_id
        FROM ${SCHEMA}tag_links
    ]], {})
    if nerr then return nil, nil, nerr end
    local link_id = tonumber(pick(qrows(nres)[1], "next_id")) or 1
    local _, ierr = H.query_sync([[
        INSERT INTO ${SCHEMA}tag_links (
            tag_link_id, tag_id, entity_type_a2009, entity_id,
            valid_after, valid_until, created_id, created_at, updated_id, updated_at
        ) VALUES (
            :LINK_ID, :TAG_ID, :ENTITY_TYPE, :ENTITY_ID,
            NULL, NULL, :ACTOR_CREATED, ${NOW}, :ACTOR_UPDATED, ${NOW}
        )
    ]], {
        LINK_ID = link_id, TAG_ID = tag_id,
        ENTITY_TYPE = entity_type, ENTITY_ID = entity_id,
        ACTOR_CREATED = actor, ACTOR_UPDATED = actor,
    })
    if ierr then return nil, nil, ierr end
    return link_id, true, nil
end

for i = 1, #params.tags do
    local spec = params.tags[i]
    if type(spec) ~= "table" then
        return fail("tags", "each tag must be an object", { links = links })
    end
    local tag_id = int(spec.tag_id)
    if tag_id then
        local res, err = H.query_sync([[
            SELECT tag_id FROM ${SCHEMA}tags WHERE tag_id = :TAG_ID
        ]], { TAG_ID = tag_id })
        if err then return fail("partial_write", tostring(err), { links = links }) end
        if not qrows(res)[1] then
            return fail("not_found", "tag not found", { links = links, tag_id = tag_id })
        end
    else
        local name = nonempty(spec.name)
        if not name then
            return fail("name_required", "tag name or tag_id is required", { links = links })
        end
        local org = int(spec.organization_id)
        local match_global = org and 0 or 1
        local org_bind = org or -1
        local res, err = H.query_sync([[
            SELECT tag_id
            FROM ${SCHEMA}tags
            WHERE name = :TAG_NAME
              AND (
                    (CAST(:MATCH_GLOBAL AS ${CAST_INTEGER}) = 1 AND organization_id IS NULL)
                    OR organization_id = CAST(:ORG_ID AS ${CAST_INTEGER})
                  )
            ORDER BY tag_id
        ]], { TAG_NAME = name, MATCH_GLOBAL = match_global, ORG_ID = org_bind })
        if err then return fail("partial_write", tostring(err), { links = links }) end
        local found = qrows(res)[1]
        if found then
            tag_id = tonumber(pick(found, "tag_id"))
        else
            local nres, nerr = H.query_sync([[
                SELECT COALESCE(MAX(tag_id), 0) + 1 AS next_id FROM ${SCHEMA}tags
            ]], {})
            if nerr then return fail("partial_write", tostring(nerr), { links = links }) end
            tag_id = tonumber(pick(qrows(nres)[1], "next_id")) or 1
            local _, ierr = H.query_sync([[
                INSERT INTO ${SCHEMA}tags (
                    tag_id, organization_id, name, summary, collection,
                    valid_after, valid_until, created_id, created_at, updated_id, updated_at
                ) VALUES (
                    :TAG_ID,
                    CASE WHEN CAST(:USE_ORG AS ${CAST_INTEGER}) = 0 THEN CAST(NULL AS ${CAST_INTEGER}) ELSE CAST(:ORG_ID AS ${CAST_INTEGER}) END,
                    :TAG_NAME, NULL, ${JIS}CAST(:TAG_COLLECTION AS ${CAST_TEXT})${JIE},
                    NULL, NULL, :ACTOR_CREATED, ${NOW}, :ACTOR_UPDATED, ${NOW}
                )
            ]], {
                TAG_ID = tag_id, USE_ORG = org and 1 or 0, ORG_ID = org or 0,
                TAG_NAME = name, TAG_COLLECTION = "{}",
                ACTOR_CREATED = actor, ACTOR_UPDATED = actor,
            })
            if ierr then return fail("partial_write", tostring(ierr), { links = links }) end
        end
    end
    local link_id, created, lerr = link_tag(tag_id)
    if lerr then return fail("partial_write", tostring(lerr), { links = links }) end
    links[#links + 1] = {
        tag_id = tag_id, tag_link_id = link_id, created = created,
    }
end

return ok({ links = links })
                ]==],
                'Link Argent tags to an entity',
                0,
                1,
                '{"inputSchema":{"type":"object","properties":{"entity_type":{"type":"integer"},"entity_id":{"type":"integer"},"tags":{"type":"array"}},"required":["entity_type","entity_id","tags"],"additionalProperties":true}}',
                '{"title":"Add tags","idempotentHint":true}',
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

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
                mcp_access,
                mcp_schema,
                mcp_annotations,
                ${COMMON_FIELDS}
            )
            VALUES (
                '${GROUP_NAME}',
                'RemoveTags',
                1,
                NULL, NULL, NULL, NULL,
                1,
                [==[
-- Argent.RemoveTags
-- Deletes tag links. The tag row stays.

local function qrows(res)
    if type(res) ~= "table" then return {} end
    if type(res.rows) == "table" then return res.rows end
    return res
end

local function fail(code, message)
    H.set_result_json({ ok = false, code = code, message = message or code })
    return 0
end

local function ok(body)
    body.ok = true
    H.set_result_json(body)
    return 0
end

local function num(v)
    if type(v) == "number" then return v end
    if type(v) == "string" and v ~= "" then return tonumber(v) end
    return nil
end

local function int(v)
    local n = num(v)
    if not n or n ~= n or n % 1 ~= 0 then return nil end
    return n
end

if type(params) ~= "table" then
    return fail("params_required", "params must be an object")
end

local entity_type = int(params.entity_type)
local entity_id = int(params.entity_id)
if not entity_type or entity_type < 1 or entity_type > 9 then
    return fail("entity_type", "entity_type must be lookup 2009 key 1 through 9")
end
if not entity_id then return fail("entity_id_required", "entity_id is required") end

local ids = {}
if int(params.tag_id) then ids[#ids + 1] = int(params.tag_id) end
if type(params.tag_ids) == "table" then
    for i = 1, #params.tag_ids do
        local n = int(params.tag_ids[i])
        if not n then return fail("tag_id", "tag_ids must be integers") end
        ids[#ids + 1] = n
    end
end
if #ids == 0 then return fail("tag_id_required", "tag_id or tag_ids is required") end

local removed = {}
for i = 1, #ids do
    local before, berr = H.query_sync([[
        SELECT tag_link_id
        FROM ${SCHEMA}tag_links
        WHERE tag_id = :TAG_ID
          AND entity_type_a2009 = :ENTITY_TYPE
          AND entity_id = :ENTITY_ID
    ]], { TAG_ID = ids[i], ENTITY_TYPE = entity_type, ENTITY_ID = entity_id })
    if berr then return fail("query_failed", tostring(berr)) end
    local existed = qrows(before)[1] ~= nil
    if existed then
        local _, derr = H.query_sync([[
            DELETE FROM ${SCHEMA}tag_links
            WHERE tag_id = :TAG_ID
              AND entity_type_a2009 = :ENTITY_TYPE
              AND entity_id = :ENTITY_ID
        ]], { TAG_ID = ids[i], ENTITY_TYPE = entity_type, ENTITY_ID = entity_id })
        if derr then return fail("delete_failed", tostring(derr)) end
    end
    removed[#removed + 1] = { tag_id = ids[i], removed = existed }
end

return ok({ removed = removed })
                ]==],
                'Remove Argent tag links',
                0,
                1,
                '{"inputSchema":{"type":"object","properties":{"entity_type":{"type":"integer"},"entity_id":{"type":"integer"},"tag_id":{"type":"integer"},"tag_ids":{"type":"array","items":{"type":"integer"}}},"required":["entity_type","entity_id"],"additionalProperties":true}}',
                '{"title":"Remove tags","idempotentHint":true}',
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

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
                mcp_access,
                mcp_schema,
                mcp_annotations,
                ${COMMON_FIELDS}
            )
            VALUES (
                '${GROUP_NAME}',
                'AddAttachment',
                1,
                NULL, NULL, NULL, NULL,
                1,
                [==[
-- Argent.AddAttachment
-- A new attachment_id starts at rev_id 1. A supplied attachment_id
-- adds the next revision. A note defaults to att_type 1 and text/plain.
-- file_data without att_type defaults to 5. collection is always empty.

local function qrows(res)
    if type(res) ~= "table" then return {} end
    if type(res.rows) == "table" then return res.rows end
    return res
end

local function pick(row, name)
    if type(row) ~= "table" then return nil end
    if row[name] ~= nil then return row[name] end
    local up = string.upper(name)
    if row[up] ~= nil then return row[up] end
    return nil
end

local function fail(code, message)
    H.set_result_json({ ok = false, code = code, message = message or code })
    return 0
end

local function ok(body)
    body.ok = true
    H.set_result_json(body)
    return 0
end

local function num(v)
    if type(v) == "number" then return v end
    if type(v) == "string" and v ~= "" then return tonumber(v) end
    return nil
end

local function int(v)
    local n = num(v)
    if not n or n ~= n or n % 1 ~= 0 then return nil end
    return n
end

local function nonempty(v)
    if type(v) ~= "string" or v == "" then return nil end
    return v
end

local function actor_id()
    local h = {}
    if type(params) == "table" and type(params._hydrogen) == "table" then
        h = params._hydrogen
    end
    return tonumber(h.user_id) or tonumber(h.sub) or 0
end

if type(params) ~= "table" then
    return fail("params_required", "params must be an object")
end

local actor = actor_id()
local entity_type = int(params.entity_type)
local entity_id = int(params.entity_id)
local name = nonempty(params.name)
if not entity_type or entity_type < 1 or entity_type > 9 then
    return fail("entity_type", "entity_type must be lookup 2009 key 1 through 9")
end
if not entity_id then return fail("entity_id_required", "entity_id is required") end
if not name then return fail("name_required", "name is required") end

local file_data = params.file_data
local file_text = params.file_text
if file_data ~= nil and type(file_data) ~= "string" then
    return fail("file_data", "file_data must be a string")
end
if file_text ~= nil and type(file_text) ~= "string" then
    return fail("file_text", "file_text must be a string")
end
file_data = file_data or ""
file_text = file_text or ""

local att_type = int(params.att_type)
if params.att_type ~= nil and not att_type then
    return fail("att_type", "att_type must be an integer")
end
if not att_type then
    if file_data == "" and file_text ~= "" then
        att_type = 1
    else
        att_type = 5
    end
end
if att_type < 1 or att_type > 5 then
    return fail("att_type", "att_type must be lookup 2010 key 1 through 5")
end

local mime = nonempty(params.mime_type) or ""
if mime == "" and att_type == 1 then mime = "text/plain" end
local file_name = nonempty(params.file_name) or ""
local summary = nonempty(params.summary) or ""
local byte_len = int(params.byte_len)
if params.byte_len ~= nil and byte_len == nil then
    return fail("byte_len", "byte_len must be an integer")
end
if byte_len == nil then
    if file_data ~= "" then
        byte_len = #file_data
    else
        byte_len = #file_text
    end
end
if byte_len < 0 then return fail("byte_len", "byte_len cannot be negative") end

local txn_id = int(params.txn_id)
if params.txn_id == nil and entity_type == 3 then txn_id = entity_id end
local use_txn = txn_id and 1 or 0

local attachment_id = int(params.attachment_id)
local rev_id
if attachment_id then
    local res, err = H.query_sync([[
        SELECT COALESCE(MAX(rev_id), 0) + 1 AS next_rev
        FROM ${SCHEMA}attachments
        WHERE attachment_id = :ATTACHMENT_ID
    ]], { ATTACHMENT_ID = attachment_id })
    if err then return fail("query_failed", tostring(err)) end
    rev_id = tonumber(pick(qrows(res)[1], "next_rev")) or 1
else
    local res, err = H.query_sync([[
        SELECT COALESCE(MAX(attachment_id), 0) + 1 AS next_id
        FROM ${SCHEMA}attachments
    ]], {})
    if err then return fail("query_failed", tostring(err)) end
    attachment_id = tonumber(pick(qrows(res)[1], "next_id")) or 1
    rev_id = 1
end

local _, ierr = H.query_sync([[
    INSERT INTO ${SCHEMA}attachments (
        attachment_id, rev_id, entity_type_a2009, entity_id, txn_id,
        att_type_a2010, mime_type, file_name, file_data, file_text,
        byte_len, name, summary, collection,
        valid_after, valid_until, created_id, created_at, updated_id, updated_at
    ) VALUES (
        :ATTACHMENT_ID, :REV_ID, :ENTITY_TYPE, :ENTITY_ID,
        CASE WHEN CAST(:USE_TXN AS ${CAST_INTEGER}) = 0 THEN CAST(NULL AS ${CAST_INTEGER}) ELSE CAST(:TXN_ID AS ${CAST_INTEGER}) END,
        :ATT_TYPE, NULLIF(CAST(:MIME_TYPE AS ${CAST_TEXT}), ''), NULLIF(CAST(:FILE_NAME AS ${CAST_TEXT}), ''),
        NULLIF(CAST(:FILE_DATA AS ${CAST_TEXT}), ''), NULLIF(CAST(:FILE_TEXT AS ${CAST_TEXT}), ''),
        :BYTE_LEN, :ATT_NAME, NULLIF(CAST(:ATT_SUMMARY AS ${CAST_TEXT}), ''),
        ${JIS}CAST(:ATT_COLLECTION AS ${CAST_TEXT})${JIE},
        NULL, NULL, :ACTOR_CREATED, ${NOW}, :ACTOR_UPDATED, ${NOW}
    )
]], {
    ATTACHMENT_ID = attachment_id, REV_ID = rev_id,
    ENTITY_TYPE = entity_type, ENTITY_ID = entity_id,
    USE_TXN = use_txn, TXN_ID = txn_id or 0,
    ATT_TYPE = att_type, MIME_TYPE = mime, FILE_NAME = file_name,
    FILE_DATA = file_data, FILE_TEXT = file_text, BYTE_LEN = byte_len,
    ATT_NAME = name, ATT_SUMMARY = summary, ATT_COLLECTION = "{}",
    ACTOR_CREATED = actor, ACTOR_UPDATED = actor,
})
if ierr then return fail("insert_failed", tostring(ierr)) end
return ok({ attachment_id = attachment_id, rev_id = rev_id, created = true })
                ]==],
                'Add an Argent note or file attachment',
                0,
                1,
                '{"inputSchema":{"type":"object","properties":{"attachment_id":{"type":"integer"},"entity_type":{"type":"integer"},"entity_id":{"type":"integer"},"txn_id":{"type":"integer"},"att_type":{"type":"integer"},"name":{"type":"string"},"mime_type":{"type":"string"},"file_name":{"type":"string"},"file_data":{"type":"string"},"file_text":{"type":"string"},"byte_len":{"type":"integer"},"summary":{"type":"string"}},"required":["entity_type","entity_id","name"],"additionalProperties":true}}',
                '{"title":"Add attachment"}',
                ${COMMON_VALUES}
            );
            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed Argent tag and attachment tools'                                                    AS name,
        [=[
            # Forward Migration ${MIGRATION}: Argent tags and attachments

            Inserts the Argent script rows named in the reverse migration.
            Group `Argent`. `mcp_access=1`, `invokable=0`.
            Does not install a QueryRef. Does not create a table.
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
            WHERE group_name = 'Argent'
              AND script_name IN ('AddTags', 'RemoveTags', 'AddAttachment');

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Argent tag and attachment tools'                                                    AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Argent tags and attachments

            Deletes those script rows. Does not drop Argent tables.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

-- ----------------------------------------------------------------------------
-- Diagram
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
        ${TYPE_DIAGRAM_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        'JSON script definition in collection'                              AS code,
        'Diagram Argent tags and attachments'                                                   AS name,
        [=[
            # Diagram Migration ${MIGRATION}

            Script rows. This does not define a table.
        ]=]
                                                                            AS summary,
                                                                            -- DIAGRAM_START
        ${JSON_INGEST_START}
        [=[
            {
                "diagram": [
                    {
                        "object_type": "script",
                        "object_id": "script.Argent.AddTags",
                        "object_ref": "${MIGRATION}",
                        "name": "Argent.AddTags"
                    },
                    {
                        "object_type": "script",
                        "object_id": "script.Argent.RemoveTags",
                        "object_ref": "${MIGRATION}",
                        "name": "Argent.RemoveTags"
                    },
                    {
                        "object_type": "script",
                        "object_id": "script.Argent.AddAttachment",
                        "object_ref": "${MIGRATION}",
                        "name": "Argent.AddAttachment"
                    }
                ]
            }
        ]=]
        ${JSON_INGEST_END}
                                                                            -- DIAGRAM_END
                                                                            AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
