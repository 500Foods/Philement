-- schemahelper_mouse.lua
-- Mouse support: terminal.lua 0.1.0 ships no mouse module, so we enable SGR
-- 1006 tracking and parse the escape report ourselves, mapping clicks to hot
-- regions recorded during paint. Clicks synthesize the same raw/name a
-- keyboard press would yield, so the existing input loops need no rewrites.
--
-- CHANGELOG
-- 0.5.8 - 2026-08-25 - Extracted from schemahelper.lua (mouse cluster)

local C = require("schemahelper_const")
local UI = require("schemahelper_ui")

local t = C.t
local keys = C.keys

local ESC = "\27"

local function enable_mouse()
    t.output.write(ESC .. "[?1003h")
    t.output.write(ESC .. "[?1006h")
    t.output.flush()
end

local function disable_mouse()
    t.output.write(ESC .. "[?1003l")
    t.output.write(ESC .. "[?1006l")
    t.output.flush()
end

local function is_mouse(raw)
    return type(raw) == "string" and raw:sub(1, 3) == ESC .. "[<"
end

local function parse_mouse(raw)
    local cb, x, y, tail = raw:match(ESC .. "%[<(%d+);(%d+);(%d+)([Mm])")
    if not cb then
        return nil
    end
    return {
        btn = tonumber(cb),
        x = tonumber(x),
        y = tonumber(y),
        release = (tail == "m"),
    }
end

-- Map a recorded hot region key to a virtual key.
local function map_hotkey(key)
    if key:sub(1, 5) == "PICK:" then
        return { pick = tonumber(key:sub(6)) }
    end
    if key == "Enter" then
        return { name = keys.enter }
    end
    if key == "ESC" or key == "Esc" then
        return { name = keys.escape }
    end
    return { raw = key:lower() }
end

local function find_hotspot(x, y)
    local hotspots = UI.hotspots_get()
    for _, h in ipairs(hotspots) do
        if y == h.row and x >= h.c0 and x <= h.c1 then
            return h
        end
    end
    return nil
end

local function mouse_vkey(raw)
    local m = parse_mouse(raw)
    if not m or m.btn ~= 0 then
        return nil
    end
    local h = find_hotspot(m.x, m.y)
    if not h then
        return nil
    end
    return map_hotkey(h.key)
end

local function highlight_hotspot(screen, x, y)
    local mx, my = UI.mouse_hot_get()
    if not screen or not x or not y then
        if mx or my then
            UI.mouse_hot_set(nil, nil)
            screen:calculate_layout()
            screen:render()
        end
        return
    end
    if x ~= mx or y ~= my then
        UI.mouse_hot_set(x, y)
        if screen then
            screen:calculate_layout()
            screen:render()
        end
    end
end

return {
    ESC = ESC,
    enable_mouse = enable_mouse,
    disable_mouse = disable_mouse,
    is_mouse = is_mouse,
    parse_mouse = parse_mouse,
    map_hotkey = map_hotkey,
    find_hotspot = find_hotspot,
    mouse_vkey = mouse_vkey,
    highlight_hotspot = highlight_hotspot,
}
