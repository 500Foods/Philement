-- fontkit/opentype/shapers/init.lua
--   -- port of fontkit/src/opentype/shapers/index.js
--
-- Which shaper handles which script.
--
-- Upstream has four: the default one, plus Arabic, Hangul and Indic/Universal.
-- This port has the default one and the Arabic one. Indic and Universal carry
-- syllable grammars this port does not, and Hangul composes jamo into blocks.
-- A script that needs one of those is refused rather than approximated: the
-- default shaper would emit isolated letters and it would look like a broken
-- font. An error at the point of choosing says what is actually wrong.
--
-- Latin, Greek, Cyrillic, Armenian, Georgian, Hebrew, Thai, Lao, CJK and
-- everything else not listed below all shape through the default shaper here
-- as they do upstream. Hebrew is right-to-left and still uses that shaper;
-- the layout engine reverses the run. Arabic and the other joining scripts
-- use ArabicShaper.

local DefaultShaper = require("pdfkit.fontkit.opentype.shapers.DefaultShaper")
local ArabicShaper = require("pdfkit.fontkit.opentype.shapers.ArabicShaper")

-- The scripts upstream routes to a shaper this port does not have, and the name
-- of the shaper it routes them to. Kept as data rather than dropped so the error
-- can say which one is missing.
local UNSUPPORTED = {
    -- HangulShaper: composes jamo into syllable blocks.
    hang = "HangulShaper",      -- Hangul

    -- IndicShaper: syllable analysis and glyph reordering. Each script has an
    -- old tag and a version 2 tag; a font may carry rules for either.
    bng2 = "IndicShaper",       -- Bengali
    beng = "IndicShaper",       -- Bengali
    dev2 = "IndicShaper",       -- Devanagari
    deva = "IndicShaper",       -- Devanagari
    gjr2 = "IndicShaper",       -- Gujarati
    gujr = "IndicShaper",       -- Gujarati
    guru = "IndicShaper",       -- Gurmukhi
    gur2 = "IndicShaper",       -- Gurmukhi
    knda = "IndicShaper",       -- Kannada
    knd2 = "IndicShaper",       -- Kannada
    mlm2 = "IndicShaper",       -- Malayalam
    mlym = "IndicShaper",       -- Malayalam
    ory2 = "IndicShaper",       -- Oriya
    orya = "IndicShaper",       -- Oriya
    taml = "IndicShaper",       -- Tamil
    tml2 = "IndicShaper",       -- Tamil
    telu = "IndicShaper",       -- Telugu
    tel2 = "IndicShaper",       -- Telugu
    khmr = "IndicShaper",       -- Khmer
    sinh = "IndicShaper",       -- Sinhala

    -- UniversalShaper: the Universal Shaping Engine, for Brahmi-derived and
    -- other complex scripts that have no shaper of their own.
    bali = "UniversalShaper",   -- Balinese
    batk = "UniversalShaper",   -- Batak
    brah = "UniversalShaper",   -- Brahmi
    bugi = "UniversalShaper",   -- Buginese
    buhd = "UniversalShaper",   -- Buhid
    cakm = "UniversalShaper",   -- Chakma
    cham = "UniversalShaper",   -- Cham
    dupl = "UniversalShaper",   -- Duployan
    egyp = "UniversalShaper",   -- Egyptian Hieroglyphs
    gran = "UniversalShaper",   -- Grantha
    hano = "UniversalShaper",   -- Hanunoo
    java = "UniversalShaper",   -- Javanese
    kthi = "UniversalShaper",   -- Kaithi
    kali = "UniversalShaper",   -- Kayah Li
    khar = "UniversalShaper",   -- Kharoshthi
    khoj = "UniversalShaper",   -- Khojki
    sind = "UniversalShaper",   -- Khudawadi
    lepc = "UniversalShaper",   -- Lepcha
    limb = "UniversalShaper",   -- Limbu
    mahj = "UniversalShaper",   -- Mahajani
    mtei = "UniversalShaper",   -- Meitei Mayek
    modi = "UniversalShaper",   -- Modi
    hmng = "UniversalShaper",   -- Pahawh Hmong
    rjng = "UniversalShaper",   -- Rejang
    saur = "UniversalShaper",   -- Saurashtra
    shrd = "UniversalShaper",   -- Sharada
    sidd = "UniversalShaper",   -- Siddham
    sund = "UniversalShaper",   -- Sundanese
    sylo = "UniversalShaper",   -- Syloti Nagri
    tglg = "UniversalShaper",   -- Tagalog
    tagb = "UniversalShaper",   -- Tagbanwa
    tale = "UniversalShaper",   -- Tai Le
    lana = "UniversalShaper",   -- Tai Tham
    tavt = "UniversalShaper",   -- Tai Viet
    takr = "UniversalShaper",   -- Takri
    tibt = "UniversalShaper",   -- Tibetan
    tfng = "UniversalShaper",   -- Tifinagh
    tirh = "UniversalShaper",   -- Tirhuta
}

-- The scripts this port does shape. Upstream lists exactly these two and lets
-- everything unlisted fall through to the same shaper, which is what happens
-- below -- the table is here for symmetry with the source, not to gate anything.
local SHAPERS = {
    latn = DefaultShaper,   -- Latin
    DFLT = DefaultShaper,   -- Default

    -- ArabicShaper: cursive scripts that join letter to letter.
    arab = ArabicShaper,      -- Arabic
    mong = ArabicShaper,      -- Mongolian
    syrc = ArabicShaper,      -- Syriac
    ["nko "] = ArabicShaper,  -- N'Ko
    phag = ArabicShaper,      -- Phags Pa
    mand = ArabicShaper,      -- Mandaic
    mani = ArabicShaper,      -- Manichaean
    phlp = ArabicShaper,      -- Psalter Pahlavi
}

local M = {}

--- Pick a shaper for a script tag, or for the first of several tags.
---
--- The argument is a list when a script has both an old and a version 2 tag; the
--- first that a shaper is registered for wins, as upstream.
function M.choose(script)
    if type(script) ~= "table" then
        script = { script }
    end

    for _, s in ipairs(script) do
        local shaper = SHAPERS[s]
        if shaper then
            return shaper
        end
    end

    -- Nothing registered. Upstream would return the default shaper here; this
    -- port first checks whether the script is one whose shaper is missing rather
    -- than absent, and says so instead of laying the text out wrongly.
    for _, s in ipairs(script) do
        local missing = UNSUPPORTED[s]
        if missing then
            error(string.format(
                "This port does not implement %s, which the script %q requires. "
                .. "Text in this script cannot be laid out correctly.",
                missing, s))
        end
    end

    return DefaultShaper
end

return M
