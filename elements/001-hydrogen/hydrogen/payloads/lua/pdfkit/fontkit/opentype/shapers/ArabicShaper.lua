-- fontkit/opentype/shapers/ArabicShaper.lua
--   -- port of fontkit/src/opentype/shapers/ArabicShaper.js
--
-- Joining forms for Arabic and the other cursive scripts fontkit sends here
-- (Syriac, Mongolian, N'Ko, and the rest of the list in shapers/init.lua).
--
-- The state machine is HarfBuzz's, transcribed by fontkit. Each letter is
-- Non-joining, left, right, or dual, plus two Syriac groups (ALAPH, DALATH
-- RISH). Walking the word assigns isol / init / medi / fina (and the Syriac
-- fin2 / fin3 / med2) to each glyph. The font's GSUB then picks the glyph.
-- Marks and format characters are transparent: they do not change the state
-- and they do not take a joining feature.

local DefaultShaper = require("pdfkit.fontkit.opentype.shapers.DefaultShaper")
local data = require("pdfkit.fontkit.opentype.shapers.arabic_data")

local FEATURES = { "isol", "fina", "fin2", "fin3", "medi", "med2", "init" }

local TRANSPARENT = 6

-- Each entry is { prevAction, curAction, nextState }. nil action means none.
-- Columns: Non_Joining, Left_Joining, Right_Joining, Dual_Joining, ALAPH, DALATH RISH.
local STATE_TABLE = {
    -- State 0: previous was non-joining, not willing to join.
    {
        { nil, nil, 0 }, { nil, "isol", 2 }, { nil, "isol", 1 },
        { nil, "isol", 2 }, { nil, "isol", 1 }, { nil, "isol", 6 },
    },
    -- State 1: previous was right-joining or an isolated ALAPH, not willing to join.
    {
        { nil, nil, 0 }, { nil, "isol", 2 }, { nil, "isol", 1 },
        { nil, "isol", 2 }, { nil, "fin2", 5 }, { nil, "isol", 6 },
    },
    -- State 2: previous was dual or left, isolated, willing to join.
    {
        { nil, nil, 0 }, { nil, "isol", 2 }, { "init", "fina", 1 },
        { "init", "fina", 3 }, { "init", "fina", 4 }, { "init", "fina", 6 },
    },
    -- State 3: previous was dual in final form, willing to join.
    {
        { nil, nil, 0 }, { nil, "isol", 2 }, { "medi", "fina", 1 },
        { "medi", "fina", 3 }, { "medi", "fina", 4 }, { "medi", "fina", 6 },
    },
    -- State 4: previous was final ALAPH, not willing to join.
    {
        { nil, nil, 0 }, { nil, "isol", 2 }, { "med2", "isol", 1 },
        { "med2", "isol", 2 }, { "med2", "fin2", 5 }, { "med2", "isol", 6 },
    },
    -- State 5: previous was fin2/fin3 ALAPH, not willing to join.
    {
        { nil, nil, 0 }, { nil, "isol", 2 }, { "isol", "isol", 1 },
        { "isol", "isol", 2 }, { "isol", "fin2", 5 }, { "isol", "isol", 6 },
    },
    -- State 6: previous was DALATH/RISH, not willing to join.
    {
        { nil, nil, 0 }, { nil, "isol", 2 }, { nil, "isol", 1 },
        { nil, "isol", 2 }, { nil, "fin3", 5 }, { nil, "isol", 6 },
    },
}

local ArabicShaper = setmetatable({}, { __index = DefaultShaper })

function ArabicShaper:planFeatures(plan)
    plan:add({ "ccmp", "locl" })
    for _, feature in ipairs(FEATURES) do
        plan:addStage(feature, false)
    end
    plan:addStage("mset")
end

function ArabicShaper:assignFeatures(plan, glyphs)
    DefaultShaper.assignFeatures(self, plan, glyphs)

    local prev = 0
    local state = 0
    local actions = {}

    for i = 1, #glyphs do
        local glyph = glyphs[i]
        local cp = glyph.codePoints[1] or 0
        local joining = data.joining_class(cp)
        if joining == TRANSPARENT then
            actions[i] = nil
        else
            local row = STATE_TABLE[state + 1][joining + 1]
            local prevAction, curAction, nextState = row[1], row[2], row[3]
            if prevAction ~= nil and prev ~= 0 then
                actions[prev] = prevAction
            end
            actions[i] = curAction
            prev = i
            state = nextState
        end
    end

    for index = 1, #glyphs do
        local feature = actions[index]
        if feature then
            glyphs[index].features[feature] = true
        end
    end
end

return ArabicShaper
