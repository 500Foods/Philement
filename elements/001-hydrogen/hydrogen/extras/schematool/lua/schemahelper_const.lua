-- schemahelper_const.lua
-- Constants, terminal module references, and attribute tables for SchemaHelper.
--
-- CHANGELOG
-- 0.6.5 - 2026-09-09 - Phase 4 accept hash / un-accept
-- 0.6.4 - 2026-09-08 - Progress bar dark-grey background
-- 0.6.3 - 2026-09-08 - Phase 3 progress bar
-- 0.6.2 - 2026-09-08 - Drop WRAPPER_BLURB family wildcards
-- 0.6.1 - 2026-09-08 - Instance off splash; label vs value attrs
-- 0.6.0 - 2026-09-08 - Chrome titles + Instance log name
-- 0.5.8 - 2026-08-25 - Extracted from schemahelper.lua (constants + terminal refs)

local VERSION = "0.6.5"
local RELEASED = "2026-09-09"

local INSTANCE_LOG = "schemahelper_schematool.log"

local CHROME_TITLE = {
    splash = " SchemaHelper ",
    picker = " SchemaHelper: Target ",
    running = " SchemaHelper: SchemaTool ",
    result = " SchemaHelper: SchemaTool ",
    dashboard = " SchemaHelper: Dashboard ",
    review = " SchemaHelper: Review ",
    note = " SchemaHelper: Review ",
    explore = " SchemaHelper: Explore ",
    apply = " SchemaHelper: Apply ",
}

local LUA_RELEASES = {
    ["5.5.1"] = "2026-08-03",
    ["5.5.0"] = "2025-12-22",
    ["5.5"] = "2025-12-22",
}

local TERMINAL_RELEASES = {
    ["0.1.0"] = "2026-06-07",
}

local WRAPPER_ORDER = {
    "postgresql",
    "mysql",
    "mariadb",
    "sqlite",
    "db2",
    "cockroachdb",
    "yugabytedb",
}

local t = require("terminal")
local Screen = require("terminal.ui.panel.screen")
local Panel = require("terminal.ui.panel")

local key_map = t.input.keymap.default_key_map
local keys = t.input.keymap.default_keys

local ATTR = {
    TITLE = { fg = "yellow", brightness = "bright" },
    SUB = { fg = "cyan", brightness = "bright" },
    VERSION = { fg = "green", brightness = "bright" },
    DATE = { fg = "cyan" },
    RUNTIME = { fg = "magenta" },
    SECTION = { fg = "yellow" },
    PATH = { fg = "white", brightness = "dim" },
    PROMPT = { fg = "white", brightness = "bright" },
    ERR = { fg = "red", brightness = "bright" },
    OK = { fg = "green", brightness = "bright" },
    RULE = { fg = "red", brightness = "bright" },
    HL = { bg = "red", fg = "white", brightness = "bright" },
    HOT = { bg = "blue", fg = "green", brightness = "bright" },
    HOTLINK = { fg = "green", brightness = "bright" },
    COLHEAD = { fg = "yellow", brightness = "bright" },
    BARBG = 236,
}

return {
    VERSION = VERSION,
    RELEASED = RELEASED,
    INSTANCE_LOG = INSTANCE_LOG,
    CHROME_TITLE = CHROME_TITLE,
    LUA_RELEASES = LUA_RELEASES,
    TERMINAL_RELEASES = TERMINAL_RELEASES,
    WRAPPER_ORDER = WRAPPER_ORDER,
    t = t,
    Screen = Screen,
    Panel = Panel,
    key_map = key_map,
    keys = keys,
    ATTR = ATTR,
}
