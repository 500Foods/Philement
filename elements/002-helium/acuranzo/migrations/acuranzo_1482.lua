-- Migration: acuranzo_1482.lua
-- Reports.Render.Items (Reporting)
-- Phase 77: carry foreColor, backColor, backgroundColor on item_out
--
-- Rule 12: one Render module. sql_old is the current body from
-- acuranzo_1471.lua. Reverse restores it.
-- QueryRef 175 stays unused. No lookup.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.1 - 2026-11-09 - none/transparent colour
-- 1.0.0 - 2026-11-09 - Phase 77 item colours on item_out

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1482"
cfg.GROUP_NAME = "Reports"
cfg.SCRIPT_NAME = "Render.Items"

local sql_old = [==[
-- Reports.Render.Items (Reporting). Always loaded: bound cells and item_out (one item on one row).
-- Loaded by Reports.Render into its per-request environment.
-- Names defined here are shared with Reports.Render and its other modules.

warned_field = {}
function bound_cell(item, row, band_id, legacy, blank_note)
    local field_name = type(item.field) == "string" and (item.field:match("^%s*(.-)%s*$")) or ""
    local source_name = type(item.datasource) == "string" and (item.datasource:match("^%s*(.-)%s*$")) or ""
    local field_missing = field_name == ""
    local source_missing = source_name == ""
    if field_missing or (source_missing and not legacy) then
        local k = tostring(band_id) .. "/" .. tostring(item.id)
        if not warned_field[k] then
            warned_field[k] = true
            add_log("warn", "item " .. tostring(item.id) .. " in band " .. tostring(band_id)
                .. " " .. blank_note)
        end
        return nil, true
    end
    local source_row
    if source_missing then
        source_row = row
    elseif driver ~= nil and source_name == driver then
        source_row = row
    else
        local list = rows_for(source_name, band_id)
        source_row = type(list) == "table" and list[1] or nil
    end
    return source_row and source_row[field_name], false
end
function item_out(item, row, band_id, band_tk)
    local tk = item.typeKey
    local text
    if tk == 0 then
        text = item.text ~= nil and plain(item.text) or ""
    elseif FIELD_KEYS[tk] then
        local v, missing = bound_cell(item, row, band_id, true, "has no field; printed blank")
        if missing then
            text = ""
        else
            -- Unix-number datetimes are instants and take the item timezone when set;
            -- civil strings never shift.
            if tk == 24 and item.timezone and type(v) == "number" then need_module("Zone") v = v + zone_offset(item, v) end
            text = format_value(v, tk, item.mask, item.id, band_id)
        end
    elseif tk == 5 then
        need_module("Zone")
        local s = now()
        local m = item.mask
        if m == nil or m == "" then m = DT_DEFAULT end
        text = format_value(s + zone_offset(item, s), 24, m, item.id, band_id)
    elseif tk == 6 then
        local u = HY.username
        if u == nil or u == "" then
            if not warned_user then
                warned_user = true
                add_log("warn", "item " .. tostring(item.id) .. " in band " .. tostring(band_id)
                    .. " is System user but the caller has no username; printed blank")
            end
            text = ""
        else
            text = plain(u)
        end
    elseif PAGE_KEYS[tk] then
        text = "" -- filled after the page list exists
    elseif tk == 63 then
        local v = param_value(item.param)
        local ptk = type(v) == "number" and 21 or 20
        text = format_value(v, ptk, item.mask, item.id, band_id)
    elseif tk == 34 or tk == 35 or tk == 38 then
        local fname = item["function"]
        if type(fname) == "string" and fname ~= "" then
            agg_warn(item, band_id, "names function " .. fname
                .. "; function totals are not available yet; printed blank")
            text = ""
        elseif item.field == nil or item.field == "" then
            agg_warn(item, band_id, "has no field; printed blank")
            text = ""
        elseif not AGG_OPS[item.op] then
            agg_warn(item, band_id, "has op " .. tostring(item.op)
                .. " which this version does not reduce; printed blank")
            text = ""
        elseif band_tk == 13 or band_tk == 17 then
            text = "" -- filled after the page list exists
        elseif type(ctx) ~= "table" or type(ctx.aggregates) ~= "table" then
            agg_warn(item, band_id, "is not on a group, page, or report header or footer; printed blank")
            text = ""
        else
            text = format_aggregate(agg_raw(ctx.aggregates, item), item, band_id)
        end
    elseif tk == 1 then
        local v = item.value
        if v == nil then
            text = ""
        elseif type(v) ~= "number" or v ~= v or v == math.huge or v == -math.huge then
            format_warn(item.id, band_id, "value " .. raw(v) .. " is not a number; printed blank")
            text = ""
        else
            text = format_value(v, 21, item.mask, item.id, band_id)
        end
    elseif tk == 4 then
        local v = item.value
        if v == nil then
            text = ""
        else
            text = format_value(v, 24, item.mask, item.id, band_id)
        end
    elseif tk == 3 then
        text = item.text ~= nil and plain(item.text) or ""
    elseif tk == 23 then
        local v, missing = bound_cell(item, row, band_id, false, "has no datasource or field; printed blank")
        text = missing and "" or plain(v)
    elseif tk == 2 or tk == 22 then
        text = ""
    elseif tk == 25 or tk == 26 then
        text = ""
    elseif tk == 43 or tk == 45 then
        text = ""
    elseif LATER_KEYS[tk] then
        text = ""
    else
        add_log("warn", "item " .. tostring(item.id) .. " in band " .. tostring(band_id)
            .. " has unknown typeKey " .. tostring(tk) .. "; skipped")
        return nil
    end
    local f = type(item.font) == "table" and item.font or {}
    local o = {
        id = item.id,
        typeKey = tk,
        text = text,
        align = item.align or "Left",
        font = {
            family = f.family or rfont.family,
            size = f.size or rfont.size,
            style = f.style or rfont.style,
        },
        box = item.box,
    }
    if PAGE_KEYS[tk] then page_src[o] = item end
    if tk == 2 then
        o.image = item.image
        o.dpi = item.dpi
        o.fit = item.fit
        o.fitAlign = item.fitAlign
        o.fitValign = item.fitValign
        o.flip = item.flip
        o.opacity = item.opacity
        o.grayscale = item.grayscale
        o.invert = item.invert
        o.rotation = item.rotation
    end
    if tk == 22 then
        local v, missing = bound_cell(item, row, band_id, false, "has no datasource or field; printed blank")
        if not missing then
            need_module("Images")
            local uri = image_data_uri(v)
            if uri then
                o.src = uri
            elseif v ~= nil and plain(v) ~= "" then
                local ik = "uri:" .. tostring(band_id) .. "/" .. tostring(item.id)
                if not warned_field[ik] then
                    warned_field[ik] = true
                    add_log("warn", "item " .. tostring(item.id) .. " in band " .. tostring(band_id)
                        .. " image is not a png or svg data uri; skipped")
                end
            end
        end
        o.dpi = item.dpi
        o.fit = item.fit
        o.fitAlign = item.fitAlign
        o.fitValign = item.fitValign
        o.flip = item.flip
        o.opacity = item.opacity
        o.grayscale = item.grayscale
        o.invert = item.invert
        o.rotation = item.rotation
    end
    if tk == 25 or tk == 26 then
        o.kind = item.kind
        o.side = item.side
        o.corners = item.corners
        o.strokeWidth = item.strokeWidth
        o.strokeStyle = item.strokeStyle
        o.strokeColor = item.strokeColor
        o.fillStyle = item.fillStyle
        o.fillPaint = item.fillPaint
        o.rotation = item.rotation
        if tk == 26 then
            local v, missing = bound_cell(item, row, band_id, false, "has no datasource or field; drew the item kind")
            if not missing then
                local code = plain(v)
                need_module("Shapes")
                if shape_source(code) then o.kind = code end
            end
        end
    end
    if tk == 43 or tk == 45 then
        local bc = type(item.barcode) == "table" and item.barcode or {}
        local copy = {}
        for k, v in pairs(bc) do copy[k] = v end
        if tk == 45 then
            local v, missing = bound_cell(item, row, band_id, false, "has no datasource or field; printed blank")
            copy.data = missing and "" or plain(v)
        end
        o.barcode = copy
        o.dpi = item.dpi
        o.fit = item.fit
        o.fitAlign = item.fitAlign
        o.fitValign = item.fitValign
        o.rotation = item.rotation
    end
    return o
end

]==]

local sql_new = [==[
-- Reports.Render.Items (Reporting). Always loaded: bound cells and item_out (one item on one row).
-- Loaded by Reports.Render into its per-request environment.
-- Names defined here are shared with Reports.Render and its other modules.

warned_field = {}
function is_none_color(v)
    if type(v) ~= "string" then return false end
    local s = v:match("^%s*(.-)%s*$") or ""
    s = string.lower(s)
    return s == "" or s == "none" or s == "transparent"
end
function color_or_nil(v)
    if is_none_color(v) then return "none" end
    if type(v) == "string" and v ~= "" then return v end
    return nil
end
function bound_cell(item, row, band_id, legacy, blank_note)
    local field_name = type(item.field) == "string" and (item.field:match("^%s*(.-)%s*$")) or ""
    local source_name = type(item.datasource) == "string" and (item.datasource:match("^%s*(.-)%s*$")) or ""
    local field_missing = field_name == ""
    local source_missing = source_name == ""
    if field_missing or (source_missing and not legacy) then
        local k = tostring(band_id) .. "/" .. tostring(item.id)
        if not warned_field[k] then
            warned_field[k] = true
            add_log("warn", "item " .. tostring(item.id) .. " in band " .. tostring(band_id)
                .. " " .. blank_note)
        end
        return nil, true
    end
    local source_row
    if source_missing then
        source_row = row
    elseif driver ~= nil and source_name == driver then
        source_row = row
    else
        local list = rows_for(source_name, band_id)
        source_row = type(list) == "table" and list[1] or nil
    end
    return source_row and source_row[field_name], false
end
function item_out(item, row, band_id, band_tk)
    local tk = item.typeKey
    local text
    if tk == 0 then
        text = item.text ~= nil and plain(item.text) or ""
    elseif FIELD_KEYS[tk] then
        local v, missing = bound_cell(item, row, band_id, true, "has no field; printed blank")
        if missing then
            text = ""
        else
            -- Unix-number datetimes are instants and take the item timezone when set;
            -- civil strings never shift.
            if tk == 24 and item.timezone and type(v) == "number" then need_module("Zone") v = v + zone_offset(item, v) end
            text = format_value(v, tk, item.mask, item.id, band_id)
        end
    elseif tk == 5 then
        need_module("Zone")
        local s = now()
        local m = item.mask
        if m == nil or m == "" then m = DT_DEFAULT end
        text = format_value(s + zone_offset(item, s), 24, m, item.id, band_id)
    elseif tk == 6 then
        local u = HY.username
        if u == nil or u == "" then
            if not warned_user then
                warned_user = true
                add_log("warn", "item " .. tostring(item.id) .. " in band " .. tostring(band_id)
                    .. " is System user but the caller has no username; printed blank")
            end
            text = ""
        else
            text = plain(u)
        end
    elseif PAGE_KEYS[tk] then
        text = "" -- filled after the page list exists
    elseif tk == 63 then
        local v = param_value(item.param)
        local ptk = type(v) == "number" and 21 or 20
        text = format_value(v, ptk, item.mask, item.id, band_id)
    elseif tk == 34 or tk == 35 or tk == 38 then
        local fname = item["function"]
        if type(fname) == "string" and fname ~= "" then
            agg_warn(item, band_id, "names function " .. fname
                .. "; function totals are not available yet; printed blank")
            text = ""
        elseif item.field == nil or item.field == "" then
            agg_warn(item, band_id, "has no field; printed blank")
            text = ""
        elseif not AGG_OPS[item.op] then
            agg_warn(item, band_id, "has op " .. tostring(item.op)
                .. " which this version does not reduce; printed blank")
            text = ""
        elseif band_tk == 13 or band_tk == 17 then
            text = "" -- filled after the page list exists
        elseif type(ctx) ~= "table" or type(ctx.aggregates) ~= "table" then
            agg_warn(item, band_id, "is not on a group, page, or report header or footer; printed blank")
            text = ""
        else
            text = format_aggregate(agg_raw(ctx.aggregates, item), item, band_id)
        end
    elseif tk == 1 then
        local v = item.value
        if v == nil then
            text = ""
        elseif type(v) ~= "number" or v ~= v or v == math.huge or v == -math.huge then
            format_warn(item.id, band_id, "value " .. raw(v) .. " is not a number; printed blank")
            text = ""
        else
            text = format_value(v, 21, item.mask, item.id, band_id)
        end
    elseif tk == 4 then
        local v = item.value
        if v == nil then
            text = ""
        else
            text = format_value(v, 24, item.mask, item.id, band_id)
        end
    elseif tk == 3 then
        text = item.text ~= nil and plain(item.text) or ""
    elseif tk == 23 then
        local v, missing = bound_cell(item, row, band_id, false, "has no datasource or field; printed blank")
        text = missing and "" or plain(v)
    elseif tk == 2 or tk == 22 then
        text = ""
    elseif tk == 25 or tk == 26 then
        text = ""
    elseif tk == 43 or tk == 45 then
        text = ""
    elseif LATER_KEYS[tk] then
        text = ""
    else
        add_log("warn", "item " .. tostring(item.id) .. " in band " .. tostring(band_id)
            .. " has unknown typeKey " .. tostring(tk) .. "; skipped")
        return nil
    end
    local f = type(item.font) == "table" and item.font or {}
    local o = {
        id = item.id,
        typeKey = tk,
        text = text,
        align = item.align or "Left",
        font = {
            family = f.family or rfont.family,
            size = f.size or rfont.size,
            style = f.style or rfont.style,
        },
        box = item.box,
    }
    do
        local fc = color_or_nil(item.foreColor)
        if fc then o.foreColor = fc end
        local bc = color_or_nil(item.backColor)
        if bc then o.backColor = bc end
        local bg = item.backgroundColor
        if is_none_color(bg) then
            o.backgroundColor = "none"
        else
            if type(bg) ~= "string" or bg == "" then
                local paint = type(item.fillPaint) == "table" and item.fillPaint or nil
                if paint and type(paint.color) == "string" and paint.color ~= ""
                    and not is_none_color(paint.color) then
                    bg = paint.color
                elseif paint and type(paint.stops) == "table" and paint.stops[1]
                    and type(paint.stops[1].color) == "string" and paint.stops[1].color ~= ""
                    and not is_none_color(paint.stops[1].color) then
                    bg = paint.stops[1].color
                end
            end
            if type(bg) == "string" and bg ~= "" then o.backgroundColor = bg end
        end
    end
    if PAGE_KEYS[tk] then page_src[o] = item end
    if tk == 2 then
        o.image = item.image
        o.dpi = item.dpi
        o.fit = item.fit
        o.fitAlign = item.fitAlign
        o.fitValign = item.fitValign
        o.flip = item.flip
        o.opacity = item.opacity
        o.grayscale = item.grayscale
        o.invert = item.invert
        o.rotation = item.rotation
    end
    if tk == 22 then
        local v, missing = bound_cell(item, row, band_id, false, "has no datasource or field; printed blank")
        if not missing then
            need_module("Images")
            local uri = image_data_uri(v)
            if uri then
                o.src = uri
            elseif v ~= nil and plain(v) ~= "" then
                local ik = "uri:" .. tostring(band_id) .. "/" .. tostring(item.id)
                if not warned_field[ik] then
                    warned_field[ik] = true
                    add_log("warn", "item " .. tostring(item.id) .. " in band " .. tostring(band_id)
                        .. " image is not a png or svg data uri; skipped")
                end
            end
        end
        o.dpi = item.dpi
        o.fit = item.fit
        o.fitAlign = item.fitAlign
        o.fitValign = item.fitValign
        o.flip = item.flip
        o.opacity = item.opacity
        o.grayscale = item.grayscale
        o.invert = item.invert
        o.rotation = item.rotation
    end
    if tk == 25 or tk == 26 then
        o.kind = item.kind
        o.side = item.side
        o.corners = item.corners
        o.strokeWidth = item.strokeWidth
        o.strokeStyle = item.strokeStyle
        o.strokeColor = item.strokeColor
        o.fillStyle = item.fillStyle
        o.fillPaint = item.fillPaint
        o.rotation = item.rotation
        if tk == 26 then
            local v, missing = bound_cell(item, row, band_id, false, "has no datasource or field; drew the item kind")
            if not missing then
                local code = plain(v)
                need_module("Shapes")
                if shape_source(code) then o.kind = code end
            end
        end
    end
    if tk == 43 or tk == 45 then
        local bc = type(item.barcode) == "table" and item.barcode or {}
        local copy = {}
        for k, v in pairs(bc) do copy[k] = v end
        if tk == 45 then
            local v, missing = bound_cell(item, row, band_id, false, "has no datasource or field; printed blank")
            copy.data = missing and "" or plain(v)
        end
        o.barcode = copy
        o.dpi = item.dpi
        o.fit = item.fit
        o.fitAlign = item.fitAlign
        o.fitValign = item.fitValign
        o.rotation = item.rotation
    end
    return o
end

]==]

-- ----------------------------------------------------------------------------
-- Forward: amend Reports.Render.Items
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
]====] .. sql_new .. [====[
            ]==]
            WHERE group_name = '${GROUP_NAME}'
              AND script_name = '${SCRIPT_NAME}';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Amend Reports.Render.Items colours'                               AS name,
        [=[
            # Forward Migration ${MIGRATION}: Amend Reports.Render.Items

            item_out copies foreColor, backColor, and backgroundColor (with fillPaint colour fallback for shapes).
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

-- ----------------------------------------------------------------------------
-- Reverse: restore prior Reports.Render.Items body
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
]====] .. sql_old .. [====[
            ]==]
            WHERE group_name = '${GROUP_NAME}'
              AND script_name = '${SCRIPT_NAME}';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Restore prior Reports.Render.Items'                               AS name,
        [=[
            # Reverse Migration ${MIGRATION}: restore Reports.Render.Items

            Puts the pre-Phase-77 body back.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
