-- fontkit/opentype/ShapingPlan.lua
--   -- port of fontkit/src/opentype/ShapingPlan.js
--
-- Which features to apply, and in what order.
--
-- A shaper does not hand the processor one list of features; it hands it a list
-- of *stages*. Everything in one stage is applied together, in font order, and
-- one stage finishes before the next begins. That matters because features
-- depend on each other: `ccmp` composes characters into the forms `liga` knows
-- how to ligate, and running them in one pass would sometimes look for a
-- ligature before its parts existed.
--
-- A stage can also be a function rather than a list -- a hook the shaper uses to
-- rearrange glyphs between passes. The Arabic and Indic shapers need those; the
-- default shaper does not, but the machinery is here because a stage list can
-- hold both and `process` has to tell them apart.
--
-- Features come in two kinds. A *global* feature is switched on for every glyph
-- in the run -- `liga`, `kern`. A *local* one is switched on for particular
-- glyphs by the shaper -- `numr` for the digits above a fraction slash. Both are
-- planned here; only global ones are assigned to glyphs by this class.

local ShapingPlan = {}
ShapingPlan.__index = ShapingPlan

function ShapingPlan.new(font, script, direction)
    return setmetatable({
        font = font,
        script = script,
        direction = direction,
        stages = {},
        globalFeatures = {},
        allFeatures = {},
    }, ShapingPlan)
end

--- Add features to the last stage, skipping any already planned.
---
--- "Already planned" is checked across every stage, not just this one: a feature
--- belongs to exactly one stage, and the first stage that asked for it wins.
--- That is what lets a shaper add a broad set of features late without
--- disturbing the ones it placed deliberately early.
function ShapingPlan:_addFeatures(features, global)
    local stageIndex = #self.stages
    local stage = self.stages[stageIndex]

    for _, feature in ipairs(features) do
        if self.allFeatures[feature] == nil then
            stage[#stage + 1] = feature
            self.allFeatures[feature] = stageIndex

            if global then
                self.globalFeatures[feature] = true
            end
        end
    end
end

--- Add to the last stage. `arg` is a tag, a list of tags, or `{global=, local=}`.
---
--- Note that `local` is a Lua keyword, so the table key is written `["local"]`
--- at both ends -- here and in the shaper that builds the argument.
function ShapingPlan:add(arg, global)
    if global == nil then
        global = true
    end

    if #self.stages == 0 then
        self.stages[1] = {}
    end

    if type(arg) == "string" then
        arg = { arg }
    end

    if type(arg) ~= "table" then
        error("Unsupported argument to ShapingPlan#add")
    end

    if #arg > 0 or (arg.global == nil and arg["local"] == nil) then
        -- A list of tags. The second test catches the empty list, which is
        -- indistinguishable from an empty map in Lua and behaves the same way
        -- either branch it takes.
        self:_addFeatures(arg, global)
    else
        self:_addFeatures(arg.global or {}, true)
        self:_addFeatures(arg["local"] or {}, false)
    end
end

--- Begin a new stage, then add to it.
---
--- Passing a function pushes the function *and* an empty list, so whatever is
--- added next lands in a list rather than being appended to the hook.
function ShapingPlan:addStage(arg, global)
    if type(arg) == "function" then
        self.stages[#self.stages + 1] = arg
        self.stages[#self.stages + 1] = {}
    else
        self.stages[#self.stages + 1] = {}
        self:add(arg, global)
    end
end

--- Apply the caller's feature choices on top of the plan.
---
--- A tag mapped to true is added -- to the last stage, wherever that is. A tag
--- mapped to false is removed from whichever stage the shaper put it in, which
--- is how `features = {liga = false}` switches ligatures off.
---
--- The tags are sorted before they are walked. JS iterates an object in
--- insertion order, which is deterministic and which Lua's `pairs` is not, and a
--- port that shapes the same text two ways on two runs would be worse than one
--- that picks an order and keeps it. The order is only observable when two
--- newly-added features name the same lookup, which decides which of the two the
--- font applies first.
function ShapingPlan:setFeatureOverrides(features)
    if type(features) ~= "table" then
        return
    end

    if #features > 0 then
        self:add(features)
        return
    end

    local tags = {}
    for tag in pairs(features) do
        tags[#tags + 1] = tag
    end
    table.sort(tags)

    for _, tag in ipairs(tags) do
        local on = features[tag]
        if on then
            self:add(tag)
        elseif self.allFeatures[tag] ~= nil then
            local stage = self.stages[self.allFeatures[tag]]
            for i, t in ipairs(stage) do
                if t == tag then
                    table.remove(stage, i)
                    break
                end
            end
            self.allFeatures[tag] = nil
            self.globalFeatures[tag] = nil
        end
    end
end

--- Switch every global feature on for every glyph.
function ShapingPlan:assignGlobalFeatures(glyphs)
    for _, glyph in ipairs(glyphs) do
        for feature in pairs(self.globalFeatures) do
            glyph.features[feature] = true
        end
    end
end

--- Run the plan.
---
--- Called twice per run, once for substitution and once for positioning. The
--- `positions` argument is what distinguishes them, and it is also why the
--- function stages only run on the first pass: they rearrange glyphs, and doing
--- that again during positioning would undo the substitution pass's work.
function ShapingPlan:process(processor, glyphs, positions)
    for _, stage in ipairs(self.stages) do
        if type(stage) == "function" then
            if not positions then
                stage(self.font, glyphs, self)
            end
        elseif #stage > 0 then
            processor:applyFeatures(stage, glyphs, positions)
        end
    end
end

return ShapingPlan
