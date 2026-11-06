-- page.lua -- port of pdfkit/lib/page.js
--
-- A single page. Each page owns three indirect objects, and the ORDER in which
-- it creates them decides their object numbers, which decides the xref table.
-- The constructor creates them as:
--
--     content     (the content stream)
--     resources   (fonts, xobjects, colour spaces...)
--     dictionary  (the /Page dict itself)
--
-- ...and end() writes them out in the reverse of that: dictionary, resources,
-- content. So a one-page document numbers them 5, 6, 7 and emits them 7, 6, 5.
-- Both facts have to hold for the bytes to line up.
--
-- The resource sub-dictionaries (/Font, /XObject, /ExtGState, ...) are created
-- lazily, on first use. That laziness is not an optimisation -- it is what
-- decides whether a key appears in the output at all, and in what order. A page
-- with text gets /Font because the text mixin asked for it; a page without text
-- has no /Font key. Meanwhile /ColorSpace is forced into existence in end(),
-- so it always appears, and always last.

local utils = require("pdfkit.utils")
local object = require("pdfkit.object")

local DEFAULT_MARGINS = { top = 72, left = 72, bottom = 72, right = 72 }

-- Named page sizes, in points. Keys are compared upper-cased, so both
-- "letter" and "LETTER" work, as in the original.
local SIZES = {
    ["4A0"] = { 4767.87, 6740.79 },
    ["2A0"] = { 3370.39, 4767.87 },
    A0 = { 2383.94, 3370.39 },
    A1 = { 1683.78, 2383.94 },
    A2 = { 1190.55, 1683.78 },
    A3 = { 841.89, 1190.55 },
    A4 = { 595.28, 841.89 },
    A5 = { 419.53, 595.28 },
    A6 = { 297.64, 419.53 },
    A7 = { 209.76, 297.64 },
    A8 = { 147.4, 209.76 },
    A9 = { 104.88, 147.4 },
    A10 = { 73.7, 104.88 },
    B0 = { 2834.65, 4008.19 },
    B1 = { 2004.09, 2834.65 },
    B2 = { 1417.32, 2004.09 },
    B3 = { 1000.63, 1417.32 },
    B4 = { 708.66, 1000.63 },
    B5 = { 498.9, 708.66 },
    B6 = { 354.33, 498.9 },
    B7 = { 249.45, 354.33 },
    B8 = { 175.75, 249.45 },
    B9 = { 124.72, 175.75 },
    B10 = { 87.87, 124.72 },
    C0 = { 2599.37, 3676.54 },
    C1 = { 1836.85, 2599.37 },
    C2 = { 1298.27, 1836.85 },
    C3 = { 918.43, 1298.27 },
    C4 = { 649.13, 918.43 },
    C5 = { 459.21, 649.13 },
    C6 = { 323.15, 459.21 },
    C7 = { 229.61, 323.15 },
    C8 = { 161.57, 229.61 },
    C9 = { 113.39, 161.57 },
    C10 = { 79.37, 113.39 },
    RA0 = { 2437.8, 3458.27 },
    RA1 = { 1729.13, 2437.8 },
    RA2 = { 1218.9, 1729.13 },
    RA3 = { 864.57, 1218.9 },
    RA4 = { 609.45, 864.57 },
    SRA0 = { 2551.18, 3628.35 },
    SRA1 = { 1814.17, 2551.18 },
    SRA2 = { 1275.59, 1814.17 },
    SRA3 = { 907.09, 1275.59 },
    SRA4 = { 637.8, 907.09 },
    EXECUTIVE = { 521.86, 756.0 },
    FOLIO = { 612.0, 936.0 },
    LEGAL = { 612.0, 1008.0 },
    LETTER = { 612.0, 792.0 },
    TABLOID = { 792.0, 1224.0 },
}

local Page = {}
Page.__index = Page

Page.SIZES = SIZES
Page.DEFAULT_MARGINS = DEFAULT_MARGINS

function Page.new(document, options)
    options = options or {}
    local self = setmetatable({}, Page)

    self.document = document
    self._options = options
    self.size = options.size or "letter"
    self.layout = options.layout or "portrait"
    self.userUnit = options.userUnit or 1.0

    -- A size is either an explicit {width, height} pair or a name to look up.
    local dimensions
    if type(self.size) == "table" then
        dimensions = self.size
    else
        dimensions = SIZES[self.size:upper()]
        if not dimensions then
            error("unknown page size: " .. tostring(self.size))
        end
    end

    if self.layout == "portrait" then
        self.width, self.height = dimensions[1], dimensions[2]
    else
        self.width, self.height = dimensions[2], dimensions[1]
    end

    -- Object number N: the content stream.
    self.content = document:ref()

    -- Font selection happens before margins are computed, because a margin can
    -- be given in em units and so depends on the current font size.
    if options.font then
        document:font(options.font, options.fontFamily)
    end
    if options.fontSize then
        document:fontSize(options.fontSize)
    end

    local margin = options.margin
    if margin == nil then margin = options.margins end
    self.margins = utils.normalizeSides(margin, DEFAULT_MARGINS, function(x)
        return document:sizeToPoint(x, 0, self)
    end)

    -- Object number N+1: the resource dictionary.
    self.resources = document:ref(object.dict({
        { "ProcSet", object.array({ "PDF", "Text", "ImageB", "ImageC", "ImageI" }) },
    }))

    -- Object number N+2: the page dictionary.
    self.dictionary = document:ref(object.dict({
        { "Type",      "Page" },
        { "Parent",    document._root.data.Pages },
        { "MediaBox",  object.array({ 0, 0, self.width, self.height }) },
        { "Contents",  self.content },
        { "Resources", self.resources },
        { "UserUnit",  self.userUnit },
    }))

    self.markings = {}

    return self
end

--------------------------------------------------------------------------------
-- Lazily-created resource sub-dictionaries
--------------------------------------------------------------------------------
--
-- In the JS these are property getters, so `page.fonts` both creates and
-- returns. Lua has no such sugar at a sensible cost here, so they are methods.
-- The important behaviour -- create on first access, and therefore append to
-- the resource dictionary at the moment of first use -- is preserved.

local function lazy_dict(container, key)
    local existing = container[key]
    if existing ~= nil then
        return existing
    end
    local created = object.dict()
    container[key] = created
    return created
end

function Page:fonts()      return lazy_dict(self.resources.data, "Font") end
function Page:xobjects()   return lazy_dict(self.resources.data, "XObject") end
function Page:ext_gstates() return lazy_dict(self.resources.data, "ExtGState") end
function Page:patterns()   return lazy_dict(self.resources.data, "Pattern") end
function Page:colorSpaces() return lazy_dict(self.resources.data, "ColorSpace") end

function Page:annotations()
    local data = self.dictionary.data
    if data.Annots == nil then
        data.Annots = object.array()
    end
    return data.Annots
end

function Page:structParentTreeKey()
    local data = self.dictionary.data
    if data.StructParents == nil then
        data.StructParents = self.document:createStructParentTreeNextKey()
    end
    return data.StructParents
end

--------------------------------------------------------------------------------
-- Geometry
--------------------------------------------------------------------------------

function Page:contentWidth()
    return self.width - self.margins.left - self.margins.right
end

function Page:contentHeight()
    return self.height - self.margins.top - self.margins.bottom
end

function Page:maxY()
    return self.height - self.margins.bottom
end

function Page:write(chunk)
    return self.content:write(chunk)
end

--------------------------------------------------------------------------------
-- Finishing
--------------------------------------------------------------------------------

-- Tab order, for documents tagged for accessibility.
--
-- Faithful port, including a quirk: the JS tests `this.dictionary.Tabs`, but
-- the dictionary's contents live under `.data`, so that read is always
-- undefined and the guard reduces to the hasMarkInfoDictionary() call. Copying
-- the behaviour rather than the evident intent is the right call here -- the
-- goal is matching bytes, and "fixing" it would change output.
function Page:_setTabOrder()
    if self.dictionary.Tabs == nil and self.document:hasMarkInfoDictionary() then
        self.dictionary.data.Tabs = "S"
    end
end

function Page:finish()
    self:_setTabOrder()
    self.dictionary:finish()

    -- /ColorSpace is forced into existence here even when nothing used it, so
    -- every page dictionary carries an (often empty) /ColorSpace, appended
    -- after whatever else the page picked up along the way.
    if self.resources.data.ColorSpace == nil then
        self.resources.data.ColorSpace = object.dict()
    end
    for _, color in object.dict_pairs(self.document.spotColors) do
        self.resources.data.ColorSpace[color.id] = color
    end

    self.resources:finish()
    return self.content:finish()
end

return Page
