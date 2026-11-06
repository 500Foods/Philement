-- mixins/images.lua -- port of pdfkit/lib/mixins/images.js
--
-- Places an image on the page. The image itself is embedded once, cached under
-- a label, and drawn by referencing that label -- so the same photo used on
-- forty pages is forty short operators and one copy of the data.
--
-- Nearly all of this file is geometry. Two things have to be reconciled:
--
--   PDF's image space runs bottom-up and the document's transform runs
--   top-down, so every image is drawn with a negative vertical scale. That is
--   the `hTransform = -h; yTransform += h` that appears in almost every branch
--   below, not a special case.
--
--   A photograph carries an EXIF orientation saying how the camera was held,
--   which is a rotation and possibly a mirror. The eight cases are worked
--   through explicitly; combined with the flip above, each one collapses to a
--   single transform plus at most one rotation.
--
-- Sizing happens before any of that: width, height, scale, fit and cover are
-- mutually exclusive ways of saying how big, and only fit and cover also imply
-- an alignment within a box.

local object = require("pdfkit.object")
local PDFImage = require("pdfkit.image")

local M = {}

-- JavaScript's `if (v)` for the option values this file tests.
local function truthy(v)
    return v ~= nil and v ~= false and v ~= 0 and v ~= ""
end

function M.initImages(doc)
    doc._imageRegistry = {}
    doc._imageCount = 0
end

function M.image(doc, src, x, y, options)
    options = options or {}

    -- image(src, options): the third argument doubling as the options table.
    if type(x) == "table" then
        options = x
        x = nil
    end

    -- Off by document setting, unless this call overrides it. The `~= false`
    -- test is what lets a per-image `false` win over a document-wide `true`.
    local ignoreOrientation = options.ignoreOrientation
        or (options.ignoreOrientation ~= false and doc.options.ignoreOrientation)

    -- Only a numeric y means "put it here"; anything else leaves the image in
    -- the flow and pushes the cursor down past it afterwards.
    local inDocumentFlow = (type(y) ~= "number")

    -- Position: the argument, else the option, else wherever the cursor is.
    if x == nil then x = options.x end
    if x == nil then x = doc.x end
    if y == nil then y = options.y end
    if y == nil then y = doc.y end

    local image
    if type(src) == "string" then
        image = doc._imageRegistry[src]
    end

    if not image then
        -- An already-opened image can be passed straight in; it is recognised
        -- by having dimensions of its own.
        if type(src) == "table" and src.width and src.height then
            image = src
        else
            image = doc:openImage(src)
        end
    end

    if not image.obj then
        image:embed(doc)
    end

    local xobjects = doc.page:xobjects()
    if xobjects[image.label] == nil then
        xobjects[image.label] = image.obj
    end

    local width, height = image.width, image.height

    -- Orientations above 4 involve a quarter turn, which swaps the axes.
    if not ignoreOrientation and image.orientation and image.orientation > 4 then
        width, height = height, width
    end

    local w = truthy(options.width) and options.width or width
    local h = truthy(options.height) and options.height or height
    local bw, bh

    if truthy(options.width) and not truthy(options.height) then
        local wp = w / width
        w = width * wp
        h = height * wp
    elseif truthy(options.height) and not truthy(options.width) then
        local hp = h / height
        w = width * hp
        h = height * hp
    elseif truthy(options.scale) then
        w = width * options.scale
        h = height * options.scale
    elseif options.fit then
        -- Largest size that fits inside the box, aspect preserved.
        bw, bh = options.fit[1], options.fit[2]
        local bp = bw / bh
        local ip = width / height
        if ip > bp then
            w = bw
            h = bw / ip
        else
            h = bh
            w = bh * ip
        end
    elseif options.cover then
        -- Smallest size that covers the box, aspect preserved. The overflow is
        -- not clipped here; a caller wanting that clips first.
        bw, bh = options.cover[1], options.cover[2]
        local bp = bw / bh
        local ip = width / height
        if ip > bp then
            h = bh
            w = bh * ip
        else
            w = bw
            h = bw / ip
        end
    end

    if options.fit or options.cover then
        if options.align == "center" then
            x = x + bw / 2 - w / 2
        elseif options.align == "right" then
            x = x + bw - w
        end

        if options.valign == "center" then
            y = y + bh / 2 - h / 2
        elseif options.valign == "bottom" then
            y = y + bh - h
        end
    end

    -- The transform that will place the image, built up per orientation.
    local rotateAngle = 0
    local originX, originY
    local xTransform, yTransform = x, y
    local wTransform, hTransform = w, h

    if not ignoreOrientation then
        local o = image.orientation

        if o == 2 then
            -- Mirrored horizontally.
            wTransform = -w
            hTransform = -h
            xTransform = xTransform + w
            yTransform = yTransform + h
        elseif o == 3 then
            -- Half turn.
            originX, originY = x, y
            hTransform = -h
            xTransform = xTransform - w
            rotateAngle = 180
        elseif o == 4 then
            -- Mirrored vertically -- which the document's own flip already
            -- does, so there is deliberately nothing to apply here.
        elseif o == 5 then
            -- Mirrored, then a quarter turn clockwise.
            originX, originY = x, y
            wTransform = h
            hTransform = w
            yTransform = yTransform - hTransform
            rotateAngle = 90
        elseif o == 6 then
            -- Quarter turn clockwise.
            originX, originY = x, y
            wTransform = h
            hTransform = -w
            rotateAngle = 90
        elseif o == 7 then
            -- Mirrored, then a quarter turn anticlockwise.
            originX, originY = x, y
            hTransform = -w
            wTransform = -h
            xTransform = xTransform + h
            rotateAngle = 90
        elseif o == 8 then
            -- Quarter turn anticlockwise.
            originX, originY = x, y
            wTransform = h
            hTransform = -w
            xTransform = xTransform - h
            yTransform = yTransform + w
            rotateAngle = -90
        else
            -- Orientation 1, or none recorded: just the document flip.
            hTransform = -h
            yTransform = yTransform + h
        end
    else
        hTransform = -h
        yTransform = yTransform + h
    end

    if options.link ~= nil then
        doc:link(x, y, w, h, options.link)
    end
    if options.goTo ~= nil then
        doc:goTo(x, y, w, h, options.goTo)
    end
    if options.destination ~= nil then
        doc:addNamedDestination(options.destination, "XYZ", x, y, object.NULL)
    end

    if inDocumentFlow then
        doc.y = doc.y + h
    end

    doc:save()

    if options.opacity ~= nil then
        doc:_doOpacity(options.opacity, nil)
    end

    if rotateAngle ~= 0 then
        doc:rotate(rotateAngle, { origin = { originX, originY } })
    end

    doc:transform(wTransform, 0, 0, hTransform, xTransform, yTransform)
    doc:addContent("/" .. image.label .. " Do")
    doc:restore()

    return doc
end

--- Open an image without drawing it, so the same one can be placed repeatedly
--- without being re-read. Only images given by filename are cached; raw data
--- has no key to cache under.
function M.openImage(doc, src)
    local image
    if type(src) == "string" then
        image = doc._imageRegistry[src]
    end

    if not image then
        doc._imageCount = doc._imageCount + 1
        image = PDFImage.open(src, "I" .. doc._imageCount)
        if type(src) == "string" then
            doc._imageRegistry[src] = image
        end
    end

    return image
end

return M
