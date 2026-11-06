-- fontkit/init.lua -- port of fontkit/src/index.js, plus the registry half of
--                     fontkit/src/base.js
--
-- The public face of the font library: hand it some bytes, get back something
-- that can be measured, laid out and subsetted.
--
-- Upstream splits this across two files -- base.js owns the registry and
-- index.js does nothing but register five formats into it. The split exists
-- because ES modules are hoisted, so index.js can import base.js and the formats
-- can import base.js back without either seeing a half-built module. Lua has no
-- such hoisting: `require` runs the file. So the two settings that everything
-- reads live in base.lua on their own, and the registry -- which has to be
-- loaded *after* the formats it holds -- lives here.
--
-- Of the five formats upstream registers, three are here:
--
--   TTFFont             SFNT files: .ttf, and .otf with PostScript outlines
--   WOFFFont            WOFF1, the same tables inside a zlib wrapper
--   TrueTypeCollection  .ttc, several fonts sharing one file
--
-- Absent are WOFF2, which compresses with brotli, and DFont, which is a
-- Macintosh resource fork. A caller handed one of those gets the "Unknown font
-- format" error, which is what upstream would say if its own probe missed.

local base = require("pdfkit.fontkit.base")

local M = {}

-- Re-exported so a caller has one module to require. `export * from './base'` in
-- index.js does the same job.
M.logErrors = base.logErrors
M.defaultLanguage = base.defaultLanguage
M.setDefaultLanguage = base.setDefaultLanguage

--------------------------------------------------------------------------------
-- The format registry
--------------------------------------------------------------------------------

local formats = {}

--- Add a format. Each needs a `probe(bytes)` and a `new(reader)`.
---
--- Order matters and is the order of registration: probes are tried in turn and
--- the first to claim the bytes wins.
function M.registerFormat(format)
    formats[#formats + 1] = format
end

--- Identify and open a font.
---
--- @param buffer          the whole file, as a string
--- @param postscriptName  which font inside a collection to pick, or nil
---
--- Note what the second argument does to a *single* font rather than a
--- collection: TTFFont's getFont is the variation selector, so passing a name
--- alongside a plain .ttf asks for a named instance of a variable font and fails
--- if the font has no axes. That is upstream's shape -- one "get me the font
--- called X" hook, meaning two different things depending on what was opened --
--- and pdfkit relies on it, passing the family through whenever a caller gave
--- one.
function M.create(buffer, postscriptName)
    local Reader = require("pdfkit.fontkit.reader").Reader

    for _, format in ipairs(formats) do
        if format.probe(buffer) then
            local font = format.new(Reader.new(buffer, 0))

            if postscriptName then
                return font:getFont(postscriptName)
            end

            return font
        end
    end

    error("Unknown font format")
end

--- Read a font from disk. `openSync` upstream; there is no async half here
--- because there is no event loop to be async against.
function M.openSync(filename, postscriptName)
    local handle, err = io.open(filename, "rb")
    if not handle then
        error("fontkit: cannot open " .. filename .. " (" .. tostring(err) .. ")")
    end

    local data = handle:read("a")
    handle:close()

    return M.create(data, postscriptName)
end

--------------------------------------------------------------------------------
-- Registration
--------------------------------------------------------------------------------
--
-- Required after M.create is defined, because a format module may require this
-- one back. TrueTypeCollection needs TTFFont, which needs this module's
-- registry only after create exists.

M.registerFormat(require("pdfkit.fontkit.TTFFont"))
M.registerFormat(require("pdfkit.fontkit.WOFFFont"))
M.registerFormat(require("pdfkit.fontkit.TrueTypeCollection"))

return M
