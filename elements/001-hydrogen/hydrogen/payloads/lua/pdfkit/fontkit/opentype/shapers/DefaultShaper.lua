-- fontkit/opentype/shapers/DefaultShaper.lua
--   -- port of fontkit/src/opentype/shapers/DefaultShaper.js
--
-- The shaper for scripts that do not need a shaper.
--
-- A shaper's job is to decide which features to apply and to which glyphs. For
-- Latin, Greek, Cyrillic and most of the rest of the world's alphabets that is a
-- short, fixed list applied to everything -- which is what this is. Scripts that
-- need more have their own shapers. Arabic joining is ArabicShaper. Indic
-- reordering, Hangul composition, and the Universal shaper are not ported;
-- shapers/init.lua refuses those rather than approximating them.
--
-- The one piece of real logic here is fractions: `1/2` should set the `1` as a
-- numerator and the `2` as a denominator, but only when they surround a fraction
-- slash, so those features are switched on per glyph rather than globally.

local unicode = require("pdfkit.fontkit.unicode")

-- rvrn -- rules that apply only at particular points in a variable font's
-- design space. Planned first because a variable font expects it before
-- anything else; harmless in a static one, which has no such feature.
local VARIATION_FEATURES = { "rvrn" }

-- ccmp  compose and decompose, so later rules see a predictable form
-- locl  localised forms -- a Serbian italic `б` differs from a Russian one
-- rlig  required ligatures, which are not optional the way `liga` is
-- mark  attach marks to bases
-- mkmk  attach marks to other marks
local COMMON_FEATURES = { "ccmp", "locl", "rlig", "mark", "mkmk" }

-- Switched on per glyph by assignFeatures, never globally.
local FRACTIONAL_FEATURES = { "frac", "numr", "dnom" }

-- calt  contextual alternates
-- clig  contextual ligatures
-- liga  standard ligatures -- fi, fl
-- rclt  required contextual alternates
-- curs  cursive attachment
-- kern  kerning
local HORIZONTAL_FEATURES = { "calt", "clig", "liga", "rclt", "curs", "kern" }

-- Declared upstream and never read: postprocessing plans the horizontal set
-- unconditionally, even for vertical text. Kept for the same reason the port
-- keeps anything else unused -- so the two files can be read side by side.
local VERTICAL_FEATURES = { "vert" }

-- Features that mark the run's direction, applied at the very start.
local DIRECTIONAL_FEATURES = {
    ltr = { "ltra", "ltrm" },
    rtl = { "rtla", "rtlm" },
}

local DefaultShaper = {}

-- When to zero the advance width of mark glyphs. GPOS-aware fonts position
-- their own marks and expect the width zeroed afterwards; read by
-- OTLayoutEngine.
DefaultShaper.zeroMarkWidths = "AFTER_GPOS"

--- Plan a run: choose the features, then decide which glyphs get them.
---
--- Written with `self` even though there is only one shaper, because upstream's
--- three others are subclasses that override `planFeatures` and are called
--- through exactly this dispatch.
function DefaultShaper:plan(plan, glyphs, features)
    -- Plan the features we want to apply
    self:planPreprocessing(plan)
    self:planFeatures(plan)
    self:planPostprocessing(plan, features)

    -- Assign the global features to all the glyphs
    plan:assignGlobalFeatures(glyphs)

    -- Assign local features to glyphs
    self:assignFeatures(plan, glyphs)
end

function DefaultShaper:planPreprocessing(plan)
    local global = {}
    for _, f in ipairs(VARIATION_FEATURES) do
        global[#global + 1] = f
    end
    for _, f in ipairs(DIRECTIONAL_FEATURES[plan.direction] or {}) do
        global[#global + 1] = f
    end

    plan:add({ global = global, ["local"] = FRACTIONAL_FEATURES })
end

function DefaultShaper:planFeatures(plan)
    -- Do nothing by default. Let subclasses override this.
end

function DefaultShaper:planPostprocessing(plan, userFeatures)
    local all = {}
    for _, f in ipairs(COMMON_FEATURES) do
        all[#all + 1] = f
    end
    for _, f in ipairs(HORIZONTAL_FEATURES) do
        all[#all + 1] = f
    end

    plan:add(all)
    plan:setFeatureOverrides(userFeatures)
end

--- The first character a glyph came from, or nil if it came from none.
local function first_code_point(glyph)
    return glyph.codePoints[1]
end

--- Switch the fraction features on around any fraction slash in the run.
---
--- U+2044 FRACTION SLASH, not the ASCII `/`: the digits either side of a plain
--- slash are a date as often as a fraction, so the font only reshapes them when
--- the text says so explicitly.
function DefaultShaper:assignFeatures(plan, glyphs)
    -- Enable contextual fractions
    local i = 1
    while i <= #glyphs do
        local glyph = glyphs[i]
        if first_code_point(glyph) == 0x2044 then -- fraction slash
            local start = i
            local finish = i + 1

            -- Apply numerator
            while start > 1 do
                local cp = first_code_point(glyphs[start - 1])
                if not (cp and unicode.is_digit(cp)) then
                    break
                end
                glyphs[start - 1].features.numr = true
                glyphs[start - 1].features.frac = true
                start = start - 1
            end

            -- Apply denominator
            while finish <= #glyphs do
                local cp = first_code_point(glyphs[finish])
                if not (cp and unicode.is_digit(cp)) then
                    break
                end
                glyphs[finish].features.dnom = true
                glyphs[finish].features.frac = true
                finish = finish + 1
            end

            -- Apply fraction slash
            glyph.features.frac = true

            -- Resume past the denominator: the digits already have their
            -- features and a second slash inside them is not possible.
            i = finish
        else
            i = i + 1
        end
    end
end

return DefaultShaper
