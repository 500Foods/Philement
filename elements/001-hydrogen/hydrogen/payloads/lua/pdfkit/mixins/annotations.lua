-- mixins/annotations.lua -- port of pdfkit/lib/mixins/annotations.js
--
-- Links, notes, highlights: things layered over a page rather than drawn into
-- it. Two details in here are worth reading before changing anything.
--
-- KEY ORDER. An annotation dictionary is written in the order its keys were
-- assigned, and the caller's own keys are assigned first. A plain Lua table has
-- no order to recover, so pass object.dict{...} when the exact byte sequence
-- matters; a plain table is accepted and its keys are sorted, which is at least
-- reproducible.
--
-- PDFKit 0.20 stopped capitalising option keys, and stopped writing into the
-- table the caller passed. `open` stays `open`. A capitalised twin is not
-- added. Helpers copy, then annotate copies again.

local object = require("pdfkit.object")
local PDFAnnotationReference = require("pdfkit.structure_annotation")

local M = {}

-- Accept either an ordered dictionary or a plain table. Only the former can
-- reproduce a specific key order; the latter is sorted so that at least the
-- same input always produces the same output.
local function to_dict(options)
    if options == nil then
        return object.dict()
    end
    if object.is_dict(options) then
        local d = object.dict()
        for k, v in object.dict_pairs(options) do
            d[k] = v
        end
        return d
    end

    local keys = {}
    for k in pairs(options) do
        keys[#keys + 1] = k
    end
    table.sort(keys)

    local d = object.dict()
    for _, k in ipairs(keys) do
        d[k] = options[k]
    end
    return d
end

M._annotation_options = to_dict

function M.annotate(doc, x, y, w, h, options)
    options = to_dict(options)

    local structParent = options.structParent
    local color = options.color

    -- A new dictionary. 0.20 does not write back into the caller's table, and
    -- it does not invent a capitalised alias for a lowercase key.
    local annotations = object.dict()
    for k, v in object.dict_pairs(options) do
        if k ~= "structParent" and k ~= "color" then
            annotations[k] = v
        end
    end

    annotations.Type = "Annot"
    annotations.Rect = doc:_convertRect(x, y, w, h)
    annotations.Border = object.array({ 0, 0, 0 })

    if annotations.Subtype == "Link" and annotations.F == nil then
        annotations.F = 1 << 2 -- Print annotation flag
    end

    if annotations.Subtype ~= "Link" then
        if annotations.C == nil then
            annotations.C = object.array(
                doc:_normalizeColor(color or { 0, 0, 0 })
            )
        end
    end

    if type(annotations.Dest) == "string" then
        annotations.Dest = object.str(annotations.Dest)
    end

    local ref = doc:ref(annotations)
    -- A method rather than a property here, so the /Annots array is created on
    -- first access exactly as the JS getter does.
    local annots = doc.page:annotations()
    annots[#annots + 1] = ref

    -- Attaching the annotation to a structure element makes it reachable to a
    -- screen reader rather than being decoration a reader cannot describe.
    if structParent and type(structParent.add) == "function" then
        structParent:add(PDFAnnotationReference.new(ref))
    end

    ref:finish()
    return doc
end

function M.note(doc, x, y, w, h, contents, options)
    options = to_dict(options)
    options.Subtype = "Text"
    options.Contents = object.str(contents)
    if options.Name == nil then
        options.Name = "Comment"
    end
    if options.color == nil then
        options.color = { 243, 223, 92 }
    end
    return doc:annotate(x, y, w, h, options)
end

-- A link to a named destination elsewhere in this document.
function M.goTo(doc, x, y, w, h, name, options)
    options = to_dict(options)
    options.Subtype = "Link"
    options.A = doc:ref(object.dict({
        { "S", "GoTo" },
        { "D", object.str(name) },
    }))
    options.A:finish()
    return doc:annotate(x, y, w, h, options)
end

-- `url` is either a URL, or a zero-based page index within this document.
function M.link(doc, x, y, w, h, url, options)
    options = to_dict(options)
    options.Subtype = "Link"

    if type(url) == "number" then
        local pages = doc._root.data.Pages.data
        if url >= 0 and url < #pages.Kids then
            options.A = doc:ref(object.dict({
                { "S", "GoTo" },
                -- Three nulls: keep the reader's current position and zoom.
                { "D", object.array({
                    pages.Kids[url + 1], "XYZ",
                    object.NULL, object.NULL, object.NULL,
                }) },
            }))
            options.A:finish()
        else
            error("The document has no page " .. tostring(url))
        end
    else
        options.A = doc:ref(object.dict({
            { "S",   "URI" },
            { "URI", object.str(url) },
        }))
        options.A:finish()
    end

    if options.structParent and not options.Contents then
        options.Contents = object.str("")
    end

    return doc:annotate(x, y, w, h, options)
end

-- Text markup annotations name the region they cover with four corner points,
-- in the order top-left, top-right, bottom-left, bottom-right.
function M._markup(doc, x, y, w, h, options)
    options = to_dict(options)
    local rect = doc:_convertRect(x, y, w, h)
    local x1, y1, x2, y2 = rect[1], rect[2], rect[3], rect[4]
    options.QuadPoints = object.array({ x1, y2, x2, y2, x1, y1, x2, y1 })
    options.Contents = object.str("")
    return doc:annotate(x, y, w, h, options)
end

function M.highlight(doc, x, y, w, h, options)
    options = to_dict(options)
    options.Subtype = "Highlight"
    if options.color == nil then
        options.color = { 241, 238, 148 }
    end
    return doc:_markup(x, y, w, h, options)
end

function M.underline(doc, x, y, w, h, options)
    options = to_dict(options)
    options.Subtype = "Underline"
    return doc:_markup(x, y, w, h, options)
end

function M.strike(doc, x, y, w, h, options)
    options = to_dict(options)
    options.Subtype = "StrikeOut"
    return doc:_markup(x, y, w, h, options)
end

function M.lineAnnotation(doc, x1, y1, x2, y2, options)
    options = to_dict(options)
    options.Subtype = "Line"
    options.Contents = object.str("")
    options.L = object.array({
        x1, doc.page.height - y1,
        x2, doc.page.height - y2,
    })
    return doc:annotate(x1, y1, x2, y2, options)
end

function M.rectAnnotation(doc, x, y, w, h, options)
    options = to_dict(options)
    options.Subtype = "Square"
    options.Contents = object.str("")
    return doc:annotate(x, y, w, h, options)
end

function M.ellipseAnnotation(doc, x, y, w, h, options)
    options = to_dict(options)
    options.Subtype = "Circle"
    options.Contents = object.str("")
    return doc:annotate(x, y, w, h, options)
end

function M.textAnnotation(doc, x, y, w, h, text, options)
    options = to_dict(options)
    options.Subtype = "FreeText"
    options.Contents = object.str(text)
    options.DA = object.str("")
    return doc:annotate(x, y, w, h, options)
end

function M.fileAnnotation(doc, x, y, w, h, file, options)
    file = file or {}
    options = to_dict(options)

    -- The file itself is embedded but kept out of the attachments panel; the
    -- annotation is how the reader is meant to reach it.
    local fileOptions = { hidden = true }
    for k, v in pairs(file) do
        fileOptions[k] = v
    end
    local filespec = doc:file(file.src, fileOptions)

    options.Subtype = "FileAttachment"
    options.FS = filespec

    -- Fall back to the file's own description when none was given here.
    if options.Contents then
        options.Contents = object.str(options.Contents)
    elseif filespec.data.Desc then
        options.Contents = filespec.data.Desc
    end

    return doc:annotate(x, y, w, h, options)
end

-- Convert a top-left-origin rectangle into the two corner points a /Rect wants,
-- with the current transformation matrix applied.
--
-- Read the y1 line closely: it multiplies by x1 AFTER x1 has been overwritten
-- with its own transformed value. That is what the original computes, so it is
-- what this computes. For the identity-like matrices PDFKit actually uses, m1
-- is zero and the term vanishes -- which is presumably why it has gone
-- unnoticed. Under a rotation it would not.
function M._convertRect(doc, x1, y1, w, h)
    -- flip y1 and y2
    local y2 = y1
    y1 = y1 + h

    -- make x2
    local x2 = x1 + w

    local ctm = doc._ctm
    local m0, m1, m2, m3, m4, m5 = ctm[1], ctm[2], ctm[3], ctm[4], ctm[5], ctm[6]

    x1 = m0 * x1 + m2 * y1 + m4
    y1 = m1 * x1 + m3 * y1 + m5
    x2 = m0 * x2 + m2 * y2 + m4
    y2 = m1 * x2 + m3 * y2 + m5

    return object.array({ x1, y1, x2, y2 })
end

return M
