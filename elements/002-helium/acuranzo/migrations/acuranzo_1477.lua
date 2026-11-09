-- Migration: acuranzo_1477.lua
-- Reports.Render.Barcode (Reporting)
-- Barcode SVG through zint from the payload. Loaded on the first barcode in
-- stage html.
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
cfg.MIGRATION = "1477"
cfg.GROUP_NAME = "Reports"
cfg.SCRIPT_NAME = "Render.Barcode"

local sql_new = [==[
-- Reports.Render.Barcode (Reporting). Lazy: barcode SVG through zint from the Hydrogen payload.
-- Loaded by Reports.Render into its per-request environment.
-- Names defined here are shared with Reports.Render and its other modules.

BARCODE_NAMES = {
    qr = "qrcode", code39 = "code39", code128 = "code128",
    upca = "upca", ["gs1-128"] = "gs1128",
    code128ab = "code128ab", flat = "flat",
    ean8 = "ean8", ean13 = "ean13", upce = "upce",
    c25inter = "c25inter", itf14 = "itf14", nve18 = "nve18",
    dbaromn = "dbaromn", dbaromnstk = "dbaromnstk",
    dbarexp = "dbarexp", dbarexpstk = "dbarexpstk",
    datamatrix = "datamatrix", ean2 = "ean2", ean5 = "ean5",
    isbn = "isbn", pdf417 = "pdf417", pdf417comp = "pdf417comp",
    aztec = "aztec", maxicode = "maxicode", usps = "uspsimail",
}
BARCODE_BOOL = {
    "gs1", "gs1parens", "nobackground", "heightperrow", "compliantheight",
    "bind", "bindtop", "box", "quietzones", "noquietzones", "notext", "small",
    "bold", "embedfont", "dotty", "reverse", "init", "esc", "extraesc",
    "fast", "square", "dmre", "dmiso144", "gssep", "azfull",
}
BARCODE_NUM = {
    "fg", "bg", "scale", "scalexdimdp", "height", "border", "whitesp",
    "vwhitesp", "textgap", "guarddescent", "dotsize",
    "vers", "cols", "rows", "secure", "mode", "scmvv",
}
BARCODE_TEXT = { "primary" }
BARCODE_OMIT = { scale = 1, textgap = 1, guarddescent = 5, border = 0, whitesp = 0, vwhitesp = 0 }
FIT_X = { Left = "Min", Center = "Mid", Right = "Max" }
FIT_Y = { Top = "Min", Middle = "Mid", Bottom = "Max" }
barcode_mod = nil
barcode_missing = false

function barcode_symbol(bc)
    local name = bc.symbology
    if type(name) ~= "string" or name == "" then name = "code128" end
    if name == "upca" and bc.check == "supplied" then return "upcachk" end
    if name == "upce" and bc.check == "supplied" then return "upcechk" end
    local sym = BARCODE_NAMES[name]
    if not sym then return nil, "unknown symbology" end
    return sym
end

function barcode_key_ok(sym, key)
    if key == "guarddescent" then
        return sym == "upca" or sym == "upcachk" or sym == "upce" or sym == "upcechk"
            or sym == "ean8" or sym == "ean13" or sym == "isbn"
    end
    if key == "gs1" then
        return sym == "qrcode" or sym == "datamatrix" or sym == "aztec"
    end
    if key == "gs1parens" then
        return sym == "qrcode" or sym == "datamatrix" or sym == "aztec"
            or sym == "gs1128" or sym == "dbarexp" or sym == "dbarexpstk"
    end
    if key == "dotty" or key == "dotsize" then
        return sym == "qrcode" or sym == "datamatrix" or sym == "aztec"
    end
    if key == "extraesc" then return sym == "code128" or sym == "aztec" end
    if key == "vers" then
        return sym == "datamatrix" or sym == "aztec" or sym == "c25inter"
    end
    if key == "cols" or key == "rows" then
        return sym == "dbarexpstk" or sym == "pdf417" or sym == "pdf417comp"
    end
    if key == "secure" then return sym == "pdf417" or sym == "pdf417comp" or sym == "aztec" end
    if key == "fast" then
        return sym == "datamatrix" or sym == "pdf417" or sym == "pdf417comp" or sym == "aztec"
    end
    if key == "azfull" then return sym == "aztec" end
    if key == "square" or key == "dmre" or key == "dmiso144" or key == "gssep" then
        return sym == "datamatrix"
    end
    if key == "mode" or key == "scmvv" or key == "primary" then return sym == "maxicode" end
    return true
end

function barcode_color(value)
    if type(value) ~= "string" or value == "" then return nil end
    if value:sub(1, 1) == "#" then return value:sub(2) end
    return value
end

function barcode_options(sym, bc, data)
    local opts = { barcode = sym, filetype = "SVG", data = data == nil and "" or tostring(data) }
    for _, key in ipairs(BARCODE_BOOL) do
        if barcode_key_ok(sym, key) and bc[key] == true then opts[key] = true end
    end
    for _, key in ipairs(BARCODE_NUM) do
        if barcode_key_ok(sym, key) and bc[key] ~= nil and bc[key] ~= "" then
            local value = bc[key]
            if key == "fg" or key == "bg" then
                value = barcode_color(value)
            else
                value = tonumber(value)
                if value ~= nil and BARCODE_OMIT[key] == value then value = nil end
            end
            if value ~= nil then opts[key] = value end
        end
    end
    for _, key in ipairs(BARCODE_TEXT) do
        if barcode_key_ok(sym, key) and type(bc[key]) == "string" and bc[key] ~= "" then
            opts[key] = bc[key]
        end
    end
    return opts
end

function barcode_fn()
    if barcode_mod or barcode_missing then return barcode_mod end
    local ok, fn = pcall(require, "zint.barcode")
    if not ok or type(fn) ~= "function" then
        barcode_missing = true
        return nil
    end
    barcode_mod = fn
    return fn
end

function barcode_note(id, message, warned)
    local key = tostring(id) .. "\0" .. message
    if warned[key] then return end
    warned[key] = true
    add_log("warn", "item " .. tostring(id) .. " " .. message)
end

function fit_anchor(name, horizontal)
    if horizontal then
        if name == "Left" then return 0 end
        if name == "Right" then return 1 end
        return 0.5
    end
    if name == "Top" then return 0 end
    if name == "Bottom" then return 1 end
    return 0.5
end

function barcode_fit_name(item)
    local fit = item.fit
    if fit == "cover" or fit == "none" or fit == "scale-down" or fit == "contain" then
        return fit
    end
    return "contain"
end

function fit_aspect(item, mode)
    local x = FIT_X[item.fitAlign] or "Mid"
    local y = FIT_Y[item.fitValign] or "Mid"
    if mode ~= "slice" then mode = "meet" end
    return "x" .. x .. "Y" .. y .. " " .. mode
end

function fit_svg(svg, item, box)
    if type(svg) ~= "string" then return nil end
    local w, h = svg:match('<svg width="(%d+)" height="(%d+)"')
    if not w then return nil end
    local sw, sh = tonumber(w), tonumber(h)
    local fit = barcode_fit_name(item)
    local box_w = shape_px(type(box) == "table" and box.width or nil) or 0
    local box_h = shape_px(type(box) == "table" and box.height or nil) or 0
    local intrinsic = fit == "none"
    if fit == "scale-down" and box_w > 0 and box_h > 0 then
        intrinsic = sw <= box_w and sh <= box_h
    end
    local open
    local shift = nil
    if intrinsic then
        local ax = fit_anchor(item.fitAlign, true)
        local ay = fit_anchor(item.fitValign, false)
        shift = {
            x = shape_fmt((box_w - sw) * ax) .. "px",
            y = shape_fmt((box_h - sh) * ay) .. "px",
        }
        open = string.format(
            '<svg width="%d" height="%d" viewBox="0 0 %s %s" preserveAspectRatio="%s" xmlns="http://www.w3.org/2000/svg">',
            sw, sh, w, h, fit_aspect(item, "meet"))
    else
        local mode = fit == "cover" and "slice" or "meet"
        open = string.format(
            '<svg width="100%%" height="100%%" viewBox="0 0 %s %s" preserveAspectRatio="%s" xmlns="http://www.w3.org/2000/svg">',
            w, h, fit_aspect(item, mode))
    end
    local fitted = svg:gsub("<svg[^>]*>", function() return open end, 1)
    return fitted, shift
end

function emit_barcode(o, warned)
    local bc = type(o.barcode) == "table" and o.barcode or {}
    local sym, why = barcode_symbol(bc)
    if not sym then
        barcode_note(o.id, why or "unknown symbology", warned)
        return nil
    end
    if o.typeKey == 45 and (bc.data == nil or bc.data == "") then
        return nil
    end
    local fn = barcode_fn()
    if not fn then
        barcode_note(o.id, "zint.barcode is not loaded", warned)
        return nil
    end
    local opts = barcode_options(sym, bc, bc.data)
    local ok, out, warning = pcall(fn, opts)
    if not ok or out == nil or type(out) ~= "string" then
        local msg = "barcode failed"
        if ok and type(warning) == "string" then
            local err = warning:match("^(Error %d+)")
            if err then msg = err end
        end
        barcode_note(o.id, msg, warned)
        return nil
    end
    local box = type(o.box) == "table" and o.box or {}
    local fitted, shift = fit_svg(out, o, box)
    if not fitted then
        barcode_note(o.id, "barcode failed", warned)
        return nil
    end
    if shift then
        fitted = string.format(
            '<div style="margin-left:%s;margin-top:%s">%s</div>',
            shift.x, shift.y, fitted)
    end
    local turn = tonumber(o.rotation) or 0
    local extra = ""
    if turn ~= 0 then extra = string.format(";transform:rotate(%ddeg)", turn) end
    return string.format(
        '<div style="position:absolute;left:%s;top:%s;width:%s;height:%s;overflow:hidden;padding:0%s">%s</div>',
        css_or(box.left, "0in", "box left"),
        css_or(box.top, "0in", "box top"),
        css_or(box.width, "0in", "box width"),
        css_or(box.height, "0in", "box height"),
        extra,
        fitted)
end

]==]

-- ----------------------------------------------------------------------------
-- Forward: insert Reports.Render.Barcode
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
                'Reports.Render.Barcode module (Reporting)',
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
        'Insert Reports.Render.Barcode'                                        AS name,
        [=[
            # Forward Migration ${MIGRATION}: Reports.Render.Barcode

            Inserts group Reports, script Render.Barcode. Barcode SVG through zint from the payload. Loaded on the first barcode in stage html.
            Reports.Render loads it. It is not invokable.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

-- ----------------------------------------------------------------------------
-- Reverse: delete Reports.Render.Barcode
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
        'Delete Reports.Render.Barcode'                                        AS name,
        [=[
            # Reverse Migration ${MIGRATION}: delete Reports.Render.Barcode

            Deletes the group Reports script Render.Barcode row.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
