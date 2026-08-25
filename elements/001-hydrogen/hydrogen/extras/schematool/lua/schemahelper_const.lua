-- schemahelper_const.lua
-- Constants, terminal module references, and attribute tables for SchemaHelper.
--
-- CHANGELOG
-- 0.5.8 - 2026-08-25 - Extracted from schemahelper.lua (constants + terminal refs)

local VERSION = "0.5.8"
local RELEASED = "2026-08-25"

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

local WRAPPER_BLURB = {
    postgresql = "ACURANZO_DB_* / schema demo",
    mysql = "CANVAS_DB_* / schema demo",
    mariadb = "CANVAS_DB_* / schema demomrdb",
    sqlite = "hydrodemo.sqlite",
    db2 = "HYDROTST_DB_*",
    cockroachdb = "ACURANZO_DB_* / schema democrdb",
    yugabytedb = "YUGABYTE_DB_*  (never ACURANZO)",
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
}

return {
    VERSION = VERSION,
    RELEASED = RELEASED,
    LUA_RELEASES = LUA_RELEASES,
    TERMINAL_RELEASES = TERMINAL_RELEASES,
    WRAPPER_ORDER = WRAPPER_ORDER,
    WRAPPER_BLURB = WRAPPER_BLURB,
    t = t,
    Screen = Screen,
    Panel = Panel,
    key_map = key_map,
    keys = keys,
    ATTR = ATTR,
}
