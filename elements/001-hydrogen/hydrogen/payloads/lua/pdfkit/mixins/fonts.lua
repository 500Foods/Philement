-- mixins/fonts.lua -- port of pdfkit/lib/mixins/fonts.js
--
-- Font selection and the size-unit parser.
--
-- The font registry is an ORDERED map. It is walked at the end of the document
-- to embed every font that was used, and the walk order sets the order those
-- font objects appear in the file. Lua's pairs() would shuffle it, so this uses
-- the ordered dictionary from object.lua for the same reason the PDF
-- dictionaries do.
--
-- Note also what does NOT happen here: loading a font does not create an
-- indirect object. That is deferred until something actually references the
-- font (see font.lua's ref()), which is why a document's font object is
-- numbered after its page objects even though the font was loaded first.

local object = require("pdfkit.object")
local utils = require("pdfkit.utils")
local FontFactory = require("pdfkit.font_factory")

local M = {}

-- Two fonts count as the same if their internal tables agree. Standard fonts
-- carry no such tables, so any two of them compare equal -- which is correct,
-- since a standard font is fully identified by its name.
local function isEqualFont(font1, font2)
    local t1 = font1.font and font1.font._tables
    local t2 = font2.font and font2.font._tables

    if t1 == nil and t2 == nil then
        return true
    end
    if t1 == nil or t2 == nil then
        return false
    end

    local h1 = t1.head and t1.head.checkSumAdjustment
    local h2 = t2.head and t2.head.checkSumAdjustment
    if h1 ~= h2 then
        return false
    end

    return t1.nameRecordsKey == t2.nameRecordsKey
end

function M.initFonts(doc, defaultFont, defaultFontFamily, defaultFontSize)
    if defaultFont == nil then defaultFont = "Helvetica" end
    if defaultFontSize == nil then defaultFontSize = 12 end

    doc._fontFamilies = object.dict()
    doc._fontCount = 0

    -- Current font state. Exposed so a caller can snapshot and restore it.
    doc._fontSource = defaultFont
    doc._fontFamily = defaultFontFamily
    doc._fontSize = defaultFontSize
    doc._font = nil

    -- The document is the root element, so rem is fixed for its lifetime.
    doc._remSize = defaultFontSize

    doc._registeredFonts = {}

    if defaultFont then
        doc:font(defaultFont, defaultFontFamily)
    end
end

function M.font(doc, src, family, size)
    if type(family) == "number" then
        size = family
        family = nil
    end

    local cacheKey
    if type(src) == "string" and doc._registeredFonts[src] then
        cacheKey = src
        local registered = doc._registeredFonts[src]
        src, family = registered.src, registered.family
    else
        cacheKey = family or src
        if type(cacheKey) ~= "string" then
            cacheKey = nil
        end
    end

    doc._fontSource = src
    doc._fontFamily = family
    if size ~= nil then
        doc:fontSize(size)
    end

    -- Already in the document under this key: reuse it untouched.
    if cacheKey and doc._fontFamilies[cacheKey] then
        doc._font = doc._fontFamilies[cacheKey]
        return doc
    end

    doc._fontCount = doc._fontCount + 1
    local id = "F" .. doc._fontCount
    doc._font = FontFactory.open(doc, src, family, id)

    -- The same font may already be present under its own name -- likely when it
    -- was supplied as raw bytes rather than by name.
    local existing = doc._font.name and doc._fontFamilies[doc._font.name]
    if existing and isEqualFont(doc._font, existing) then
        doc._font = existing
        return doc
    end

    if cacheKey then
        doc._fontFamilies[cacheKey] = doc._font
    end

    if doc._font.name and not doc._fontFamilies[doc._font.name] then
        doc._fontFamilies[doc._font.name] = doc._font
    end

    -- If it ended up registered under no key at all it would never be embedded,
    -- leaving a dangling reference; fall back to registering it by id.
    if not cacheKey
        and (not doc._font.name or doc._fontFamilies[doc._font.name] ~= doc._font)
    then
        doc._fontFamilies[doc._font.id] = doc._font
    end

    return doc
end

function M.fontSize(doc, size)
    doc._fontSize = doc:sizeToPoint(size)
    return doc
end

function M.currentLineHeight(doc, includeGap)
    return doc._font:lineHeight(doc._fontSize, includeGap)
end

function M.registerFont(doc, name, src, family)
    doc._registeredFonts[name] = { src = src, family = family }
    return doc
end

--------------------------------------------------------------------------------
-- Size units
--------------------------------------------------------------------------------
--
-- Sizes may be plain numbers or CSS-ish strings: "12pt", "1.5em", "50%", "2cm".
--
-- The original parses these with a regular expression in which every part is
-- optional, so it always matches -- "abc" parses as the number 0 with no unit
-- and yields 0 rather than an error. This reproduces that, including the order
-- the units are tried in, which is what makes "rem" parse as rem and not as a
-- stray "em".

local UNITS = {
    "em", "in", "px", "cm", "mm", "pc", "ex", "ch",
    "rem", "vw", "vh", "vmin", "vmax", "%", "pt",
}

local function parse_size(s)
    -- ((\d+)?(\.\d+)?) -- an optional integer part, then an optional fraction.
    -- Note "1." is not accepted as a number by this grammar, but ".5" is.
    local intpart = s:match("^%d+") or ""
    local frac = s:sub(#intpart + 1):match("^%.%d+") or ""
    local numstr = intpart .. frac

    local tail = s:sub(#numstr + 1)
    local unit = nil
    for _, u in ipairs(UNITS) do
        if tail:sub(1, #u) == u then
            unit = u
            break
        end
    end

    return tonumber(numstr) or 0, unit
end

function M.sizeToPoint(doc, size, defaultValue, page, percentageWidth)
    if defaultValue == nil then defaultValue = 0 end
    if page == nil then page = doc.page end
    if not percentageWidth then percentageWidth = doc._fontSize end

    if type(defaultValue) ~= "number" then
        defaultValue = doc:sizeToPoint(defaultValue)
    end

    if size == nil then return defaultValue end
    if type(size) == "number" then return size end
    if type(size) == "boolean" then return size and 1 or 0 end

    local value, unit = parse_size(tostring(size))

    local multiplier
    if unit == "em" then
        multiplier = doc._fontSize
    elseif unit == "in" then
        multiplier = utils.IN_TO_PT
    elseif unit == "px" then
        multiplier = utils.PX_TO_IN * utils.IN_TO_PT
    elseif unit == "cm" then
        multiplier = utils.CM_TO_IN * utils.IN_TO_PT
    elseif unit == "mm" then
        multiplier = utils.MM_TO_CM * utils.CM_TO_IN * utils.IN_TO_PT
    elseif unit == "pc" then
        multiplier = utils.PC_TO_PT
    elseif unit == "ex" then
        multiplier = doc:currentLineHeight()
    elseif unit == "ch" then
        multiplier = doc:widthOfString("0")
    elseif unit == "rem" then
        multiplier = doc._remSize
    elseif unit == "vw" then
        multiplier = page.width / 100
    elseif unit == "vh" then
        multiplier = page.height / 100
    elseif unit == "vmin" then
        multiplier = math.min(page.width, page.height) / 100
    elseif unit == "vmax" then
        multiplier = math.max(page.width, page.height) / 100
    elseif unit == "%" then
        multiplier = percentageWidth / 100
    else -- "pt", or no unit at all
        multiplier = 1
    end

    return multiplier * value
end

return M
