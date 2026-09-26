-- database.lua
-- Defines the Helium schema, supported database engines, and SQL defaults for migrations used in Hydrogen

-- luacheck: no max line length

-- CHANGELOG
-- 3.4.3 - 2026-09-22 - Firebird: rewrite NOT NULL DEFAULT <v> to DEFAULT <v> NOT NULL
-- 3.4.2 - 2026-09-22 - Firebird: rewrite ALTER TABLE ADD/DROP COLUMN to ADD/DROP (no COLUMN keyword)
-- 3.4.1 - 2026-09-22 - Firebird: also rewrite CTE AS (VALUES ...) bodies to SELECT...UNION ALL FROM RDB$DATABASE
-- 3.4.0 - 2026-09-22 - Firebird: rewrite multi-row INSERT VALUES to INSERT...SELECT...UNION ALL in replace_query
-- 3.3.0 - 2026-09-19 - Removed Firebase dialect (C-level Firebase fully removed in Phase 3)
-- 3.2.0 - 2026-09-18 - Added Firebird dialect (query_dialects = 6, empty schema prefix)
-- 3.1.0 - 2026-09-16 - Added Firebase dialect (query_dialects = 6, underscore schema prefix)
-- 3.0.0 - 2025-11-27 - Added Brotli compression for large base64-encoded strings (>1KB threshold)
-- 2.2.0 - 2025-10-26 - Added more boilerplates for common_insert, common_create, common_diagram
-- 2.1.2 - 2025-10-15 - Added info{} element to track version information
-- 2.1.1 - 2025-10-15 - Changed replace_query to have a loop to check for nested macro replacements
-- 2.1.0 - 2025-10-15 - Changed run_migration to treet migration as a function call to make overrides easier
-- 2.0.0 - 2025-10-15 - Moved engine specifics into their own files (eg: database_db2.lua)
-- 1.0.0 - 2025-09-13 - Initial creation with support for SQLite, PostgreSQL, MySQL, DB2

local database = {

    -- Database.lua versioning information
    info = {
      script = "database.lua",
    version = "3.4.3",
        release = "2026-09-22"
     },

    -- Lookup #27 - Query Status
    query_status = {
        inactive = 0,
        active = 1
    },

    -- Lookup #28 - Query types
    query_types = {
        internal_sql = 0,
        system_sql = 1,
        system_ddl = 2,
        reporting_sql = 3,
        public = 10,
        protected = 11,
        forward_migration = 1000,
        reverse_migration = 1001,
        diagram_migration = 1002,
        applied_migration = 1003
    },

    -- Lookup #58 - Query Queues
    query_queues = {
        slow = 0,
        medium = 1,
        fast = 2,
        cached = 3
    },

    -- Currently supported database engines - mirrors Lookup #30
    engines = {
        postgresql = true,
        sqlite = true,
        mysql = true,
        db2 = true,
        mssql = true,
        firebird = true,
        mariadb = true
    },

    -- Lookup #30 - Query Dialects (key 5 is MS SQL Server; Firebird is 6)
    query_dialects = {
        postgresql = 1,
        sqlite = 2,
        mysql = 3,
        db2 = 4,
        mssql = 5,
        firebird = 6,
        mariadb = 7
    },

    -- Saves repeating it in virtually every single template
    queries_insert =    [[
                            query_id,
                            query_ref,
                            query_status_a27,
                            query_type_a28,
                            query_dialect_a30,
                            query_queue_a58,
                            query_timeout,
                            code,
                            name,
                            summary,
                            collection,
                            valid_after,
                            valid_until,
                            created_id,
                            created_at,
                            updated_id,
                            updated_at
                        ]],

    -- This is the default for nearly every table created
    common_insert = [[
                        NULL            AS valid_after ,
                        NULL            AS valid_until ,
                        0               AS created_id ,
                        ${NOW}          AS created_at ,
                        0               AS updated_id ,
                        ${NOW}          AS updated_at
                    ]],

    common_create = [[
                        valid_after             ${TIMESTAMP_TZ}             ,
                        valid_until             ${TIMESTAMP_TZ}             ,
                        created_id              ${INTEGER}          NOT NULL,
                        created_at              ${TIMESTAMP_TZ}     NOT NULL,
                        updated_id              ${INTEGER}          NOT NULL,
                        updated_at              ${TIMESTAMP_TZ}     NOT NULL,
                    ]],

    common_fields = [[
                        valid_after,
                        valid_until,
                        created_id,
                        created_at,
                        updated_id,
                        updated_at
                    ]],
    common_values = [[
                        NULL,
                        NULL,
                        0,
                        ${NOW},
                        0,
                        ${NOW}
                    ]],


    common_diagram =    [[
                            {
                                "name": "valid_after",
                                "datatype": "${TIMESTAMP_TZ}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false,
                                "standard": true
                            },
                            {
                                "name": "valid_until",
                                "datatype": "${TIMESTAMP_TZ}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false,
                                "standard": true
                            },
                            {
                                "name": "created_id",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "standard": true
                            },
                            {
                                "name": "created_at",
                                "datatype": "${TIMESTAMP_TZ}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "standard": true
                            },
                            {
                                "name": "updated_id",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "standard": true
                            },
                            {
                                "name": "updated_at",
                                "datatype": "${TIMESTAMP_TZ}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "standard": true
                            }
                        ]],

    -- Load engine-specific configurations from separate files
    defaults = {
        sqlite = require("database_sqlite"),
        postgresql = require("database_postgresql"),
        mysql = require("database_mysql"),
        db2 = require("database_db2"),
        firebird = require("database_firebird")
    },

    replace_query = function(self, template, engine, design_name, schema_name)

        if not self.engines[engine] then
            error("Unsupported engine: " .. engine .. " / " .. design_name)
        end

        local cfg = self.defaults[engine]

        -- Generate schema prefix based on design name and database conventions
        -- Perfectly acceptable for schema to be empty (typical for SQLite)
        local schema_prefix = ''
        if schema_name and schema_name ~= '' and schema_name ~= '.' then
          if engine == 'firebird' then
            -- Firebird has no schema support; schema prefix is empty
            schema_prefix = ''
          else
            local prefixed_name = (engine == 'db2') and schema_name:upper() or schema_name
            schema_prefix = prefixed_name .. '.'
          end
        end

        -- Add additional placeholders to cfg for unified processing
        cfg.SUBQUERY_DELIMITER = "-- SUBQUERY DELIMITER"
        cfg.SCHEMA = schema_prefix
        cfg.QUERIES = "queries" -- default table name for queries
        cfg.TIMEOUT = "5000" -- default query timeout value (ms)
        cfg.DIALECT = self.query_dialects[engine]
        cfg.QUERIES_INSERT = self.queries_insert
        cfg.COMMON_INSERT = self.common_insert
        cfg.COMMON_CREATE = self.common_create
        cfg.COMMON_DIAGRAM = self.common_diagram
        cfg.COMMON_FIELDS = self.common_fields
        cfg.COMMON_VALUES = self.common_values

        -- Lookup #27
        cfg.STATUS_INACTIVE = self.query_status.inactive
        cfg.STATUS_ACTIVE = self.query_status.active

        -- Lookup #28
        cfg.TYPE_INTERNAL_SQL = self.query_types.internal_sql
        cfg.TYPE_SQL = self.query_types.system_sql
        cfg.TYPE_PUBLIC = self.query_types.public
        cfg.TYPE_PROTECTED = self.query_types.protected
        cfg.TYPE_FORWARD_MIGRATION = self.query_types.forward_migration
        cfg.TYPE_REVERSE_MIGRATION = self.query_types.reverse_migration
        cfg.TYPE_DIAGRAM_MIGRATION = self.query_types.diagram_migration
        cfg.TYPE_APPLIED_MIGRATION = self.query_types.applied_migration

        -- Lookup #58
        cfg.QTC_SLOW = self.query_queues.slow
        cfg.QTC_MEDIUM = self.query_queues.medium
        cfg.QTC_FAST = self.query_queues.fast
        cfg.QTC_CACHED = self.query_queues.cached

        -- Add environment variables to cfg if not already present
        for key in template:gmatch("${([^}]+)}") do
            if not cfg[key] then
                local env_val = os.getenv(key)
                if env_val then
                    cfg[key] = env_val
                end
            end
        end

        -- Get text block to work with
        local sql = template

        -- Allows for 5 levels of macro nesting before giving up
        local unresolved = 6
        while unresolved > 0 do
            -- Run macro expansion
            for key, value in pairs(cfg) do
                -- Use a function replacement so values containing '%' (e.g. SQL
                -- LIKE patterns or printf-style format specifiers) are not
                -- misinterpreted as gsub capture references.
                sql = sql:gsub("${" .. key .. "}", function() return value end)
            end
            -- Check if we still have macros left to expand
            if sql:match("${([^}]+)}") then
              unresolved = unresolved - 1
            else
              unresolved = 0
            end
        end


        -- Firebird does not support multi-row INSERT ... VALUES (...), (...);
        -- (langref: VALUES inserts exactly one row; CORE1978 still open). Rewrite
        -- those statements to INSERT ... SELECT ... FROM RDB$DATABASE UNION ALL
        -- before [=[...]=] encoding seals them into queries.code.
        local function firebird_rewrite_multi_row_values(src)
            local n = #src

            local function skip_ws_and_comments(i)
                while i <= n do
                    local c = src:sub(i, i)
                    if c == " " or c == "\t" or c == "\n" or c == "\r" then
                        i = i + 1
                    elseif c == "-" and src:sub(i + 1, i + 1) == "-" then
                        i = i + 2
                        while i <= n and src:sub(i, i) ~= "\n" do
                            i = i + 1
                        end
                    elseif c == "/" and src:sub(i + 1, i + 1) == "*" then
                        i = i + 2
                        while i <= n and not (src:sub(i, i) == "*" and src:sub(i + 1, i + 1) == "/") do
                            i = i + 1
                        end
                        if i <= n then
                            i = i + 2
                        end
                    else
                        break
                    end
                end
                return i
            end

            -- Balanced (...) starting at open paren; respects quotes and -- comments.
            local function scan_paren_group(open_i)
                if src:sub(open_i, open_i) ~= "(" then
                    return nil
                end
                local depth = 0
                local i = open_i
                local in_sq, in_dq = false, false
                while i <= n do
                    local c = src:sub(i, i)
                    if in_sq then
                        if c == "'" then
                            if src:sub(i + 1, i + 1) == "'" then
                                i = i + 2
                            else
                                in_sq = false
                                i = i + 1
                            end
                        else
                            i = i + 1
                        end
                    elseif in_dq then
                        if c == '"' then
                            if src:sub(i + 1, i + 1) == '"' then
                                i = i + 2
                            else
                                in_dq = false
                                i = i + 1
                            end
                        else
                            i = i + 1
                        end
                    elseif c == "'" then
                        in_sq = true
                        i = i + 1
                    elseif c == '"' then
                        in_dq = true
                        i = i + 1
                    elseif c == "(" then
                        depth = depth + 1
                        i = i + 1
                    elseif c == ")" then
                        depth = depth - 1
                        i = i + 1
                        if depth == 0 then
                            return i - 1
                        end
                    elseif c == "-" and src:sub(i + 1, i + 1) == "-" then
                        i = i + 2
                        while i <= n and src:sub(i, i) ~= "\n" do
                            i = i + 1
                        end
                    else
                        i = i + 1
                    end
                end
                return nil
            end

            local function match_word(i, word)
                local last = i + #word - 1
                if last > n then
                    return false
                end
                if src:sub(i, last):lower() ~= word:lower() then
                    return false
                end
                local after = src:sub(last + 1, last + 1)
                if after ~= "" and after:match("[%w_]") then
                    return false
                end
                return true
            end

            local function scan_identifier(i)
                i = skip_ws_and_comments(i)
                if i > n then
                    return nil
                end
                if src:sub(i, i) == '"' then
                    i = i + 1
                    while i <= n do
                        if src:sub(i, i) == '"' then
                            if src:sub(i + 1, i + 1) == '"' then
                                i = i + 2
                            else
                                return i + 1
                            end
                        else
                            i = i + 1
                        end
                    end
                    return nil
                end
                if not src:sub(i, i):match("[%a_]") then
                    return nil
                end
                i = i + 1
                while i <= n and src:sub(i, i):match("[%w_]") do
                    i = i + 1
                end
                return i
            end

            -- Try to parse one INSERT...VALUES multi-row statement at start_i.
            -- Returns end index (inclusive of ';') and rewritten text, or nil.
            local function try_rewrite_at(start_i)
                if not match_word(start_i, "insert") then
                    return nil
                end
                local prev = (start_i > 1) and src:sub(start_i - 1, start_i - 1) or ""
                if prev ~= "" and prev:match("[%w_]") then
                    return nil
                end
                local j = skip_ws_and_comments(start_i + 6)
                if not match_word(j, "into") then
                    return nil
                end
                j = skip_ws_and_comments(j + 4)
                local table_start = j
                local after_table = scan_identifier(j)
                if not after_table then
                    return nil
                end
                j = after_table
                local k = skip_ws_and_comments(j)
                if src:sub(k, k) == "." then
                    after_table = scan_identifier(k + 1)
                    if not after_table then
                        return nil
                    end
                    j = after_table
                end
                local table_end = j -- exclusive
                j = skip_ws_and_comments(j)
                local cols_start, cols_end
                if src:sub(j, j) == "(" then
                    cols_start = j
                    cols_end = scan_paren_group(j)
                    if not cols_end then
                        return nil
                    end
                    j = skip_ws_and_comments(cols_end + 1)
                end
                if not match_word(j, "values") then
                    return nil
                end
                j = skip_ws_and_comments(j + 6)
                local rows = {}
                while src:sub(j, j) == "(" do
                    local close_r = scan_paren_group(j)
                    if not close_r then
                        return nil
                    end
                    rows[#rows + 1] = src:sub(j + 1, close_r - 1)
                    j = skip_ws_and_comments(close_r + 1)
                    if src:sub(j, j) == "," then
                        j = skip_ws_and_comments(j + 1)
                    else
                        break
                    end
                end
                if #rows < 2 or src:sub(j, j) ~= ";" then
                    return nil
                end
                local prefix = "INSERT INTO " .. src:sub(table_start, table_end - 1)
                if cols_start then
                    prefix = prefix .. " " .. src:sub(cols_start, cols_end)
                end
                local parts = {}
                for r = 1, #rows do
                    parts[#parts + 1] = "SELECT " .. rows[r] .. " FROM RDB$DATABASE"
                end
                return j, prefix .. "\n" .. table.concat(parts, "\nUNION ALL\n") .. ";"
            end

            -- CTE / derived: WITH name(cols) AS ( VALUES (...),(... ) )
            -- Firebird rejects multi-row VALUES here; rewrite body only (keep AS parens).
            local function try_rewrite_cte_values_at(start_i)
                if not match_word(start_i, "values") then
                    return nil
                end
                local prev = (start_i > 1) and src:sub(start_i - 1, start_i - 1) or ""
                if prev ~= "" and prev:match("[%w_]") then
                    return nil
                end

                -- Lookbehind must be: AS (
                local k = start_i - 1
                while k >= 1 and src:sub(k, k):match("[ \t\n\r]") do
                    k = k - 1
                end
                if k < 1 or src:sub(k, k) ~= "(" then
                    return nil
                end
                k = k - 1
                while k >= 1 and src:sub(k, k):match("[ \t\n\r]") do
                    k = k - 1
                end
                if k < 2 or src:sub(k - 1, k):lower() ~= "as" then
                    return nil
                end
                local before_as = (k - 2 >= 1) and src:sub(k - 2, k - 2) or ""
                if before_as ~= "" and before_as:match("[%w_]") then
                    return nil
                end

                local j = skip_ws_and_comments(start_i + 6)
                local rows = {}
                local last_close = nil
                while src:sub(j, j) == "(" do
                    local close_r = scan_paren_group(j)
                    if not close_r then
                        return nil
                    end
                    rows[#rows + 1] = src:sub(j + 1, close_r - 1)
                    last_close = close_r
                    j = skip_ws_and_comments(close_r + 1)
                    if src:sub(j, j) == "," then
                        j = skip_ws_and_comments(j + 1)
                    else
                        break
                    end
                end
                if #rows < 2 or not last_close then
                    return nil
                end
                j = skip_ws_and_comments(j)
                -- CTE body ends at closing paren of AS (...); do not consume it.
                if src:sub(j, j) ~= ")" then
                    return nil
                end

                local parts = {}
                for r = 1, #rows do
                    parts[#parts + 1] = "SELECT " .. rows[r] .. " FROM RDB$DATABASE"
                end
                return last_close, table.concat(parts, "\nUNION ALL\n")
            end

            local out = {}
            local i = 1
            while i <= n do
                local end_i, rewritten = try_rewrite_at(i)
                if not rewritten then
                    end_i, rewritten = try_rewrite_cte_values_at(i)
                end
                if rewritten then
                    out[#out + 1] = rewritten
                    i = end_i + 1
                else
                    out[#out + 1] = src:sub(i, i)
                    i = i + 1
                end
            end
            return table.concat(out)
        end


        -- Firebird: ALTER TABLE ... ADD|DROP COLUMN ...  →  ADD|DROP (no COLUMN keyword).
        local function firebird_rewrite_alter_column_keyword(src)
            local n = #src
            local out = {}
            local i = 1
            local in_sq, in_dq = false, false

            local function match_word_at(pos, word)
                local last = pos + #word - 1
                if last > n then
                    return false
                end
                if src:sub(pos, last):lower() ~= word:lower() then
                    return false
                end
                local before = (pos > 1) and src:sub(pos - 1, pos - 1) or ""
                if before ~= "" and before:match("[%w_]") then
                    return false
                end
                local after = src:sub(last + 1, last + 1)
                if after ~= "" and after:match("[%w_]") then
                    return false
                end
                return true
            end

            local function skip_ws(pos)
                while pos <= n do
                    local c = src:sub(pos, pos)
                    if c == " " or c == "\t" or c == "\n" or c == "\r" then
                        pos = pos + 1
                    elseif c == "-" and src:sub(pos + 1, pos + 1) == "-" then
                        while pos <= n and src:sub(pos, pos) ~= "\n" do
                            pos = pos + 1
                        end
                    elseif c == "/" and src:sub(pos + 1, pos + 1) == "*" then
                        pos = pos + 2
                        while pos <= n and not (src:sub(pos, pos) == "*" and src:sub(pos + 1, pos + 1) == "/") do
                            pos = pos + 1
                        end
                        if pos <= n then
                            pos = pos + 2
                        end
                    else
                        break
                    end
                end
                return pos
            end

            while i <= n do
                local c = src:sub(i, i)
                if in_sq then
                    out[#out + 1] = c
                    if c == "'" then
                        if src:sub(i + 1, i + 1) == "'" then
                            out[#out + 1] = "'"
                            i = i + 2
                        else
                            in_sq = false
                            i = i + 1
                        end
                    else
                        i = i + 1
                    end
                elseif in_dq then
                    out[#out + 1] = c
                    if c == '"' then
                        if src:sub(i + 1, i + 1) == '"' then
                            out[#out + 1] = '"'
                            i = i + 2
                        else
                            in_dq = false
                            i = i + 1
                        end
                    else
                        i = i + 1
                    end
                elseif c == "'" then
                    in_sq = true
                    out[#out + 1] = c
                    i = i + 1
                elseif c == '"' then
                    in_dq = true
                    out[#out + 1] = c
                    i = i + 1
                elseif c == "-" and src:sub(i + 1, i + 1) == "-" then
                    out[#out + 1] = c
                    i = i + 1
                    out[#out + 1] = src:sub(i, i)
                    i = i + 1
                    while i <= n and src:sub(i, i) ~= "\n" do
                        out[#out + 1] = src:sub(i, i)
                        i = i + 1
                    end
                elseif c == "/" and src:sub(i + 1, i + 1) == "*" then
                    out[#out + 1] = "/*"
                    i = i + 2
                    while i <= n and not (src:sub(i, i) == "*" and src:sub(i + 1, i + 1) == "/") do
                        out[#out + 1] = src:sub(i, i)
                        i = i + 1
                    end
                    if i <= n then
                        out[#out + 1] = "*/"
                        i = i + 2
                    end
                elseif match_word_at(i, "add") or match_word_at(i, "drop") then
                    local kw = match_word_at(i, "add") and "add" or "drop"
                    local kw_end = i + #kw - 1
                    -- emit ADD/DROP as in source (preserve case of first keyword)
                    out[#out + 1] = src:sub(i, kw_end)
                    local j = skip_ws(kw_end + 1)
                    -- copy intervening whitespace/comments already skipped — emit them
                    if j > kw_end + 1 then
                        out[#out + 1] = src:sub(kw_end + 1, j - 1)
                    end
                    if match_word_at(j, "column") then
                        -- Skip COLUMN and its trailing whitespace. Leading ws before
                        -- COLUMN was already copied; if there was none, emit one space.
                        local after_col = skip_ws(j + 6)
                        if j == kw_end + 1 then
                            out[#out + 1] = " "
                        end
                        i = after_col
                    else
                        i = j
                    end
                else
                    out[#out + 1] = c
                    i = i + 1
                end
            end
            return table.concat(out)
        end


        -- Firebird column defs require DEFAULT before NOT NULL.
        -- SQLite/PG accept "NOT NULL DEFAULT 0"; Firebird rejects with token DEFAULT.
        local function firebird_rewrite_not_null_default_order(src)
            local n = #src
            local out = {}
            local i = 1
            local in_sq, in_dq = false, false

            local function match_word_at(pos, word)
                local last = pos + #word - 1
                if last > n then return false end
                if src:sub(pos, last):lower() ~= word:lower() then return false end
                local before = (pos > 1) and src:sub(pos - 1, pos - 1) or ""
                if before ~= "" and before:match("[%w_]") then return false end
                local after = src:sub(last + 1, last + 1)
                if after ~= "" and after:match("[%w_]") then return false end
                return true
            end

            local function skip_ws(pos)
                while pos <= n do
                    local c = src:sub(pos, pos)
                    if c == " " or c == "\t" or c == "\n" or c == "\r" then
                        pos = pos + 1
                    elseif c == "-" and src:sub(pos + 1, pos + 1) == "-" then
                        while pos <= n and src:sub(pos, pos) ~= "\n" do pos = pos + 1 end
                    elseif c == "/" and src:sub(pos + 1, pos + 1) == "*" then
                        pos = pos + 2
                        while pos <= n and not (src:sub(pos, pos) == "*" and src:sub(pos + 1, pos + 1) == "/") do
                            pos = pos + 1
                        end
                        if pos <= n then pos = pos + 2 end
                    else
                        break
                    end
                end
                return pos
            end

            -- Parse a DEFAULT value starting at pos; return exclusive end index.
            local function scan_default_value(pos)
                pos = skip_ws(pos)
                if pos > n then return nil end
                local c = src:sub(pos, pos)
                if c == "'" then
                    pos = pos + 1
                    while pos <= n do
                        if src:sub(pos, pos) == "'" then
                            if src:sub(pos + 1, pos + 1) == "'" then
                                pos = pos + 2
                            else
                                return pos + 1
                            end
                        else
                            pos = pos + 1
                        end
                    end
                    return nil
                end
                if c == "+" or c == "-" then
                    pos = pos + 1
                    c = src:sub(pos, pos)
                end
                if c:match("%d") then
                    while pos <= n and src:sub(pos, pos):match("[%d%.]") do
                        pos = pos + 1
                    end
                    return pos
                end
                if c:match("[%a_]") then
                    while pos <= n and src:sub(pos, pos):match("[%w_]") do
                        pos = pos + 1
                    end
                    return pos
                end
                return nil
            end

            while i <= n do
                local c = src:sub(i, i)
                if in_sq then
                    out[#out + 1] = c
                    if c == "'" then
                        if src:sub(i + 1, i + 1) == "'" then
                            out[#out + 1] = "'"
                            i = i + 2
                        else
                            in_sq = false
                            i = i + 1
                        end
                    else
                        i = i + 1
                    end
                elseif in_dq then
                    out[#out + 1] = c
                    if c == '"' then
                        if src:sub(i + 1, i + 1) == '"' then
                            out[#out + 1] = '"'
                            i = i + 2
                        else
                            in_dq = false
                            i = i + 1
                        end
                    else
                        i = i + 1
                    end
                elseif c == "'" then
                    in_sq = true
                    out[#out + 1] = c
                    i = i + 1
                elseif c == '"' then
                    in_dq = true
                    out[#out + 1] = c
                    i = i + 1
                elseif c == "-" and src:sub(i + 1, i + 1) == "-" then
                    out[#out + 1] = c
                    i = i + 1
                    out[#out + 1] = src:sub(i, i)
                    i = i + 1
                    while i <= n and src:sub(i, i) ~= "\n" do
                        out[#out + 1] = src:sub(i, i)
                        i = i + 1
                    end
                elseif match_word_at(i, "not") then
                    local after_not = skip_ws(i + 3)
                    if match_word_at(after_not, "null") then
                        local after_null = skip_ws(after_not + 4)
                        if match_word_at(after_null, "default") then
                            local val_start = skip_ws(after_null + 7)
                            local val_end = scan_default_value(val_start)
                            if val_end then
                                local val = src:sub(val_start, val_end - 1)
                                out[#out + 1] = "DEFAULT " .. val .. " NOT NULL"
                                i = val_end
                            else
                                out[#out + 1] = c
                                i = i + 1
                            end
                        else
                            out[#out + 1] = src:sub(i, after_not + 3)
                            i = after_not + 4
                        end
                    else
                        out[#out + 1] = c
                        i = i + 1
                    end
                else
                    out[#out + 1] = c
                    i = i + 1
                end
            end
            return table.concat(out)
        end

        if engine == "firebird" then
            sql = firebird_rewrite_multi_row_values(sql)
            sql = firebird_rewrite_alter_column_keyword(sql)
            sql = firebird_rewrite_not_null_default_order(sql)
        end

        -- Brotli compression function
        -- Requires lua-brotli library: luarocks install lua-brotli
        local function brotli_compress(data)
            local success, brotli = pcall(require, "brotli")
            if not success then
                error("Brotli compression library not available. Install with: luarocks install lua-brotli")
            end

            -- Compress with maximum quality (11) for best compression ratio
            local compressed = brotli.compress(data, 11)
            if not compressed then
                error("Brotli compression failed")
            end

            return compressed
        end

        -- Size threshold for compression (1KB = 1024 bytes)
        local COMPRESSION_THRESHOLD = 1024

        -- Base64 encode text blocks inside [=[ markers
        local function base64_encode(data)
            local b='ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'
            return ((data:gsub('.', function(x)
                local r,byte_val='',x:byte()
                for i=8,1,-1 do r=r..(byte_val%2^i-byte_val%2^(i-1)>0 and '1' or '0') end
                return r;
            end)..'0000'):gsub('%d%d%d?%d?%d?%d?', function(x)
                if (#x < 6) then return '' end
                local c=0
                for i=1,6 do c=c+(x:sub(i,i)=='1' and 2^(6-i) or 0) end
                return b:sub(c+1,c+1)
            end)..({ '', '==', '=' })[#data%3+1])
        end

        -- Multi-level Base64 encoding for nested multiline strings
        -- Process from highest nesting level (most equals signs) down to level 1
        -- This ensures inner blocks are encoded before outer blocks, maintaining proper nesting

        -- Step 1: Detect maximum nesting level by finding highest number of equals signs
        local max_level = 0
        local max_nesting_depth = 5  -- Support up to [=====[...]]=====]
        for equals in sql:gmatch("%[(=+)%[") do
            local level = #equals
            if level > max_level and level <= max_nesting_depth then
                max_level = level
            end
        end

        -- Step 2: Process each level from highest to lowest (inside-out approach)
        -- This handles multiple instances at the same level automatically via gsub
        for level = max_level, 1, -1 do
            -- Build pattern for this specific nesting level
            local equals_str = string.rep("=", level)
            local open_pattern = "%[" .. equals_str .. "%["
            local close_pattern = "%]" .. equals_str .. "%]"

            -- Enhanced pattern to capture leading whitespace before the opening marker
            -- This allows us to calculate the base indentation to strip
            local full_pattern = "([\n]?)(%s*)" .. open_pattern .. "(.-)" .. close_pattern

            -- Apply Base64 encoding to all blocks at this level
            -- The (.-) non-greedy matcher ensures each block is processed independently
            sql = sql:gsub(full_pattern, function(newline, leading_ws, content)
                -- Calculate base indentation: position of marker + 4 spaces
                -- The 4 spaces account for the standard indentation inside the multiline block
                local base_indent_len = #leading_ws + 4

                -- Strip the base indentation from each line of content
                -- This preserves relative indentation for lines that are further indented
                local function strip_base_indent(text)
                    local lines = {}
                    -- Split content by line
                    for line in text:gmatch("([^\n]*)\n?") do
                        if line ~= "" or text:match("\n") then
                            -- Count leading spaces on this line
                            local line_leading = line:match("^(%s*)")
                            local line_leading_len = #line_leading

                            -- Strip up to base_indent_len spaces, but not more than what exists
                            local strip_len = math.min(base_indent_len, line_leading_len)

                            -- Only strip if we have whitespace to strip
                            -- (Lua 5.5: for-loop control vars are read-only)
                            local stripped_line = line
                            if strip_len > 0 then
                                stripped_line = line:sub(strip_len + 1)
                            end

                            table.insert(lines, stripped_line)
                        end
                    end
                    return table.concat(lines, "\n")
                end

                -- Strip indentation
                local stripped_content = strip_base_indent(content)

                -- Check if content exceeds compression threshold
                local content_size = #stripped_content
                local should_compress = content_size > COMPRESSION_THRESHOLD

                -- Compress if needed, then encode
                local data_to_encode = stripped_content
                if should_compress then
                    data_to_encode = brotli_compress(stripped_content)
                end

                local encoded = "'" .. base64_encode(data_to_encode) .. "'"

                -- Wrap with database-specific functions if configured
                local result
                if should_compress and cfg.COMPRESS_START and cfg.COMPRESS_END then
                    -- Nested wrappers: COMPRESS_START + BASE64_START + encoded + BASE64_END + COMPRESS_END
                    result = cfg.COMPRESS_START .. encoded .. cfg.COMPRESS_END
                elseif cfg.BASE64_START and cfg.BASE64_END then
                    -- No compression, just base64 wrapper
                    result = cfg.BASE64_START .. encoded .. cfg.BASE64_END
                else
                    result = encoded
                end

                -- Preserve the newline and leading whitespace context
                return newline .. leading_ws .. result
            end)

            -- CRITICAL: Expand macros after each level to ensure BASE64_START/END
            -- are converted to actual function calls before the next outer level encodes them
            -- This prevents the literal strings ${BASE64_START} from being Base64 encoded
            unresolved = 5
            while unresolved > 0 do
                local changed = false
                for key, value in pairs(cfg) do
                    local before = sql
                    -- Use a function replacement so values containing '%' (e.g. SQL
                -- LIKE patterns or printf-style format specifiers) are not
                -- misinterpreted as gsub capture references.
                sql = sql:gsub("${" .. key .. "}", function() return value end)
                    if sql ~= before then
                        changed = true
                    end
                end
                -- Check if we still have macros left to expand
                if changed and sql:match("${([^}]+)}") then
                    unresolved = unresolved - 1
                else
                    unresolved = 0
                end
            end
        end

        -- Run macro expansion again to resolve any macros in BASE64_START/END wrappers
        unresolved = 5
        while unresolved > 0 do
            -- Run macro expansion
            for key, value in pairs(cfg) do
                -- Use a function replacement so values containing '%' (e.g. SQL
                -- LIKE patterns or printf-style format specifiers) are not
                -- misinterpreted as gsub capture references.
                sql = sql:gsub("${" .. key .. "}", function() return value end)
            end
            -- Check if we still have macros left to expand
            if sql:match("${([^}]+)}") then
              unresolved = unresolved - 1
            else
              unresolved = 0
            end
        end

        return sql
    end,

    indent_sql = function(sql)
        -- Split into lines, strip leading and trailing whitespace from each line
        local lines = {}
        -- Use split by newline to preserve empty lines
        for line in (sql .. "\n"):gmatch("(.-)\n") do
            local stripped_line = line:match("^%s*(.-)%s*$") or line
            table.insert(lines, stripped_line)
        end

        -- Apply proper indentation with multiline string support
        local indent_unit = "    "  -- 4 spaces
        local indent_stack = {}  -- Stack to track indentation levels
        local current_indent = 0  -- Current indentation level
        local indented_lines = {}

        -- Helper functions for stack operations
        local function push_indent(level)
            table.insert(indent_stack, level)
        end

        local function pop_indent()
            if #indent_stack > 0 then
                return table.remove(indent_stack)
            end
            return 0
        end

        local function is_in_multiline_string()
            return #indent_stack > 0
        end

        local pending_opening_quote = nil
        local last_comment_position = -1  -- Track comment alignment position

        for _, line in ipairs(lines) do
            local processed_line = line
            local has_mlstring_start = false
            local has_mlstring_end = false
            local quote_type = nil

            -- Check for multiline string start markers
            if processed_line:find("%[=%[") then
                has_mlstring_start = true
                quote_type = "single"
            elseif processed_line:find("%[==%[") then
                has_mlstring_start = true
                quote_type = "double"
            end

            -- Check for multiline string end markers
            if processed_line:find("%]=%]") then
                has_mlstring_end = true
            elseif processed_line:find("%]==%]") then
                has_mlstring_end = true
            end

            -- Handle multiline string start
            if has_mlstring_start then
                -- Remove the multiline marker from the line
                processed_line = processed_line:gsub("%[==%[", "")
                processed_line = processed_line:gsub("%[=%[", "")

                -- Output the line with current indentation (the part before the marker)
                if processed_line:match("%S") then -- if there's non-whitespace content
                    local indent = indent_unit:rep(current_indent)
                    table.insert(indented_lines, indent .. processed_line)
                end

                -- Push current indentation level to stack and reset to zero
                push_indent(current_indent)
                current_indent = 0

                -- Set up the pending opening quote
                pending_opening_quote = (quote_type == "single") and "'" or '"'

            elseif has_mlstring_end then
                -- Pop the indentation level first
                local restored_indent = pop_indent()

                -- Remove the closing marker
                processed_line = processed_line:gsub("%]==%]", "")
                processed_line = processed_line:gsub("%]=%]", "")

                -- Append closing quote to the last line
                if #indented_lines > 0 then
                    local quote_char = '"'
                    if line:find("%]=%]") then
                        quote_char = "'"
                    end
                    indented_lines[#indented_lines] = indented_lines[#indented_lines] .. quote_char
                end

                -- Add any remaining content on this line after the marker
                if processed_line:match("%S") then -- if there's non-whitespace content
                    local indent = indent_unit:rep(restored_indent)
                    table.insert(indented_lines, indent .. processed_line)
                end

                -- Restore indentation level
                current_indent = restored_indent
            else
                -- Regular line processing
                if processed_line ~= "" then
                    -- If we have a pending opening quote, prepend it to this line
                    if pending_opening_quote then
                        processed_line = pending_opening_quote .. processed_line
                        pending_opening_quote = nil
                    end

                    -- Apply indentation rules to all lines (including those in multiline strings)
                    -- If we're reducing indentation (like with ); )} )) @enduml) it applies on the current line
                    -- If we're increasing indentation (like with ( { @startuml), it applies on subsequent lines
                    if processed_line:sub(-2) == ");" or processed_line:sub(-1) == ")" or processed_line:sub(-2) == ")," or
                       processed_line:sub(-2) == "};" or processed_line:sub(-1) == "}" or processed_line:sub(-2) == "}," or
                       processed_line:sub(-1) == "];" or processed_line:sub(-2) == "]" or processed_line:sub(-2) == "]," then
                        -- Reducing indentation - apply to current line
                        current_indent = current_indent - 1
                        -- Ensure we don't go below 0
                        if current_indent < 0 then current_indent = 0 end
                    end

                    -- Calculate current indentation level AFTER adjusting for closing brackets
                    local indent = indent_unit:rep(current_indent)

                    -- Handle comment alignment for lines with content + comments
                    local final_line = processed_line
                    local content_part, comment_part = processed_line:match("^(.-)(%s*%-%-.*)")

                    if content_part and comment_part and content_part:match("%S") then
                        -- Line has content before comment
                        local content_only = content_part:gsub("%s+$", "") -- trim trailing spaces from content
                        local comment_only = comment_part:match("(%-%-.*)") -- extract just the comment part

                        -- Skip alignment for HTML comments containing <!-- or -->
                        local is_html_comment = comment_only:find("<!%-%-") or comment_only:find("%-%->");

                        if not is_html_comment and last_comment_position > 0 then
                            -- Try to align with previous comment position
                            local current_content_length = #(indent .. content_only)
                            local target_comment_pos = last_comment_position

                            if current_content_length < target_comment_pos then
                                local spaces_needed = target_comment_pos - current_content_length
                                final_line = content_only .. string.rep(" ", spaces_needed) .. comment_only
                            else
                                -- If content is too long, establish new position with at least 2 spaces
                                final_line = content_only .. "  " .. comment_only
                                last_comment_position = current_content_length + 2
                            end
                        elseif not is_html_comment then
                            -- First comment in a new block, establish the position based on longest expected line
                            -- Use a reasonable alignment position (e.g., 40 characters from start of indent)
                            local target_pos = #indent + 40
                            local current_content_length = #(indent .. content_only)

                            if current_content_length < target_pos then
                                local spaces_needed = target_pos - current_content_length
                                final_line = content_only .. string.rep(" ", spaces_needed) .. comment_only
                                last_comment_position = target_pos
                            else
                                -- Content is too long for target position, use minimum spacing
                                final_line = content_only .. "  " .. comment_only
                                last_comment_position = current_content_length + 2
                            end
                        else
                            -- HTML comment - don't align, keep original spacing
                            final_line = processed_line
                        end
                    else
                        -- No comment with content on this line, reset tracking for next comment block
                        last_comment_position = -1
                    end

                    -- Add current indentation level to the line
                    table.insert(indented_lines, indent .. final_line)

                    -- Handle opening brackets - increase indentation for subsequent lines
                    if processed_line:sub(-1) == "(" or processed_line:sub(-1) == "{" or processed_line:sub(-1) == "[" or
                       processed_line:find("@startuml") then
                        -- Increasing indentation - apply to subsequent lines
                        current_indent = current_indent + 1
                    end
                elseif not is_in_multiline_string() then
                    -- Only skip empty lines when not in multiline string
                    -- Don't add empty lines to the output
                    current_indent = current_indent
                else
                    -- In multiline string, preserve empty lines with current indentation
                    local indent = indent_unit:rep(current_indent)
                    table.insert(indented_lines, indent .. processed_line)
                end
            end
        end

        -- Remove trailing empty lines from the final output
        while #indented_lines > 0 and indented_lines[#indented_lines] == "" do
            table.remove(indented_lines)
        end

        return table.concat(indented_lines, "\n")
    end,

    run_migration = function(self, queries, engine, design_name, schema_name)
        local processed_queries = {}
        for _, q in ipairs(queries) do
            if q and q.sql then
                local sql = self:replace_query(q.sql, engine, design_name, schema_name)
                local indented_sql = self.indent_sql(sql)
                table.insert(processed_queries, indented_sql)
            end
        end
        return table.concat(processed_queries, "\n-- QUERY DELIMITER\n") .. "\n"
    end

}

return database