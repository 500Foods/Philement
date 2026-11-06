-- fontkit/tables/name.lua -- port of fontkit/src/tables/name.js
--
-- The font's human-readable strings. Physically it is a list of records, each
-- tagged with a platform, an encoding, a language and a name id, all pointing
-- into one string pool at the end of the table. A font typically carries every
-- string twice -- once as Macintosh/Roman and once as Windows/UTF-16 -- and often
-- in several languages on top of that.
--
-- decode() reshapes that into records[name][language], which is what getName
-- reads. pdfkit wants exactly one of them, postscriptName, but the reshaping is
-- where the ordering rules live, so it is ported whole.
--
-- Two of those rules are easy to miss. A record whose encoding could not be
-- decoded is stored, but any record that did decode displaces it -- see the note
-- on decode() in encodings.lua. And name ids from 256 up are not names at all,
-- they are Apple layout feature labels, which get bucketed under `fontFeatures`
-- and keyed by id.

local reader = require("pdfkit.fontkit.reader")
local encodings = require("pdfkit.fontkit.encodings")

local M = {}

-- Name ids 0..22. Index 15 is reserved and has no name; ids past the end of this
-- list are kept under their number.
local NAMES = {
    [0] = "copyright",
    "fontFamily",
    "fontSubfamily",
    "uniqueSubfamily",
    "fullName",
    "version",
    "postscriptName", -- a font has only one, and the spec requires it to be ASCII
    "trademark",
    "manufacturer",
    "designer",
    "description",
    "vendorURL",
    "designerURL",
    "license",
    "licenseURL",
    false, -- reserved
    "preferredFamily",
    "preferredSubfamily",
    "compatibleFull",
    "sampleText",
    "postscriptCIDFontName",
    "wwsFamilyName",
    "wwsSubfamilyName",
}

function M.decode(r)
    local start = r.pos
    local t = {}
    t.version = r:uint16()
    t.count = r:uint16()
    t.stringOffset = r:uint16()

    local records = {}
    for i = 1, t.count do
        local rec = {}
        rec.platformID = r:uint16()
        rec.encodingID = r:uint16()
        rec.languageID = r:uint16()
        rec.nameID = r:uint16()
        rec.length = r:uint16()
        local offset = r:uint16()

        -- The offset is relative to the string pool, which is itself relative to
        -- the start of the table.
        local bytes = r.data:sub(
            start + t.stringOffset + offset + 1,
            start + t.stringOffset + offset + rec.length
        )
        local enc = encodings.get_encoding(rec.platformID, rec.encodingID, rec.languageID)
        rec.string, rec.is_string = encodings.decode(bytes, enc)

        records[i] = rec
    end
    t.records = records

    -- Version 1 adds a table of language tags for languages that have no id.
    if t.version == 1 then
        t.langTagCount = r:uint16()
        local langTags = {}
        for i = 1, t.langTagCount do
            local length = r:uint16()
            local offset = r:uint16()
            langTags[i] = {
                length = length,
                tag = reader.decode_utf16be(r.data:sub(
                    start + t.stringOffset + offset + 1,
                    start + t.stringOffset + offset + length
                )),
            }
        end
        t.langTags = langTags
    end

    M.process(t)
    return t
end

--- Reshape the flat record list into records[name][language].
function M.process(t)
    local records = {}

    -- The order the languages were first seen in, per record table. getName's
    -- last resort is "any language at all", which upstream writes as
    -- Object.keys(record)[0] -- the first key inserted. Lua's pairs() has no
    -- order to speak of, so the order is recorded here rather than recovered.
    local langOrder = {}

    -- Which slots hold a decoded string, keyed by the table the slot is in. Kept
    -- to one side so the shape of `records` stays exactly what a caller expects.
    -- In JS this is just `typeof obj[language] === 'string'`, but here both a
    -- decoded string and undecoded bytes are Lua strings and cannot be told apart
    -- after the fact.
    local decoded = setmetatable({}, { __mode = "k" })

    for _, record in ipairs(t.records) do
        -- Work out what language this is for. Ids at 0x8000 and above index the
        -- version 1 language tag table; anything still unmatched keeps a
        -- "<platform>-<language>" placeholder so the record is not lost.
        local langs = encodings.LANGUAGES[record.platformID]
        local language = langs and langs[record.languageID]

        if language == nil and t.langTags and record.languageID >= 0x8000 then
            local tag = t.langTags[record.languageID - 0x8000 + 1]
            language = tag and tag.tag
        end

        if language == nil then
            language = record.platformID .. "-" .. record.languageID
        end

        local name = NAMES[record.nameID]
        local key
        if record.nameID >= 256 then
            key = "fontFeatures"
        elseif name then
            key = name
        else
            key = record.nameID -- reserved or beyond the list: keep the number
        end

        if records[key] == nil then
            records[key] = {}
        end

        local obj = records[key]
        if record.nameID >= 256 then
            if obj[record.nameID] == nil then
                obj[record.nameID] = {}
            end
            obj = obj[record.nameID]
        end

        -- A decoded string always wins. Undecoded bytes only fill a slot that is
        -- empty or itself undecoded.
        local flags = decoded[obj]
        if not flags then
            flags = {}
            decoded[obj] = flags
        end

        if record.is_string or not flags[language] then
            if obj[language] == nil then
                local order = langOrder[obj]
                if not order then
                    order = {}
                    langOrder[obj] = order
                end
                order[#order + 1] = language
            end

            obj[language] = record.string
            flags[language] = record.is_string
        end
    end

    t.records = records
    t.langOrder = langOrder
end

M.NAMES = NAMES

return M
