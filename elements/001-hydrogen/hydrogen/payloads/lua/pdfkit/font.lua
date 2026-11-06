-- font.lua -- port of pdfkit/lib/font.js
--
-- The base every font kind inherits from. Small, but two of its four methods
-- are load bearing for byte-identical output.
--
-- ref() creates the font's indirect object ON FIRST USE, not when the font is
-- loaded. That matters more than it sounds: PDFKit loads Helvetica during
-- construction, before the first page exists, yet the font dictionary is
-- numbered *after* the page objects because nothing referenced it until text
-- was drawn. Create the object eagerly and every subsequent object number
-- shifts by one.
--
-- finalize() is guarded so that a font registered under several keys -- its
-- cache key and its PostScript name, typically the same font twice -- embeds
-- exactly once.

local Font = {}
Font.__index = Font

-- Subclasses call this to get a metatable that inherits the shared behaviour.
function Font.subclass()
    local cls = {}
    cls.__index = cls
    setmetatable(cls, { __index = Font })
    return cls
end

function Font:encode()
    error("must be implemented by subclasses")
end

function Font:widthOfString()
    error("must be implemented by subclasses")
end

-- The font's indirect object, created lazily on first reference.
function Font:ref()
    if self.dictionary == nil then
        self.dictionary = self.document:ref()
    end
    return self.dictionary
end

function Font:finalize()
    if self.embedded or self.dictionary == nil then
        return
    end
    self:embed()
    self.embedded = true
end

function Font:embed()
    error("must be implemented by subclasses")
end

-- Distance between baselines at a given size. The gap is optional because
-- PDFKit only includes it when asked, and text layout depends on which.
function Font:lineHeight(size, includeGap)
    local gap = includeGap and self.lineGap or 0
    return ((self.ascender + gap - self.descender) / 1000) * size
end

return Font
