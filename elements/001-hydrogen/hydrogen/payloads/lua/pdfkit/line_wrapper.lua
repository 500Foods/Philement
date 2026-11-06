-- line_wrapper.lua -- port of pdfkit/lib/line_wrapper.js
--
-- Takes a run of text and an available width and decides where the lines fall.
-- It does not draw anything: it emits events, and the text mixin listens for
-- them and draws. That split is why paragraph indents, list bullets, column
-- breaks and page breaks can all hook into wrapping without wrapping knowing
-- about any of them.
--
-- Events, and who listens:
--
--   line          a line is ready to be drawn (text mixin draws it)
--   firstLine     before the first line of a segment (indent handling)
--   lastLine      before the final line (justify becomes left-align)
--   sectionStart  a column or page has begun
--   sectionEnd    a column or page is finished
--   columnBreak   moving to the next column
--   pageBreak     moving to the next page
--
-- Line break OPPORTUNITIES come from line_breaker.lua, which implements the
-- Unicode algorithm. This file decides which of those opportunities to take.
--
-- Two things to know before editing:
--
--   * Widths are compared through PDFNumber, which floors to 32-bit float
--     precision. That is not cosmetic -- it is what keeps a line that only just
--     fits from fitting on one run and not the next. Every comparison the
--     original routes through PDFNumber is routed through it here.
--
--   * document:widthOfString(word, self) passes the WRAPPER as the options
--     table. That is why characterSpacing, horizontalScaling and features are
--     fields on this object with exactly those names.

local LineBreaker = require("pdfkit.line_breaker").LineBreaker
local jsstring = require("pdfkit.jsstring")
local utils = require("pdfkit.utils")

local PDFNumber = utils.PDFNumber

-- U+00AD. Invisible unless a line breaks at it, where it becomes a hyphen.
local SOFT_HYPHEN = "\194\173"
local HYPHEN = "-"

local LineWrapper = {}
LineWrapper.__index = LineWrapper

function LineWrapper.new(document, options)
    local self = setmetatable({}, LineWrapper)

    self._listeners = {}
    self.document = document
    self.horizontalScaling = options.horizontalScaling or 100
    self.indent = ((options.indent or 0) * self.horizontalScaling) / 100
    self.indentAllLines = options.indentAllLines or false
    self.characterSpacing = ((options.characterSpacing or 0) * self.horizontalScaling) / 100

    -- Transcribed exactly, bug and all. The original writes
    --
    --     this.wordSpacing = ((options.wordSpacing === 0) * this.horizontalScaling) / 100;
    --
    -- where a `===` comparison was clearly meant to be a `||` default. What it
    -- actually does is coerce a boolean to 0 or 1, so passing wordSpacing: 0
    -- sets the spacing to 1 and passing nothing sets it to 0. It is harmless in
    -- practice -- wrap() recomputes this field from the same option before any
    -- width is measured -- but it is reproduced rather than quietly corrected,
    -- because "quietly corrected" is how a port stops matching.
    self.wordSpacing = ((options.wordSpacing == 0) and 1 or 0) * self.horizontalScaling / 100

    -- `options.columns || 1` upstream, and the JavaScript falsiness of 0 is
    -- doing real work: _initOptions normalises "no columns asked for" to the
    -- number 0, which arrives here and becomes 1. A plain Lua `or` would keep
    -- the 0 and divide by it a line later.
    self.columns = (options.columns ~= nil and options.columns ~= false
        and options.columns ~= 0) and options.columns or 1
    self.columnGap = ((options.columnGap ~= nil and options.columnGap or 18)
        * self.horizontalScaling) / 100 -- 1/4 inch
    self.lineWidth = ((options.width * self.horizontalScaling) / 100
        - self.columnGap * (self.columns - 1)) / self.columns
    self.spaceLeft = self.lineWidth
    self.startX = self.document.x
    self.startY = self.document.y
    self.column = 1
    self.ellipsis = options.ellipsis
    self.continuedX = 0
    self.features = options.features

    -- How far down the page text may go. An explicit height caps it there;
    -- otherwise the page's own bottom margin does.
    if options.height ~= nil then
        self.height = options.height
        self.maxY = PDFNumber(self.startY + options.height)
    else
        self.maxY = PDFNumber(self.document.page:maxY())
    end

    -- Paragraph indents. The first line of a segment is pushed in, and the
    -- push is undone before the second line -- unless indentAllLines is set,
    -- in which case it stays for the whole paragraph.
    self:on("firstLine", function(opts)
        -- Continuing where a previous segment left off takes priority over the
        -- caller's indent: the text has to resume at the right column.
        local indent = (self.continuedX ~= 0 and self.continuedX) or self.indent
        self.document.x = self.document.x + indent
        self.lineWidth = self.lineWidth - indent

        if opts.indentAllLines then
            return
        end

        self:once("line", function()
            self.document.x = self.document.x - indent
            self.lineWidth = self.lineWidth + indent
            if opts.continued and self.continuedX == 0 then
                self.continuedX = self.indent
            end
            if not opts.continued then
                self.continuedX = 0
            end
        end)
    end)

    -- The last line of a justified paragraph is set left-aligned. Stretching
    -- three words across a full measure is how ransom notes are typeset.
    self:on("lastLine", function(opts)
        local align = opts.align
        if align == "justify" then
            opts.align = "left"
        end
        self.lastLine = true

        self:once("line", function()
            self.document.y = self.document.y + (opts.paragraphGap or 0)
            opts.align = align
            self.lastLine = false
        end)
    end)

    return self
end

--------------------------------------------------------------------------------
-- Events
--------------------------------------------------------------------------------

function LineWrapper:on(event, listener)
    local list = self._listeners[event]
    if not list then
        list = {}
        self._listeners[event] = list
    end
    list[#list + 1] = listener
end

--- Fire once, then unregister. The listener removes itself BEFORE running, so
--- a listener that emits its own event cannot re-enter itself.
function LineWrapper:once(event, listener)
    local wrapper
    wrapper = function(...)
        local list = self._listeners[event]
        for i = 1, #list do
            if list[i] == wrapper then
                table.remove(list, i)
                break
            end
        end
        listener(...)
    end
    self:on(event, wrapper)
end

--- Fire over a snapshot of the listener list, so that a listener adding or
--- removing listeners does not disturb the walk in progress.
function LineWrapper:emit(event, ...)
    local list = self._listeners[event]
    if not list then return end

    local snapshot = table.move(list, 1, #list, 1, {})
    for i = 1, #snapshot do
        snapshot[i](...)
    end
end

--------------------------------------------------------------------------------
-- Measuring
--------------------------------------------------------------------------------

--- The width a word occupies, including the spacing that follows it.
--- Floored to float32 so that the fit comparisons are stable.
function LineWrapper:wordWidth(word)
    return PDFNumber(
        self.document:widthOfString(word, self) + self.characterSpacing + self.wordSpacing
    )
end

--- Does a word of width `w` still fit on the current line?
---
--- A word ending in a soft hyphen needs room for the visible hyphen it will
--- grow if the break is taken there.
function LineWrapper:canFit(word, w)
    if word:sub(-#SOFT_HYPHEN) ~= SOFT_HYPHEN then
        return w <= self.spaceLeft
    end
    return w + self:wordWidth(HYPHEN) <= self.spaceLeft
end

--- Walk `text` one break-opportunity at a time, calling
--- fn(word, width, break, previous_break) for each.
---
--- A word wider than the whole line is chopped into pieces that do fit, and
--- each piece is handed over as though it were a word with a required break
--- after it. Returning false from fn stops the walk.
function LineWrapper:eachWord(text, fn)
    local breaker = LineBreaker.new(text)
    local last = nil
    local wordWidths = {}

    -- Hoisted, as JavaScript's `var` hoists them: both keep their value across
    -- iterations, and the chopping loop below reads mightGrow before assigning
    -- it on any pass where the word already fits.
    local shouldContinue
    local mightGrow

    local bk = breaker:nextBreak()
    while bk do
        local from = last and last.position or 0
        local word = text:sub(from + 1, bk.position)

        local w = wordWidths[word]
        if w == nil then
            w = self:wordWidth(word)
            wordWidths[word] = w
        end

        if w > self.lineWidth + self.continuedX then
            -- Too long for any line, so it has to be cut. The pieces are given
            -- fabricated break records: the cut itself is a required break,
            -- and the join before each piece is not.
            local lbk = last
            local fbk = {}

            while word ~= "" do
                local wordLen = jsstring.length(word)
                local l

                if w > self.spaceLeft then
                    -- Guess the cut point from the average character width
                    -- rather than stepping one character at a time. On a very
                    -- long run -- a thousand spaces, say -- the difference is
                    -- between one measurement and a thousand.
                    l = math.ceil(self.spaceLeft / (w / wordLen))
                    w = self:wordWidth(jsstring.prefix(word, l))
                    mightGrow = w <= self.spaceLeft and l < wordLen
                else
                    l = wordLen
                end

                -- Correct the guess. Widths grow monotonically with the prefix,
                -- so this settles on the longest prefix that fits regardless of
                -- which side the guess landed on.
                local mustShrink = w > self.spaceLeft and l > 0
                while mustShrink or mightGrow do
                    if mustShrink then
                        l = l - 1
                        w = self:wordWidth(jsstring.prefix(word, l))
                        mustShrink = w > self.spaceLeft and l > 0
                    else
                        l = l + 1
                        w = self:wordWidth(jsstring.prefix(word, l))
                        mustShrink = w > self.spaceLeft and l > 0
                        mightGrow = w <= self.spaceLeft and l < wordLen
                    end
                end

                -- A single character too wide for an empty line still has to go
                -- somewhere, or nothing would ever be emitted.
                if l == 0 and self.spaceLeft == self.lineWidth then
                    l = 1
                end

                fbk.required = bk.required or l < wordLen
                shouldContinue = fn(jsstring.prefix(word, l), w, fbk, lbk)
                lbk = { required = false }

                word = jsstring.suffix(word, l)
                w = self:wordWidth(word)

                if shouldContinue == false then
                    break
                end
            end
        else
            shouldContinue = fn(word, w, bk, last)
        end

        if shouldContinue == false then
            break
        end

        last = bk
        bk = breaker:nextBreak()
    end
end

--------------------------------------------------------------------------------
-- Wrapping
--------------------------------------------------------------------------------

function LineWrapper:wrap(text, options)
    local document = self.document

    -- A continued segment reuses the wrapper, so anything the new segment
    -- specifies has to overwrite what the previous one left behind.
    self.horizontalScaling = options.horizontalScaling or 100
    if options.indent ~= nil then
        self.indent = (options.indent * self.horizontalScaling) / 100
    end
    if options.characterSpacing ~= nil then
        self.characterSpacing = (options.characterSpacing * self.horizontalScaling) / 100
    end
    if options.wordSpacing ~= nil then
        self.wordSpacing = (options.wordSpacing * self.horizontalScaling) / 100
    end
    if options.ellipsis ~= nil then
        self.ellipsis = options.ellipsis
    end

    -- Make sure we are actually on the page, and that the first line of a
    -- paragraph is never left stranded at the bottom of one.
    local nextY = document.y + document:currentLineHeight(true)
    if document.y > self.maxY or nextY > self.maxY then
        self:nextSection()
    end

    local buffer = ""
    local textWidth = 0
    local wc = 0 -- words on the line so far
    local lc = 0 -- lines emitted so far

    local continueY = document.y -- where to put y back if the text continues

    local function emitLine()
        options.textWidth = textWidth + self.wordSpacing * (wc - 1)
        options.wordCount = wc
        options.lineWidth = self.lineWidth
        continueY = document.y
        self:emit("line", buffer, options, self)
        lc = lc + 1
    end

    self:emit("sectionStart", options, self)

    self:eachWord(text, function(word, w, bk, last)
        if last == nil or last.required then
            self:emit("firstLine", options, self)
            self.spaceLeft = self.lineWidth
        end

        if self:canFit(word, w) then
            buffer = buffer .. word
            textWidth = textWidth + w
            wc = wc + 1
        end

        if bk.required or not self:canFit(word, w) then
            local lh = document:currentLineHeight(true)

            -- If a height and an ellipsis were both given and the next line
            -- would not fit, this is the last line there will be -- so end it
            -- with the ellipsis rather than cutting mid-sentence.
            if self.height ~= nil and self.ellipsis
                and PDFNumber(document.y + lh * 2) > self.maxY
                and self.column >= self.columns
            then
                if self.ellipsis == true then
                    self.ellipsis = "\226\128\166" -- U+2026, the default
                end
                buffer = jsstring.rtrim(buffer)
                textWidth = self:wordWidth(buffer .. self.ellipsis)

                -- Give characters back until the ellipsis has room. The empty
                -- check is what stops this when even "..." is too wide.
                while buffer ~= "" and textWidth > self.lineWidth do
                    buffer = jsstring.rtrim(jsstring.drop_last(buffer))
                    textWidth = self:wordWidth(buffer .. self.ellipsis)
                end

                if textWidth <= self.lineWidth then
                    buffer = buffer .. self.ellipsis
                end

                textWidth = self:wordWidth(buffer)
            end

            if bk.required then
                -- The word did not fit, so it starts the next line instead of
                -- ending this one.
                if w > self.spaceLeft then
                    emitLine()
                    buffer = word
                    textWidth = w
                    wc = 1
                end

                self:emit("lastLine", options, self)
            end

            -- A soft hyphen that ends up at a break becomes a real one.
            if buffer:sub(-#SOFT_HYPHEN) == SOFT_HYPHEN then
                buffer = jsstring.drop_last(buffer) .. HYPHEN
                self.spaceLeft = self.spaceLeft - self:wordWidth(HYPHEN)
            end

            emitLine()

            -- Off the bottom of the column or page?
            if PDFNumber(document.y + lh) > self.maxY then
                self:emit("sectionEnd", options, self)
                local shouldContinue = self:nextSection()

                -- Nowhere left to go: a height was given and it is used up.
                if not shouldContinue then
                    wc = 0
                    buffer = ""
                    return false
                end

                self:emit("sectionStart", options, self)
            end

            if bk.required then
                self.spaceLeft = self.lineWidth
                buffer = ""
                textWidth = 0
                wc = 0
                return wc
            else
                self.spaceLeft = self.lineWidth - w
                buffer = word
                textWidth = w
                wc = 1
                return wc
            end
        else
            self.spaceLeft = self.spaceLeft - w
            return self.spaceLeft
        end
    end)

    -- Whatever is left in the buffer is the final line.
    if wc > 0 then
        self:emit("lastLine", options, self)
        emitLine()
    end

    self:emit("sectionEnd", options, self)

    -- A continued segment leaves x where the text stopped, so the next call
    -- picks up mid-line; a finished one puts x back where it started.
    if options.continued == true then
        if lc > 1 then
            self.continuedX = 0
        end
        self.continuedX = self.continuedX + (options.textWidth or 0)
        document.y = continueY
    else
        document.x = self.startX
    end
end

--- Move to the next column, or the next page when the columns run out.
--- Returns false when there is nowhere left to go.
function LineWrapper:nextSection(options)
    self.column = self.column + 1

    if self.column > self.columns then
        -- A caller-specified height is a hard stop. Without one, the default
        -- is to keep going on a fresh page.
        if self.height ~= nil then
            return false
        end

        self.document:continueOnNewPage()
        self.column = 1
        self.startY = self.document.page.margins.top
        self.maxY = self.document.page:maxY()

        if self.indentAllLines then
            local indent = (self.continuedX ~= 0 and self.continuedX) or self.indent
            self.document.x = self.document.x + indent
            self.lineWidth = self.lineWidth - indent
        else
            self.document.x = self.startX
        end

        -- The fill colour lives in the page's content stream, so a new page
        -- starts back at black unless it is set again.
        if self.document._fillColor then
            self.document:fillColor(table.unpack(self.document._fillColor))
        end

        self:emit("pageBreak", options, self)
    else
        self.document.x = self.document.x + self.lineWidth + self.columnGap
        self.document.y = self.startY
        self:emit("columnBreak", options, self)
    end

    return true
end

return LineWrapper
