-- fontkit/layout/Script.lua -- port of fontkit/src/layout/Script.js
--
-- The bridge between Unicode's Script property and OpenType's four-character
-- script tags, plus the writing direction of each.
--
-- Two spellings of the same thing, because the two standards were written by
-- different committees for different purposes: Unicode names a script
-- "Devanagari", OpenType calls it "deva", and a handful of Indic scripts have
-- *two* OpenType tags -- an original and a "version 2" reworking with different
-- shaping rules. Where both exist the table holds them in preference order,
-- newest first, and callers try them in turn.
--
-- Data from the OpenType script tag registry and PropertyValueAliases.txt.

local unicode = require("pdfkit.fontkit.unicode")
local utils = require("pdfkit.fontkit.utils")

local M = {}

--- Unicode script name -> OpenType tag, or a list of tags in preference order.
---
--- Four of the tags are padded with trailing spaces. OpenType tags are exactly
--- four bytes and these scripts have shorter names, so the padding is part of
--- the tag and not a typo -- "lao " will not match "lao".
local UNICODE_SCRIPTS = {
    Caucasian_Albanian = "aghb",
    Arabic = "arab",
    Imperial_Aramaic = "armi",
    Armenian = "armn",
    Avestan = "avst",
    Balinese = "bali",
    Bamum = "bamu",
    Bassa_Vah = "bass",
    Batak = "batk",
    Bengali = { "bng2", "beng" },
    Bopomofo = "bopo",
    Brahmi = "brah",
    Braille = "brai",
    Buginese = "bugi",
    Buhid = "buhd",
    Chakma = "cakm",
    Canadian_Aboriginal = "cans",
    Carian = "cari",
    Cham = "cham",
    Cherokee = "cher",
    Coptic = "copt",
    Cypriot = "cprt",
    Cyrillic = "cyrl",
    Devanagari = { "dev2", "deva" },
    Deseret = "dsrt",
    Duployan = "dupl",
    Egyptian_Hieroglyphs = "egyp",
    Elbasan = "elba",
    Ethiopic = "ethi",
    Georgian = "geor",
    Glagolitic = "glag",
    Gothic = "goth",
    Grantha = "gran",
    Greek = "grek",
    Gujarati = { "gjr2", "gujr" },
    Gurmukhi = { "gur2", "guru" },
    Hangul = "hang",
    Han = "hani",
    Hanunoo = "hano",
    Hebrew = "hebr",
    Hiragana = "hira",
    Pahawh_Hmong = "hmng",
    Katakana_Or_Hiragana = "hrkt",
    Old_Italic = "ital",
    Javanese = "java",
    Kayah_Li = "kali",
    Katakana = "kana",
    Kharoshthi = "khar",
    Khmer = "khmr",
    Khojki = "khoj",
    Kannada = { "knd2", "knda" },
    Kaithi = "kthi",
    Tai_Tham = "lana",
    Lao = "lao ",
    Latin = "latn",
    Lepcha = "lepc",
    Limbu = "limb",
    Linear_A = "lina",
    Linear_B = "linb",
    Lisu = "lisu",
    Lycian = "lyci",
    Lydian = "lydi",
    Mahajani = "mahj",
    Mandaic = "mand",
    Manichaean = "mani",
    Mende_Kikakui = "mend",
    Meroitic_Cursive = "merc",
    Meroitic_Hieroglyphs = "mero",
    Malayalam = { "mlm2", "mlym" },
    Modi = "modi",
    Mongolian = "mong",
    Mro = "mroo",
    Meetei_Mayek = "mtei",
    Myanmar = { "mym2", "mymr" },
    Old_North_Arabian = "narb",
    Nabataean = "nbat",
    Nko = "nko ",
    Ogham = "ogam",
    Ol_Chiki = "olck",
    Old_Turkic = "orkh",
    Oriya = { "ory2", "orya" },
    Osmanya = "osma",
    Palmyrene = "palm",
    Pau_Cin_Hau = "pauc",
    Old_Permic = "perm",
    Phags_Pa = "phag",
    Inscriptional_Pahlavi = "phli",
    Psalter_Pahlavi = "phlp",
    Phoenician = "phnx",
    Miao = "plrd",
    Inscriptional_Parthian = "prti",
    Rejang = "rjng",
    Runic = "runr",
    Samaritan = "samr",
    Old_South_Arabian = "sarb",
    Saurashtra = "saur",
    Shavian = "shaw",
    Sharada = "shrd",
    Siddham = "sidd",
    Khudawadi = "sind",
    Sinhala = "sinh",
    Sora_Sompeng = "sora",
    Sundanese = "sund",
    Syloti_Nagri = "sylo",
    Syriac = "syrc",
    Tagbanwa = "tagb",
    Takri = "takr",
    Tai_Le = "tale",
    New_Tai_Lue = "talu",
    Tamil = { "tml2", "taml" },
    Tai_Viet = "tavt",
    Telugu = { "tel2", "telu" },
    Tifinagh = "tfng",
    Tagalog = "tglg",
    Thaana = "thaa",
    Thai = "thai",
    Tibetan = "tibt",
    Tirhuta = "tirh",
    Ugaritic = "ugar",
    Vai = "vai ",
    Warang_Citi = "wara",
    Old_Persian = "xpeo",
    Cuneiform = "xsux",
    Yi = "yi  ",
    Inherited = "zinh",
    Common = "zyyy",
    Unknown = "zzzz",
}

--- The same table read backwards. Built at load time rather than written out,
--- so the two can never drift apart. Where a script has two tags, both map back
--- to the one Unicode name.
local OPENTYPE_SCRIPTS = {}
for script, tag in pairs(UNICODE_SCRIPTS) do
    if type(tag) == "table" then
        for _, t in ipairs(tag) do
            OPENTYPE_SCRIPTS[t] = script
        end
    else
        OPENTYPE_SCRIPTS[tag] = script
    end
end

--- The OpenType tag(s) for a Unicode script name. May be a list -- see above.
function M.fromUnicode(script)
    return UNICODE_SCRIPTS[script]
end

--- The Unicode script name for an OpenType tag.
function M.fromOpenType(tag)
    return OPENTYPE_SCRIPTS[tag]
end

--- The script of a run of code points: the first one that belongs to a real
--- script wins.
---
--- Common, Inherited and Unknown are skipped. Those cover punctuation, digits,
--- spaces and combining marks -- characters that take their script from
--- whatever they are sitting next to -- so a string like `"(hello)"` must be
--- decided by the `h`, not by the bracket that came first.
local function scan(codePoints)
    for _, codePoint in ipairs(codePoints) do
        local script = unicode.get_script(codePoint)
        if script ~= "Common" and script ~= "Inherited" and script ~= "Unknown" then
            return UNICODE_SCRIPTS[script]
        end
    end

    return UNICODE_SCRIPTS.Unknown
end

--- The script of a string.
---
--- Upstream walks UTF-16 code units and reassembles surrogate pairs by hand;
--- here the string is UTF-8 and utils.code_points does the decoding, which
--- comes to the same sequence of code points.
function M.forString(str)
    return scan(utils.code_points(str))
end

function M.forCodePoints(codePoints)
    return scan(codePoints)
end

--- The scripts written right to left.
local RTL = {
    arab = true,    -- Arabic
    hebr = true,    -- Hebrew
    syrc = true,    -- Syriac
    thaa = true,    -- Thaana
    cprt = true,    -- Cypriot Syllabary
    khar = true,    -- Kharosthi
    phnx = true,    -- Phoenician
    ["nko "] = true, -- N'Ko
    lydi = true,    -- Lydian
    avst = true,    -- Avestan
    armi = true,    -- Imperial Aramaic
    phli = true,    -- Inscriptional Pahlavi
    prti = true,    -- Inscriptional Parthian
    sarb = true,    -- Old South Arabian
    orkh = true,    -- Old Turkic, Orkhon Runic
    samr = true,    -- Samaritan
    mand = true,    -- Mandaic, Mandaean
    merc = true,    -- Meroitic Cursive
    mero = true,    -- Meroitic Hieroglyphs

    -- Unicode 7.0, which the OpenType registry had not caught up with.
    mani = true,    -- Manichaean
    mend = true,    -- Mende Kikakui
    nbat = true,    -- Nabataean
    narb = true,    -- Old North Arabian
    palm = true,    -- Palmyrene
    phlp = true,    -- Psalter Pahlavi
}

--- "rtl" or "ltr" for a script tag.
---
--- A script with two tags arrives here as a list, which is not a key in RTL and
--- so falls through to "ltr". That is upstream's behaviour and it happens to be
--- right: every script with a version 2 tag is an Indic one, written left to
--- right.
function M.direction(script)
    if RTL[script] then
        return "rtl"
    end

    return "ltr"
end

return M
