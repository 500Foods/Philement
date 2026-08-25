-- schemahelper_qload.lua
-- Ingest SchemaTool metadata + catalog JSON into raw finding rows.
-- Depends only on schemahelper_qutil.
--
-- CHANGELOG
-- 0.5.8 - 2026-08-25 - Extracted from schemahelper_queue.lua (findings load cluster)

local U = require("schemahelper_qutil")

local M = {}

local function add_finding(list, item)
    list[#list + 1] = item
end

local function load_metadata(path, tmp_dir, findings)
    if not U.file_exists(path) then
        return {
            total = 0,
            ok = 0,
            orphans = 0,
        }
    end
    local counts = {
        total = U.jq_num(".counts.total // 0", path, tmp_dir),
        ok = U.jq_num(".counts.ok // 0", path, tmp_dir),
        orphans = U.jq_num(".counts.orphans // 0", path, tmp_dir),
    }

    for _, obj in ipairs(U.jq_lines(".drifts[]?", path, tmp_dir)) do
        local ref = U.json_num_field(obj, "ref") or 0
        local db_type = U.json_num_field(obj, "db_type") or 1003
        local st_kind = U.json_string_field(obj, "kind")
        if st_kind == "" then
            st_kind = "drift"
        end
        local expected = U.json_subobj(obj, "expected")
        local actual = U.json_subobj(obj, "actual")
        local file = U.json_string_field(obj, "file")
        local detail = U.payload_text(obj)
        local specs = U.drift_field_specs(obj, expected, actual)
        for i = 1, #specs do
            local spec = specs[i]
            local field = spec.field
            local view = spec.view or "raw"
            local id_field = field
            if view == "decoded" then
                id_field = field .. ":decoded"
            end
            local what
            if view == "decoded" then
                what = field .. " decoded"
            else
                what = field
            end
            local summary
            if db_type == 1003 then
                summary = string.format(
                    "APPLY check ref %d — %s mismatch", ref, what)
            else
                summary = string.format(
                    "LOAD check ref %d type %d — %s mismatch",
                    ref, db_type, what)
            end
            add_finding(findings, {
                id = string.format("meta:drift:%d:%d:%s", ref, db_type, id_field),
                class = "metadata content drift",
                kind = st_kind,
                ref = ref,
                db_type = db_type,
                field = field,
                view = view,
                file = file,
                summary = summary,
                expected = expected,
                actual = actual,
                detail = detail,
            })
        end
    end

    for _, obj in ipairs(U.jq_lines(".missing_load[]?", path, tmp_dir)) do
        local ref = U.json_num_field(obj, "ref") or 0
        add_finding(findings, {
            id = string.format("meta:missing_load:%d", ref),
            class = "missing LOAD",
            kind = "missing_load",
            ref = ref,
            file = U.json_string_field(obj, "file"),
            summary = string.format("ref %d on disk, not loaded", ref),
            detail = U.payload_text(obj),
        })
    end

    for _, obj in ipairs(U.jq_lines(".missing_apply[]?", path, tmp_dir)) do
        local ref = U.json_num_field(obj, "ref") or 0
        add_finding(findings, {
            id = string.format("meta:missing_apply:%d", ref),
            class = "missing APPLY",
            kind = "missing_apply",
            ref = ref,
            file = U.json_string_field(obj, "file"),
            summary = string.format("ref %d loaded, not applied", ref),
            detail = U.payload_text(obj),
        })
    end

    for _, obj in ipairs(U.jq_lines(".anomalies[]?", path, tmp_dir)) do
        local ref = U.json_num_field(obj, "ref") or 0
        local kind = U.json_string_field(obj, "kind")
        if kind == "" then
            kind = "anomaly"
        end
        add_finding(findings, {
            id = string.format("meta:anomaly:%s:%d", kind, ref),
            class = "anomaly 1000+1003",
            kind = kind,
            ref = ref,
            file = U.json_string_field(obj, "file"),
            summary = string.format("ref %d %s", ref, kind),
        })
    end

    for _, obj in ipairs(U.jq_lines(".orphans[]?", path, tmp_dir)) do
        local ref = U.json_num_field(obj, "ref") or 0
        add_finding(findings, {
            id = string.format("orphan:%d", ref),
            class = "orphan DB ref",
            kind = "orphan",
            ref = ref,
            file = "(orphan)",
            summary = string.format("ref %d in DB, not on disk", ref),
        })
    end

    return counts
end

local function catalog_class(check)
    if check == "table" then
        return "catalog missing table"
    end
    if check == "column" then
        return "catalog missing column"
    end
    if check == "nullable" then
        return "catalog nullability"
    end
    if check == "extra_table" or check == "extra_column" then
        return "catalog live extra"
    end
    return "catalog " .. check
end

local function add_catalog_rows(findings, rows, kind_fallback)
    for _, obj in ipairs(rows) do
        local check = U.json_string_field(obj, "check")
        if check == "" then
            check = kind_fallback
        end
        local object = U.json_string_field(obj, "object")
        local column = U.json_string_field(obj, "column")
        if column == "" then
            column = "-"
        end
        add_finding(findings, {
            id = string.format("cat:%s:%s:%s", object, column, check),
            class = catalog_class(check),
            kind = check,
            object = object,
            column = column,
            ref = U.json_num_field(obj, "ref"),
            expected = U.json_string_field(obj, "expected"),
            live = U.json_string_field(obj, "live"),
            summary = U.json_string_field(obj, "notes"),
        })
    end
end

local function load_catalog(path, tmp_dir, findings)
    if not U.file_exists(path) then
        return { checked = 0, ok = 0 }
    end
    add_catalog_rows(findings, U.jq_lines(".failures[]?", path, tmp_dir), "catalog")
    add_catalog_rows(findings, U.jq_lines(".live_extras[]?", path, tmp_dir), "extra_column")
    return {
        checked = U.jq_num(".counts.checked // 0", path, tmp_dir),
        ok = U.jq_num(".counts.ok // 0", path, tmp_dir),
    }
end

M.load_metadata = load_metadata
M.load_catalog = load_catalog
M.add_catalog_rows = add_catalog_rows

return M
