-- Migration: acuranzo_1478.lua
-- Reports.Render.Html (Reporting)
-- Stage html: font CSS and emit_html. Loaded for stage html.
--
-- One module of the split Reports.Render. Reports.Render (acuranzo_1479.lua)
-- reads this row with H.altquery_sync and runs it with load() in its
-- per-request environment. The row is not invokable and has no MCP access.
-- Reverse deletes the row. QueryRef 175 stays unused. No lookup.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-09 - Split from the acuranzo_1479.lua Reports.Render body

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1478"
cfg.GROUP_NAME = "Reports"
cfg.SCRIPT_NAME = "Render.Html"

local sql_new = [==[
-- Reports.Render.Html (Reporting). Lazy: stage html, font CSS, emit_html.
-- Loaded by Reports.Render into its per-request environment.
-- Names defined here are shared with Reports.Render and its other modules.

function embed_font_css(def, pages)
    if type(def) ~= "table" or def.embedFonts ~= true then return "" end
    local fonts = type(P.fonts) == "table" and P.fonts or nil
    if not fonts then
        add_log("warn", "embedFonts is on and params.fonts is missing; named the family")
        return ""
    end
    local used, order = {}, {}
    for _, page in ipairs(pages or {}) do
        for _, band in ipairs(page.bands or {}) do
            for _, o in ipairs(band.items or {}) do
                local fam = type(o.font) == "table" and o.font.family or nil
                if type(fam) == "string" and fam ~= "" and fam ~= "Helvetica" and not used[fam] then
                    used[fam] = true
                    order[#order + 1] = fam
                end
            end
        end
    end
    local rules, missing = {}, false
    for _, fam in ipairs(order) do
        local src = fonts[fam]
        if type(src) ~= "string" or src == "" then
            if not missing then
                missing = true
                add_log("warn", "embedFonts has no bytes for " .. fam .. "; named the family")
            end
        else
            local uri = src
            if not uri:find("^data:") then uri = "data:font/woff2;base64," .. uri end
            rules[#rules + 1] = "@font-face{font-family:" .. css_family(fam)
                .. ";src:url(" .. uri .. ") format('woff2');}"
        end
    end
    if #rules == 0 then return "" end
    return "<style>" .. table.concat(rules) .. "</style>"
end

function emit_html(def, pages)
    local shape_warned = {}
    local barcode_warned = {}
    local image_state = { ids = {}, list = {}, n = 0, warned = {}, dpi = false }
    local pg = type(def.page) == "table" and def.page or {}
    local mg = type(pg.margin) == "table" and pg.margin or {}
    local width = css_or(pg.width, "8.5in", "page width")
    local height = css_or(pg.height, "11in", "page height")
    local top = css_or(mg.top, "0.25in", "margin top")
    local right = css_or(mg.right, "0.25in", "margin right")
    local bottom = css_or(mg.bottom, "0.25in", "margin bottom")
    local left = css_or(mg.left, "0.25in", "margin left")
    local parts = {}
    for _, page in ipairs(pages or {}) do
        local bands = {}
        for _, band in ipairs(page.bands or {}) do
            local items = {}
            for _, o in ipairs(band.items or {}) do
                local box = type(o.box) == "table" and o.box or nil
                if not box then
                    add_log("warn", "item " .. tostring(o.id) .. " has no box; skipped")
                elseif o.typeKey == 25 or o.typeKey == 26 then
                    need_module("Shapes")
                    local svg, swarn = shape_svg(o, shape_spacing(def))
                    if swarn ~= "" then
                        local skey = tostring(o.id) .. "\0" .. swarn
                        if not shape_warned[skey] then
                            shape_warned[skey] = true
                            add_log("warn", "item " .. tostring(o.id) .. " " .. swarn)
                        end
                    else
                        items[#items + 1] = string.format(
                            '<div style="position:absolute;left:%s;top:%s;width:%s;height:%s;overflow:visible;padding:0">%s</div>',
                            css_or(box.left, "0in", "box left"),
                            css_or(box.top, "0in", "box top"),
                            css_or(box.width, "0in", "box width"),
                            css_or(box.height, "0in", "box height"),
                            svg)
                    end
                elseif o.typeKey == 43 or o.typeKey == 45 then
                    need_module("Barcode")
                    local html = emit_barcode(o, barcode_warned)
                    if html then items[#items + 1] = html end
                elseif type(o.lines) == "table" then
                    local font = type(o.font) == "table" and o.font or {}
                    local weight, slant = font_css(font.style)
                    local align = ALIGN_CSS[o.align]
                    if not align then
                        if o.align ~= nil then
                            add_log("warn", "align " .. tostring(o.align) .. " is not Left, Center, or Right; printed left")
                        end
                        align = "left"
                    end
                    local pad = o.offsetPt or 0
                    local padding = math.abs(pad) < 1e-6 and "0" or (fmt_pt(pad) .. " 0 0 0")
                    local chunks = {}
                    for _, line in ipairs(o.lines) do chunks[#chunks + 1] = esc(line) end
                    items[#items + 1] = string.format(
                        '<div style="position:absolute;left:%s;top:%s;width:%s;height:%s;overflow:hidden;padding:%s;white-space:pre;line-height:%s;text-align:%s;font-family:%s;font-size:%s;font-weight:%s;font-style:%s">%s</div>',
                        css_or(box.left, "0in", "box left"),
                        css_or(box.top, "0in", "box top"),
                        css_or(box.width, "0in", "box width"),
                        css_or(box.height, "0in", "box height"),
                        padding,
                        fmt_pt(o.lineHeightPt or 12),
                        align,
                        css_family(font.family),
                        css_or(font.size, "10pt", "font size"),
                        weight,
                        slant,
                        table.concat(chunks, "\n"))
                elseif o.typeKey == 2 or o.typeKey == 22 then
                    need_module("Images")
                    local html = emit_image(def, o, box, image_state)
                    if html then items[#items + 1] = html end
                else
                    local font = type(o.font) == "table" and o.font or {}
                    local weight, slant = font_css(font.style)
                    local align = ALIGN_CSS[o.align]
                    if not align then
                        if o.align ~= nil then
                            add_log("warn", "align " .. tostring(o.align) .. " is not Left, Center, or Right; printed left")
                        end
                        align = "left"
                    end
                    items[#items + 1] = string.format(
                        '<div style="position:absolute;left:%s;top:%s;width:%s;height:%s;overflow:hidden;white-space:pre-line;text-align:%s;font-family:%s;font-size:%s;font-weight:%s;font-style:%s">%s</div>',
                        css_or(box.left, "0in", "box left"),
                        css_or(box.top, "0in", "box top"),
                        css_or(box.width, "0in", "box width"),
                        css_or(box.height, "0in", "box height"),
                        align,
                        css_family(font.family),
                        css_or(font.size, "10pt", "font size"),
                        weight,
                        slant,
                        esc(o.text))
                end
            end
            bands[#bands + 1] = string.format(
                '<div style="position:relative;height:%s">%s</div>',
                css_or(band.usedHeight or band.height, "0in", "band height"),
                table.concat(items))
        end
        parts[#parts + 1] = string.format(
            '<section style="position:relative;box-sizing:border-box;width:%s;height:%s;padding:%s %s %s %s;break-after:page">%s</section>',
            width, height, top, right, bottom, left, table.concat(bands))
    end
    local defs = ""
    if #image_state.list > 0 then
        local tags = {}
        for _, im in ipairs(image_state.list) do
            tags[#tags + 1] = string.format('<image id="%s" href="%s" width="%d" height="%d"/>', im.id, im.data, im.w, im.h)
        end
        defs = '<svg width="0" height="0"><defs>' .. table.concat(tags) .. '</defs></svg>'
    end
    return embed_font_css(def, pages) .. defs .. table.concat(parts, "\n") .. "\n"
end

]==]

-- ----------------------------------------------------------------------------
-- Forward: insert Reports.Render.Html
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
                'Reports.Render.Html module (Reporting)',
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
        'Insert Reports.Render.Html'                                        AS name,
        [=[
            # Forward Migration ${MIGRATION}: Reports.Render.Html

            Inserts group Reports, script Render.Html. Stage html: font CSS and emit_html. Loaded for stage html.
            Reports.Render loads it. It is not invokable.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

-- ----------------------------------------------------------------------------
-- Reverse: delete Reports.Render.Html
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
        'Delete Reports.Render.Html'                                        AS name,
        [=[
            # Reverse Migration ${MIGRATION}: delete Reports.Render.Html

            Deletes the group Reports script Render.Html row.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
