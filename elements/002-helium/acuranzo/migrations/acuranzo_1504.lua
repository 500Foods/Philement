-- Migration: acuranzo_1504.lua
-- Reports.Render.Metrics (Reporting)
-- Phase 79: runtime font metrics (insert)
--
-- Rule 12: one Render module. Reverse deletes the row.
-- QueryRef 175 stays unused. No lookup.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-09 - Phase 79 Render.Metrics insert (option B SemiExtended)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1504"
cfg.GROUP_NAME = "Reports"
cfg.SCRIPT_NAME = "Render.Metrics"

local sql_new = [==[
-- Reports.Render.Metrics (Reporting). Lazy: real-font advances via pdfkit.fontkit.
-- Loaded by Reports.Render into its per-request environment.
-- Names defined here are shared with Reports.Render and its other modules.
--
-- Measure path: cmap glyphsForString + glyph:advanceWidth (not font:layout —
-- Vanadium GSUB currently errors in lua-pdfkit). Ligatures are not applied.
-- Option B (2026-11-09): Vanadium Sans/Mono map to full SemiExtended TTFs to
-- match the designer @font-face. Swap filenames here for a future Mail
-- SemiExtended build (option C).

METRICS_FONT_ROOT = (function()
    local ok, v = pcall(function() return os.getenv and os.getenv("REPORTS_FONT_ROOT") end)
    if ok and type(v) == "string" and v ~= "" then return v end
    local hy = type(params) == "table" and type(params._hydrogen) == "table" and params._hydrogen or {}
    if type(hy.fontRoot) == "string" and hy.fontRoot ~= "" then return hy.fontRoot end
    return "/usr/share/fonts/vanadium"
end)()

-- family -> style -> filename under METRICS_FONT_ROOT
-- Italic files are not shipped for SemiExtended; Italic uses the upright face.
METRICS_FONT_FILES = {
    ["Vanadium Sans"] = {
        Normal = "VanadiumSans-SemiExtended.ttf",
        Italic = "VanadiumSans-SemiExtended.ttf",
        Bold = "VanadiumSans-SemiExtendedMedium.ttf",
        ["Bold + Italic"] = "VanadiumSans-SemiExtendedMedium.ttf",
    },
    ["Vanadium Mono"] = {
        Normal = "VanadiumMono-SemiExtended.ttf",
        Italic = "VanadiumMono-SemiExtended.ttf",
        Bold = "VanadiumMono-SemiExtendedMedium.ttf",
        ["Bold + Italic"] = "VanadiumMono-SemiExtendedMedium.ttf",
    },
}

METRICS_STANDARD = {
    Helvetica = true, ["Helvetica-Bold"] = true, ["Helvetica-Oblique"] = true,
    ["Helvetica-BoldOblique"] = true, Courier = true, Times = true,
    ["Times-Roman"] = true, Symbol = true, ZapfDingbats = true,
}

metrics_faces = metrics_faces or {}
metrics_fontkit = metrics_fontkit or nil
metrics_warned = metrics_warned or {}

function metrics_warn(key, message)
    if metrics_warned[key] then return end
    metrics_warned[key] = true
    if add_log then add_log("warn", message) end
end

function metrics_is_standard(family)
    return type(family) == "string" and METRICS_STANDARD[family] == true
end

function metrics_file_for(family, style)
    local styles = METRICS_FONT_FILES[family]
    if not styles then return nil end
    local name = styles[style] or styles.Normal
    if type(name) ~= "string" or name == "" then return nil end
    local root = METRICS_FONT_ROOT:gsub("/+$", "")
    return root .. "/" .. name
end

function metrics_load_fontkit()
    if metrics_fontkit ~= nil then return metrics_fontkit end
    local ok, mod = pcall(require, "pdfkit.fontkit")
    if ok and type(mod) == "table" and type(mod.openSync) == "function" then
        metrics_fontkit = mod
    else
        metrics_fontkit = false
        metrics_warn("fontkit", "pdfkit.fontkit is not available (" .. tostring(mod) .. "); using Helvetica AFM")
    end
    return metrics_fontkit
end

function metrics_open_face(family, style)
    style = style or "Normal"
    family = family or ""
    if metrics_is_standard(family) or family == "" then return nil end
    local path = metrics_file_for(family, style)
    if not path then return nil end
    if metrics_faces[path] ~= nil then
        return metrics_faces[path] or nil
    end
    local fk = metrics_load_fontkit()
    if not fk then
        metrics_faces[path] = false
        return nil
    end
    local ok, font = pcall(fk.openSync, path)
    if not ok or type(font) ~= "table" then
        metrics_warn("open:" .. path, "font file " .. path .. " could not be opened (" .. tostring(font) .. "); using Helvetica AFM")
        metrics_faces[path] = false
        return nil
    end
    metrics_faces[path] = font
    return font
end

function metrics_afm_units(ch, style)
    local b = string.byte(ch)
    local table = (style == "Bold" or style == "Bold + Italic") and AFM_BOLD or AFM_ROMAN
    if not b or b < 32 or b > 126 then return AFM_MISSING end
    return table[b - 31]
end

--- Advance of one character in 1/1000 em (AFM-compatible units).
function metrics_advance_units(ch, family, style)
    style = style or "Normal"
    local font = metrics_open_face(family, style)
    if not font then return metrics_afm_units(ch, style) end
    local b = string.byte(ch)
    if not b then return metrics_afm_units(ch, style) end
    -- ASCII path; multi-byte falls back to AFM missing width.
    if b >= 128 then
        -- try utf8 if available
        local cp = b
        if utf8 and utf8.codepoint then
            local ok, c = pcall(utf8.codepoint, ch)
            if ok and type(c) == "number" then cp = c end
        end
        if not font:hasGlyphForCodePoint(cp) then return AFM_MISSING end
        local upem = font:unitsPerEm() or 1000
        return font:glyphForCodePoint(cp):advanceWidth() * 1000 / upem
    end
    if b < 32 or b > 126 then return AFM_MISSING end
    if not font:hasGlyphForCodePoint(b) then return AFM_MISSING end
    local upem = font:unitsPerEm() or 1000
    return font:glyphForCodePoint(b):advanceWidth() * 1000 / upem
end

function metrics_advance_pt(ch, size, family, style)
    return metrics_advance_units(ch, family, style) * size / 1000
end

function metrics_measure_pt(text, size, family, style)
    local width = 0
    local s = text == nil and "" or tostring(text)
    for i = 1, #s do
        width = width + metrics_advance_pt(s:sub(i, i), size, family, style)
    end
    return width
end

function metrics_line_height_pt(size, family, style, multiple)
    local factor = 1.2
    if type(multiple) == "number" and multiple == multiple and multiple > 0 and multiple ~= math.huge then
        factor = multiple
    end
    -- Face ascent/descent available via metrics_open_face for Phase 80 Shrink/Fit;
    -- keep Layout parity (factor * size) for Phase 79 wrap widths.
    return factor * size
end

]==]

-- ----------------------------------------------------------------------------
-- Forward: insert Reports.Render.Metrics
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
                '${SCRIPT_NAME}',
                1,
                NULL, NULL, NULL, NULL,
                1,
                [==[
]====] .. sql_new .. [====[
                ]==],
                'Reports.Render.Metrics module (Reporting)',
                0,
                0,
                NULL,
                NULL,
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Insert Reports.Render.Metrics'                                        AS name,
        [=[
            # Forward Migration ${MIGRATION}: Reports.Render.Metrics

            Inserts group Reports, script Render.Metrics. Lazy cmap advances for Vanadium SemiExtended TTFs (option B) with Helvetica AFM fallback. Layout calls need_module("Metrics"). Not invokable.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

-- ----------------------------------------------------------------------------
-- Reverse: delete Reports.Render.Metrics
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
            WHERE group_name = '${GROUP_NAME}'
              AND script_name = '${SCRIPT_NAME}';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Delete Reports.Render.Metrics'                                        AS name,
        [=[
            # Reverse Migration ${MIGRATION}: delete Reports.Render.Metrics

            Deletes the group Reports script Render.Metrics row.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
