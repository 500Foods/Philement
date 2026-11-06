-- fontkit/opentype/OTLayoutEngine.lua
--   -- port of fontkit/src/opentype/OTLayoutEngine.js
--
-- The engine LayoutEngine delegates to when a font has GSUB or GPOS. It owns the
-- two processors and the shaping plan, and it is what turns a run of glyphs into
-- a run of shaped, positioned glyphs.
--
-- The four calls LayoutEngine makes, in order:
--
--   setup       wrap each glyph in a GlyphInfo, pick a script, choose a shaper,
--               and let the shaper build the plan.
--   substitute  run the plan against GSUB, then map the results back to real
--               Glyph objects.
--   position    run the plan against GPOS, zero the mark advances, and reverse
--               the run if it is right to left.
--   cleanup     drop the per-run state.
--
-- The GlyphInfo indirection in `setup` is the reason for that class: a Glyph is
-- shared and cached by the font, and shaping needs somewhere to write per-run
-- state -- what this glyph's ligature id is, which features apply to it --
-- without that state leaking into the next run that uses the same glyph.

local ShapingPlan = require("pdfkit.fontkit.opentype.ShapingPlan")
local Shapers = require("pdfkit.fontkit.opentype.shapers")
local GlyphInfo = require("pdfkit.fontkit.opentype.GlyphInfo")
local GSUBProcessor = require("pdfkit.fontkit.opentype.GSUBProcessor")
local GPOSProcessor = require("pdfkit.fontkit.opentype.GPOSProcessor")

local OTLayoutEngine = {}
OTLayoutEngine.__index = OTLayoutEngine

function OTLayoutEngine.new(font)
    local self = setmetatable({}, OTLayoutEngine)

    self.font = font
    self.glyphInfos = nil
    self.plan = nil
    self.GSUBProcessor = nil
    self.GPOSProcessor = nil

    -- A font with GPOS may still have no mark positioning in it, so the Unicode
    -- fallback stays available. LayoutEngine decides, from what `position`
    -- reports back.
    self.fallbackPosition = true

    if font.GSUB then
        self.GSUBProcessor = GSUBProcessor.new(font, font.GSUB)
    end

    if font.GPOS then
        self.GPOSProcessor = GPOSProcessor.new(font, font.GPOS)
    end

    return self
end

--- Prepare a run: glyph infos, script, shaper, plan.
function OTLayoutEngine:setup(glyphRun)
    -- Map glyphs to GlyphInfo objects so data can be passed between
    -- GSUB and GPOS without mutating the real (shared) Glyph objects.
    self.glyphInfos = {}
    for i, glyph in ipairs(glyphRun.glyphs) do
        local codePoints = {}
        for j, cp in ipairs(glyph.codePoints) do
            codePoints[j] = cp
        end
        self.glyphInfos[i] = GlyphInfo.new(self.font, glyph.id, codePoints)
    end

    -- Select a script based on what is available in GSUB/GPOS.
    --
    -- Both are asked, and GSUB's answer wins: the two tables can support
    -- different sets of scripts, and it is the substitutions that decide what
    -- the text turns into.
    local script = nil
    if self.GPOSProcessor then
        script = self.GPOSProcessor:selectScript(
            glyphRun.script, glyphRun.language, glyphRun.direction)
    end

    if self.GSUBProcessor then
        script = self.GSUBProcessor:selectScript(
            glyphRun.script, glyphRun.language, glyphRun.direction)
    end

    -- Choose a shaper based on the script, and setup a shaping plan.
    -- This determines which features to apply to which glyphs.
    self.shaper = Shapers.choose(script)
    self.plan = ShapingPlan.new(self.font, script, glyphRun.direction)
    self.shaper:plan(self.plan, self.glyphInfos, glyphRun.features)

    -- Assign chosen features to output glyph run
    for key in pairs(self.plan.allFeatures) do
        glyphRun.features[key] = true
    end
end

--- Run the substitution half of the plan.
function OTLayoutEngine:substitute(glyphRun)
    if self.GSUBProcessor then
        self.plan:process(self.GSUBProcessor, self.glyphInfos)

        -- Map glyph infos back to normal Glyph objects
        local glyphs = {}
        for i, glyphInfo in ipairs(self.glyphInfos) do
            glyphs[i] = self.font:getGlyph(glyphInfo.id, glyphInfo.codePoints)
        end
        glyphRun.glyphs = glyphs
    end
end

--- Run the positioning half, and report which features were applied.
---
--- The return value is the GPOS processor's feature map, which LayoutEngine
--- tests for `kern` to decide whether the old kern table is still needed. Nil
--- when there is no GPOS at all, which is the same signal one step stronger.
function OTLayoutEngine:position(glyphRun)
    if self.shaper.zeroMarkWidths == "BEFORE_GPOS" then
        self:zeroMarkAdvances(glyphRun.positions)
    end

    if self.GPOSProcessor then
        self.plan:process(self.GPOSProcessor, self.glyphInfos, glyphRun.positions)
    end

    if self.shaper.zeroMarkWidths == "AFTER_GPOS" then
        self:zeroMarkAdvances(glyphRun.positions)
    end

    -- Reverse the glyphs and positions if the script is right-to-left
    if glyphRun.direction == "rtl" then
        local n = #glyphRun.glyphs
        for i = 1, n // 2 do
            glyphRun.glyphs[i], glyphRun.glyphs[n - i + 1] =
                glyphRun.glyphs[n - i + 1], glyphRun.glyphs[i]
            glyphRun.positions[i], glyphRun.positions[n - i + 1] =
                glyphRun.positions[n - i + 1], glyphRun.positions[i]
        end
    end

    return self.GPOSProcessor and self.GPOSProcessor.features or nil
end

--- Give every mark a zero advance.
---
--- A mark is drawn at an offset from the glyph before it and the pen does not
--- move for it, so whatever advance the font gave the glyph is not wanted. When
--- this happens matters: a font that positions its own marks needs its GPOS
--- rules to see the real advances first, which is what the shaper's
--- `zeroMarkWidths` selects between.
function OTLayoutEngine:zeroMarkAdvances(positions)
    for i = 1, #self.glyphInfos do
        if self.glyphInfos[i].isMark then
            positions[i].xAdvance = 0
            positions[i].yAdvance = 0
        end
    end
end

function OTLayoutEngine:cleanup()
    self.glyphInfos = nil
    self.plan = nil
    self.shaper = nil
end

--- Every feature tag the font offers for a script and language.
---
--- Sorted, where upstream returns them in the order the font listed them.
--- `Object.keys` on a JS object is insertion order and Lua's `pairs` is not any
--- order at all, so a stable one is chosen; this feeds `font.availableFeatures`,
--- which is a report for a caller rather than an input to layout.
function OTLayoutEngine:getAvailableFeatures(script, language)
    local features = {}

    local function collect(processor)
        processor:selectScript(script, language)

        local tags = {}
        for tag in pairs(processor.features) do
            tags[#tags + 1] = tag
        end
        table.sort(tags)

        for _, tag in ipairs(tags) do
            features[#features + 1] = tag
        end
    end

    if self.GSUBProcessor then
        collect(self.GSUBProcessor)
    end

    if self.GPOSProcessor then
        collect(self.GPOSProcessor)
    end

    return features
end

return OTLayoutEngine
