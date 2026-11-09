-- Migration: acuranzo_1476.lua
-- Reports.Render.Images (Reporting)
-- Static and field images. Loaded on the first image item.
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
cfg.MIGRATION = "1476"
cfg.GROUP_NAME = "Reports"
cfg.SCRIPT_NAME = "Render.Images"

local sql_new = [==[
-- Reports.Render.Images (Reporting). Lazy: static and field images.
-- Loaded by Reports.Render into its per-request environment.
-- Names defined here are shared with Reports.Render and its other modules.

IMAGE_FITS = { stretch = true, contain = true, cover = true, none = true, ["scale-down"] = true }
IMAGE_FLIPS = { none = true, horizontal = true, vertical = true, both = true }
IMAGE_ALIGNS = { Left = true, Center = true, Right = true }
IMAGE_VALIGNS = { Top = true, Middle = true, Bottom = true }
SVG_TAGS = {
    svg = true, g = true, path = true, rect = true, circle = true,
    ellipse = true, line = true, polyline = true, polygon = true,
    defs = true, style = true,
}

function svg_subset(uri)
    if type(uri) ~= "string" then return "" end
    local comma = uri:find(",", 1, true)
    if not comma then return "" end
    local meta = uri:sub(1, comma - 1):lower()
    if meta:find("base64", 1, true) then return "" end
    local payload = uri:sub(comma + 1):gsub("%%(%x%x)", function(h)
        return string.char(tonumber(h, 16))
    end)
    local lower = payload:lower()
    if lower:find("javascript:", 1, true) or lower:find("<script", 1, true)
        or lower:find("<foreignobject", 1, true) or lower:find("<text", 1, true)
        or lower:find("<image", 1, true) or lower:find("<filter", 1, true)
        or lower:find(" on%w+%s*=") then
        return ""
    end
    if not lower:find("<svg", 1, true) then return "" end
    for tag in payload:gmatch("</?([%a][%w:%-]*)") do
        local name = tag:match("^[%a]+") or tag
        if not SVG_TAGS[name:lower()] then return "" end
    end
    return payload
end

function display_px(len, dpi)
    local pt = length_pt(len)
    if not pt then return 1 end
    local density = 72
    if type(dpi) == "number" and dpi == dpi and dpi > 0 and dpi ~= math.huge then density = dpi end
    local px = half_away(pt / 72 * density)
    if px < 1 then px = 1 end
    return px
end

function raster_lookup(key)
    local rasters = type(P.rasters) == "table" and P.rasters or nil
    if not rasters then return nil end
    local entry = rasters[key]
    if type(entry) == "string" and entry ~= "" then return entry end
    if type(entry) == "table" and type(entry.data) == "string" and entry.data ~= "" then return entry.data end
    return nil
end

function image_once(state, key, message)
    if state.warned[key] then return end
    state.warned[key] = true
    add_log("warn", message)
end

function image_fit(o, state)
    local fit = o.fit
    if fit == nil or fit == "" or IMAGE_FITS[fit] then return (fit ~= nil and fit ~= "") and fit or "contain" end
    image_once(state, "fit:" .. tostring(o.id),
        "item " .. tostring(o.id) .. " fit " .. tostring(fit) .. " is not a fit; used contain")
    return "contain"
end

function image_flip(o, state)
    local flip = o.flip
    if flip == nil or flip == "" or IMAGE_FLIPS[flip] then return (flip ~= nil and flip ~= "") and flip or "none" end
    image_once(state, "flip:" .. tostring(o.id),
        "item " .. tostring(o.id) .. " flip " .. tostring(flip) .. " is not a flip; used none")
    return "none"
end

function image_anchor(o, state, horizontal)
    local value = horizontal and o.fitAlign or o.fitValign
    local allowed = horizontal and IMAGE_ALIGNS or IMAGE_VALIGNS
    local fallback = horizontal and "Center" or "Middle"
    if value == nil or value == "" or allowed[value] then
        return (value ~= nil and value ~= "") and value or fallback
    end
    image_once(state, (horizontal and "ax:" or "ay:") .. tostring(o.id),
        "item " .. tostring(o.id) .. " anchor " .. tostring(value) .. " is not an anchor; used " .. fallback)
    return fallback
end

function image_opacity(o)
    if o.opacity == nil or o.opacity == "" then return "1" end
    local n = tonumber(o.opacity)
    if not n or n ~= n or n == math.huge or n == -math.huge then return "1" end
    if n < 0 then n = 0 elseif n > 1 then n = 1 end
    return fmt_num(n)
end

function image_rotation(o, state)
    if o.rotation == nil or o.rotation == "" then return 0 end
    local n = tonumber(o.rotation)
    if not n or n ~= n or n == math.huge or n == -math.huge then
        image_once(state, "rot:" .. tostring(o.id),
            "item " .. tostring(o.id) .. " rotation " .. tostring(o.rotation) .. " is not a number; used 0")
        return 0
    end
    return n
end

function image_transform(o, state, pxw, pxh)
    local flip = image_flip(o, state)
    local fx, fy = 1, 1
    if flip == "horizontal" or flip == "both" then fx = -1 end
    if flip == "vertical" or flip == "both" then fy = -1 end
    local rot = image_rotation(o, state)
    local cx, cy = pxw / 2, pxh / 2
    return string.format(
        "rotate(%s %s %s) translate(%s %s) scale(%s %s) translate(%s %s)",
        fmt_num(rot), fmt_num(cx), fmt_num(cy),
        fmt_num(cx), fmt_num(cy), fmt_num(fx), fmt_num(fy),
        fmt_num(-cx), fmt_num(-cy))
end

function image_data_uri(value)
    if type(value) ~= "string" then return nil end
    local text = (value:match("^%s*(.-)%s*$"))
    if text == "" then return nil end
    local lower = text:lower()
    local prefix = nil
    if lower:find("data:image/png;base64,", 1, true) == 1 then
        prefix = "data:image/png;base64,"
    elseif lower:find("data:image/svg+xml;base64,", 1, true) == 1 then
        prefix = "data:image/svg+xml;base64,"
    end
    if not prefix then return nil end
    local uri_body = text:sub(#prefix + 1)
    if uri_body == "" or uri_body:find("[^%w%+/=]") then return nil end
    return text
end

function emit_image(def, o, box, state)
    local direct = type(o.src) == "string" and o.src ~= ""
    local hash = direct and o.src or o.image
    if type(hash) ~= "string" or hash == "" then
        image_once(state, "missing:" .. tostring(o.id),
            "item " .. tostring(o.id) .. " has no image; skipped")
        return nil
    end
    local canonical
    if direct then
        canonical = o.src
    else
        local images = type(def.images) == "table" and def.images or {}
        canonical = images[hash]
    end
    local left = css_or(box.left, "0in", "box left")
    local top = css_or(box.top, "0in", "box top")
    local width = css_or(box.width, "0in", "box width")
    local height = css_or(box.height, "0in", "box height")
    local opacity = image_opacity(o)
    if type(canonical) == "string" and canonical:lower():find("^data:image/svg%+xml") then
        local markup = svg_subset(canonical)
        if markup == "" then
            image_once(state, "svg:" .. tostring(o.id),
                "item " .. tostring(o.id) .. " svg is outside the path subset; skipped")
            return nil
        end
        return string.format(
            '<div style="position:absolute;left:%s;top:%s;width:%s;height:%s;overflow:hidden;padding:0;opacity:%s">%s</div>',
            left, top, width, height, opacity, markup)
    end
    local pxw = display_px(box.width, o.dpi)
    local pxh = display_px(box.height, o.dpi)
    local fit = image_fit(o, state)
    local align = image_anchor(o, state, true)
    local valign = image_anchor(o, state, false)
    local key = table.concat({
        hash, tostring(pxw), tostring(pxh), fit, align, valign,
        o.grayscale == true and "true" or "false",
        o.invert == true and "true" or "false",
    }, "|")
    local data = raster_lookup(key)
    if data == nil then
        if type(canonical) == "string" and canonical ~= "" then
            data = canonical
            if not state.dpi then
                state.dpi = true
                add_log("warn", "display dpi was not applied")
            end
        else
            image_once(state, "bytes:" .. hash,
                "item " .. tostring(o.id) .. " image " .. hash .. " has no bytes; skipped")
            return nil
        end
    end
    local id = state.ids[key]
    if not id then
        state.n = state.n + 1
        id = "rptimg" .. tostring(state.n)
        state.ids[key] = id
        state.list[#state.list + 1] = { id = id, data = data, w = pxw, h = pxh }
    end
    return string.format(
        '<div style="position:absolute;left:%s;top:%s;width:%s;height:%s;overflow:hidden;padding:0;opacity:%s"><svg width="100%%" height="100%%" viewBox="0 0 %d %d" preserveAspectRatio="none"><use href="#%s" transform="%s"/></svg></div>',
        left, top, width, height, opacity, pxw, pxh, id, image_transform(o, state, pxw, pxh))
end

]==]

-- ----------------------------------------------------------------------------
-- Forward: insert Reports.Render.Images
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
                'Reports.Render.Images module (Reporting)',
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
        'Insert Reports.Render.Images'                                        AS name,
        [=[
            # Forward Migration ${MIGRATION}: Reports.Render.Images

            Inserts group Reports, script Render.Images. Static and field images. Loaded on the first image item.
            Reports.Render loads it. It is not invokable.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

-- ----------------------------------------------------------------------------
-- Reverse: delete Reports.Render.Images
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
        'Delete Reports.Render.Images'                                        AS name,
        [=[
            # Reverse Migration ${MIGRATION}: delete Reports.Render.Images

            Deletes the group Reports script Render.Images row.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
