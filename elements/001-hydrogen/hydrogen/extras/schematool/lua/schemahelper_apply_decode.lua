-- schemahelper_apply_decode.lua
-- Catalog JSON lookup helpers: jq-driven extraction of column and table
-- metadata from the fold catalog. Depends on schemahelper_apply_sqlutil.
--
-- CHANGELOG
-- 0.6.3 - 2026-10-09 - Extracted from schemahelper_apply.lua (jq/catalog lookup)

local U = require("schemahelper_apply_sqlutil")

local M = {}

M.COL_FILTER = [[
.tables[]? | select(.table == $t) | .columns[]? | select(.name == $c)
| [.name, (.data_type // ""), (if .nullable == false then "0" else "1" end)]
| @tsv
]]

M.TABLE_COLS = [[
.tables[]? | select(.table == $t) | .columns[]?
| [.name, (.data_type // ""), (if .nullable == false then "0" else "1" end)]
| @tsv
]]

M.TABLE_PK = [[.tables[]? | select(.table == $t) | .primary_key[]?]]

M.TABLE_HIT = [[.tables[]? | select(.table == $t) | .table]]

function M.jq_capture(out_dir, filter, args)
    local exp_path = (out_dir or "") .. "/catalog_expected.json"
    if not U.file_exists(exp_path) then
        return nil, "catalog_expected.json not found"
    end
    local tmp = (os.getenv("TMPDIR") or "/tmp")
        .. "/schemahelper_apply_"
        .. tostring(os.time())
        .. "_"
        .. tostring(math.random(100000))
    os.execute('mkdir -p "' .. tmp .. '"')
    local fpath = tmp .. "/q.jq"
    U.write_all(fpath, filter .. "\n")
    local parts = { "jq", "-r" }
    for i = 1, #args do
        parts[#parts + 1] = "--arg"
        parts[#parts + 1] = args[i][1]
        parts[#parts + 1] = U.sh_quote(args[i][2])
    end
    parts[#parts + 1] = "-f"
    parts[#parts + 1] = U.sh_quote(fpath)
    parts[#parts + 1] = U.sh_quote(exp_path)
    parts[#parts + 1] = "2>/dev/null"
    local h = io.popen(table.concat(parts, " "))
    if not h then
        os.execute('rm -rf "' .. tmp .. '"')
        return nil, "jq failed"
    end
    local result = h:read("*a") or ""
    h:close()
    os.execute('rm -rf "' .. tmp .. '"')
    return result
end

function M.nonempty_lines(text)
    local rows = {}
    for line in tostring(text or ""):gmatch("[^\n]+") do
        if line ~= "" and line ~= "null" then
            rows[#rows + 1] = line
        end
    end
    return rows
end

function M.lookup_column(out_dir, table_name, column)
    local text, err = M.jq_capture(out_dir, M.COL_FILTER, {
        { "t", table_name },
        { "c", column },
    })
    if not text then
        return nil, err
    end
    local line = M.nonempty_lines(text)[1]
    if not line then
        return nil, "cannot determine column type for "
            .. table_name .. "." .. column
    end
    local name, dtype, flag = line:match("^([^\t]*)\t([^\t]*)\t([^\t]*)$")
    if not name then
        return nil, "cannot determine column type for "
            .. table_name .. "." .. column
    end
    return {
        name = name,
        data_type = dtype,
        nullable = flag ~= "0",
    }
end

function M.lookup_table(out_dir, table_name)
    local hit, hit_err = M.jq_capture(out_dir, M.TABLE_HIT, {
        { "t", table_name },
    })
    if not hit then
        return nil, nil, hit_err
    end
    if not M.nonempty_lines(hit)[1] then
        return nil, nil, "table not in the fold"
    end
    local cols_text, cols_err = M.jq_capture(out_dir, M.TABLE_COLS, {
        { "t", table_name },
    })
    if not cols_text then
        return nil, nil, cols_err
    end
    local cols = {}
    for _, line in ipairs(M.nonempty_lines(cols_text)) do
        local name, dtype, flag = line:match("^([^\t]*)\t([^\t]*)\t([^\t]*)$")
        if name then
            cols[#cols + 1] = {
                name = name,
                data_type = dtype,
                nullable = flag ~= "0",
            }
        end
    end
    if #cols == 0 then
        return nil, nil, "no columns in the fold"
    end
    local pk_text, pk_err = M.jq_capture(out_dir, M.TABLE_PK, {
        { "t", table_name },
    })
    if not pk_text then
        return nil, nil, pk_err
    end
    return cols, M.nonempty_lines(pk_text)
end

return M
