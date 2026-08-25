-- schemahelper_ui.lua
-- Shared, mutable UI state for SchemaHelper. The original monolith kept these
-- as file-scope upvalues; now that painting and mouse input live in separate
-- modules they must read/write the same instance. All access goes through the
-- functions here so behaviour is identical to the single-file version.
--
-- CHANGELOG
-- 0.5.8 - 2026-08-25 - Extracted shared UI state (hotspots + mouseover)

local M = {}

-- Rebuilt on every paint_framed for the live screen; read by find_hotspot.
local hotspots = {}

-- Current mouseover highlight cell; read by paint, written by mouse motion.
local mouse_hot_x, mouse_hot_y = nil, nil

function M.hotspots_reset()
    hotspots = {}
end

function M.hotspots_add(entry)
    hotspots[#hotspots + 1] = entry
end

function M.hotspots_get()
    return hotspots
end

function M.mouse_hot_get()
    return mouse_hot_x, mouse_hot_y
end

function M.mouse_hot_set(x, y)
    mouse_hot_x, mouse_hot_y = x, y
end

return M
