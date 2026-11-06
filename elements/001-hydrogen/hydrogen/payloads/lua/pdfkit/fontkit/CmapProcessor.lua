-- fontkit/CmapProcessor.lua -- port of fontkit/src/CmapProcessor.js
--
-- Character to glyph id. tables/cmap.lua decodes the subtables; this picks one
-- of them and does the lookups.
--
-- Picking is a preference list, not a search: a 32-bit Unicode subtable if the
-- font has one, then a 16-bit one, and only if neither exists does it fall back
-- to a legacy 8-bit subtable, in which case every lookup has to go through the
-- byte encoding on the way in. That fallback is why encodings.lua carries the
-- Mac script tables at all.

local encodings = require("pdfkit.fontkit.encodings")
local utils = require("pdfkit.fontkit.utils")

local binary_search = utils.binary_search
local range = utils.range
local extend = utils.extend

local CmapProcessor = {}
CmapProcessor.__index = CmapProcessor

-- Unicode subtables in the order they are preferred. The 32-bit ones come
-- first so a font that can address the astral planes does; (3,1) is the entry
-- almost every Windows font actually matches.
local UNICODE_PREFERENCE = {
    -- 32-bit subtables
    { 3, 10 },
    { 0, 6 },
    { 0, 4 },

    -- 16-bit subtables
    { 3, 1 },
    { 0, 3 },
    { 0, 2 },
    { 0, 1 },
    { 0, 0 },
}

--- The first subtable matching any of `pairs`, in the order given, or nil.
---
--- Note the loop nesting: the whole subtable list is scanned for the first
--- preferred pair before the second pair is considered at all, so preference
--- order beats the order the font happens to store its subtables in.
local function find_subtable(cmap_table, pairs_)
    for _, pair in ipairs(pairs_) do
        local platformID, encodingID = pair[1], pair[2]
        for _, cmap in ipairs(cmap_table.tables) do
            if cmap.platformID == platformID and cmap.encodingID == encodingID then
                return cmap.table
            end
        end
    end

    return nil
end

CmapProcessor.find_subtable = find_subtable

function CmapProcessor.new(cmap_table)
    local self = setmetatable({}, CmapProcessor)

    -- Attempt to find a Unicode cmap first.
    self.encoding = nil
    self.cmap = find_subtable(cmap_table, UNICODE_PREFERENCE)

    -- If no unicode cmap was found, take the first table with a supported
    -- encoding -- except that the loop does not break, so it is really the
    -- *last* such table that is kept. Reproduced: a font with two legacy
    -- subtables resolves through the later one either way, and picking the
    -- earlier would change which glyphs some old fonts map to.
    --
    -- `language - 1` looks wrong and is not: the mac language ids in the name
    -- table are zero-based, while a cmap subtable stores them one-based with
    -- zero meaning "language independent".
    if not self.cmap then
        for _, cmap in ipairs(cmap_table.tables) do
            local encoding = encodings.get_encoding(cmap.platformID, cmap.encodingID, cmap.table.language - 1)
            local mapping = encodings.get_encoding_mapping(encoding)
            if mapping then
                self.cmap = cmap.table
                self.encoding = mapping
            end
        end
    end

    if not self.cmap then
        error("Could not find a supported cmap table")
    end

    -- Variation sequences live in their own subtable at (0, 5). The version
    -- check is not paranoia: (0, 5) meant "ISO 10646 1993 semantics" in an
    -- older revision of the spec, and a font from that era has something else
    -- entirely sitting there.
    self.uvs = find_subtable(cmap_table, { { 0, 5 } })
    if self.uvs and self.uvs.version ~= 14 then
        self.uvs = nil
    end

    return self
end

--------------------------------------------------------------------------------
-- Lookup
--------------------------------------------------------------------------------

--- The glyph id for a code point, or 0 if the font does not cover it.
---
--- `variation_selector` is the code point of a following selector (U+FE00..,
--- U+E0100..), not an index.
function CmapProcessor:lookup(codepoint, variation_selector)
    local cmap = self.cmap

    if self.encoding then
        -- If there is no Unicode cmap in this font, we need to re-encode the
        -- codepoint in the encoding that the cmap supports. A code point the
        -- encoding has no byte for falls through unchanged, which for anything
        -- above 0xFF then misses the subtable and yields 0.
        codepoint = self.encoding[codepoint] or codepoint

        -- Otherwise, try to get a Unicode variation selector for this codepoint
        -- if one is provided.
    elseif variation_selector then
        local gid = self:get_variation_selector(codepoint, variation_selector)
        if gid ~= 0 then
            return gid
        end
    end

    local version = cmap.version

    if version == 0 then
        return cmap.codeMap:get(codepoint) or 0
    elseif version == 4 then
        -- Segments are sorted by end code, so this is a plain binary search for
        -- the segment containing the code point.
        local min, max = 0, cmap.segCount - 1
        while min <= max do
            local mid = (min + max) >> 1

            if codepoint < cmap.startCode:get(mid) then
                max = mid - 1
            elseif codepoint > cmap.endCode:get(mid) then
                min = mid + 1
            else
                local rangeOffset = cmap.idRangeOffset:get(mid)
                local gid

                if rangeOffset == 0 then
                    -- The whole segment shifts by one constant.
                    gid = codepoint + cmap.idDelta:get(mid)
                else
                    -- The segment indexes into the trailing glyph array. The
                    -- offset is stored in bytes from its own slot, which is why
                    -- the segment's distance from the end of idRangeOffset has
                    -- to be subtracted back out.
                    local index = rangeOffset // 2 + (codepoint - cmap.startCode:get(mid)) - (cmap.segCount - mid)
                    gid = cmap.glyphIndexArray:get(index) or 0
                    if gid ~= 0 then
                        gid = gid + cmap.idDelta:get(mid)
                    end
                end

                -- idDelta is added modulo 65536 by the spec, and can be
                -- negative, so the mask is doing real work.
                return gid & 0xFFFF
            end
        end

        return 0
    elseif version == 8 then
        error("TODO: cmap format 8")
    elseif version == 6 or version == 10 then
        return cmap.glyphIndices:get(codepoint - cmap.firstCode) or 0
    elseif version == 12 or version == 13 then
        local min, max = 0, cmap.nGroups - 1
        while min <= max do
            local mid = (min + max) >> 1
            local group = cmap.groups:get(mid)

            if codepoint < group.startCharCode then
                max = mid - 1
            elseif codepoint > group.endCharCode then
                min = mid + 1
            else
                if version == 12 then
                    -- Format 12: the run of characters maps to a run of ids.
                    return group.glyphID + (codepoint - group.startCharCode)
                else
                    -- Format 13: the whole run maps to the one id. Used for
                    -- "last resort" fonts, where every character gets the same
                    -- box glyph.
                    return group.glyphID
                end
            end
        end

        return 0
    elseif version == 14 then
        error("TODO: cmap format 14")
    else
        error("Unknown cmap format " .. tostring(version))
    end
end

--- The glyph for a base character followed by a variation selector, or 0.
---
--- Two tables per selector: the default set, meaning "the ordinary glyph is
--- already right", and the non-default set, which names a specific glyph.
function CmapProcessor:get_variation_selector(codepoint, variation_selector)
    if not self.uvs then
        return 0
    end

    local selectors = self.uvs.varSelectors:toArray()
    local i = binary_search(selectors, function(x) return variation_selector - x.varSelector end)
    local sel = i and selectors[i]

    if i and sel.defaultUVS then
        i = binary_search(sel.defaultUVS, function(x)
            if codepoint < x.startUnicodeValue then
                return -1
            elseif codepoint > x.startUnicodeValue + x.additionalCount then
                return 1
            else
                return 0
            end
        end)
    end

    -- `i` has been overwritten by the search above, which is what makes the
    -- default set gate the non-default one. A code point in the default set
    -- reaches this search and, finding nothing, returns 0 -- correct, since
    -- "default" means the unadorned glyph. A code point in *neither* set also
    -- returns 0. A code point in the non-default set but not the default set
    -- returns 0 as well, which is an upstream bug; it is reproduced rather
    -- than fixed, because fixing it would change which glyph such a font
    -- renders and there is nothing to check the new answer against.
    if i and sel.nonDefaultUVS then
        i = binary_search(sel.nonDefaultUVS, function(x) return codepoint - x.unicodeValue end)
        if i then
            return sel.nonDefaultUVS[i].glyphID
        end
    end

    return 0
end

--------------------------------------------------------------------------------
-- Coverage
--------------------------------------------------------------------------------

--- Every code point the selected subtable maps, as a one-based array.
---
--- Cached, because it walks the whole cmap and the result is thousands of
--- entries for a text font. Not deduplicated or sorted -- a format 4 cmap with
--- overlapping segments would list a code point twice, as upstream does.
function CmapProcessor:get_character_set()
    if self._characterSet then
        return self._characterSet
    end

    local cmap = self.cmap
    local version = cmap.version
    local res

    if version == 0 then
        res = range(0, cmap.codeMap.length)
    elseif version == 4 then
        res = {}
        local endCodes = cmap.endCode:toArray()
        for i = 1, #endCodes do
            local tail = endCodes[i] + 1
            local start = cmap.startCode:get(i - 1)
            extend(res, range(start, tail))
        end
    elseif version == 8 then
        error("TODO: cmap format 8")
    elseif version == 6 or version == 10 then
        res = range(cmap.firstCode, cmap.firstCode + cmap.glyphIndices.length)
    elseif version == 12 or version == 13 then
        res = {}
        for _, group in ipairs(cmap.groups:toArray()) do
            extend(res, range(group.startCharCode, group.endCharCode + 1))
        end
    elseif version == 14 then
        error("TODO: cmap format 14")
    else
        error("Unknown cmap format " .. tostring(version))
    end

    self._characterSet = res
    return res
end

--- Every code point that maps to `gid`, as a one-based array.
---
--- The reverse of lookup, and there is no index for it, so this rebuilds the
--- forward mapping and filters. Cached per glyph id. Used to write the
--- ToUnicode CMap of an embedded subset, where a glyph reached through shaping
--- has no character of its own to point back at.
---
--- Formats 6, 8, 10 and 14 raise here where lookup handles them, matching
--- upstream -- the switch simply has fewer arms.
function CmapProcessor:code_points_for_glyph(gid)
    local cache = self._codePointsForGlyph
    if not cache then
        cache = {}
        self._codePointsForGlyph = cache
    end
    local hit = cache[gid]
    if hit then
        return hit
    end

    local cmap = self.cmap
    local version = cmap.version
    local res

    if version == 0 then
        res = {}
        for i = 0, 255 do
            if cmap.codeMap:get(i) == gid then
                res[#res + 1] = i
            end
        end
    elseif version == 4 then
        res = {}
        for i = 0, cmap.segCount - 1 do
            local finish = cmap.endCode:get(i)
            local start = cmap.startCode:get(i)
            local rangeOffset = cmap.idRangeOffset:get(i)
            local delta = cmap.idDelta:get(i)

            -- The same arithmetic as the format 4 arm of lookup, run forwards
            -- over every code point in the segment.
            for c = start, finish do
                local g
                if rangeOffset == 0 then
                    g = c + delta
                else
                    local index = rangeOffset // 2 + (c - start) - (cmap.segCount - i)
                    g = cmap.glyphIndexArray:get(index) or 0
                    if g ~= 0 then
                        g = g + delta
                    end
                end

                if g == gid then
                    res[#res + 1] = c
                end
            end
        end
    elseif version == 12 then
        res = {}
        for _, group in ipairs(cmap.groups:toArray()) do
            if gid >= group.glyphID and gid <= group.glyphID + (group.endCharCode - group.startCharCode) then
                res[#res + 1] = group.startCharCode + (gid - group.glyphID)
            end
        end
    elseif version == 13 then
        res = {}
        for _, group in ipairs(cmap.groups:toArray()) do
            if gid == group.glyphID then
                extend(res, range(group.startCharCode, group.endCharCode + 1))
            end
        end
    else
        error("Unknown cmap format " .. tostring(version))
    end

    cache[gid] = res
    return res
end

return CmapProcessor
