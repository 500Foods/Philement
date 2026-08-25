-- schemahelper_qstate.lua
-- Sidecar state JSON: load / create / update (cursor + decisions), and
-- artifact presence checks. Depends only on schemahelper_qutil.
--
-- CHANGELOG
-- 0.5.8 - 2026-08-25 - Extracted from schemahelper_queue.lua (state cluster)

local U = require("schemahelper_qutil")

local M = {}

function M.default_state_path(out_dir, design, engine)
    return string.format("%s/schemahelper_%s_%s.json", out_dir, design, engine)
end

function M.load_state(path)
    local state = {
        version = 1,
        design = "",
        engine = "",
        schema = "",
        updated_utc = "",
        cursor_id = "",
        decisions = {},
        by_id = {},
    }
    if not U.file_exists(path) then
        return state
    end
    local tmp = (os.getenv("TMPDIR") or "/tmp")
        .. "/schemahelper_st_"
        .. tostring(os.time())
        .. "_"
        .. tostring(math.random(100000))
    os.execute('mkdir -p "' .. tmp .. '"')
    state.design = (U.jq_lines(".design", path, tmp)[1] or ""):gsub('^"', ""):gsub('"$', "")
    state.engine = (U.jq_lines(".engine", path, tmp)[1] or ""):gsub('^"', ""):gsub('"$', "")
    state.schema = (U.jq_lines(".schema // \"\"", path, tmp)[1] or ""):gsub('^"', ""):gsub('"$', "")
    state.cursor_id = (U.jq_lines(".cursor_id // \"\"", path, tmp)[1] or ""):gsub('^"', ""):gsub('"$', "")
    for _, obj in ipairs(U.jq_lines(".decisions[]?", path, tmp)) do
        local id = U.json_string_field(obj, "id")
        local action = U.json_string_field(obj, "action")
        if id ~= "" then
            local rec = { id = id, action = action }
            rec.note = U.json_string_field(obj, "note")
            rec.hash = U.json_string_field(obj, "hash")
            rec.packet = U.json_string_field(obj, "packet")
            rec.ref = U.json_num_field(obj, "ref")
            state.decisions[#state.decisions + 1] = rec
            state.by_id[id] = rec
        end
    end
    os.execute('rm -rf "' .. tmp .. '"')
    return state
end

function M.create_state(path, design, engine, schema)
    local now = os.date("!%Y-%m-%dT%H:%M:%SZ")
    local body = string.format(
        '{\n  "version": 1,\n  "design": "%s",\n  "engine": "%s",\n  "schema": "%s",'
            .. '\n  "updated_utc": "%s",\n  "cursor_id": "",\n  "decisions": []\n}\n',
        U.json_escape(design),
        U.json_escape(engine),
        U.json_escape(schema or ""),
        now
    )
    return U.write_all(path, body)
end

function M.artifacts_present(out_dir, track)
    local meta = out_dir .. "/findings.json"
    local cat = out_dir .. "/catalog_findings.json"
    if track == "catalog" then
        return U.file_exists(cat)
    end
    if track == "metadata" then
        return U.file_exists(meta)
    end
    return U.file_exists(meta) or U.file_exists(cat)
end

local function jq_update_state(state_file, update_filter)
    if not U.file_exists(state_file) then
        return false, "state file not found: " .. state_file
    end
    local tmp_dir = (os.getenv("TMPDIR") or "/tmp")
        .. "/schemahelper_u_"
        .. tostring(os.time())
        .. "_"
        .. tostring(math.random(100000))
    os.execute('mkdir -p "' .. tmp_dir .. '"')
    local fpath = tmp_dir .. "/u.jq"
    local f = io.open(fpath, "w")
    if not f then
        os.execute('rm -rf "' .. tmp_dir .. '"')
        return false, "cannot write jq filter"
    end
    f:write(update_filter .. "\n")
    f:close()
    local cmd = string.format(
        'jq -S -f "%s" "%s" > "%s.tmp" && mv "%s.tmp" "%s"',
        fpath, state_file, state_file, state_file, state_file
    )
    local ok, why, code = os.execute(cmd)
    os.execute('rm -rf "' .. tmp_dir .. '"')
    if ok == true then
        return true
    end
    if why == "exit" then
        return false, "jq update failed with exit code " .. tostring(code)
    end
    return false, "jq update failed"
end

function M.jq_update_state(state_file, update_filter)
    return jq_update_state(state_file, update_filter)
end

function M.save_cursor(state_file, finding_id)
    if not finding_id then
        return false, "no finding_id provided"
    end
    local esc_id = finding_id:gsub('"', '\\"')
    local filter = string.format('.cursor_id = "%s" | .updated_utc = "%s"',
        esc_id, os.date("!%Y-%m-%dT%H:%M:%SZ"))
    return jq_update_state(state_file, filter)
end

function M.save_decision(state_file, finding_id, action, extra)
    if not finding_id or not action then
        return false, "finding_id and action are required"
    end
    local esc_id = finding_id:gsub('"', '\\"')
    local now = os.date("!%Y-%m-%dT%H:%M:%SZ")
    local parts = {}
    parts[#parts + 1] = string.format('.cursor_id = "%s"', esc_id)
    parts[#parts + 1] = string.format('.updated_utc = "%s"', now)
    local dec_fields = {}
    dec_fields[#dec_fields + 1] = string.format('"id": "%s"', esc_id)
    dec_fields[#dec_fields + 1] = string.format('"action": "%s"', action)
    if extra then
        if extra.hash and extra.hash ~= "" then
            local esc_hash = extra.hash:gsub('"', '\\"')
            dec_fields[#dec_fields + 1] = string.format('"hash": "%s"', esc_hash)
        end
        if extra.note and extra.note ~= "" then
            local esc_note = extra.note:gsub('"', '\\"'):gsub("\n", "\\n")
            dec_fields[#dec_fields + 1] = string.format('"note": "%s"', esc_note)
        end
        if extra.ref then
            dec_fields[#dec_fields + 1] = string.format('"ref": %d', extra.ref)
        end
        if extra.packet and extra.packet ~= "" then
            local esc_pkt = extra.packet:gsub('"', '\\"')
            dec_fields[#dec_fields + 1] = string.format('"packet": "%s"', esc_pkt)
        end
    end
    dec_fields[#dec_fields + 1] = string.format('"at": "%s"', now)
    local dec_json = '{' .. table.concat(dec_fields, ", ") .. '}'
    local filter = string.format(
        '.cursor_id = "%s" | .updated_utc = "%s" | '
      .. '.decisions |= (map(select(.id != "%s")) + [%s])',
        esc_id, now, esc_id, dec_json
    )
    table.insert(parts, filter)
    local combined = table.concat(parts, " | ")
    return jq_update_state(state_file, combined)
end

return M
