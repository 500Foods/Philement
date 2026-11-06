-- font_factory.lua -- port of pdfkit/lib/font_factory.js
--
-- Decides what kind of font a caller asked for. A string that names one of the
-- fourteen standard fonts loads a generated metric module. Anything else is a
-- font file, read from disk and parsed.

local StandardFont = require("pdfkit.font.standard")
local EmbeddedFont = require("pdfkit.font.embedded")
local fontkit = require("pdfkit.fontkit")

local M = {}

--- @param src     a standard font name, a path to a font file, or its bytes
--- @param family  which font to take out of a collection, or nil
--- @param id      the name the page resources will use -- "F1", "F2"
---
--- The one ambiguity worth naming: a string may be a font name or a file path,
--- and the two are told apart by asking whether it is one of the fourteen
--- standard names. So a file called "Symbol" in the working directory is
--- unreachable, upstream and here.
function M.open(document, src, family, id)
    if type(src) == "string" and StandardFont.isStandardFont(src) then
        return StandardFont.new(document, src, id)
    end

    -- Lua has one string type, so unlike upstream there is no way to tell a path
    -- from a buffer by type. A string that names a readable file is treated as a
    -- path; anything else is taken as the bytes themselves, which is what makes
    -- both `doc:font("fonts/Roboto.ttf")` and `doc:font(fontBytes)` work.
    local data = src
    if type(src) == "string" then
        local handle = io.open(src, "rb")
        if handle then
            data = handle:read("a")
            handle:close()
        end
    end

    if type(data) ~= "string" then
        error("Not a supported font format or standard PDF font.")
    end

    return EmbeddedFont.new(document, fontkit.create(data, family), id)
end

return M
