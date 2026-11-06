-- font/afm.lua -- port of pdfkit/lib/font/afm.js
--
-- Width tables for the fourteen standard PDF fonts. PDFKit 0.20.2 does not
-- parse AFM text at runtime. Each font arrives as a generated module: a name,
-- a bounding box, the vertical metrics, a space-separated glyphNames list, a
-- glyphWidths array parallel to that list, and a packed kernPairs table.
-- AFMFont.new expands that module into the maps the rest of the port asks for.
--
-- The kern encoding is the one in the JavaScript constructor. Entries come in
-- pairs: an amount, then a list of deltas. pairId starts at 0 and each delta
-- is added to it. floor(pairId / glyphCount) and pairId % glyphCount index
-- glyphNames. The WinAnsi table further down is a different list. It turns a
-- character into a glyph name, and widths and kerns are looked up by that name.
--
-- Two parts of the expanded font decide where every glyph lands on the page:
--
--   glyphWidths -- the advance width of each glyph, in 1/1000 em
--   kernPairs   -- per-pair adjustments, e.g. "Wa" tucks closer than W + a
--
-- The kern pairs are why "hello, world" does not come out as one flat run of
-- characters: PDFKit emits a TJ array with the kern values interleaved. Miss
-- them and the text is subtly, uniformly wrong.
--
-- One porting note on text. JavaScript strings are UTF-16 and charCodeAt gives
-- a code point for anything in the basic plane; Lua strings are bytes. Source
-- files are UTF-8 in both languages, so the equivalent of charCodeAt is to
-- decode UTF-8 into code points -- not to walk bytes, which would turn every
-- accented character into two wrong glyphs.

local M = {}

--------------------------------------------------------------------------------
-- WinAnsi remapping
--------------------------------------------------------------------------------
--
-- WinAnsiEncoding is Latin-1 with a different set of characters in 0x80-0x9F.
-- This table maps the Unicode code points of those characters down to the byte
-- WinAnsi uses for them.

local WIN_ANSI_MAP = {
    [402] = 131, [8211] = 150, [8212] = 151, [8216] = 145, [8217] = 146,
    [8218] = 130, [8220] = 147, [8221] = 148, [8222] = 132, [8224] = 134,
    [8225] = 135, [8226] = 149, [8230] = 133, [8364] = 128, [8240] = 137,
    [8249] = 139, [8250] = 155, [710] = 136, [8482] = 153, [338] = 140,
    [339] = 156, [732] = 152, [352] = 138, [353] = 154, [376] = 159,
    [381] = 142, [382] = 158,
}

M.WIN_ANSI_MAP = WIN_ANSI_MAP

--------------------------------------------------------------------------------
-- Glyph names by WinAnsi code
--------------------------------------------------------------------------------
--
-- Indexed 0-255. Kept in the original's layout -- four names per line, in code
-- order -- so it can be diffed against the source it came from.

local CHARACTER_SOURCE = [[
.notdef       .notdef        .notdef        .notdef
.notdef       .notdef        .notdef        .notdef
.notdef       .notdef        .notdef        .notdef
.notdef       .notdef        .notdef        .notdef
.notdef       .notdef        .notdef        .notdef
.notdef       .notdef        .notdef        .notdef
.notdef       .notdef        .notdef        .notdef
.notdef       .notdef        .notdef        .notdef

space         exclam         quotedbl       numbersign
dollar        percent        ampersand      quotesingle
parenleft     parenright     asterisk       plus
comma         hyphen         period         slash
zero          one            two            three
four          five           six            seven
eight         nine           colon          semicolon
less          equal          greater        question

at            A              B              C
D             E              F              G
H             I              J              K
L             M              N              O
P             Q              R              S
T             U              V              W
X             Y              Z              bracketleft
backslash     bracketright   asciicircum    underscore

grave         a              b              c
d             e              f              g
h             i              j              k
l             m              n              o
p             q              r              s
t             u              v              w
x             y              z              braceleft
bar           braceright     asciitilde     .notdef

Euro          .notdef        quotesinglbase florin
quotedblbase  ellipsis       dagger         daggerdbl
circumflex    perthousand    Scaron         guilsinglleft
OE            .notdef        Zcaron         .notdef
.notdef       quoteleft      quoteright     quotedblleft
quotedblright bullet         endash         emdash
tilde         trademark      scaron         guilsinglright
oe            .notdef        zcaron         ydieresis

space         exclamdown     cent           sterling
currency      yen            brokenbar      section
dieresis      copyright      ordfeminine    guillemotleft
logicalnot    hyphen         registered     macron
degree        plusminus      twosuperior    threesuperior
acute         mu             paragraph      periodcentered
cedilla       onesuperior    ordmasculine   guillemotright
onequarter    onehalf        threequarters  questiondown

Agrave        Aacute         Acircumflex    Atilde
Adieresis     Aring          AE             Ccedilla
Egrave        Eacute         Ecircumflex    Edieresis
Igrave        Iacute         Icircumflex    Idieresis
Eth           Ntilde         Ograve         Oacute
Ocircumflex   Otilde         Odieresis      multiply
Oslash        Ugrave         Uacute         Ucircumflex
Udieresis     Yacute         Thorn          germandbls

agrave        aacute         acircumflex    atilde
adieresis     aring          ae             ccedilla
egrave        eacute         ecircumflex    edieresis
igrave        iacute         icircumflex    idieresis
eth           ntilde         ograve         oacute
ocircumflex   otilde         odieresis      divide
oslash        ugrave         uacute         ucircumflex
udieresis     yacute         thorn          ydieresis
]]

-- Split on whitespace, then index from 0 so the index IS the character code.
local characters = {}
do
    local i = 0
    for word in CHARACTER_SOURCE:gmatch("%S+") do
        characters[i] = word
        i = i + 1
    end
end

M.characters = characters

--------------------------------------------------------------------------------
-- Text to code points
--------------------------------------------------------------------------------
--
-- The stand-in for JavaScript's charCodeAt. Valid UTF-8 is decoded properly;
-- anything else falls back to byte values, which is the sensible reading of a
-- string that was never UTF-8 to begin with.

local function codepoints(s)
    local ok, result = pcall(function()
        local out = {}
        for _, code in utf8.codes(s) do
            out[#out + 1] = code
        end
        return out
    end)

    if ok then
        return result
    end

    local out = {}
    for i = 1, #s do
        out[i] = s:byte(i)
    end
    return out
end

M.codepoints = codepoints

--------------------------------------------------------------------------------
-- AFMFont
--------------------------------------------------------------------------------

local AFMFont = {}
AFMFont.__index = AFMFont

-- Split on single spaces, the way JavaScript's split(" ") does.
local function split_spaces(text)
    local names = {}
    local start_at = 1
    while true do
        local space = string.find(text, " ", start_at, true)
        if not space then
            names[#names + 1] = string.sub(text, start_at)
            break
        end
        names[#names + 1] = string.sub(text, start_at, space - 1)
        start_at = space + 1
    end
    return names
end

function AFMFont.new(data)
    if type(data) ~= "table" or type(data.glyphNames) ~= "string" then
        error("pdfkit: standard-font metrics must be a generated module")
    end

    local self = setmetatable({}, AFMFont)

    self.name = data.name
    -- A copy, matching AFMFont's bbox.slice() in the JavaScript.
    self.bbox = { data.bbox[1], data.bbox[2], data.bbox[3], data.bbox[4] }
    self.ascender = data.ascender
    self.descender = data.descender
    self.xHeight = data.xHeight
    self.capHeight = data.capHeight

    -- JavaScript indexes the box from 0, so ury is [3] and lly is [1].
    -- Those are [4] and [2] in this array.
    self.lineGap = self.bbox[4] - self.bbox[2] - (self.ascender - self.descender)

    local glyph_names = split_spaces(data.glyphNames)
    local glyph_count = #glyph_names

    self.glyphWidths = {}
    for i = 1, glyph_count do
        local width = data.glyphWidths[i]
        if width ~= nil then
            self.glyphWidths[glyph_names[i]] = width
        end
    end

    self.kernPairs = {}
    local packed = data.kernPairs or {}
    local index = 1
    while index <= #packed do
        local amount = packed[index]
        local deltas = packed[index + 1]
        local pair_id = 0
        for _, delta in ipairs(deltas) do
            pair_id = pair_id + delta
            local left = pair_id // glyph_count
            local right = pair_id % glyph_count
            local left_name = glyph_names[left + 1]
            local right_name = glyph_names[right + 1]
            if left_name == nil or left_name == "" or right_name == nil or right_name == "" then
                error("pdfkit: kern pair id " .. pair_id
                    .. " is outside " .. tostring(data.name))
            end
            self.kernPairs[left_name .. "\0" .. right_name] = amount
        end
        index = index + 2
    end

    return self
end

--------------------------------------------------------------------------------
-- Encoding and measurement
--------------------------------------------------------------------------------

-- Each code point as a hex string. Note the original does not zero-pad
-- (`char.toString(16)`), so a code below 0x10 yields a single digit. That is
-- faithful, quirk included -- the affected code points are control characters
-- that do not appear in text.
function AFMFont:encodeText(text)
    local res = {}
    for _, code in ipairs(codepoints(text)) do
        code = WIN_ANSI_MAP[code] or code
        res[#res + 1] = string.format("%x", code)
    end
    return res
end

function AFMFont:glyphsForString(str)
    local glyphs = {}
    for _, code in ipairs(codepoints(str)) do
        glyphs[#glyphs + 1] = self:characterToGlyph(code)
    end
    return glyphs
end

function AFMFont:characterToGlyph(character)
    local index = WIN_ANSI_MAP[character] or character
    return characters[index] or ".notdef"
end

function AFMFont:widthOfGlyph(glyph)
    return self.glyphWidths[glyph] or 0
end

function AFMFont:getKernPair(left, right)
    if left == nil or right == nil then
        return 0
    end
    return self.kernPairs[left .. "\0" .. right] or 0
end

-- The advance for each glyph is its own width plus the kern against whatever
-- follows it. The last glyph has no follower, so it gets no kern.
function AFMFont:advancesForGlyphs(glyphs)
    local advances = {}
    for index = 1, #glyphs do
        local left = glyphs[index]
        local right = glyphs[index + 1]
        advances[index] = self:widthOfGlyph(left) + self:getKernPair(left, right)
    end
    return advances
end

M.AFMFont = AFMFont
M.new = AFMFont.new

return M
