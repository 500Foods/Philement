-- fontkit/opentype/GlyphIterator.lua
--   -- port of fontkit/src/opentype/GlyphIterator.js
--
-- A cursor over the glyph run that can skip glyphs.
--
-- This is what makes lookup flags work. A kern lookup with `ignoreMarks` set
-- wants to see `A` next to `V` even when the text is `Á V`; the accent is still
-- in the run, but this iterator steps straight over it, so the lookup matches a
-- pair that is not adjacent. Everything a lookup does to find its neighbours
-- goes through here, which is why the flags live on the iterator rather than
-- being consulted at each rule.
--
-- Indices are one-based, like the glyph array they point into. Out of range is
-- `#glyphs + 1` at the top and `0` at the bottom, and both are reachable: a
-- lookup that runs off the end of the run is normal, not an error.

local GlyphIterator = {}
GlyphIterator.__index = GlyphIterator

function GlyphIterator.new(glyphs, options)
    local self = setmetatable({ glyphs = glyphs }, GlyphIterator)
    self:reset(options)
    return self
end

--- Point at `index` (default: the first glyph) with a new set of flags.
---
--- Called once per lookup, and again around every nested lookup, because a
--- contextual rule invokes other lookups whose flags differ from its own.
function GlyphIterator:reset(options, index)
    options = options or {}
    self.options = options
    self.flags = options.flags or {}
    self.markAttachmentType = options.markAttachmentType or 0
    self.index = index or 1
end

--- The glyph under the cursor, or nil past either end.
function GlyphIterator:cur()
    return self.glyphs[self.index]
end

--- Should this lookup pretend the glyph is not there?
---
--- The last clause is the mark filtering one: a lookup can name a single mark
--- attachment class and ignore every mark that is not in it, which is how a font
--- positions the marks above a letter without the ones below getting in the way.
function GlyphIterator:shouldIgnore(glyph)
    return (self.flags.ignoreMarks and glyph.isMark)
        or (self.flags.ignoreBaseGlyphs and glyph.isBase)
        or (self.flags.ignoreLigatures and glyph.isLigature)
        or (self.markAttachmentType ~= 0 and glyph.isMark
            and glyph.markAttachmentType ~= self.markAttachmentType)
        or false
end

--- Step one glyph in direction `dir`, skipping any this lookup ignores.
---
--- Returns the glyph landed on, or nil if that was off the end. The index is
--- left off the end in that case rather than being clamped, which is what lets
--- the caller in `applyLookups` use `index > #glyphs` as its loop condition.
function GlyphIterator:move(dir)
    self.index = self.index + dir
    while 1 <= self.index and self.index <= #self.glyphs
        and self:shouldIgnore(self.glyphs[self.index]) do
        self.index = self.index + dir
    end

    if self.index < 1 or self.index > #self.glyphs then
        return nil
    end

    return self.glyphs[self.index]
end

function GlyphIterator:next()
    return self:move(1)
end

function GlyphIterator:prev()
    return self:move(-1)
end

--- What `increment` would land on, without moving.
function GlyphIterator:peek(count)
    local idx = self.index
    local res = self:increment(count)
    self.index = idx
    return res
end

--- Where `increment` would land, without moving.
function GlyphIterator:peekIndex(count)
    local idx = self.index
    self:increment(count)
    local res = self.index
    self.index = idx
    return res
end

--- Step `count` glyphs, in either direction; a negative count goes backwards.
---
--- Not the same as adding `count` to the index, because each step skips ignored
--- glyphs -- "three glyphs forward" means three glyphs this lookup can see.
--- Zero is a real argument and means "stay put", which contextual rules use to
--- apply a nested lookup to the glyph they are already on.
function GlyphIterator:increment(count)
    count = count or 1
    local dir = count < 0 and -1 or 1
    count = math.abs(count)

    while count > 0 do
        self:move(dir)
        count = count - 1
    end

    return self.glyphs[self.index]
end

return GlyphIterator
