-- schematool_rows.lua
-- Net of keyed INSERT, UPDATE, and DELETE from disk forward SQL.
-- A live key the migrations never name is not a finding and is not deleted.
--
-- The queries table is the metadata track. DML against it is not a default row.
-- A statement that does not bind one key is unkeyed DML. Apply stays refused.
-- AutoMigration owns that statement.
--
-- This file is the orchestrator entry point: it requires the row/model,
-- json, parse, and live submodules and re-exports their public API as M.
-- The CLI dispatch (main) and jq-powered catalog/expected extraction live
-- here; the implementation is split across schematool_rows_{json, parse,
-- model, live}.lua for readability.
--
-- Usage:
--   lua schematool_rows.lua --expected PATH --catalog PATH --engine E
--        --schema S [--only-tables a,b] [--from N] [--to N] --out PATH
--   lua schematool_rows.lua --probe-sqlite DB --rows PATH --live-out PATH
--   lua schematool_rows.lua --compare --rows PATH [--live PATH]
--        --findings-out PATH [--sql-only]
--
-- CHANGELOG
-- 1.1.0 - 2026-10-09 - Refactored into submodules (json/parse/model/live)
-- 1.0.2 - 2026-10-07 - Encode does not shadow the unkeyed helper
-- 1.0.1 - 2026-10-07 - apply_insert no longer returns an undefined name
-- 1.0.0 - 2026-10-07 - Keyed default rows, targeted probe, row findings
--
-- luacheck: globals arg package

local function script_dir()
    local src = (arg and arg[0]) or ""
    local dir = src:match("^(.*)/[^/]+$")
    return dir or "."
end

package.path = script_dir() .. "/?.lua;" .. package.path

local json = require("schematool_rows_json")
local model = require("schematool_rows_model")
local live = require("schematool_rows_live")

local M = {}

function M.extract(items, catalog_keys, opts)
    return model.extract(items, catalog_keys, opts)
end

function M.encode_model(model_obj)
    return model.encode_model(model_obj)
end

function M.decode_model(text)
    return model.decode_model(text)
end

function M.compare(model_obj, live_obj, sql_only)
    return live.compare(model_obj, live_obj, sql_only)
end

function M.encode_findings(findings, counts)
    return live.encode_findings(findings, counts)
end

function M.expected_payload(row, cols)
    return live.expected_payload(row, cols)
end

function M.probe_sql(engine, schema, table_name, key_cols, data_cols, probes)
    return model.probe_sql(engine, schema, table_name, key_cols, data_cols, probes)
end

function M.probe_sqlite(db_path, model_obj)
    return live.probe_sqlite(db_path, model_obj)
end

function M.encode_live(live_obj)
    return live.encode_live(live_obj)
end

function M.decode_live(text)
    return live.decode_live(text)
end

function M.live_from_probe(model_obj, raw_by_table)
    return live.live_from_probe(model_obj, raw_by_table)
end

local function csv_only(text)
    if not text or text == "" then
        return nil
    end
    local only = {}
    for raw in tostring(text):gmatch("[^,]+") do
        local name = raw:gsub("^%s+", ""):gsub("%s+$", ""):lower()
        if name ~= "" then
            only[name] = true
        end
    end
    return only
end

local function arg_map(argv)
    local out = {}
    local i = 1
    while i <= #argv do
        local a = argv[i]
        if a:sub(1, 2) == "--" then
            local nxt = argv[i + 1]
            if nxt and nxt:sub(1, 2) ~= "--" then
                out[a] = nxt
                i = i + 2
            else
                out[a] = true
                i = i + 1
            end
        else
            i = i + 1
        end
    end
    return out
end

local function die(msg)
    io.stderr:write("Error: " .. tostring(msg) .. "\n")
    os.exit(1)
end

local function jq_codes(path, from_ref, to_ref)
    local filter = [[
      def rows:
        if type == "array" then .[]
        elif type == "object" and has("payloads") then .
        else empty end;
      [ rows
        | . as $m
        | ($m.ref // 0) as $r
        | ($m.payloads // []) as $ps
        | ([$ps[] | select(.query_type == 1000)]) as $fwd
        | (if ($fwd | length) > 0 then $fwd
           else [$ps[] | select(.query_type == 1003)] end)[]
        | {r: $r, c: .code}
      ] | sort_by(.r) | .[]
    ]]
    local tmp = (os.getenv("TMPDIR") or "/tmp")
        .. "/schematool_rows_"
        .. tostring(os.time())
        .. "_"
        .. tostring(math.random(100000))
    os.execute('mkdir -p ' .. json.sh_quote(tmp))
    local fpath = tmp .. "/fold.jq"
    local ok, err = json.write_all(fpath, filter .. "\n")
    if not ok then
        os.execute("rm -rf " .. json.sh_quote(tmp))
        return nil, err
    end
    local cmd = "jq -c -f " .. json.sh_quote(fpath) .. " " .. json.sh_quote(path)
    local h = io.popen(cmd)
    if not h then
        os.execute("rm -rf " .. json.sh_quote(tmp))
        return nil, "jq failed"
    end
    local lines = {}
    for line in h:lines() do
        lines[#lines + 1] = line
    end
    h:close()
    os.execute("rm -rf " .. json.sh_quote(tmp))
    local items = {}
    for _, line in ipairs(lines) do
        local obj = json.parse_json(line)
        if type(obj) == "table" then
            local ref = tonumber(obj.r) or 0
            local keep = true
            if from_ref and ref < from_ref then
                keep = false
            end
            if to_ref and ref > to_ref then
                keep = false
            end
            if keep then
                items[#items + 1] = { ref = ref, code = obj.c or "" }
            end
        end
    end
    return items
end

local function load_catalog_keys(path)
    local cmd = "jq -c "
        .. json.sh_quote(".tables[]? | {t:.table, pk:(.primary_key // [])}")
        .. " " .. json.sh_quote(path)
    local h = io.popen(cmd)
    if not h then
        return nil, "jq catalog failed"
    end
    local keys = {}
    for line in h:lines() do
        local obj = json.parse_json(line)
        if type(obj) == "table" and obj.t then
            keys[tostring(obj.t):lower()] = { pk = obj.pk or {} }
        end
    end
    h:close()
    return keys
end

function M.main(argv)
    local opt = arg_map(argv)
    if opt["--probe-sqlite"] then
        local text, err = json.read_all(opt["--rows"] or "")
        if not text then
            die(err)
        end
        local model_obj, merr = M.decode_model(text)
        if not model_obj then
            die(merr)
        end
        local live_obj, perr = M.probe_sqlite(opt["--probe-sqlite"], model_obj)
        if not live_obj then
            die(perr)
        end
        local ok, werr = json.write_all(opt["--live-out"], M.encode_live(live_obj) .. "\n")
        if not ok then
            die(werr)
        end
        return 0
    end
    if opt["--compare"] then
        local text, err = json.read_all(opt["--rows"] or "")
        if not text then
            die(err)
        end
        local model_obj, merr = M.decode_model(text)
        if not model_obj then
            die(merr)
        end
        local live_obj
        if opt["--sql-only"] or not opt["--live"] then
            live_obj = nil
        else
            local ltext, lerr = json.read_all(opt["--live"])
            if not ltext then
                die(lerr)
            end
            live_obj, lerr = M.decode_live(ltext)
            if not live_obj then
                die(lerr)
            end
        end
        local findings, counts = M.compare(model_obj, live_obj, opt["--sql-only"] and true or false)
        local body = M.encode_findings(findings, counts)
        local ok, werr = json.write_all(opt["--findings-out"], body .. "\n")
        if not ok then
            die(werr)
        end
        io.stderr:write(string.format(
            "phase: rows missing=%d diff=%d present=%d unkeyed=%d\n",
            counts.row_missing, counts.row_diff, counts.row_present, counts.unkeyed))
        return 0
    end
    if not opt["--expected"] or not opt["--catalog"] or not opt["--out"] then
        die("need --expected, --catalog, and --out")
    end
    local from_ref = tonumber(opt["--from"] or "")
    local to_ref = tonumber(opt["--to"] or "")
    local items, ierr = jq_codes(opt["--expected"], from_ref, to_ref)
    if not items then
        die(ierr)
    end
    local ckeys, cerr = load_catalog_keys(opt["--catalog"])
    if not ckeys then
        die(cerr)
    end
    local model_obj = M.extract(items, ckeys, {
        engine = opt["--engine"] or "sqlite",
        schema = opt["--schema"] or "",
        only = csv_only(opt["--only-tables"]),
        from_ref = from_ref,
        to_ref = to_ref,
    })
    local ok, werr = json.write_all(opt["--out"], M.encode_model(model_obj) .. "\n")
    if not ok then
        die(werr)
    end
    return 0
end

if arg and arg[0] and arg[0]:match("schematool_rows%.lua$") then
    os.exit(M.main(arg) or 0)
end

return M
