-- mixins/text.lua -- port of pdfkit/lib/mixins/text.js
--
-- Everything that puts glyphs on a page. Wrapping itself lives next door in
-- line_wrapper.lua; this file decides where each finished line goes and emits
-- the operators that draw it.
--
-- The division of labour is worth holding on to while reading:
--
--   text()        the public entry point
--   _text()       sets up options, drives the wrapper, handles rotation
--   _line()       one wrapped line: draw it, then move down
--   _fragment()   one run of glyphs: all the operator emission happens here
--
-- _fragment is the only place in the library that writes BT/Tm/Tf/TJ/ET, so if
-- a character lands in the wrong place, it landed there because of this file.
--
-- A NOTE ON TRUTHINESS. JavaScript treats 0 and "" as false, and PDFKit relies
-- on that constantly: `if (characterSpacing)` means "if it is set and not
-- zero", and `options.width` guards on a width that is both present and
-- non-zero. Lua treats 0 as true, so every such test is written out explicitly
-- below. Getting one of them wrong emits a spurious operator, which changes the
-- content stream without changing what anyone sees -- the worst kind of bug to
-- go looking for.

local LineWrapper = require("pdfkit.line_wrapper")
local object = require("pdfkit.object")
local jsstring = require("pdfkit.jsstring")
local utils = require("pdfkit.utils")

local number = object.number
local js_tostring = utils.js_tostring
local cosine, sine = utils.cosine, utils.sine

local M = {}

-- JavaScript's `if (v)` for the values PDFKit actually puts through it.
local function truthy(v)
    return v ~= nil and v ~= false and v ~= 0 and v ~= ""
end

--- Label for one item of an ordered list: "1." or "a." and, past the alphabet,
--- "aa." then "aaa." -- the letter repeats rather than carrying like a number.
local function format_list_label(n, listType)
    if listType == "numbered" then
        return js_tostring(n) .. "."
    end

    -- lettered
    local letter = string.char(((n - 1) % 26) + 65)
    local times = math.floor((n - 1) / 26 + 1)
    return string.rep(letter, times) .. "."
end

M._formatListLabel = format_list_label

--------------------------------------------------------------------------------
-- Setup and position
--------------------------------------------------------------------------------

function M.initText(doc)
    -- The original opens with `this._line = this._line.bind(this)`, because
    -- _line is only ever handed to the wrapper as a callback and is never
    -- called method-style. Binding here is the same move: an own field that
    -- shadows the mixin's method and already knows its document, so
    -- `lineCallback(text, options, wrapper)` works with no colon in sight.
    local unbound = doc._line
    doc._line = function(text, options, wrapper)
        return unbound(doc, text, options, wrapper)
    end

    -- Current coordinates, in PDFKit's top-left origin.
    doc.x = 0
    doc.y = 0
    doc._lineGap = 0
end

function M.lineGap(doc, _lineGap)
    doc._lineGap = _lineGap
    return doc
end

function M.moveDown(doc, lines)
    if lines == nil then lines = 1 end
    doc.y = doc.y + doc:currentLineHeight(true) * lines + doc._lineGap
    return doc
end

function M.moveUp(doc, lines)
    if lines == nil then lines = 1 end
    doc.y = doc.y - (doc:currentLineHeight(true) * lines + doc._lineGap)
    return doc
end

--------------------------------------------------------------------------------
-- Options
--------------------------------------------------------------------------------

--- Normalise the (x, y, options) argument triple and fold in the state a
--- continued run left behind.
---
--- The original leans on JavaScript's optional arguments: text(str, options)
--- and text(str, x, y, options) both arrive here, and the first is detected by
--- x being an object. The same test is made below.
function M._initOptions(doc, x, y, options)
    if x == nil then x = {} end
    if options == nil then options = {} end

    if type(x) == "table" then
        options = x
        x = nil
    end

    -- Shallow clone, so that the caller's table is not written into.
    local result = {}
    for k, v in pairs(options) do
        result[k] = v
    end

    -- A continued run inherits everything the previous run was given, except
    -- the continuation flag itself -- otherwise it could never stop.
    if doc._textOptions then
        for key, val in pairs(doc._textOptions) do
            if key ~= "continued" and result[key] == nil then
                result[key] = val
            end
        end
    end

    if x ~= nil then doc.x = x end
    if y ~= nil then doc.y = y end

    -- With no width given, wrap to the right margin. lineBreak = false opts out
    -- of wrapping entirely and leaves width unset.
    if result.lineBreak ~= false then
        if result.width == nil then
            result.width = doc.page.width - doc.x - doc.page.margins.right
        end
        result.width = math.max(result.width, 0)
    end

    if not truthy(result.columns) then
        result.columns = 0
    end
    if result.columnGap == nil then
        result.columnGap = 18 -- 1/4 inch
    end

    -- Normalise rotation to 0..360. Lua's % is floored and JavaScript's is
    -- truncated, so math.fmod is the one that matches: fmod(-90, 360) is -90,
    -- which the following line then turns into 270.
    result.rotation = math.fmod(tonumber(result.rotation or 0) or 0, 360)
    if result.rotation < 0 then
        result.rotation = result.rotation + 360
    end

    return result
end

--------------------------------------------------------------------------------
-- Drawing text
--------------------------------------------------------------------------------

function M._text(doc, text, x, y, options, lineCallback)
    options = doc:_initOptions(x, y, options)

    text = (text == nil) and "" or tostring(text)

    -- Word spacing is applied per gap, so runs of spaces would multiply it.
    if truthy(options.wordSpacing) then
        text = jsstring.collapse_space_runs(text)
    end

    local function addStructure()
        if options.structParent then
            local structType = options.structType or "P"
            options.structParent:add(
                doc:struct(structType, { doc:markStructureContent(structType) })
            )
        end
    end

    -- Rotation costs a save/restore pair and two matrices in the content
    -- stream, so it is skipped when there is nothing to rotate.
    if options.rotation ~= 0 then
        doc:save()
        doc:rotate(-options.rotation, { origin = { doc.x, doc.y } })
    end

    if truthy(options.width) then
        -- Word wrapping. A continued run keeps its wrapper so that the next
        -- call resumes mid-line rather than starting a new paragraph.
        local wrapper = doc._wrapper
        if not wrapper then
            wrapper = LineWrapper.new(doc, options)
            wrapper:on("line", lineCallback)
            wrapper:on("firstLine", addStructure)
        end

        doc._wrapper = options.continued and wrapper or nil
        doc._textOptions = options.continued and options or nil
        wrapper:wrap(text, options)
    else
        -- No width, so no wrapping: each paragraph is one line, however long.
        for _, line in ipairs(jsstring.split_lines(text)) do
            addStructure()
            lineCallback(line, options)
        end
    end

    if options.rotation ~= 0 then
        doc:restore()
    end

    return doc
end

function M.text(doc, text, x, y, options)
    return doc:_text(text, x, y, options, doc._line)
end

--------------------------------------------------------------------------------
-- Measuring
--------------------------------------------------------------------------------

function M.widthOfString(doc, str, options)
    options = options or {}
    local horizontalScaling = options.horizontalScaling or 100

    -- The (length - 1) is a count of GAPS between characters, not characters,
    -- and JavaScript counts them in UTF-16 code units.
    local gaps = jsstring.utf16_length(str) - 1

    return ((doc._font:widthOfString(str, doc._fontSize, options.features)
        + (options.characterSpacing or 0) * gaps) * horizontalScaling) / 100
end

--- The bounding box of a string as doc:text() would actually lay it out.
---
--- Runs the wrapper for real -- same wrapping, same justification -- but with
--- a listener that measures instead of drawing. x and y are restored
--- afterwards, so calling this leaves no trace.
function M.boundsOfString(doc, str, x, y, options)
    options = doc:_initOptions(x, y, options)
    x, y = doc.x, doc.y

    local lineGap = options.lineGap or doc._lineGap or 0
    local lineHeight = doc:currentLineHeight(true) + lineGap
    local contentWidth = 0

    str = (str == nil) and "" or tostring(str)

    if truthy(options.wordSpacing) then
        str = jsstring.collapse_space_runs(str)
    end

    if truthy(options.width) then
        local wrapper = LineWrapper.new(doc, options)
        wrapper:on("line", function(text, opts)
            doc.y = doc.y + lineHeight
            text = jsstring.strip_newlines(text)

            if #text > 0 then
                local wordSpacing = opts.wordSpacing or 0
                local characterSpacing = opts.characterSpacing or 0

                -- Justified text is wider than its glyphs: the extra space is
                -- shared out between the words, so it has to be added back.
                if truthy(opts.width) and opts.align == "justify" then
                    local words = jsstring.split_space(jsstring.trim(text))
                    local textWidth = doc:widthOfString(jsstring.strip_all_space(text), opts)
                    local spaceWidth = doc:widthOfString(" ") + characterSpacing
                    wordSpacing = math.max(
                        0,
                        (opts.lineWidth - textWidth) / math.max(1, #words - 1) - spaceWidth
                    )
                end

                contentWidth = math.max(
                    contentWidth,
                    opts.textWidth
                    + wordSpacing * (opts.wordCount - 1)
                    + characterSpacing * (jsstring.utf16_length(text) - 1)
                )
            end
        end)
        wrapper:wrap(str, options)
    else
        for _, line in ipairs(jsstring.split_lines(str)) do
            local lineWidth = doc:widthOfString(line, options)
            doc.y = doc.y + lineHeight
            contentWidth = math.max(contentWidth, lineWidth)
        end
    end

    local contentHeight = doc.y - y
    if truthy(options.height) then
        contentHeight = math.min(contentHeight, options.height)
    end

    doc.x, doc.y = x, y

    -- The box rotates about its own top-left corner:
    --
    --   [x1,y1]  >  [x2,y2]
    --      ^           v
    --   [x4,y4]  <  [x3,y3]
    --
    -- The four right angles are special-cased because trig would give
    -- 6.1e-17 where zero belongs.
    if options.rotation == 0 then
        return { x = x, y = y, width = contentWidth, height = contentHeight }
    elseif options.rotation == 90 then
        return { x = x, y = y - contentWidth, width = contentHeight, height = contentWidth }
    elseif options.rotation == 180 then
        return { x = x - contentWidth, y = y - contentHeight,
                 width = contentWidth, height = contentHeight }
    elseif options.rotation == 270 then
        return { x = x - contentHeight, y = y, width = contentHeight, height = contentWidth }
    end

    local cos = cosine(options.rotation)
    local sin = sine(options.rotation)

    local x1, y1 = x, y
    local x2 = x + contentWidth * cos
    local y2 = y - contentWidth * sin
    local x3 = x + contentWidth * cos + contentHeight * sin
    local y3 = y - contentWidth * sin + contentHeight * cos
    local x4 = x + contentHeight * sin
    local y4 = y + contentHeight * cos

    local xMin = math.min(x1, x2, x3, x4)
    local xMax = math.max(x1, x2, x3, x4)
    local yMin = math.min(y1, y2, y3, y4)
    local yMax = math.max(y1, y2, y3, y4)

    return { x = xMin, y = yMin, width = xMax - xMin, height = yMax - yMin }
end

--- How tall the string will be once laid out.
---
--- Runs the whole text path with a callback that only advances y, and with
--- height set to infinity so that measuring never triggers a page break.
function M.heightOfString(doc, text, options)
    local x, y = doc.x, doc.y

    options = doc:_initOptions(options)
    options.height = math.huge -- don't break pages

    local lineGap = options.lineGap or doc._lineGap or 0
    doc:_text(text, doc.x, doc.y, options, function()
        doc.y = doc.y + doc:currentLineHeight(true) + lineGap
    end)

    local height = doc.y - y
    doc.x, doc.y = x, y

    return height
end

--------------------------------------------------------------------------------
-- Lists
--------------------------------------------------------------------------------

--- Draw a bulleted, numbered or lettered list.
---
--- A nested table is a sub-list: its items are drawn one level further in.
--- The whole structure is flattened first into parallel arrays of items, their
--- levels and (for ordered lists) their numbers, because the wrapper works
--- through one item at a time and needs to know the level of the item it is
--- about to start.
function M.list(doc, list, x, y, options)
    options = doc:_initOptions(x, y, options)

    local listType = options.listType or "bullet"
    local unit = math.floor((doc._font.ascender / 1000) * doc._fontSize + 0.5)
    local midLine = unit / 2
    local r = options.bulletRadius or unit / 3
    local indent = options.textIndent or (listType == "bullet" and r * 5 or unit * 2)
    local itemIndent = options.bulletIndent or (listType == "bullet" and r * 8 or unit * 2)

    local level = 1
    local items, levels, numbers = {}, {}, {}

    local function flatten(l)
        local n = 1
        for i = 1, #l do
            local item = l[i]
            if type(item) == "table" then
                level = level + 1
                flatten(item)
                level = level - 1
            else
                items[#items + 1] = item
                levels[#levels + 1] = level
                if listType ~= "bullet" then
                    numbers[#numbers + 1] = n
                    n = n + 1
                end
            end
        end
    end

    flatten(list)

    local function drawListItem(listItem, i)
        local wrapper = LineWrapper.new(doc, options)
        wrapper:on("line", doc._line)

        level = 1
        wrapper:once("firstLine", function()
            local item, itemType, labelType, bodyType

            if options.structParent then
                if options.structTypes then
                    itemType, labelType, bodyType =
                        options.structTypes[1], options.structTypes[2], options.structTypes[3]
                else
                    itemType, labelType, bodyType = "LI", "Lbl", "LBody"
                end
            end

            if itemType then
                item = doc:struct(itemType)
                options.structParent:add(item)
            elseif options.structParent then
                item = options.structParent
            end

            -- The index arithmetic mirrors the original's i++ inside the
            -- comparison: `levels` is read at i, then i is bumped so that
            -- `numbers` is read at the same place.
            local l = levels[i + 1]
            i = i + 1
            if l ~= level then
                local diff = itemIndent * (l - level)
                doc.x = doc.x + diff
                wrapper.lineWidth = wrapper.lineWidth - diff
                level = l
            end

            if item and (labelType or bodyType) then
                local t = labelType or bodyType
                item:add(doc:struct(t, { doc:markStructureContent(t) }))
            end

            if listType == "bullet" then
                doc:circle(doc.x - indent + r, doc.y + midLine, r)
                doc:fill()
            elseif listType == "numbered" or listType == "lettered" then
                local label = format_list_label(numbers[i], listType)
                doc:_fragment(label, doc.x - indent, doc.y, options)
            end

            if item and labelType and bodyType then
                item:add(doc:struct(bodyType, { doc:markStructureContent(bodyType) }))
            end
            if item and item ~= options.structParent then
                item:finish()
            end
        end)

        -- Each column or page of an item is indented to its level, and the
        -- indent is given back at the end so the next item starts level again.
        wrapper:on("sectionStart", function()
            local pos = indent + itemIndent * (level - 1)
            doc.x = doc.x + pos
            wrapper.lineWidth = wrapper.lineWidth - pos
        end)

        wrapper:on("sectionEnd", function()
            local pos = indent + itemIndent * (level - 1)
            doc.x = doc.x - pos
            wrapper.lineWidth = wrapper.lineWidth + pos
        end)

        wrapper:wrap(listItem, options)
    end

    for i = 1, #items do
        drawListItem(items[i], i - 1)
    end

    return doc
end

--------------------------------------------------------------------------------
-- One line
--------------------------------------------------------------------------------

--- Draw one line and move on.
---
--- With a wrapper, "move on" means down to the next line; without one, it means
--- along, because unwrapped fragments sit side by side.
function M._line(doc, text, options, wrapper)
    options = options or {}
    doc:_fragment(text, doc.x, doc.y, options)

    if wrapper then
        local lineGap = options.lineGap or doc._lineGap or 0
        doc.y = doc.y + doc:currentLineHeight(true) + lineGap
    else
        doc.x = doc.x + doc:widthOfString(text, options)
    end
end

--------------------------------------------------------------------------------
-- One fragment -- where the operators are written
--------------------------------------------------------------------------------

function M._fragment(doc, text, x, y, options)
    text = jsstring.strip_newlines(tostring(text))
    if #text == 0 then
        return
    end

    local align = options.align or "left"
    local wordSpacing = options.wordSpacing or 0
    local characterSpacing = options.characterSpacing or 0
    local horizontalScaling = options.horizontalScaling or 100
    local textWidth, words

    -- Alignment. Left needs nothing: x is already where the line starts.
    if truthy(options.width) then
        if align == "right" then
            -- Trailing spaces are not drawn, so they must not push the line
            -- left off the right margin either.
            textWidth = doc:widthOfString(jsstring.rtrim(text), options)
            x = x + options.lineWidth - textWidth
        elseif align == "center" then
            x = x + options.lineWidth / 2 - options.textWidth / 2
        elseif align == "justify" then
            -- Share the slack out over the gaps between words. max(1, ...)
            -- keeps a single-word line from dividing by zero.
            words = jsstring.split_space(jsstring.trim(text))
            textWidth = doc:widthOfString(jsstring.strip_all_space(text), options)
            local spaceWidth = doc:widthOfString(" ") + characterSpacing
            wordSpacing = math.max(
                0,
                (options.lineWidth - textWidth) / math.max(1, #words - 1) - spaceWidth
            )
        end
    end

    -- Baseline. dy is how far below the given y the glyph baseline sits.
    -- Names follow http://wiki.apache.org/xmlgraphics-fop/LineLayout/AlignmentHandling
    local dy
    if type(options.baseline) == "number" then
        dy = -options.baseline
    else
        local b = options.baseline
        if b == "svg-middle" then
            dy = 0.5 * doc._font.xHeight
        elseif b == "middle" or b == "svg-central" then
            dy = 0.5 * (doc._font.descender + doc._font.ascender)
        elseif b == "bottom" or b == "ideographic" then
            dy = doc._font.descender
        elseif b == "alphabetic" then
            dy = 0
        elseif b == "mathematical" then
            dy = 0.5 * doc._font.ascender
        elseif b == "hanging" then
            dy = 0.8 * doc._font.ascender
        elseif b == "top" then
            dy = doc._font.ascender
        else
            dy = doc._font.ascender
        end
        dy = (dy / 1000) * doc._fontSize
    end

    -- What the line actually occupies once spacing is counted -- the width an
    -- underline or a link hotspot has to cover.
    --
    -- textWidth and wordCount are set by LineWrapper as it emits each line, so
    -- a fragment that came through the wrapper has both. Two callers do not:
    -- `_line` with no wrapper (the `lineBreak = false` path) and the list-label
    -- draw, which fires on `firstLine` -- before any line has been emitted.
    --
    -- In JavaScript those are `undefined`, and the arithmetic below quietly
    -- yields NaN rather than throwing. Lua would raise on nil, so the two are
    -- coerced to NaN explicitly to keep the same shape: harmless when the width
    -- is unused, and when it IS used -- an underline, a strike, a link box --
    -- it reaches utils.number(), which rejects it exactly as the original does.
    -- Note 0 * NaN is NaN in both languages, so a zero wordSpacing does not
    -- rescue a missing wordCount in either.
    local NaN = 0 / 0
    local optTextWidth = options.textWidth or NaN
    local optWordCount = options.wordCount or NaN

    local renderedWidth = optTextWidth
        + wordSpacing * (optWordCount - 1)
        + characterSpacing * (jsstring.utf16_length(text) - 1)

    -- Annotations layered over this fragment.
    if options.link ~= nil then
        local linkOptions = {}
        if doc._currentStructureElement
            and doc._currentStructureElement.dictionary.data.S == "Link"
        then
            linkOptions.structParent = doc._currentStructureElement
        end
        doc:link(x, y, renderedWidth, doc:currentLineHeight(), options.link, linkOptions)
    end
    if options.goTo ~= nil then
        doc:goTo(x, y, renderedWidth, doc:currentLineHeight(), options.goTo)
    end
    if options.destination ~= nil then
        doc:addNamedDestination(options.destination, "XYZ", x, y, object.NULL)
    end

    -- Underline and strikethrough are drawn as strokes, not as font features,
    -- and they inherit the fill colour unless the caller is stroking already.
    if options.underline then
        doc:save()
        if not options.stroke then
            doc:strokeColor(table.unpack(doc._fillColor or {}))
        end

        local lineWidth = doc._fontSize < 10 and 0.5 or math.floor(doc._fontSize / 10)
        doc:lineWidth(lineWidth)

        local lineY = y + doc:currentLineHeight() - lineWidth
        doc:moveTo(x, lineY)
        doc:lineTo(x + renderedWidth, lineY)
        doc:stroke()
        doc:restore()
    end

    if options.strike then
        doc:save()
        if not options.stroke then
            doc:strokeColor(table.unpack(doc._fillColor or {}))
        end

        local lineWidth = doc._fontSize < 10 and 0.5 or math.floor(doc._fontSize / 10)
        doc:lineWidth(lineWidth)

        local lineY = y + doc:currentLineHeight() / 2
        doc:moveTo(x, lineY)
        doc:lineTo(x + renderedWidth, lineY)
        doc:stroke()
        doc:restore()
    end

    doc:save()

    -- Oblique is a faked italic: shear the text sideways. The three transforms
    -- move the origin to the anchor, shear there, and move it back.
    if truthy(options.oblique) then
        local skew
        if type(options.oblique) == "number" then
            skew = -math.tan((options.oblique * math.pi) / 180)
        else
            skew = -0.25
        end
        doc:transform(1, 0, 0, 1, x, y)
        doc:transform(1, 0, skew, 1, -skew * dy, 0)
        doc:transform(1, 0, 0, 1, -x, -y)
    end

    -- PDF text runs bottom-up; PDFKit's API is top-down. Flipping here rather
    -- than negating every coordinate is what keeps the rest of the file sane.
    doc:transform(1, 0, 0, -1, 0, doc.page.height)
    y = doc.page.height - y - dy

    -- Register the font on this page, if this is its first appearance.
    local pageFonts = doc.page:fonts()
    if pageFonts[doc._font.id] == nil then
        pageFonts[doc._font.id] = doc._font:ref()
    end

    doc:addContent("BT")
    doc:addContent("1 0 0 1 " .. number(x) .. " " .. number(y) .. " Tm")
    doc:addContent("/" .. doc._font.id .. " " .. number(doc._fontSize) .. " Tf")

    -- Rendering mode: 0 fill, 1 stroke, 2 both. Zero is the default and is
    -- left out of the stream.
    local mode = (options.fill and options.stroke) and 2 or (options.stroke and 1 or 0)
    if mode ~= 0 then
        doc:addContent(js_tostring(mode) .. " Tr")
    end

    if characterSpacing ~= 0 then
        doc:addContent(number(characterSpacing) .. " Tc")
    end

    -- Plain interpolation upstream, not number(), so this one gets JavaScript's
    -- own number-to-string rules and keeps its full precision.
    if horizontalScaling ~= 100 then
        doc:addContent(js_tostring(horizontalScaling) .. " Tz")
    end

    -- Encode the text.
    --
    -- With word spacing, each word is encoded separately so the extra space can
    -- be folded into the last glyph's advance. The Tw operator would be the
    -- obvious tool, but it only acts on character code 32, which an embedded
    -- subset font may not use for a space at all.
    local encoded, positions
    if wordSpacing ~= 0 then
        words = jsstring.split_space(jsstring.trim(text))
        wordSpacing = wordSpacing + doc:widthOfString(" ") + characterSpacing
        wordSpacing = wordSpacing * (1000 / doc._fontSize)

        encoded, positions = {}, {}
        for _, word in ipairs(words) do
            local encodedWord, positionsWord = doc._font:encode(word, options.features)
            for _, e in ipairs(encodedWord) do encoded[#encoded + 1] = e end
            for _, p in ipairs(positionsWord) do positions[#positions + 1] = p end

            -- Copy before adjusting: the font caches these position records and
            -- handing back a mutated one would corrupt every later use.
            local last = positions[#positions]
            local space = {}
            for k, v in pairs(last) do space[k] = v end
            space.xAdvance = space.xAdvance + wordSpacing
            positions[#positions] = space
        end
    else
        encoded, positions = doc._font:encode(text, options.features)
    end

    local scale = doc._fontSize / 1000
    local commands = {}
    local last = 0
    local hadOffset = false

    -- Append glyphs [last, cur) to the TJ buffer as one run, followed by the
    -- kern adjustment that the run's final glyph carries. TJ takes its numbers
    -- in thousandths of an em and subtracts them, hence the negation.
    local function addSegment(cur)
        if last < cur then
            local hex = table.concat(encoded, "", last + 1, cur)
            local pos = positions[cur]
            local advance = pos.xAdvance - pos.advanceWidth
            commands[#commands + 1] = "<" .. hex .. "> " .. number(-advance)
        end
        last = cur
    end

    local function flush(upTo)
        addSegment(upTo)
        if #commands > 0 then
            doc:addContent("[" .. table.concat(commands, " ") .. "] TJ")
            commands = {}
        end
    end

    for index = 1, #positions do
        local i = index - 1 -- the original's zero-based index
        local pos = positions[index]

        if truthy(pos.xOffset) or truthy(pos.yOffset) then
            -- An offset glyph cannot be expressed as a kern, so the run is
            -- closed, the pen is moved, and the glyph is emitted alone.
            flush(i)
            doc:addContent(
                "1 0 0 1 " .. number(x + pos.xOffset * scale)
                .. " " .. number(y + pos.yOffset * scale) .. " Tm"
            )
            flush(i + 1)
            hadOffset = true
        else
            if hadOffset then
                doc:addContent("1 0 0 1 " .. number(x) .. " " .. number(y) .. " Tm")
                hadOffset = false
            end

            -- Glyphs whose advance matches their width need no adjustment, so
            -- they stay in the current run and are written as one hex string.
            if pos.xAdvance - pos.advanceWidth ~= 0 then
                addSegment(i + 1)
            end
        end

        x = x + pos.xAdvance * scale
    end

    flush(#positions)

    doc:addContent("ET")
    doc:restore()
end

return M
