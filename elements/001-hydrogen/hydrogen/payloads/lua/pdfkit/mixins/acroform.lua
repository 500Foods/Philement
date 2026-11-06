-- mixins/acroform.lua -- port of pdfkit/lib/mixins/acroform.js
--
-- Interactive form fields: text boxes, checkboxes, dropdowns, buttons. A PDF
-- form is two overlapping trees. There is a *field* tree, which is names and
-- values and has no position, and there are *widget* annotations, which are
-- rectangles on a page. A leaf usually merges the two into one dictionary --
-- which is why `formText` both creates a field and calls `annotate`.
--
-- The document keeps one AcroForm dictionary in its catalog holding the roots
-- of that tree (/Fields), the default appearance string every field inherits
-- (/DA) and the font resources those strings name (/DR). `initForm` builds it,
-- `endAcroForm` closes it, and everything between adds to it.
--
-- ALMOST ALL OF THIS FILE IS ONE FUNCTION. `_fieldDict` takes the caller's
-- friendly options -- `required = true`, `align = "center"`, `format = {...}` --
-- and rewrites them into the terse dictionary keys the spec actually defines:
-- /Ff, /Q, /AA. The eight `_resolve*` helpers each own one slice of that
-- rewrite and run in a fixed order, because several of them read keys the
-- previous one wrote.
--
-- 0.20.2 builds a fresh dictionary and copies only the keys it understands.
-- Friendly options (fontSize, format, label, align) never become PDF keys, and
-- a table reused for a second field is read again rather than carrying the
-- first field's /FT and /Ff. The helpers below still rewrite a working copy;
-- _fieldDict then emits that copy's PDF keys in the order the JavaScript
-- mappers assign them.
--
-- KEY ORDER. As everywhere else in this port, dictionaries are built with
-- object.dict() so the output key sequence matches the order the source assigns
-- them. `delete` followed by a fresh assignment moves a key to the end in
-- JavaScript, and does the same here -- which is why VALUE_MAP below is an
-- ordered list and not a plain table.

local object = require("pdfkit.object")
local utils = require("pdfkit.utils")
local annotations = require("pdfkit.mixins.annotations")

-- Options may arrive as a plain Lua table or as an ordered dictionary; the
-- annotation mixin already has the coercion, so borrow it rather than repeat it.
local to_dict = annotations._annotation_options

local M = {}

--------------------------------------------------------------------------------
-- Tables
--------------------------------------------------------------------------------

-- Bits in the /Ff field-flags integer. Table 227 of the spec numbers these from
-- 1, so bit 1 is 0x1 and bit 25 is 0x1000000; the values here are already
-- shifted. The first three apply to every field type and the rest are specific
-- to one, which the spec expresses by reusing bit positions -- 0x8000 means
-- "radio" on a button and something else entirely on a text field. PDFKit does
-- not police that, and neither does this.
local FIELD_FLAGS = {
    readOnly = 1,
    required = 2,
    noExport = 4,
    multiline = 0x1000,
    password = 0x2000,
    toggleToOffButton = 0x4000,
    radioButton = 0x8000,
    pushButton = 0x10000,
    combo = 0x20000,
    edit = 0x40000,
    sort = 0x80000,
    multiSelect = 0x200000,
    noSpell = 0x400000,
}

local FIELD_JUSTIFY = {
    left = 0,
    center = 1,
    right = 2,
}

-- An ordered list, not a table, because the rewrite below appends /V and /DV in
-- *this* order regardless of the order the caller wrote `value` and
-- `defaultValue` -- and appending in a different order would change the bytes.
local VALUE_MAP = {
    { "value", "V" },
    { "defaultValue", "DV" },
}

-- Format types that Acrobat's built-in AFSpecial_* handlers cover. The value is
-- the numeric argument those handlers take.
local FORMAT_SPECIAL = {
    zip = "0",
    zipPlus4 = "1",
    zip4 = "1",
    phone = "2",
    ssn = "3",
}

local FORMAT_DEFAULT = {
    number = {
        nDec = 0,
        sepComma = false,
        negStyle = "MinusBlack",
        currency = "",
        currencyPrepend = true,
    },
    percent = {
        nDec = 0,
        sepComma = false,
    },
}

--------------------------------------------------------------------------------
-- Small helpers
--------------------------------------------------------------------------------

-- JavaScript's `if (v)`, which this file leans on for option values. Note that
-- a flag set to 0 is therefore "not set", matching `if (options[key])` in
-- _resolveFlags.
local function truthy(v)
    return v ~= nil and v ~= false and v ~= 0 and v ~= ""
end

-- JavaScript's String(v). Only the cases this file can produce are handled: a
-- number goes through the shared number-to-string rules so 0 does not become
-- "0.0", a boolean gives "true"/"false", and a missing value gives "undefined"
-- -- which is exactly what a caller who forgot `format.param` gets from PDFKit.
local function js_String(v)
    if v == nil then return "undefined" end
    if type(v) == "number" then return utils.js_tostring(v) end
    if type(v) == "boolean" then return v and "true" or "false" end
    return tostring(v)
end

-- The JS `x.foo = x.foo ? x.foo : {}` idiom, plus the coercion Lua needs: a
-- sub-dictionary the caller supplied as a plain table has to become an ordered
-- one before anything is written into it, or object.convert will refuse it.
-- Reassigning an existing key updates in place, so this never reorders.
local function ensure_dict(container, key)
    local v = container[key]
    if v == nil then
        v = object.dict()
        container[key] = v
    elseif not object.is_dict(v) then
        v = to_dict(v)
        container[key] = v
    end
    return v
end

local function shallow_dict_copy(d)
    local out = object.dict()
    for k, v in object.dict_pairs(d) do
        out[k] = v
    end
    return out
end

-- "date" -> "Date". JavaScript's charAt(0).toUpperCase() + slice(1).
local function capitalize(s)
    return s:sub(1, 1):upper() .. s:sub(2)
end

--------------------------------------------------------------------------------
-- Document-level form setup
--------------------------------------------------------------------------------

--- Create the document's AcroForm dictionary. Must be called before any form
--- element is added, and after `font()` -- the current font at this moment
--- becomes the form's default, named by /DA and carried in /DR.
function M.initForm(doc)
    if not doc._font then
        error("Must set a font before calling initForm method")
    end

    doc._acroform = {
        -- Every font any field ends up using, accumulated here and merged into
        -- /DR at the end. Ordered, because it is written out as a dictionary.
        fonts = object.dict(),
        defaultFont = doc._font.name,
    }
    doc._acroform.fonts[doc._font.id] = doc._font:ref()

    -- NeedAppearances asks the reader to draw the field contents itself. PDFKit
    -- does not generate appearance streams, so without this the fields would be
    -- present and interactive but invisible until clicked.
    local fontResources = object.dict()
    fontResources[doc._font.id] = doc._font:ref()

    local data = object.dict({
        { "Fields", object.array({}) },
        { "NeedAppearances", true },
        { "DA", object.str("/" .. doc._font.id .. " 0 Tf 0 g") },
        { "DR", object.dict({ { "Font", fontResources } }) },
    })

    local AcroForm = doc:ref(data)
    doc._root.data.AcroForm = AcroForm
    return doc
end

--- Called automatically when the document is finished.
---
--- Fonts discovered while building fields are folded into /DR now, because a
--- field created on page 40 can introduce a font the form did not know about
--- when it started. Then every field is closed -- see _endChild for why that is
--- recursive and why leaves are skipped.
function M.endAcroForm(doc)
    if doc._root.data.AcroForm then
        if object.dict_count(doc._acroform.fonts) == 0
            and not doc._acroform.defaultFont then
            error("No fonts specified for PDF form")
        end

        local fontDict = doc._root.data.AcroForm.data.DR.Font
        for name, ref in object.dict_pairs(doc._acroform.fonts) do
            fontDict[name] = ref
        end

        for _, fieldRef in ipairs(doc._root.data.AcroForm.data.Fields) do
            doc:_endChild(fieldRef)
        end

        doc._root.data.AcroForm:finish()
    end
    return doc
end

--- Close a field and everything below it.
---
--- Only nodes with /Kids are closed here. A leaf is a widget annotation, and
--- `annotate` already finished it the moment it was created -- finishing it
--- twice would write the object twice. Intermediate nodes have no such owner,
--- so this is their only chance.
function M._endChild(doc, ref)
    if object.is_arrayish(ref.data.Kids) then
        for _, childRef in ipairs(ref.data.Kids) do
            doc:_endChild(childRef)
        end
        ref:finish()
    end
    return doc
end

--------------------------------------------------------------------------------
-- Creating fields
--------------------------------------------------------------------------------

--- An intermediate node in the field tree. It has a name and can hold shared
--- defaults, but no rectangle and nothing on a page: its purpose is to let
--- several widgets share a name prefix, so `address.city` and `address.zip`
--- hang off one `address` field.
---
--- Returns the reference, which is what a child passes as its `parent` option.
function M.formField(doc, name, options)
    local fieldDict = doc:_fieldDict(name, nil, options or {})
    local fieldRef = doc:ref(fieldDict)
    doc:_addToParent(fieldRef)
    return fieldRef
end

--- A leaf: one dictionary that is both a form field and a widget annotation.
---
--- Unlike formField this returns the *document*, not the reference -- because
--- _addToParent returns the document and its result is passed straight through.
--- The asymmetry is upstream's; a caller needing the ref can read it back off
--- the page's annotation list.
function M.formAnnotation(doc, name, fieldType, x, y, w, h, options)
    local fieldDict = doc:_fieldDict(name, fieldType, options or {})
    fieldDict.Subtype = "Widget"
    if fieldDict.F == nil then
        fieldDict.F = 4 -- print the annotation
    end

    -- Add the field annotation to the page, then recover the ref annotate made.
    doc:annotate(x, y, w, h, fieldDict)
    local annots = doc.page:annotations()
    local annotRef = annots[#annots]

    return doc:_addToParent(annotRef)
end

function M.formText(doc, name, x, y, w, h, options)
    return doc:formAnnotation(name, "text", x, y, w, h, options or {})
end

function M.formPushButton(doc, name, x, y, w, h, options)
    return doc:formAnnotation(name, "pushButton", x, y, w, h, options or {})
end

function M.formCombo(doc, name, x, y, w, h, options)
    return doc:formAnnotation(name, "combo", x, y, w, h, options or {})
end

function M.formList(doc, name, x, y, w, h, options)
    return doc:formAnnotation(name, "list", x, y, w, h, options or {})
end

function M.formRadioButton(doc, name, x, y, w, h, options)
    return doc:formAnnotation(name, "radioButton", x, y, w, h, options or {})
end

function M.formCheckbox(doc, name, x, y, w, h, options)
    return doc:formAnnotation(name, "checkbox", x, y, w, h, options or {})
end

--- File a new field under its parent, or at the root of the form if it has none.
function M._addToParent(doc, fieldRef)
    local parent = fieldRef.data.Parent
    if parent then
        if not parent.data.Kids then
            parent.data.Kids = object.array({})
        end
        local kids = parent.data.Kids
        kids[#kids + 1] = fieldRef
    else
        local fields = doc._root.data.AcroForm.data.Fields
        fields[#fields + 1] = fieldRef
    end
    return doc
end

--------------------------------------------------------------------------------
-- Turning options into a field dictionary
--------------------------------------------------------------------------------

--- The rewrite. Order matters: _resolveType writes the flag keys that
--- _resolveFlags consumes, _resolveFont writes the /DA that nothing else may
--- overwrite, and _resolveStrings must run before _resolveColors so that a
--- `label` has already become MK.CA by the time MK gains its colours.
function M._fieldDict(doc, name, fieldType, options)
    if not doc._acroform then
        error("Call document.initForm() method before adding form elements to document")
    end

    options = to_dict(options or {})

    -- Work on a copy. 0.20.2 does not write back into the caller's options.
    local opts = shallow_dict_copy(options)
    if fieldType ~= nil then
        doc:_resolveType(fieldType, opts)
    end

    opts = doc:_resolveFlags(opts)
    opts = doc:_resolveJustify(opts)
    opts = doc:_resolveFont(opts)
    opts = doc:_resolveStrings(opts)
    opts = doc:_resolveColors(opts)
    opts = doc:_resolveFormat(opts)

    opts.T = object.str(name)

    if opts.parent then
        opts.Parent = opts.parent
        opts.parent = nil
    end

    -- The mappers in acroform.js assign these keys in this order, and nothing
    -- else. Subtype and F are added afterwards by formAnnotation.
    local ordered = object.dict()
    for _, key in ipairs({
        "FT", "Ff", "Q", "DR", "DA", "Opt", "V", "DV", "MK", "AA", "T", "Parent",
    }) do
        local value = opts[key]
        if value ~= nil and not (key == "Ff" and value == 0) then
            ordered[key] = value
        end
    end
    return ordered
end

--- /FT, plus the flag that distinguishes the four things a button can be.
---
--- The flags are set as ordinary option keys and picked up by _resolveFlags a
--- moment later, which is why this has to run first.
function M._resolveType(doc, fieldType, opts)
    if fieldType == "text" then
        opts.FT = "Tx"
    elseif fieldType == "pushButton" then
        opts.FT = "Btn"
        opts.pushButton = true
    elseif fieldType == "radioButton" then
        opts.FT = "Btn"
        opts.radioButton = true
    elseif fieldType == "checkbox" then
        -- A checkbox is the plain case: a button with neither flag set.
        opts.FT = "Btn"
    elseif fieldType == "combo" then
        opts.FT = "Ch"
        opts.combo = true
    elseif fieldType == "list" then
        opts.FT = "Ch"
    else
        error("Invalid form annotation type '" .. tostring(fieldType) .. "'")
    end
    return opts
end

--- Collapse the named boolean options into the /Ff bitfield.
---
--- Every recognised key is removed whether or not it was set, so a `required =
--- false` does not survive into the dictionary as a stray /required. An /Ff the
--- caller supplied directly is OR-ed with, not replaced -- the two ways of
--- saying the same thing combine rather than fight.
function M._resolveFlags(doc, options)
    local result = 0

    -- Snapshot the keys first: the loop deletes from the dictionary it walks.
    local keys = {}
    for key in object.dict_pairs(options) do
        keys[#keys + 1] = key
    end

    for _, key in ipairs(keys) do
        if FIELD_FLAGS[key] then
            if truthy(options[key]) then
                result = result | FIELD_FLAGS[key]
            end
            options[key] = nil
        end
    end

    if result ~= 0 then
        options.Ff = truthy(options.Ff) and options.Ff or 0
        options.Ff = options.Ff | result
    end
    return options
end

--- /Q, the quadding (text alignment) inside the field.
---
--- Left is the default and is encoded as 0, so `align = "left"` removes the key
--- and writes nothing -- the same dictionary a caller who said nothing gets.
function M._resolveJustify(doc, options)
    local result = 0
    if options.align ~= nil then
        if type(FIELD_JUSTIFY[options.align]) == "number" then
            result = FIELD_JUSTIFY[options.align]
        end
        options.align = nil
    end
    if result ~= 0 then
        options.Q = result -- default
    end
    return options
end

--- Make sure the font this field needs will exist by the time a reader looks
--- for it, and give the field its own /DA if it is not using the form default.
---
--- A field size of 0 is not "invisible": in a /DA it means auto-size, which is
--- what a caller who did not ask for a size wants.
function M._resolveFont(doc, options)
    -- Add the current font to the document-level AcroForm dict if necessary.
    if doc._acroform.fonts[doc._font.id] == nil then
        doc._acroform.fonts[doc._font.id] = doc._font:ref()
    end

    -- Add the current font to the field's resource dict (DR) if it is not the
    -- default acroform font.
    if doc._acroform.defaultFont ~= doc._font.name then
        local fontResources = object.dict()
        options.DR = object.dict({ { "Font", fontResources } })

        -- Get the fontSize option. If not set use auto sizing.
        -- `fontSize || 0` upstream: 0 and a missing size both mean auto-size.
        local fontSize = options.fontSize
        if type(fontSize) ~= "number" or fontSize == 0 then
            fontSize = 0
        end
        options.fontSize = nil

        fontResources[doc._font.id] = doc._font:ref()
        options.DA = object.str(
            "/" .. doc._font.id .. " " .. utils.js_tostring(fontSize) .. " Tf 0 g"
        )
    end
    return options
end

--- Everything that has to become a PDF *string* rather than a name, plus the
--- choice list for combo and list fields.
---
--- The friendly names `value` and `defaultValue` become /V and /DV here; note
--- that only string values get wrapped, because a checkbox's /V is a name
--- (/Yes, /Off) and wrapping it would break it.
function M._resolveStrings(doc, options)
    local select = {}

    local function appendChoices(a)
        if object.is_arrayish(a) then
            for idx = 1, #a do
                if type(a[idx]) == "string" then
                    select[#select + 1] = object.str(a[idx])
                else
                    -- A two-element [export, display] pair, or something the
                    -- caller has already wrapped. Passed through untouched.
                    select[#select + 1] = a[idx]
                end
            end
        end
    end

    appendChoices(options.Opt)
    if truthy(options.select) then
        appendChoices(options.select)
        options.select = nil
    end
    if #select > 0 then
        options.Opt = object.array(select)
    end

    for _, entry in ipairs(VALUE_MAP) do
        local from, to = entry[1], entry[2]
        if options[from] ~= nil then
            options[to] = options[from]
            options[from] = nil
        end
    end

    for _, key in ipairs({ "V", "DV" }) do
        if type(options[key]) == "string" then
            options[key] = object.str(options[key])
        end
    end

    -- MK.CA is the "normal caption": the text drawn on a push button, or the
    -- character a checkbox shows when ticked ("4" in ZapfDingbats is the tick).
    if options.MK ~= nil then
        local mk = ensure_dict(options, "MK")
        if truthy(mk.CA) then
            mk.CA = object.str(mk.CA)
        end
    end
    if truthy(options.label) then
        local mk = ensure_dict(options, "MK")
        mk.CA = object.str(options.label)
        options.label = nil
    end

    return options
end

--- Background and border colours, which live under /MK rather than at the top
--- level because they belong to the widget's appearance, not to the field.
---
--- _normalizeColor returns nil for anything that is not a colour, and a nil is
--- simply skipped -- so a mistyped colour name leaves the field unstyled rather
--- than raising.
function M._resolveColors(doc, options)
    local color = doc:_normalizeColor(options.backgroundColor)
    if color then
        local mk = ensure_dict(options, "MK")
        mk.BG = object.array(color)
    end

    color = doc:_normalizeColor(options.borderColor)
    if color then
        local mk = ensure_dict(options, "MK")
        mk.BC = object.array(color)
    end

    options.backgroundColor = nil
    options.borderColor = nil
    return options
end

--- Turn `format = { type = "number", nDec = 2 }` into the pair of JavaScript
--- actions Acrobat expects.
---
--- There is no declarative way to format a field value in PDF. What readers
--- actually implement is a convention: a keystroke action (/AA /K) validates
--- each key press and a format action (/AA /F) renders the stored value for
--- display, both calling handlers that ship inside the reader -- AFNumber_Format
--- and friends. So the "format" written here is a one-line script.
---
--- Readers other than Acrobat may not implement these handlers, in which case
--- the field still works and simply shows its raw value.
function M._resolveFormat(doc, opts)
    local f = opts.format

    if f and truthy(f.type) then
        local fnKeystroke, fnFormat
        local params = ""

        if FORMAT_SPECIAL[f.type] ~= nil then
            -- Fixed-shape values -- zip codes, phone numbers, social security
            -- numbers -- all share one handler that takes a selector.
            fnKeystroke = "AFSpecial_Keystroke"
            fnFormat = "AFSpecial_Format"
            params = FORMAT_SPECIAL[f.type]
        else
            local format = capitalize(f.type)
            fnKeystroke = "AF" .. format .. "_Keystroke"
            fnFormat = "AF" .. format .. "_Format"

            if f.type == "date" then
                -- The Ex variants take the format as a string. The plain ones
                -- take an index into Acrobat's predefined formats.
                fnKeystroke = fnKeystroke .. "Ex"
                fnFormat = fnFormat .. "Ex"
                params = '"' .. js_String(f.param) .. '"'
            elseif f.type == "time" then
                params = js_String(f.param)
            elseif f.type == "number" then
                local p = {}
                for k, v in pairs(FORMAT_DEFAULT.number) do p[k] = v end
                for k, v in pairs(f) do p[k] = v end

                -- Positional arguments, in the order AFNumber_Format declares
                -- them. The separator argument is inverted -- 0 means *use*
                -- thousands separators -- and the fourth is a reserved slot
                -- that must still be filled.
                params = table.concat({
                    js_String(p.nDec),
                    truthy(p.sepComma) and "0" or "1",
                    '"' .. tostring(p.negStyle) .. '"',
                    "null",
                    '"' .. tostring(p.currency) .. '"',
                    js_String(p.currencyPrepend),
                }, ",")
            elseif f.type == "percent" then
                local p = {}
                for k, v in pairs(FORMAT_DEFAULT.percent) do p[k] = v end
                for k, v in pairs(f) do p[k] = v end

                params = table.concat({
                    js_String(p.nDec),
                    truthy(p.sepComma) and "0" or "1",
                }, ",")
            end
        end

        local aa = ensure_dict(opts, "AA")
        aa.K = object.dict({
            { "S", "JavaScript" },
            { "JS", object.str(fnKeystroke .. "(" .. params .. ");") },
        })
        aa.F = object.dict({
            { "S", "JavaScript" },
            { "JS", object.str(fnFormat .. "(" .. params .. ");") },
        })
    end

    opts.format = nil
    return opts
end

return M
